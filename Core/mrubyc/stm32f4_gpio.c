/*! @file
  @brief
  GPIO class.

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

static uint16_t const TBL_NUM_TO_STM32PIN[/* num */] = {
  GPIO_PIN_0,  GPIO_PIN_1,  GPIO_PIN_2,  GPIO_PIN_3,
  GPIO_PIN_4,  GPIO_PIN_5,  GPIO_PIN_6,  GPIO_PIN_7,
  GPIO_PIN_8,  GPIO_PIN_9,  GPIO_PIN_10, GPIO_PIN_11,
  GPIO_PIN_12, GPIO_PIN_13, GPIO_PIN_14, GPIO_PIN_15 };

static GPIO_TypeDef * const TBL_PORT_TO_STM32GPIO[/* port */] = {
  0, GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, 0, 0, GPIOH,
};

static uint8_t const TBL_ARDUINO_DIGITAL_PIN_TO_PIN[/* num */] = {
  0x03,  // D0 => PA3
  0x02,  // D1 => PA2
  0x0A,  // D2 => PA10
  0x13,  // D3 => PB3
  0x15,  // D4 => PB5
  0x14,  // D5 => PB4
  0x1A,  // D6 => PB10
  0x08,  // D7 => PA8
  0x09,  // D8 => PA9
  0x27,  // D9 => PC7
  0x16,  // D10 => PB6
  0x07,  // D11 => PA7
  0x06,  // D12 => PA6
  0x05,  // D13 => PA5
  0x19,  // D14 => PB9
  0x18,  // D15 => PB8
};



//================================================================
/*! PIN handle setter

  @param  pin		dist.
  @param  val		src.
  @retval 0		No error.
*/
int gpio_set_pin_handle( PIN_HANDLE *pin, const mrbc_value *val )
{
  if( val->tt == MRBC_TT_STRING ){
    const char *s = mrbc_string_cstr(val);
    // in case of "PA0"
    if( s[0] == 'P' && ('A' <= s[1] && s[1] <= 'Z') ) {
      pin->port = s[1] - 'A' + 1;
      pin->num = mrbc_atoi( s+2, 10 );
      if( pin->num > 15 ) goto ERROR_RETURN;
      return 0;
    }
  } else if( val->tt == MRBC_TT_INTEGER ) {
    int n = mrbc_integer(*val);
    // in case of Arduino pin number.
    if( n >= 0 && n <= 15 ) {
      n = TBL_ARDUINO_DIGITAL_PIN_TO_PIN[n];
      pin->port = ((n >> 4) & 0x03) + 1;
      pin->num = (n & 0x0F);
      return 0;
    }
    // goto ERROR_RETURN;
  }

 ERROR_RETURN:
  pin->port = 0;
  pin->num = 0;

  return -1;
}


//================================================================
/*! set (change) mode

  @param  pin	target pin.
  @param  mode	mode. Sepcified by GPIO_* constant.
  @return int	zero is no error.
*/
int gpio_setmode( const PIN_HANDLE *pin, unsigned int mode )
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = TBL_NUM_TO_STM32PIN[pin->num];

  if( mode & (GPIO_IN|GPIO_OUT|GPIO_ANALOG|GPIO_HIGH_Z|GPIO_OPEN_DRAIN) ) {
    if( mode & GPIO_ANALOG ) {
      GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;

    } else if( mode & GPIO_IN ) {
      GPIO_InitStruct.Mode = GPIO_MODE_INPUT;

    } else if( mode & GPIO_OUT ) {
      GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;

    } else if( mode & GPIO_OPEN_DRAIN ) {
      GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;

    } else {
      return -1;
    }

    GPIO_InitStruct.Pull = GPIO_NOPULL;
  }

  if( mode & GPIO_PULL_UP ) {
    GPIO_InitStruct.Pull = GPIO_PULLUP;
  }
  if( mode & GPIO_PULL_DOWN ) {
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  }
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  HAL_GPIO_Init( TBL_PORT_TO_STM32GPIO[pin->port], &GPIO_InitStruct);
  return 0;
}


