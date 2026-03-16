#ifndef __DHT11_H
#define __DHT11_H

#include "stm32f10x.h"

/* 修改为你实际使用的端口和引脚（默认 PB5） */
#define DHT_PORT    GPIOB
#define DHT_PIN     GPIO_Pin_12

/* 如果你改端口，请同时修改下面 RCC 的使能（当前为 GPIOB） */
#define DHT_RCC_APB2Periph RCC_APB2Periph_GPIOB

/* 初始化 DHT 引脚（配置为开漏输出并释放） */
void DHT11_Init(void);

/* 读取 DHT11
   out_h -> 相对湿度整数部分（%）
   out_t -> 温度整数部分（°C）
   返回 1 成功，0 失败（校验不通过或超时） */
int DHT11_Read(int *out_h, int *out_t);

#endif