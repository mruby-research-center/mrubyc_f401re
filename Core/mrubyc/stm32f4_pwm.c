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

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;

static const uint32_t PWM_TIMER_FREQ = 84000000;	// 84MHz

/*
  PWM pin assign table
*/
static struct PWM_PIN_ASSIGN {
  PIN_HANDLE pin;	//!< Pin
  uint8_t unit_num;	//!< Timer unit number. (1..)
  uint8_t channel;	//!< Timer channel. (1..4)
} const PWM_PIN_ASSIGN[] =
{
  {{ 1,	6}, 3, 1 },	// PA6	TIM3_CH1
  {{ 1,	7}, 3, 2 },	// PA7	TIM3_CH2
  {{ 2,	6}, 4, 1 },	// PB6	TIM4_CH1
  {{ 3,	7}, 3, 2 },	// PC7	TIM3_CH2
  {{ 1,	8}, 1, 1 },	// PA8	TIM1_CH1
  {{ 2,10}, 2, 3 },	// PB10	TIM2_CH3
  {{ 2,	4}, 3, 1 },	// PB4	TIM3_CH1
  {{ 2,	5}, 3, 2 },	// PB5	TIM3_CH2
  {{ 1,	0}, 2, 1 },	// PA0	TIM2_CH1
  {{ 1,	1}, 2, 2 },	// PA1	TIM2_CH2
  {{ 2,	0}, 3, 3 },	// PB0	TIM3_CH3
};

static TIM_HandleTypeDef * const TBL_UNIT_TO_HAL_HANDLE[/* unit */] = {
  0, &htim1, &htim2, &htim3, &htim4,
};

static uint32_t const TBL_CHANNEL_TO_HAL_CHANNEL[/* channel */] = {
  0, TIM_CHANNEL_1, TIM_CHANNEL_2, TIM_CHANNEL_3, TIM_CHANNEL_4,
};

/*!
  PWM handle
*/
typedef struct PWM_HANDLE {
  PIN_HANDLE pin;	//!< pin
  uint8_t unit_num;	//!< timer unit number.
  uint8_t channel;	//!< timer channel number.
  uint16_t psc;         //!< value in the PSC register.
  uint16_t period;	//!< value in the ARR register.
  uint16_t duty;	//!< percent but stretch 100% to UINT16_MAX
} PWM_HANDLE;


//================================================================
/*! set frequency
*/
static int pwm_set_frequency( PWM_HANDLE *hndl, double freq )
{
  TIM_HandleTypeDef *htim = TBL_UNIT_TO_HAL_HANDLE[ hndl->unit_num ];

  if( freq == 0 ) {
    hndl->period = 0;
    __HAL_TIM_SET_COMPARE(htim, TBL_CHANNEL_TO_HAL_CHANNEL[ hndl->channel ], 0);
    return 0;
  }

  uint32_t ps_ar = PWM_TIMER_FREQ / freq;
  uint16_t psc = ps_ar >> 16;
  uint16_t arr = ps_ar / (psc+1) - 1;

  __HAL_TIM_SET_PRESCALER(htim, psc);
  __HAL_TIM_SET_AUTORELOAD(htim, arr);
  __HAL_TIM_SET_COMPARE(htim, TBL_CHANNEL_TO_HAL_CHANNEL[ hndl->channel ],
                        (uint32_t)arr * hndl->duty / UINT16_MAX);
  hndl->psc = psc;
  hndl->period = arr;

  return 0;
}

//================================================================
/*! set period (us)
*/
static int pwm_set_period_us( PWM_HANDLE *hndl, unsigned int us )
{
  double freq = (us == 0 ? 0 : 1e6 / us);
  return pwm_set_frequency( hndl, freq );
}

//================================================================
/*! set duty cycle in percentage.
*/
static int pwm_set_duty( PWM_HANDLE *hndl, double duty )
{
  TIM_HandleTypeDef *htim = TBL_UNIT_TO_HAL_HANDLE[ hndl->unit_num ];

  hndl->duty = duty / 100 * UINT16_MAX;
  __HAL_TIM_SET_COMPARE(htim, TBL_CHANNEL_TO_HAL_CHANNEL[ hndl->channel ],
                        (uint32_t)hndl->period * duty / 100);
  return 0;
}


