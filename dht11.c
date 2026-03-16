#include "stm32f10x.h"
#include "DHT11.h"
#include "Delay.h"

// 宏定义方便操作
#define DHT_OUT_HIGH() GPIO_SetBits(DHT_PORT, DHT_PIN)
#define DHT_OUT_LOW()  GPIO_ResetBits(DHT_PORT, DHT_PIN)
#define DHT_IN_READ()  GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN)

// 配置为输出模式
static void DHT_Mode_Out_PP(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = DHT_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // 推挽输出更强劲
    GPIO_Init(DHT_PORT, &GPIO_InitStructure);
}

// 配置为输入模式
static void DHT_Mode_IPU(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = DHT_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 上拉输入
    GPIO_Init(DHT_PORT, &GPIO_InitStructure);
}

// 读取一个字节
static uint8_t DHT_Read_Byte(void)
{
    uint8_t i, dat = 0;
    for (i = 0; i < 8; i++)
    {
        // 1. 等待低电平结束（等待上一位数据结束）
        // 为了防止死循环，这里加一个超时计数
        uint8_t retry = 0;
        while (DHT_IN_READ() == 0 && retry < 100) {
            Delay_us(1);
            retry++;
        }

        // 2. 延时 40us 用于判断是 0 还是 1
        // 0 的高电平约 26us，1 的高电平约 70us
        // 延时 40us 后，如果是 0 则已经变低，如果是 1 则还是高
        Delay_us(40); 

        // 3. 判断电平
        if (DHT_IN_READ() == 1)
        {
            dat |= (1 << (7 - i)); // 高位在前
            
            // 等待高电平结束
            retry = 0;
            while (DHT_IN_READ() == 1 && retry < 100) {
                Delay_us(1);
                retry++;
            }
        }
    }
    return dat;
}

int DHT11_Read(int *out_h, int *out_t)
{
    uint8_t buf[5];
    uint8_t i;

    // 1. 主机发送开始信号
    DHT_Mode_Out_PP();
    DHT_OUT_LOW();
    Delay_ms(20);  // 拉低至少 18ms
    DHT_OUT_HIGH();
    Delay_us(30);  // 拉高 20-40us

    // 2. 主机设为输入，准备接收响应
    DHT_Mode_IPU();

    // 3. 关键：关中断！防止系统滴答时钟打断时序
    __disable_irq();

    // 4. 检测 DHT11 的响应信号
    // 等待 DHT11 拉低（响应信号开始）
    if (DHT_IN_READ() == 0) // 如果已经是低电平，说明响应正常
    {
        // 等待 DHT11 变高（80us 低电平结束）
        uint8_t retry = 0;
        while (DHT_IN_READ() == 0 && retry < 100) {
            Delay_us(1);
            retry++;
        }
        
        // 等待 DHT11 变低（80us 高电平结束，准备开始发数据）
        retry = 0;
        while (DHT_IN_READ() == 1 && retry < 100) {
            Delay_us(1);
            retry++;
        }

        // 5. 开始读取 40 位数据
        for (i = 0; i < 5; i++)
        {
            buf[i] = DHT_Read_Byte();
        }

        // 6. 读取结束，拉高总线（可选）
        DHT_Mode_Out_PP();
        DHT_OUT_HIGH();

        // 7. 恢复中断
        __enable_irq();

        // 8. 校验
        // 校验和 = 湿整 + 湿小 + 温整 + 温小
        if ((buf[0] + buf[1] + buf[2] + buf[3]) == buf[4])
        {
            *out_h = buf[0]; // 湿度
            *out_t = buf[2]; // 温度
            return 1; // 成功
        }
    }
    
    // 如果失败，也要恢复中断
    __enable_irq();
    return 0; // 失败
}