#ifndef __MYFREERTOS_H
#define __MYFREERTOS_H

#include "stm32f4xx_hal.h"




#define ADC_BUF_SIZE		        100




typedef struct 
{
  uint16_t avg;
  uint8_t half;
}ADC_Info_t;



#endif