//================================================================
/*! set pulse width.
*/
static int pwm_set_pulse_width_us( PWM_HANDLE *hndl, unsigned int us )
{
  TIM_HandleTypeDef *htim = TBL_UNIT_TO_HAL_HANDLE[ hndl->unit_num ];
  uint16_t pw_cnt = (us * (PWM_TIMER_FREQ / 1000000)) / (hndl->psc + 1) - 1;

  __HAL_TIM_SET_COMPARE(htim, TBL_CHANNEL_TO_HAL_CHANNEL[ hndl->channel ],
			pw_cnt);
  return 0;
}


//================================================================
/*! constructor

  pwm1 = PWM.new("PA6")
  pwm1 = PWM.new("PA6", frequency:440, duty:30 )
*/
static void c_pwm_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  MRBC_KW_ARG(frequency, freq, duty);
  if( !MRBC_KW_END() ) goto RETURN;
  if( argc == 0 ) goto ERROR_RETURN;

  PIN_HANDLE pin;
  if( gpio_set_pin_handle( &pin, &v[1] ) != 0 ) goto ERROR_RETURN;

  // find from PWM_PIN_ASSIGN table
  static const int NUM = sizeof(PWM_PIN_ASSIGN)/sizeof(PWM_PIN_ASSIGN[0]);
  int i;
  for( i = 0; i < NUM; i++ ) {
    if( (PWM_PIN_ASSIGN[i].pin.port == pin.port) &&
	(PWM_PIN_ASSIGN[i].pin.num  == pin.num) ) break;
  }
  if( i == NUM ) goto ERROR_RETURN;

  // allocate instance with PWM_HANDLE.
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(PWM_HANDLE));
  PWM_HANDLE *hndl = (PWM_HANDLE *)(v[0].instance->data);

  hndl->pin = pin;
  hndl->unit_num = PWM_PIN_ASSIGN[i].unit_num;
  hndl->channel  = PWM_PIN_ASSIGN[i].channel;
  hndl->duty = UINT16_MAX / 2;

  // set frequency and duty
  if( MRBC_ISNUMERIC(frequency) ) {
    pwm_set_frequency( hndl, MRBC_TO_FLOAT(frequency));
  }
  if( MRBC_ISNUMERIC(freq) ) {
    pwm_set_frequency( hndl, MRBC_TO_FLOAT(freq));
  }
  if( MRBC_ISNUMERIC(duty) ) {
    pwm_set_duty( hndl, MRBC_TO_FLOAT(duty));
  }

  // set GPIO pin.
  gpio_setmode_pwm( &pin, hndl->unit_num );

  // start!
  if( hndl->period != 0 ) {
    HAL_TIM_PWM_Start( TBL_UNIT_TO_HAL_HANDLE[hndl->unit_num],
                       TBL_CHANNEL_TO_HAL_CHANNEL[hndl->channel] );
  }
  goto RETURN;


 ERROR_RETURN:
  mrbc_raise(vm, MRBC_CLASS(ArgumentError), "PWM initialize.");

 RETURN:
  MRBC_KW_DELETE(frequency, freq, duty);
}


//================================================================
/*! set frequency

  pwm1.frequency( 440 )
*/
static void c_pwm_frequency(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PWM_HANDLE *hndl = (PWM_HANDLE *)(v[0].instance->data);

  if( MRBC_ISNUMERIC(v[1]) ) {
    pwm_set_frequency( hndl, MRBC_TO_FLOAT(v[1]) );
  }
}


//================================================================
/*! set period in microsecond

  pwm1.period_us( 2273 )
*/
static void c_pwm_period_us(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PWM_HANDLE *hndl = (PWM_HANDLE *)(v[0].instance->data);

  if( MRBC_ISNUMERIC(v[1]) ) {
    pwm_set_period_us( hndl, MRBC_TO_INT(v[1]));
  }
}


//================================================================
/*! PWM set duty cycle as percentage.

  pwm1.duty( 50 )
*/
static void c_pwm_duty(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PWM_HANDLE *hndl = (PWM_HANDLE *)(v[0].instance->data);

  if( MRBC_ISNUMERIC(v[1]) ) {
    pwm_set_duty( hndl, MRBC_TO_FLOAT(v[1]));
  }
}


//================================================================
/*! PWM set pulse width in microsecond.

  pwm1.pulse_width_us( 20 )
*/
static void c_pwm_pulse_width_us(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PWM_HANDLE *hndl = (PWM_HANDLE *)(v[0].instance->data);

  if( MRBC_ISNUMERIC(v[1]) ) {
    pwm_set_pulse_width_us( hndl, MRBC_TO_INT(v[1]));
  }
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
