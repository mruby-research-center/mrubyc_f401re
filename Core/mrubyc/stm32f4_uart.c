/*! @file
  @brief
  UART class.

  <pre>
  An implementation of common peripheral I/O API for mruby/c.
  https://github.com/mruby/microcontroller-peripheral-interface-guide

  Copyright (C) 2024- Shimane IT Open-Innovation Center.

  This file is distributed under BSD 3-Clause License.

  </pre>
*/

#include "main.h"
#include "../mrubyc_src/mrubyc.h"
#include "stm32f4_uart.h"

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart6;

UART_HANDLE * const TBL_UART_HANDLE[/*unit*/] = {
  0,

  // UART1
  &(UART_HANDLE){
    .unit_num = 1,
    .delimiter = '\n',
    .hal_uart = &huart1,
    .rxfifo_size = UART_SIZE_RXFIFO,
  },

  // UART2
  &(UART_HANDLE){
    .unit_num = 2,
    .delimiter = '\n',
    .hal_uart = &huart2,
    .rxfifo_size = UART_SIZE_RXFIFO,
  },

  0,0,0,

  // UART6
  &(UART_HANDLE){
    .unit_num = 6,
    .delimiter = '\n',
    .hal_uart = &huart6,
    .rxfifo_size = UART_SIZE_RXFIFO,
  },
};


//================================================================
/*! get the Rx FIFO write position.
*/
static inline int uart_get_wr_pos( const UART_HANDLE *hndl )
{
  return hndl->rxfifo_size - hndl->hal_uart->hdmarx->Instance->NDTR;
}


//================================================================
/*! initialize unit
*/
void uart_init(void)
{
  for( int i = 0; i < sizeof(TBL_UART_HANDLE)/sizeof(UART_HANDLE *); i++ ) {
    UART_HANDLE *hndl = TBL_UART_HANDLE[i];
    if( !hndl ) continue;

    HAL_UART_Receive_DMA(hndl->hal_uart, hndl->rxfifo, hndl->rxfifo_size);
  }
}


//================================================================
/*! set mode

  @memberof UART_HANDLE
  @param  hndl		target UART_HANDLE
  @param  baud		baud rate.
  @param  parity	0:none 1:odd 2:even
  @param  stop_bits	1 or 2
  @note いずれも設定変更しないパラメータは、-1 を渡す。
*/
int uart_setmode( const UART_HANDLE *hndl, int baud, int parity, int stop_bits )
{
  if( baud >= 0 ) {
    hndl->hal_uart->Init.BaudRate = baud;
  }

  switch( parity ) {
  case 0:
    hndl->hal_uart->Init.Parity = UART_PARITY_NONE;
    hndl->hal_uart->Init.WordLength = UART_WORDLENGTH_8B;
    break;
  case 1:
    hndl->hal_uart->Init.Parity = UART_PARITY_ODD;
    hndl->hal_uart->Init.WordLength = UART_WORDLENGTH_9B;
    break;
  case 2:
    hndl->hal_uart->Init.Parity = UART_PARITY_EVEN;
    hndl->hal_uart->Init.WordLength = UART_WORDLENGTH_9B;
    break;
  }

  if( 1 <= stop_bits && stop_bits <= 2 ) {
    static uint32_t const TBL_STOPBITS[] = { UART_STOPBITS_1, UART_STOPBITS_2 };
    hndl->hal_uart->Init.StopBits = TBL_STOPBITS[ stop_bits - 1 ];
  }

  if( HAL_UART_Init( hndl->hal_uart ) != HAL_OK ) return -1;

  return 0;
}


//================================================================
/*! Receive binary data.

  @memberof UART_HANDLE
  @param  hndl		target UART_HANDLE
  @param  buffer	pointer to buffer.
  @param  size		Size of buffer.
  @return int		Num of received bytes.

  @note			If no data received, it blocks execution.
*/
int uart_read( UART_HANDLE *hndl, void *buffer, int size )
{
  uint8_t *buf = buffer;
  int cnt = size;

  while( cnt > 0 ) {
    int ba = uart_bytes_available(hndl);
    if( ba == 0 ) {
      __NOP(); __NOP(); __NOP(); __NOP();
      continue;
    }

    if( ba < cnt ) {
      cnt -= ba;
    } else {
      ba = cnt;
      cnt = 0;
    }

    // copy fifo to buffer
    for( ; ba > 0; ba-- ) {
      *buf++ = hndl->rxfifo[hndl->rx_rd++];
      if( hndl->rx_rd >= hndl->rxfifo_size ) hndl->rx_rd = 0;
    }
  }

  return size;
}


