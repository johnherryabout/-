#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"


int16_t Num;

int main(void)
{
	/*模块初始化*/
	OLED_Init();		//OLED初始化
	OLED_ShowString(1,1,"count:");
	
	while (1)
	{
		OLED_ShowSignedNum(1,5,Num,5);
	}
}

