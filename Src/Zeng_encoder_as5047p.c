#include "Zeng_encoder_as5047p.h"

#include "cmsis_os.h"
#include "usart.h"
#include "spi.h"
#include "usart.h"

#include <string.h>

#define ANGLEUNC        (0x3ffe)
#define ANGLECOM        (0x3fff)

//设计通信协议
typedef struct _TRANS_HEADER{
    uint8_t magic[2];
    uint8_t data_len;
    uint8_t data_type;
    uint32_t system_time_ms;
}TRANS_HEADER;

typedef struct _TRANS_DATA_UPPER{
    uint8_t magic[2];
    uint16_t angle_com;
    uint16_t angle_unc;
    uint8_t reserve;
    uint8_t CheckSum;
}TRANS_DATA_UPPER;


static osThreadId EncoderTaskHandle;

const uint16_t AS5047P_config[2] = {ANGLECOM ,ANGLEUNC};
static uint16_t AS5047P_angle_data;
static uint8_t get_data_flag = 0;
static uint8_t send_buff[50] = {0};

HAL_StatusTypeDef Zeng_encoder_as5047p_read_data(void)
{
    HAL_StatusTypeDef ret = 0;
    SPI_ENCODE_CS();
    ret = HAL_SPI_TransmitReceive_DMA(&hspi3, (uint8_t*)&AS5047P_config[0], (uint8_t*)&AS5047P_angle_data, 1);
    return ret;
}

void Zeng_encoder_as5047p_get_data_callback(void)
{
    SPI_ENCODE_UNCS();
    get_data_flag = 1;
}


uint8_t Zeng_cal_CheckSum(uint8_t *start, uint8_t len)
{
    uint8_t sum = 0;
    uint8_t i = 0;
    for(i=0; i<len; i++)
    {
        sum += start[i];
    }
    return sum;
}

void Zeng_Encoder_data_tran_upper(void)
{
    TRANS_DATA_UPPER data = {0};
    data.magic[0] = 0xaa;
    data.magic[1] = 0x55;
    data.angle_com = AS5047P_angle_data<<2;
    data.angle_unc = 0;

    data.CheckSum = Zeng_cal_CheckSum((uint8_t*)&data, sizeof(data)-2);

    memcpy(&send_buff, (void*)&data, sizeof(data));
    Zeng_uart_os_tran(&huart3, send_buff, sizeof(data));
}








void zeng_encoder_data_task(void const *argument)
{
    while(1)
    {
        Zeng_encoder_as5047p_read_data();
        osDelay(2);
        if(get_data_flag){
            get_data_flag = 0;
            //tran data by usart port
            Zeng_Encoder_data_tran_upper();
        }
    }
}

void Zeng_encoder_as5047p_Init(void)
{
    osThreadDef(Encoder_task, zeng_encoder_data_task, osPriorityAboveNormal, 0, 256);
    EncoderTaskHandle = osThreadCreate(osThread(Encoder_task), NULL);
}