//================================================================
/*! set mode to PWM

  @param  pin	target pin.
  @param  unit  timer unit number.
  @return int	zero is no error.
*/
int gpio_setmode_pwm( const PIN_HANDLE *pin, int unit_num )
{
  static uint8_t const PWM_GPIO_ALT[/* unit */] = {
    0, GPIO_AF1_TIM1, GPIO_AF1_TIM2, GPIO_AF2_TIM3, GPIO_AF2_TIM4,
  };

  GPIO_InitTypeDef GPIO_InitStruct = {
    .Pin = TBL_NUM_TO_STM32PIN[pin->num],
    .Mode = GPIO_MODE_AF_PP,
    .Pull = GPIO_NOPULL,
    .Speed = GPIO_SPEED_FREQ_LOW,
    .Alternate = PWM_GPIO_ALT[unit_num],
  };
  HAL_GPIO_Init( TBL_PORT_TO_STM32GPIO[pin->port], &GPIO_InitStruct);

  return 0;
}


//================================================================
/*! constructor

  gpio1 = GPIO.new("PA0", GPIO::OUT ) # specific pin. 
  gpio1 = GPIO.new(0, GPIO::OUT )	    # Arduino pin.
*/
static void c_gpio_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(PIN_HANDLE));

  PIN_HANDLE *pin = (PIN_HANDLE *)v[0].instance->data;

  if( argc != 2 ) goto ERROR_RETURN;
  if( gpio_set_pin_handle( pin, &v[1] ) != 0 ) goto ERROR_RETURN;
  if( (mrbc_integer(v[2]) & (GPIO_IN|GPIO_OUT|GPIO_HIGH_Z)) == 0 ) goto ERROR_RETURN;
  if( gpio_setmode( pin, mrbc_integer(v[2]) ) < 0 ) goto ERROR_RETURN;
  return;

 ERROR_RETURN:
  mrbc_raise(vm, MRBC_CLASS(ArgumentError), "GPIO initialize");
}


//================================================================
/*! setmode

  GPIO.setmode( "PA0", GPIO::IN )	# class method
  gpio1.setmode( GPIO::PULL_UP )	# instance method
*/
static void c_gpio_setmode(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PIN_HANDLE pin;

  if( v[0].tt == MRBC_TT_OBJECT ) goto INSTANCE_METHOD_MODE;

  /*
    Class method mode.
  */
  if( argc != 2 ) goto ERROR_RETURN;
  if( gpio_set_pin_handle( &pin, &v[1] ) != 0 ) goto ERROR_RETURN;
  if( v[2].tt != MRBC_TT_INTEGER ) goto ERROR_RETURN;
  if( gpio_setmode( &pin, mrbc_integer(v[2]) ) != 0 ) goto ERROR_RETURN;
  SET_NIL_RETURN();
  return;

  /*
    Instance method mode.
  */
 INSTANCE_METHOD_MODE:
  pin = *(PIN_HANDLE *)v[0].instance->data;

  if( v[1].tt != MRBC_TT_INTEGER ) goto ERROR_RETURN;
  if( gpio_setmode( &pin, mrbc_integer(v[1]) ) != 0 ) goto ERROR_RETURN;
  SET_NIL_RETURN();
  return;

 ERROR_RETURN:
  mrbc_raise(vm, MRBC_CLASS(ArgumentError), "GPIO Can't setup");
}


//================================================================
/*! read_at -> Integer

  v1 = GPIO.read_at("PA0")	# read from "PA0" pin.
*/
static void c_gpio_read_at(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PIN_HANDLE pin;

  if( gpio_set_pin_handle( &pin, &v[1] ) == 0 ) {
    SET_INT_RETURN( HAL_GPIO_ReadPin( TBL_PORT_TO_STM32GPIO[pin.port],
                                      TBL_NUM_TO_STM32PIN[pin.num] ));
  } else {
    SET_NIL_RETURN();
  }
}


//================================================================
/*! high_at? -> bool

  v1 = GPIO.high_at?("PA0")
*/
static void c_gpio_high_at(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PIN_HANDLE pin;

  if( gpio_set_pin_handle( &pin, &v[1] ) == 0 ) {
    SET_BOOL_RETURN( HAL_GPIO_ReadPin( TBL_PORT_TO_STM32GPIO[pin.port],
                                       TBL_NUM_TO_STM32PIN[pin.num] ));
  } else {
    SET_NIL_RETURN();
  }
}


//================================================================
/*! low_at? -> bool

  v1 = GPIO.low_at?("PA0")
*/
static void c_gpio_low_at(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PIN_HANDLE pin;

  if( gpio_set_pin_handle( &pin, &v[1] ) == 0 ) {
    SET_BOOL_RETURN(!HAL_GPIO_ReadPin( TBL_PORT_TO_STM32GPIO[pin.port],
                                       TBL_NUM_TO_STM32PIN[pin.num] ));
  } else {
    SET_NIL_RETURN();
  }
}


