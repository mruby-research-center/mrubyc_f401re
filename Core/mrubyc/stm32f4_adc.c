/*! @file
  @brief
  ADC class.

  <pre>
  An implementation of common peripheral I/O API for mruby/c.
  https://github.com/mruby/microcontroller-peripheral-interface-guide

  Copyright (C) 2024- Shimane IT Open-Innovation Center.

  This file is distributed under BSD 3-Clause License.

  </pre>
*/

#include "main.h"
#include "../mrubyc_src/mrubyc.h"
#include "stm32f4_gpio.h"


extern ADC_HandleTypeDef hadc1;

/*!
  Pin assign vs ADC channel table.
*/
static struct ADC_HANDLE {
  PIN_HANDLE pin;	//!< Pin
  uint32_t channel;	//!< ADC channel.

} const TBL_ADC_CHANNELS[] = {
				// GPIO  ADC ch.  silk
  {{1, 0}, ADC_CHANNEL_0},	// PA0   0        A0
  {{1, 1}, ADC_CHANNEL_1},	// PA1   1        A1
  {{1, 4}, ADC_CHANNEL_4},	// PA4   1        A2
  {{2, 0}, ADC_CHANNEL_8},	// PB0   8        A3
  {{3, 1}, ADC_CHANNEL_11},	// PC1   11       A4
  {{3, 0}, ADC_CHANNEL_10},	// PC0   10       A5
};
static const int NUM_TBL_ADC_CHANNELS = sizeof(TBL_ADC_CHANNELS)/sizeof(struct ADC_HANDLE);


//================================================================
/*! constructor

  adc0 = ADC.new(0)
  adc1 = ADC.new("PA1")
*/
static void c_adc_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  if( argc != 1 ) goto ERROR_RETURN;
  int idx;
  PIN_HANDLE pin;

  switch( v[1].tt ) {
  // in case of ADC.new(0)
  case MRBC_TT_INTEGER:
    idx = mrbc_integer(v[1]);
    if( idx < 0 || idx >= NUM_TBL_ADC_CHANNELS ) goto ERROR_RETURN;
    pin = TBL_ADC_CHANNELS[idx].pin;
    break;

  // in case of ADC.new("PA1")
  case MRBC_TT_STRING:
    if( gpio_set_pin_handle( &pin, &v[1] ) != 0 ) goto ERROR_RETURN;

    // find ADC channel from TBL_PIN_TO_ADC_CHANNEL table.
    for( idx = 0; idx < NUM_TBL_ADC_CHANNELS; idx++ ) {
      if( (TBL_ADC_CHANNELS[idx].pin.port == pin.port) &&
	  (TBL_ADC_CHANNELS[idx].pin.num == pin.num) ) break;
    }
    if( idx >= NUM_TBL_ADC_CHANNELS ) goto ERROR_RETURN;
    break;

  default:
    goto ERROR_RETURN;
  }

  // allocate instance with table index.
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(int));
  *((int *)(v[0].instance->data)) = idx;

  // set pin to analog input
  gpio_setmode( &pin, GPIO_ANALOG|GPIO_IN );
  return;

 ERROR_RETURN:
  mrbc_raise(vm, MRBC_CLASS(ArgumentError), "ADC initialize.");
}


//----------------------------------------------------------------
static uint32_t read_sub(mrbc_vm *vm, mrbc_value v[], int argc)
{
  int idx = *((int *)(v[0].instance->data));

  ADC_ChannelConfTypeDef sConfig = {
    .Channel = TBL_ADC_CHANNELS[idx].channel,
    .Rank = 1,
    .SamplingTime = ADC_SAMPLETIME_3CYCLES,
  };
  if( HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK ) return 0;

  HAL_ADC_Start(&hadc1);
  if( HAL_ADC_PollForConversion(&hadc1, 1000) != HAL_OK ) return 0;

  return HAL_ADC_GetValue(&hadc1);
}


//================================================================
/*! read_voltage

  adc1.read_voltage() -> Float
*/
static void c_adc_read_voltage(mrbc_vm *vm, mrbc_value v[], int argc)
{
  uint32_t raw_val = read_sub( vm, v, argc );

  SET_FLOAT_RETURN( raw_val * 3.3 / 4095 );
}


//================================================================
/*! read_raw

  adc1.read_raw() -> Integer
*/
static void c_adc_read_raw(mrbc_vm *vm, mrbc_value v[], int argc)
{
  uint32_t raw_val = read_sub( vm, v, argc );

  SET_INT_RETURN( raw_val );
}


//================================================================
/*! Initializer
*/
void mrbc_init_class_adc(void)
{
  mrbc_class *cls = mrbc_define_class(0, "ADC", 0);

  mrbc_define_method(0, cls, "new", c_adc_new);
  mrbc_define_method(0, cls, "read_voltage", c_adc_read_voltage);
  mrbc_define_method(0, cls, "read", c_adc_read_voltage);
  mrbc_define_method(0, cls, "read_raw", c_adc_read_raw);
}
