#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "DHT11.h"

int main(void)
{
    int humidity = 0;
    int temperature = 0;
    
    // 记录上一次成功的值，防止跳变
    int last_h = 0;
    int last_t = 0;

    OLED_Init();
    // DHT11_Init(); // 如果用了上面的新代码，Init 其实可以不要，或者留着也没事

    OLED_ShowString(1, 1, "Hum:   %");
    OLED_ShowString(2, 1, "Temp:  C");

    while (1)
    {
        // 尝试读取
        if (DHT11_Read(&humidity, &temperature)) 
        {
            // 只有校验通过才更新显示
            // 简单的滤波：如果温度突然跳变超过 10度，认为是脏数据，不显示
            // (除非是刚上电 last_t 为 0)
            if (last_t == 0 || (temperature - last_t < 10 && last_t - temperature < 10))
            {
                OLED_ShowNum(1, 6, humidity, 2);
                OLED_ShowSignedNum(2, 7, temperature, 2);
                
                last_h = humidity;
                last_t = temperature;
            }
        }
        
        // 必须延时！
        Delay_s(2);
    }
}