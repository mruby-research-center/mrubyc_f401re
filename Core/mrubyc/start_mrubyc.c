/*! @file
  @brief
  mruby/c startup procedure.

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
#include "mrbc_firm.h"

static void c_led_write(mrbc_vm *vm, mrbc_value v[], int argc);
static void c_sw_read(mrbc_vm *vm, mrbc_value v[], int argc);

/* mruby/c プログラムが使うワークメモリの確保 */
#define MRBC_MEMORY_SIZE (1024*30)
static uint8_t memory_pool[MRBC_MEMORY_SIZE];


/*! バイトコード書き込みモードに移行するか？

  (Strategy)
  LED1を点滅させながら、一定時間内にコンソール(UART)へ改行文字が入力されたら1を返す
*/
int check_boot_mode( void )
{
  const int MAX_WAIT_CYCLE = 256;
  int ret = 0;

  for( int i = 0; i < MAX_WAIT_CYCLE; i++ ) {
    HAL_GPIO_WritePin( GPIOA, GPIO_PIN_5,
		       ((i>>4) | (i>>1)) & 0x01 );	// Blink LED1
    if( uart_can_read_line( UART_HANDLE_CONSOLE )) {
      uart_clear_rx_buffer( UART_HANDLE_CONSOLE );
      ret = 1;
      break;
    }
    HAL_Delay( 10 );
  }
  HAL_GPIO_WritePin( GPIOA, GPIO_PIN_5, 0 );

  return ret;
}


/*! mruby/c プログラムの実行開始
*/
void start_mrubyc( void )
{
  uart_init();

  switch( check_boot_mode() ) {
  case 1:
    receive_bytecode( memory_pool, MRBC_MEMORY_SIZE );
    memset( memory_pool, 0, MRBC_MEMORY_SIZE );
    break;

  default:
    break;
  }

  mrbc_init(memory_pool, MRBC_MEMORY_SIZE);

  // 各クラスの初期化
  void mrbc_init_class_gpio(void);
  mrbc_init_class_gpio();
  void mrbc_init_class_uart(void);
  mrbc_init_class_uart();
  void mrbc_init_class_adc(void);
  mrbc_init_class_adc();
  void mrbc_init_class_pwm(void);
  mrbc_init_class_pwm();
  void mrbc_init_class_i2c(void);
  mrbc_init_class_i2c();
  void mrbc_init_class_spi(void);
  mrbc_init_class_spi();

  // ユーザ定義メソッドの登録
  mrbc_define_method(0, 0, "led_write", c_led_write);
  mrbc_define_method(0, 0, "sw_read", c_sw_read);

  // タスクの登録
#if 1
  void *task = 0;
  while( 1 ) {
    task = pickup_task( task );
    if( task == 0 ) break;

    mrbc_create_task( task, 0 );
  }

#else
  /* Or run the prepared bytecode.

     How to create "task1.c"
       mrbc --remove-lv -B task1 -o task1.c *.rb
  */
  mrbc_printf("prepared bytecode executing.\n");

  extern const uint8_t task1[];
  mrbc_create_task( task1, 0 );
#endif

  // 実行開始
  mrbc_run();
}


/*! オンボードLED ON/OFF メソッドの実装
*/
static void c_led_write(mrbc_vm *vm, mrbc_value v[], int argc)
{
  int on_off = GET_INT_ARG(1);
  HAL_GPIO_WritePin( GPIOA, GPIO_PIN_5, on_off );
}

/*! オンボードSW 読み取りメソッドの実装
*/
static void c_sw_read(mrbc_vm *vm, mrbc_value v[], int argc)
{
  switch( HAL_GPIO_ReadPin( GPIOC, GPIO_PIN_13 ) ) {
  case GPIO_PIN_SET:
    SET_INT_RETURN( 0 );
    break;
  case GPIO_PIN_RESET:
    SET_INT_RETURN( 1 );
    break;
  }
}


/*! HAL
*/
int hal_write(int fd, const void *buf, int nbytes)
{
  extern UART_HandleTypeDef huart2;
  HAL_UART_Transmit( &huart2, buf, nbytes, HAL_MAX_DELAY );

  return nbytes;
}
int _write(int file, char *ptr, int len)
{
  return hal_write(file, ptr, len);
}

int hal_flush(int fd)
{
  return 0;
}
void hal_abort(const char *s)
{
}
