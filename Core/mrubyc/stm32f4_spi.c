/*! @file
  @brief
  SPI class.

  <pre>
  An implementation of common peripheral I/O API for mruby/c.
  https://github.com/mruby/microcontroller-peripheral-interface-guide

  Copyright (C) 2024- Shimane IT Open-Innovation Center.

  This file is distributed under BSD 3-Clause License.

  </pre>
*/

#include "main.h"
#include "../mrubyc_src/mrubyc.h"

//@cond
#include <string.h>
//@endcond

extern SPI_HandleTypeDef hspi3;

static const uint32_t SPI_TIMEOUT_ms = 3000;
static const uint32_t SPI_BASEFREQ = 42000000U;	// 42MHz


uint8_t * make_output_buffer(mrb_vm *vm, mrb_value v[], int argc,
			     int start_idx, int *ret_bufsiz);
static void c_spi_setmode(mrbc_vm *vm, mrbc_value v[], int argc);


//================================================================
/*! set SPI mode and clock

  @param  hspi		pointer to HAL SPI_Handle
  @param  freq		clock frequency (Hz)
  @param  mode		mode (0..3)
  @param  msb_lsb	first bit msb = 0 or lsb = 1
  @return		zero is no error.
*/
static int spi_setmode( SPI_HandleTypeDef *hspi,
			int32_t freq, int mode, int msb_lsb )
{
  static uint32_t const TBL_PRESCALER[] = {
    SPI_BAUDRATEPRESCALER_2,   SPI_BAUDRATEPRESCALER_4,
    SPI_BAUDRATEPRESCALER_8,   SPI_BAUDRATEPRESCALER_16,
    SPI_BAUDRATEPRESCALER_32,  SPI_BAUDRATEPRESCALER_64,
    SPI_BAUDRATEPRESCALER_128, SPI_BAUDRATEPRESCALER_256,
  };
  static const int NUM_TBL_PRESCALER = sizeof(TBL_PRESCALER) / sizeof(TBL_PRESCALER[0]);

  if( freq > 0 ) {
    int prescaler = 2;
    int n;
    for( n = 0; n < NUM_TBL_PRESCALER; n++, prescaler *= 2 ) {
      uint32_t fcalc = SPI_BASEFREQ / prescaler;
      if( freq >= fcalc ) break;
    }
    hspi->Init.BaudRatePrescaler = TBL_PRESCALER[n];
  }

  switch( mode ) {
  case 0:
    hspi->Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi->Init.CLKPhase = SPI_PHASE_1EDGE;
    break;

  case 1:
    hspi->Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi->Init.CLKPhase = SPI_PHASE_2EDGE;
    break;

  case 2:
    hspi->Init.CLKPolarity = SPI_POLARITY_HIGH;
    hspi->Init.CLKPhase = SPI_PHASE_1EDGE;
    break;

  case 3:
    hspi->Init.CLKPolarity = SPI_POLARITY_HIGH;
    hspi->Init.CLKPhase = SPI_PHASE_2EDGE;
    break;
  }

  switch( msb_lsb ) {
  case 0:
    hspi->Init.FirstBit = SPI_FIRSTBIT_MSB;
    break;

  case 1:
    hspi->Init.FirstBit = SPI_FIRSTBIT_LSB;
    break;
  }

  __HAL_SPI_DISABLE(hspi);
  if (HAL_SPI_Init(hspi) != HAL_OK) return -1;
  __HAL_SPI_ENABLE(hspi);

  return 0;
}


//================================================================
/*! SPI constructor

  @verbatim
  spi = SPI.new()   # all default. (mode 0, freq 656kHz, MSB_FIRST)

  spi = SPI.new( id=nil, params )
                    # unit: (dummy)
                    # mode: 0..3
		    # frequency: 164_000..21_000_000 (164kHz - 21MHz)
		    # first_bit: SPI::MSB_FIRST, SPI::LSB_FIRST
  (note)
  CN   pin   GPIO  Usage
  CN7   1    PC10  SPI3_SCK
  CN7   2    PC11  SPI3_MISO
  CN7   3    PC12  SPI3_MOSI
  @endverbatim
*/
static void c_spi_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  // allocate instance
  v[0] = mrbc_instance_new(vm, v[0].cls, 0);

  c_spi_setmode( vm, v, argc );
}