//================================================================
/*! Send out binary data.

  @memberof UART_HANDLE
  @param  hndl		target UART_HANDLE
  @param  buffer	pointer to buffer.
  @param  size		Size of buffer.
  @return		Size of transmitted.
*/
int uart_write( UART_HANDLE *hndl, const void *buffer, int size )
{
  HAL_UART_Transmit( hndl->hal_uart, buffer, size, HAL_MAX_DELAY );

  return size;
}


//================================================================
/*! Receive string.

  @memberof UART_HANDLE
  @param  hndl		target UART_HANDLE
  @param  buffer	pointer to buffer.
  @param  size		Size of buffer.
  @return int		Num of received bytes.

  @note			If no data received, it blocks execution.
*/
int uart_gets( UART_HANDLE *hndl, void *buffer, int size )
{
  uint8_t *buf = buffer;
  int len;

  while( 1 ) {
    len = uart_can_read_line(hndl);
    if( len > 0 ) break;

    __NOP(); __NOP(); __NOP(); __NOP();
  }

  if( len >= size ) return -1;		// buffer size too small.

  // copy fifo to buffer
  for( int ba = len; ba > 0; ba-- ) {
    *buf++ = hndl->rxfifo[hndl->rx_rd++];
    if( hndl->rx_rd >= hndl->rxfifo_size ) hndl->rx_rd = 0;
  }
  *buf = '\0';

  return len;
}


//================================================================
/*! check data can be read.

  @memberof UART_HANDLE
  @param  hndl		target UART_HANDLE
  @return int           result (bool)
*/
int uart_is_readable( const UART_HANDLE *hndl )
{
  return hndl->rx_rd != uart_get_wr_pos( hndl );
}


//================================================================
/*! check data length can be read.

  @memberof UART_HANDLE
  @param  hndl		target UART_HANDLE
  @return int		result (bytes)
*/
int uart_bytes_available( const UART_HANDLE *hndl )
{
  uint16_t rx_wr = uart_get_wr_pos(hndl);

  if( hndl->rx_rd <= rx_wr ) {
    return rx_wr - hndl->rx_rd;
  }
  else {
    return hndl->rxfifo_size - hndl->rx_rd + rx_wr;
  }
}


//================================================================
/*! check data can be read a line.

  @memberof UART_HANDLE
  @param  hndl		target UART_HANDLE
  @return int		string length.
*/
int uart_can_read_line( const UART_HANDLE *hndl )
{
  uint16_t idx   = hndl->rx_rd;
  uint16_t rx_wr = uart_get_wr_pos(hndl);

  while( idx != rx_wr ) {
    if( hndl->rxfifo[idx++] == hndl->delimiter ) {
      if( hndl->rx_rd < idx ) {
	return idx - hndl->rx_rd;
      } else {
	return hndl->rxfifo_size - hndl->rx_rd + idx;
      }
    }
    if( idx >= hndl->rxfifo_size ) idx = 0;
  }

  return 0;
}


//================================================================
/*! Clear receive buffer.

  @memberof UART_HANDLE
  @param  hndl		target UART_HANDLE
*/
void uart_clear_rx_buffer( UART_HANDLE *hndl )
{
  hndl->rx_rd = uart_get_wr_pos( hndl );
}


static void c_uart_setmode(mrbc_vm *vm, mrbc_value v[], int argc);

//================================================================
/*! UART constructor

  uart1 = UART.new( id, *params )	# id = 1,2 or 6
*/
static void c_uart_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  MRBC_KW_ARG( unit );

  // get UART unit num.
  int unit_num = 1;		// default is UART1
  if( argc >= 1 && v[1].tt == MRBC_TT_INTEGER ) {
    unit_num = mrbc_integer(v[1]);
  }
  if( MRBC_KW_ISVALID(unit) ) {
    if( unit.tt != MRBC_TT_INTEGER ) goto ERROR_RETURN;
    unit_num = mrbc_integer(unit);
  }
  switch( unit_num ) {
  case 1:
  case 2:
  case 6:
    break;  // OK
  default:
    goto ERROR_RETURN;
  }

  // allocate instance with UART_HANDLE pointer.
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(UART_HANDLE *));
  *(UART_HANDLE**)(v[0].instance->data) = TBL_UART_HANDLE[unit_num];

  // process other parameters
  c_uart_setmode( vm, v, argc );
  goto RETURN;


 ERROR_RETURN:
  mrbc_raise(vm, MRBC_CLASS(ArgumentError), "UART initialize.");

 RETURN:
  MRBC_KW_DELETE( unit );
}


