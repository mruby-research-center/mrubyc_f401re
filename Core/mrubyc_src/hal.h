#ifndef MRBC_SRC_HAL_H_
#define MRBC_SRC_HAL_H_

#include "main.h"

#define MRBC_TICK_UNIT 1
#define MRBC_TIMESLICE_TICK_COUNT 10

#define hal_init()        ((void)0)
#define hal_enable_irq()  __enable_irq()
#define hal_disable_irq() __disable_irq()
//#define hal_idle_cpu()    ((void)0)
//#define hal_idle_cpu()    HAL_PWR_EnterSTOPMode(PWR_MAINREGULATOR_ON,PWR_STOPENTRY_WFI)
#define hal_idle_cpu()    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI)
//#define hal_idle_cpu()    HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI)


int hal_write(int fd, const void *buf, int nbytes);
int hal_flush(int fd);
void hal_abort(const char *s);

#endif /* MRUBYC_SRC_HAL_H_ */