//================================================================
/*! set mode

  @verbatim
  spi.setmode( *params )
  @endverbatim
*/
static void c_spi_setmode(mrbc_vm *vm, mrbc_value v[], int argc)
{
  MRBC_KW_ARG( unit, frequency, mode, first_bit );
  if( !MRBC_KW_END() ) goto RETURN;

  int32_t spi_freq = -1;
  int spi_mode = -1;
  int spi_first_bit = -1;

  if( MRBC_KW_ISVALID(frequency) ) spi_freq = mrbc_integer(frequency);
  if( MRBC_KW_ISVALID(mode) ) spi_mode = mrbc_integer(mode);
  if( MRBC_KW_ISVALID(first_bit) ) spi_first_bit = mrbc_integer(first_bit);

  if( spi_setmode(&hspi3, spi_freq, spi_mode, spi_first_bit) < 0 ) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), 0);
  }

 RETURN:
  MRBC_KW_DELETE( unit, frequency, mode, first_bit );
}


//================================================================
/*! SPI read

  @verbatim
  s = spi.read(read_bytes) -> String
  @endverbatim
*/
static void c_spi_read(mrbc_vm *vm, mrbc_value v[], int argc)
{
  if( v[1].tt != MRBC_TT_INTEGER ) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), 0);
    return;
  }

  int read_bytes = mrbc_integer(v[1]);
  mrbc_value ret = mrbc_string_new(vm, 0, read_bytes);
  uint8_t *buf = (uint8_t *)mrbc_string_cstr(&ret);

  memset(buf, 0, read_bytes);

  HAL_StatusTypeDef sts;
  sts = HAL_SPI_TransmitReceive(&hspi3, buf, buf, read_bytes, SPI_TIMEOUT_ms );

  if( sts != HAL_OK ) {
    mrbc_raisef(vm, 0, "HAL layer error (status code %d)", sts);
  }

  SET_RETURN(ret);
}


//================================================================
/*! SPI write

  @verbatim
  spi.write( "str" )
  spi.write( d1, d2, ...)
  spi.write( [d1, d2,...] )
  @endverbatim
*/
static void c_spi_write(mrbc_vm *vm, mrbc_value v[], int argc)
{
  int bufsiz;
  uint8_t *buf = make_output_buffer(vm, v, argc, 1, &bufsiz );
  if( !buf ) return;

  HAL_StatusTypeDef sts;
  sts = HAL_SPI_Transmit(&hspi3, buf, bufsiz, SPI_TIMEOUT_ms );
  mrbc_free( vm, buf );

  if( sts != HAL_OK ) {
    mrbc_raisef(vm, 0, "HAL layer error (status code %d)", sts);
  }

  SET_NIL_RETURN();
}


//================================================================
/*! SPI transfer

  @verbatim
  s = spi.transfer( out_data, additional_read_bytes = 0 ) -> String
  @endverbatim
*/
static void c_spi_transfer(mrbc_vm *vm, mrbc_value v[], int argc)
{
  uint8_t *buf = 0;
  int bufsiz;

  if( argc == 0 ) goto ERROR_ARGUMENT;

  buf = make_output_buffer(vm, v, 1, 1, &bufsiz );
  if( !buf ) return;

  if( argc >= 2 ) {
    if( v[2].tt != MRBC_TT_INTEGER ) goto ERROR_ARGUMENT;
    int additional_read_bytes = mrbc_integer(v[2]);

    uint8_t *buf2 = mrbc_realloc(vm, buf, bufsiz + additional_read_bytes);
    if( !buf2 ) {
      mrbc_free(vm, buf);
      mrbc_raise(vm, 0, 0);
      return;
    }

    buf = buf2;
    memset(buf + bufsiz, 0, additional_read_bytes);
    bufsiz += additional_read_bytes;
  }

  mrbc_value ret = mrbc_string_new_alloc(vm, buf, bufsiz);

  HAL_StatusTypeDef sts;
  sts = HAL_SPI_TransmitReceive(&hspi3, buf, buf, bufsiz, SPI_TIMEOUT_ms );

  if( sts != HAL_OK ) {
    mrbc_raisef(vm, 0, "HAL layer error (status code %d)", sts);
  }

  SET_RETURN(ret);
  return;


 ERROR_ARGUMENT:
  mrbc_raise(vm, MRBC_CLASS(ArgumentError),0);
}


//================================================================
/*! initialize
*/
void mrbc_init_class_spi(void)
{
  mrbc_class *cls = mrbc_define_class(0, "SPI", 0);

  mrbc_define_method(0, cls, "new", c_spi_new);
  mrbc_define_method(0, cls, "setmode", c_spi_setmode);
  mrbc_define_method(0, cls, "read", c_spi_read);
  mrbc_define_method(0, cls, "write", c_spi_write);
  mrbc_define_method(0, cls, "transfer", c_spi_transfer);

  mrbc_set_class_const(cls, mrbc_str_to_symid("MSB_FIRST"), &mrbc_integer_value(0));
  mrbc_set_class_const(cls, mrbc_str_to_symid("LSB_FIRST"), &mrbc_integer_value(1));
}