//================================================================
/*! set mode

  uart1.setmode( *params )
*/
static void c_uart_setmode(mrbc_vm *vm, mrbc_value v[], int argc)
{
  MRBC_KW_ARG( baudrate, baud, data_bits, stop_bits, parity, flow_control, txd_pin, rxd_pin, rts_pin, cts_pin );
  if( !MRBC_KW_END() ) goto RETURN;

  UART_HANDLE *hndl = *(UART_HANDLE **)(v[0].instance->data);
  int baud_rate = -1;

  if( MRBC_KW_ISVALID(baudrate) ) baud_rate = mrbc_integer(baudrate);
  if( MRBC_KW_ISVALID(baud) ) baud_rate = mrbc_integer(baud);
  if( MRBC_KW_ISVALID(data_bits) ) goto ERROR_NOT_IMPLEMENTED;
  if( !MRBC_KW_ISVALID(stop_bits) ) stop_bits = mrbc_integer_value(-1);
  if( !MRBC_KW_ISVALID(parity) ) parity = mrbc_integer_value(-1);
  if( MRBC_KW_ISVALID(flow_control) ) goto ERROR_NOT_IMPLEMENTED;
  if( MRBC_KW_ISVALID(txd_pin) ) goto ERROR_NOT_IMPLEMENTED;
  if( MRBC_KW_ISVALID(rxd_pin) ) goto ERROR_NOT_IMPLEMENTED;
  if( MRBC_KW_ISVALID(rts_pin) ) goto ERROR_NOT_IMPLEMENTED;
  if( MRBC_KW_ISVALID(cts_pin) ) goto ERROR_NOT_IMPLEMENTED;

  if( baud_rate > 0 && baud_rate < 2400 ) goto ERROR_ARGUMENT;

  // set to UART
  if( uart_setmode( hndl, baud_rate, mrbc_integer(parity), mrbc_integer(stop_bits) ) != 0 ) goto ERROR_ARGUMENT;
  goto RETURN;


 ERROR_NOT_IMPLEMENTED:
  mrbc_raise(vm, MRBC_CLASS(NotImplementedError), 0);
  goto RETURN;

 ERROR_ARGUMENT:
  mrbc_raise(vm, MRBC_CLASS(ArgumentError), 0);
  goto RETURN;

 RETURN:
  MRBC_KW_DELETE( baudrate, baud, data_bits, stop_bits, parity, flow_control, txd_pin, rxd_pin, rts_pin, cts_pin );
}


//================================================================
/*! read

  s = uart1.read(n)

  @param  n		Number of bytes receive.
  @return String	Received data.
*/
static void c_uart_read(mrbc_vm *vm, mrbc_value v[], int argc)
{
  UART_HANDLE *hndl = *(UART_HANDLE **)(v[0].instance->data);

  if( v[1].tt != MRBC_TT_INTEGER ) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), 0);
    return;
  }

  int read_bytes = mrbc_integer(v[1]);
  mrbc_value ret = mrbc_string_new(vm, 0, read_bytes);
  char *buf = mrbc_string_cstr(&ret);
  if( !buf ) {
    SET_RETURN(mrbc_nil_value());
    return;
  }

  uart_read( hndl, buf, read_bytes );
  buf[read_bytes] = '\0';

  SET_RETURN(ret);
}


//================================================================
/*! write

  uart1.write(s)

  @param  s	  Write data.
*/
static void c_uart_write(mrbc_vm *vm, mrbc_value v[], int argc)
{
  UART_HANDLE *hndl = *(UART_HANDLE **)(v[0].instance->data);

  if( v[1].tt != MRBC_TT_STRING ) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), 0);
    return;
  }

  int n = uart_write( hndl, mrbc_string_cstr(&v[1]), mrbc_string_size(&v[1]));
  SET_INT_RETURN(n);
}


//================================================================
/*! gets

  s = uart1.gets()

  @return String	Received string.
*/
static void c_uart_gets(mrbc_vm *vm, mrbc_value v[], int argc)
{
  UART_HANDLE *hndl = *(UART_HANDLE **)(v[0].instance->data);

  int len;
  while( 1 ) {
    len = uart_can_read_line(hndl);
    if( len > 0 ) break;

    __NOP(); __NOP(); __NOP(); __NOP();
  }

  mrbc_value ret = mrbc_string_new(vm, 0, len);
  char *buf = mrbc_string_cstr(&ret);
  if( !buf ) {
    SET_RETURN(mrbc_nil_value());
    return;
  }

  uart_read( hndl, buf, len );
  buf[len] = '\0';

  SET_RETURN(ret);
}