//================================================================
/*! write_at

  v1 = GPIO.write_at("PA0", 0 )      # output zero to "PA0" pin.
*/
static void c_gpio_write_at(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PIN_HANDLE pin;

  if( (gpio_set_pin_handle( &pin, &v[1] ) != 0) ||
      (v[2].tt != MRBC_TT_INTEGER) ) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), 0);
    return;
  }

  int val = mrbc_integer(v[2]);
  if( 0 <= val && val <= 1 ) {
    HAL_GPIO_WritePin( TBL_PORT_TO_STM32GPIO[pin.port],
                       TBL_NUM_TO_STM32PIN[pin.num], val );
  } else {
    mrbc_raise(vm, MRBC_CLASS(RangeError), 0);
  }
}


//================================================================
/*! read

  x = gpio1.read() -> Integer
*/
static void c_gpio_read(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PIN_HANDLE *pin = (PIN_HANDLE *)v[0].instance->data;

  SET_INT_RETURN( HAL_GPIO_ReadPin( TBL_PORT_TO_STM32GPIO[pin->port],
                                    TBL_NUM_TO_STM32PIN[pin->num] ));
}


//================================================================
/*! high?

  if gpio1.high?() ...
*/
static void c_gpio_high(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PIN_HANDLE *pin = (PIN_HANDLE *)v[0].instance->data;

  SET_BOOL_RETURN( HAL_GPIO_ReadPin( TBL_PORT_TO_STM32GPIO[pin->port],
                                     TBL_NUM_TO_STM32PIN[pin->num] ));
}


//================================================================
/*! low?

  if gpio1.low?() ...
*/
static void c_gpio_low(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PIN_HANDLE *pin = (PIN_HANDLE *)v[0].instance->data;

  SET_BOOL_RETURN(!HAL_GPIO_ReadPin( TBL_PORT_TO_STM32GPIO[pin->port],
                                     TBL_NUM_TO_STM32PIN[pin->num] ));
}


//================================================================
/*! write

  gpio1.write( 0 or 1 )
*/
static void c_gpio_write(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PIN_HANDLE *pin = (PIN_HANDLE *)v[0].instance->data;

  if( v[1].tt != MRBC_TT_INTEGER ) return;

  int val = mrbc_integer(v[1]);
  if( 0 <= val && val <= 1 ) {
    HAL_GPIO_WritePin( TBL_PORT_TO_STM32GPIO[pin->port],
                       TBL_NUM_TO_STM32PIN[pin->num], val );
  } else {
    mrbc_raise(vm, MRBC_CLASS(RangeError), 0);
  }
}


//================================================================
/*! set up the GPIO class.
*/
void mrbc_init_class_gpio( void )
{
  mrbc_class *cls = mrbc_define_class(0, "GPIO", 0);

  mrbc_define_method(0, cls, "new", c_gpio_new);
  mrbc_define_method(0, cls, "setmode", c_gpio_setmode);
  mrbc_define_method(0, cls, "read_at", c_gpio_read_at);
  mrbc_define_method(0, cls, "high_at?", c_gpio_high_at);
  mrbc_define_method(0, cls, "low_at?", c_gpio_low_at);
  mrbc_define_method(0, cls, "write_at", c_gpio_write_at);

  mrbc_define_method(0, cls, "read", c_gpio_read);
  mrbc_define_method(0, cls, "high?", c_gpio_high);
  mrbc_define_method(0, cls, "low?", c_gpio_low);
  mrbc_define_method(0, cls, "write", c_gpio_write);

  mrbc_set_class_const(cls, mrbc_str_to_symid("IN"),         &mrbc_integer_value(GPIO_IN));
  mrbc_set_class_const(cls, mrbc_str_to_symid("OUT"),        &mrbc_integer_value(GPIO_OUT));
  mrbc_set_class_const(cls, mrbc_str_to_symid("HIGH_Z"),     &mrbc_integer_value(GPIO_HIGH_Z));
  mrbc_set_class_const(cls, mrbc_str_to_symid("PULL_UP"),    &mrbc_integer_value(GPIO_PULL_UP));
  mrbc_set_class_const(cls, mrbc_str_to_symid("PULL_DOWN"),  &mrbc_integer_value(GPIO_PULL_DOWN));
  mrbc_set_class_const(cls, mrbc_str_to_symid("OPEN_DRAIN"), &mrbc_integer_value(GPIO_OPEN_DRAIN));
}
