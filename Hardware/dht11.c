#include "stm32f10x.h"
#include "DHT11.h"
#include "Delay.h"

/* 注意：使用 PB5 时，已在 DHT11.h 指定 RCC 宏 DHT_RCC_APB2Periph */

/* 设置为输出开漏并写0/1 或 设置为输入以读取线状态 */
static void dht_gpio_output(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(DHT_RCC_APB2Periph, ENABLE);

    GPIO_InitStructure.GPIO_Pin = DHT_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD; /* 开漏输出 */
    GPIO_Init(DHT_PORT, &GPIO_InitStructure);
}

static void dht_gpio_input(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(DHT_RCC_APB2Periph, ENABLE);

    GPIO_InitStructure.GPIO_Pin = DHT_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; /* 输入，上拉（如果有外部上拉也无妨） */
    GPIO_Init(DHT_PORT, &GPIO_InitStructure);
}

static inline void dht_set_high(void)  { GPIO_SetBits(DHT_PORT, DHT_PIN); }
static inline void dht_set_low(void)   { GPIO_ResetBits(DHT_PORT, DHT_PIN); }
static inline uint8_t dht_read_pin(void){ return GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN); }

/* 初始化：把引脚设为开漏输出并释放（高电平） */
void DHT11_Init(void)
{
    dht_gpio_output();
    dht_set_high();
    Delay_ms(5);
}

/* 等待 pin 变为期望值，超时返回0；us_timeout 单位微秒 */
static int dht_wait_level(uint8_t level, uint32_t us_timeout)
{
    uint32_t t = 0;
    while (dht_read_pin() != level) {
        Delay_us(1);
        if (++t > us_timeout) return 0;
    }
    return 1;
}

/* 读取 1 个 bit：调用前应处于总线上升沿等待状态 */
static int dht_read_bit(void)
{
    /* 读取一个 bit 的流程：
       - 主机等待线由低到高（start of bit），然后延迟约 30us，读线：
         若高电平 ~26-28us -> bit 0
         若高电平 ~70us -> bit 1
    */
    /* 等待低变高（start of data bit） */
    if (!dht_wait_level(1, 80)) return -1;

    /* 进入高电平，测高电平持续时间 */
    Delay_us(30); /* 等 30us 决定是0还是1 */
    if (dht_read_pin()) {
        /* 高电平还在，认为是 1，等待高电平结束 */
        if (!dht_wait_level(0, 80)) return -1;
        return 1;
    } else {
        /* 已变低，认为是 0 */
        return 0;
    }
}

/* 读取 DHT11，返回 1 成功，0 失败 */
int DHT11_Read(int *out_h, int *out_t)
{
    uint8_t data[5] = {0};
    int i, j;

    /* 1. 主机发送起始信号：低电平至少 18ms（Datasheet），然后拉高 20-40us */
    dht_gpio_output();
    dht_set_low();
    Delay_ms(20); /* 20ms */
    dht_set_high();
    Delay_us(30);

    /* 2. 切换为输入，等待 DHT11 响应 */
    dht_gpio_input();

    /* DHT11 响应：先拉低 ~80us，然后拉高 ~80us，然后开始传输数据 */
    if (!dht_wait_level(0, 100)) return 0; /* 等待 DHT 拉低（响应） */
    if (!dht_wait_level(1, 100)) return 0; /* 等待 DHT 拉高 */
    /* 现在 DHT 开始发送数据位 */

    for (i = 0; i < 5; ++i) {
        uint8_t byte = 0;
        for (j = 0; j < 8; ++j) {
            int bit = dht_read_bit();
            if (bit < 0) return 0;
            byte = (byte << 1) | (bit & 0x01);
        }
        data[i] = byte;
    }

    /* 校验和 */
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) return 0;

    /* DHT11 数据格式：
       data[0] = 湿度整数
       data[1] = 湿度小数 (DHT11 通常为 0)
       data[2] = 温度整数
       data[3] = 温度小数 (DHT11 通常为 0)
    */
    *out_h = data[0];
    *out_t = data[2];

    /* 读取完成后把引脚设为输出并释放（可选） */
    dht_gpio_output();
    dht_set_high();

    return 1;
}

