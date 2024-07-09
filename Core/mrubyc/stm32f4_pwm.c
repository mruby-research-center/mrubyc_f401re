/*! @file
  @brief
  PWM class.

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

static const uint32_t PWM_TIMER_FREQ = 84000000;	// 84MHz

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;


//================================================================
/*! constructor

  pwm1 = PWM.new("PA6")
  pwm1 = PWM.new("PA6", frequency:440, duty:30 )
*/
static void c_pwm_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
}


//================================================================
/*! set frequency

  pwm1.frequency( 440 )
*/
static void c_pwm_frequency(mrbc_vm *vm, mrbc_value v[], int argc)
{
}


//================================================================
/*! set period in microsecond

  pwm1.period_us( 2273 )
*/
static void c_pwm_period_us(mrbc_vm *vm, mrbc_value v[], int argc)
{
}


//================================================================
/*! PWM set duty cycle as percentage.

  pwm1.duty( 50 )
*/
static void c_pwm_duty(mrbc_vm *vm, mrbc_value v[], int argc)
{
}


//================================================================
/*! PWM set pulse width in microsecond.

  pwm1.pulse_width_us( 20 )
*/
static void c_pwm_pulse_width_us(mrbc_vm *vm, mrbc_value v[], int argc)
{
}


//================================================================
/*! Initializer
*/
void mrbc_init_class_pwm(void)
{
  mrbc_class *cls = mrbc_define_class(0, "PWM", 0);

  mrbc_define_method(0, cls, "new", c_pwm_new);
  mrbc_define_method(0, cls, "frequency", c_pwm_frequency);
  mrbc_define_method(0, cls, "period_us", c_pwm_period_us);
  mrbc_define_method(0, cls, "duty", c_pwm_duty);
  mrbc_define_method(0, cls, "pulse_width_us", c_pwm_pulse_width_us);
}
