#include "sccb.h"

void sccb_init(void)
{
    SCCB_SCL_GPIO_CLK_ENABLE();    
    SCCB_SDA_GPIO_CLK_ENABLE();  

    GPIO_InitTypeDef gpio_initure;
    gpio_initure.Pin=SCCB_SCL_GPIO_PIN;
    gpio_initure.Mode=GPIO_MODE_OUTPUT_PP;      
    gpio_initure.Pull=GPIO_PULLUP;          
    gpio_initure.Speed=GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(SCCB_SCL_GPIO_PORT,&gpio_initure);   

    gpio_initure.Pin=SCCB_SDA_GPIO_PIN;
 #if OV_SCCB_TYPE_NOD
     gpio_initure.Mode = GPIO_MODE_OUTPUT_PP;      
 #else
     gpio_initure.Mode = GPIO_MODE_OUTPUT_OD;
 #endif
    HAL_GPIO_Init(SCCB_SDA_GPIO_PORT,&gpio_initure);   

#if OV_SCCB_TYPE_NOD
    SCCB_SDA_OUT();     
    delay_us(50);
#endif
    sccb_stop();  
}

static void sccb_delay(void)
{
    delay_us(50);
}

void sccb_start(void)
{
    SCCB_SDA(1);
    SCCB_SCL(1);
    sccb_delay();
    SCCB_SDA(0);  
    sccb_delay();
    SCCB_SCL(0); 
}

void sccb_stop(void)
{
    SCCB_SDA(0);  
    sccb_delay();
    SCCB_SCL(1);
    sccb_delay();
    SCCB_SDA(1);   
    sccb_delay();
}

void sccb_nack(void)
{
    sccb_delay();
    SCCB_SDA(1);   
    SCCB_SCL(1);
    sccb_delay();
    SCCB_SCL(0);   
    sccb_delay();
    SCCB_SDA(0);
    sccb_delay();
}

uint8_t sccb_send_byte(uint8_t data)
{
    uint8_t t, res;
    
    for (t = 0; t < 8; t++)
    {
        SCCB_SDA((data & 0x80) >> 7); 
        sccb_delay();
        SCCB_SCL(1);
        sccb_delay();
        SCCB_SCL(0);
        data <<= 1;    
    }
#if OV_SCCB_TYPE_NOD
    SCCB_SDA_IN();      
    sccb_delay();
#endif
    SCCB_SDA(1);        
    sccb_delay();
    SCCB_SCL(1);        
    sccb_delay();

    if (SCCB_READ_SDA)
    {
        res = 1;       
    }
    else 
    {
        res = 0;        
    } 

    SCCB_SCL(0);
#if OV_SCCB_TYPE_NOD
    SCCB_SDA_OUT();      
    delay_us(50);
#endif    
    return res;
}

uint8_t sccb_read_byte(void)
{
    uint8_t i, receive = 0;
#if OV_SCCB_TYPE_NOD
    SCCB_SDA_IN();         
    delay_us(50);
#endif
    for (i = 0; i < 8; i++)
    {
        sccb_delay();
        receive <<= 1;    
        SCCB_SCL(1);

        if (SCCB_READ_SDA)
        {
            receive++;
        }
        sccb_delay();
        SCCB_SCL(0);
    }
#if OV_SCCB_TYPE_NOD
    SCCB_SDA_OUT();       
    delay_us(50);
#endif
    return receive;
}

















