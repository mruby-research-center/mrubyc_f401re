#include "main.h"
#include "../mrubyc_src/mrubyc.h"

static void c_led_write(mrbc_vm *vm, mrbc_value v[], int argc);
static void c_sw_read(mrbc_vm *vm, mrbc_value v[], int argc);

/* mruby/c プログラムが使うワークメモリの確保 */
#define MRBC_MEMORY_SIZE (1024*30)
static uint8_t memory_pool[MRBC_MEMORY_SIZE];

/*! mruby/c プログラムの実行開始
*/
void start_mrubyc( void )
{
  mrbc_init(memory_pool, MRBC_MEMORY_SIZE);

  // 各クラスの初期化
  void mrbc_init_class_gpio(void);
  mrbc_init_class_gpio();

  // ユーザ定義メソッドの登録
  mrbc_define_method(0, 0, "led_write", c_led_write);
  mrbc_define_method(0, 0, "sw_read", c_sw_read);

  // タスクの登録
  extern const uint8_t sample1[];
  mrbc_create_task( sample1, 0 );

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