//================================================================
/*! write string with LF

  uart1.puts(s)

  @param  s	  Write data.
*/
static void c_uart_puts(mrbc_vm *vm, mrbc_value v[], int argc)
{
  UART_HANDLE *hndl = *(UART_HANDLE **)(v[0].instance->data);

  if( v[1].tt != MRBC_TT_STRING ) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), 0);
    return;
  }

  const char *s = mrbc_string_cstr(&v[1]);
  int len = mrbc_string_size(&v[1]);

  uart_write( hndl, s, len );
  if( len == 0 || s[len-1] != '\n' ) {
#if defined(MRBC_CONVERT_CRLF)
    uart_write( hndl, "\r\n", 2 );
#else
    uart_write( hndl, "\n", 1 );
#endif
  }
  SET_NIL_RETURN();
}


//================================================================
/*! Returns the number of incoming bytes that are waiting to be read.

  uart1.bytes_available()

  @return Integer	incomming bytes
*/
static void c_uart_bytes_available(mrbc_vm *vm, mrbc_value v[], int argc)
{
  UART_HANDLE *hndl = *(UART_HANDLE **)(v[0].instance->data);

  SET_INT_RETURN( uart_bytes_available( hndl ) );
}


//================================================================
/*! Returns the number of bytes that are waiting to be written.

  uart1.bytes_to_write()

  @return  Integer
*/
static void c_uart_bytes_to_write(mrbc_vm *vm, mrbc_value v[], int argc)
{
  // always zero return because no write buffer.
  SET_INT_RETURN( 0 );
}


//================================================================
/*! Returns true if a line of data can be read.

  uart1.can_read_line()

  @return Boolean
*/
static void c_uart_can_read_line(mrbc_vm *vm, mrbc_value v[], int argc)
{
  UART_HANDLE *hndl = *(UART_HANDLE **)(v[0].instance->data);

  int len = uart_can_read_line(hndl);

  SET_BOOL_RETURN( len );
}


//================================================================
/*! flush tx buffer.

  uart1.flush()
*/
static void c_uart_flush(mrbc_vm *vm, mrbc_value v[], int argc)
{
  // nothing to do.
}


//================================================================
/*! clear tx buffer

  uart1.clear_tx_buffer()
*/
static void c_uart_clear_tx_buffer(mrbc_vm *vm, mrbc_value v[], int argc)
{
  // nothing to do.
}


//================================================================
/*! clear rx buffer

  uart1.clear_rx_buffer()
*/
static void c_uart_clear_rx_buffer(mrbc_vm *vm, mrbc_value v[], int argc)
{
  UART_HANDLE *hndl = *(UART_HANDLE **)(v[0].instance->data);

  uart_clear_rx_buffer( hndl );
}


//================================================================
/*! send break signal.

  uart1.break()
*/
static void c_uart_send_break(mrbc_vm *vm, mrbc_value v[], int argc)
{
  mrbc_raise(vm, MRBC_CLASS(NotImplementedError), 0);
}


//================================================================
/*! initialize
*/
void mrbc_init_class_uart(void)
{
  // define class and methods.
  mrbc_class *cls = mrbc_define_class(0, "UART", 0);

  mrbc_define_method(0, cls, "new",		c_uart_new);
  mrbc_define_method(0, cls, "setmode",		c_uart_setmode);
  mrbc_define_method(0, cls, "read",		c_uart_read);
  mrbc_define_method(0, cls, "write",		c_uart_write);
  mrbc_define_method(0, cls, "gets",		c_uart_gets);
  mrbc_define_method(0, cls, "puts",		c_uart_puts);
  mrbc_define_method(0, cls, "bytes_available",	c_uart_bytes_available);
  mrbc_define_method(0, cls, "bytes_to_write",	c_uart_bytes_to_write);
  mrbc_define_method(0, cls, "can_read_line",	c_uart_can_read_line);
  mrbc_define_method(0, cls, "flush",		c_uart_flush);
  mrbc_define_method(0, cls, "clear_rx_buffer",	c_uart_clear_rx_buffer);
  mrbc_define_method(0, cls, "clear_tx_buffer",	c_uart_clear_tx_buffer);
  mrbc_define_method(0, cls, "send_break",	c_uart_send_break);

  mrbc_set_class_const(cls, mrbc_str_to_symid("NONE"), &mrbc_integer_value(0));
  mrbc_set_class_const(cls, mrbc_str_to_symid("ODD"), &mrbc_integer_value(1));
  mrbc_set_class_const(cls, mrbc_str_to_symid("EVEN"), &mrbc_integer_value(2));
}
