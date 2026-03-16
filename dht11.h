#ifndef __DHT11_H
#define __DHT11_H

#include "stm32f10x.h"

// 根据实际电路修改此处
#define DHT_PORT    GPIOB
#define DHT_PIN     GPIO_Pin_12
#define DHT_RCC_APB2Periph RCC_APB2Periph_GPIOB

void DHT11_Init(void);
int DHT11_Read(int *out_h, int *out_t);

#endif