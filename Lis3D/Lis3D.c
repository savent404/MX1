#include "lis3d.h"
#include "stm32f1xx_hal.h"
//Lis3dSourceData SourceData;
/*
static void SPI1_Init(void)
{
  //SPI1 Init
  GPIO_Init(GPIOB, GPIO_Pin_4, GPIO_Mode_Out_PP_High_Fast); // CS

  GPIO_Init(GPIOB, GPIO_Pin_5, GPIO_Mode_Out_PP_High_Fast); // SCLK
  GPIO_Init(GPIOB, GPIO_Pin_6, GPIO_Mode_Out_PP_High_Fast); // MOSI
  GPIO_Init(GPIOB, GPIO_Pin_7, GPIO_Mode_In_FL_No_IT);

  // CLK_PeripheralClockConfig(CLK_Peripheral_SPI, ENABLE);
  CLK->PCKENR |= ((uint8_t)CLK_Peripheral_SPI);
  SPI1_CS_High();
  SPI_DeInit();
  // SPI_Init(SPI_FirstBit_MSB, SPI_BaudRatePrescaler_2, SPI_Mode_Master,
  //        SPI_CPOL_High, SPI_CPHA_2Edge,SPI_Direction_2Lines_FullDuplex,
  //        SPI_NSS_Soft);
  SPI->CR1 = (uint8_t)((uint8_t)(SPI_FirstBit_MSB) | (uint8_t)(SPI_BaudRatePrescaler_2) | (uint8_t)(SPI_CPOL_High) | (uint8_t)(SPI_CPHA_2Edge));
  SPI->CR2 = (uint8_t)((uint8_t)(SPI_Direction_2Lines_FullDuplex) | (uint8_t)(SPI_NSS_Soft));
  SPI->CR2 |= (uint8_t)SPI_CR2_SSI;
  SPI->CR1 |= (uint8_t)(SPI_Mode_Master);
  SPI_Cmd(ENABLE);
}
*/

void Lis3d_Init(void)
{
  LIS3DH_SPI_WR(0x23, 0xC8); //+/- 2g,4线spi
  LIS3DH_SPI_WR(0x20, 0x47); //50hz Low power mode,XYZ axis enabled
  LIS3DH_SPI_WR(0x24, 0x04); //打开6轴

  LIS3DH_SPI_WR(0x21, 0x05); //High pass filter enabled for CLICK and AOI1
  LIS3DH_SPI_WR(0x22, 0x40); //AOI interrupt on INT1

  LIS3DH_SPI_WR(0x24, 0x00);
  LIS3DH_SPI_WR(0x25, 0x80); //使能CLICK to INT2中断
  LIS3DH_SPI_WR(0x32, 0x09); // Threshold = 125 mg//8A
  LIS3DH_SPI_WR(0x33, 0x10); //100 * (1/50) = 2s 这里填的是最大值

  LIS3DH_SPI_WR(0x30, 0x95); //INT1_CFG//25

  LIS3DH_SPI_WR(0x38, 0x2A); //使能Z轴double
  LIS3DH_SPI_WR(0x3A, 0x18); //双击门槛限定  (4/65536) * 5000 / (4/256) ; 5000就是设定门限
  LIS3DH_SPI_WR(0x3B, 0x04); //双击时间限定
  LIS3DH_SPI_WR(0x3C, 0x04); //双击延迟限定  5*(1/50)
  LIS3DH_SPI_WR(0x3D, 0x08); //TIMEWIND
}

static unsigned char LIS3DH_SPI_RD(unsigned char addr)
{
  unsigned char temp;
  SPI1_CS_Low();
  //Delay_Spi(10);
  SPI_LIS3DH_SendByte((addr | 0x80) & 0xbf);
  temp = SPI_LIS3DH_SendByte(0xff);
  //Delay_Spi(10);
  SPI1_CS_High();
  return temp;
}

//SPI1写函数
static void LIS3DH_SPI_WR(unsigned char addr, unsigned char wrdata)
{
  SPI1_CS_Low();
  //Delay_Spi(10);
  SPI_LIS3DH_SendByte(addr & 0x7f);
  SPI_LIS3DH_SendByte(wrdata);
  //Delay_Spi(10);
  SPI1_CS_High();
}
/*
static void Delay_Spi( uint16_t nCount)
{
  // Decrement nCount value 
  while (nCount != 0)
  {
    nCount--;
  }
}
*/

static void SPI1_CS_Low(void)
{
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);
}
static void SPI1_CS_High(void)
{
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);
}

static uint8_t SPI_LIS3DH_SendByte(uint8_t byte)
{
    extern SPI_HandleTypeDef hspi2;
    uint8_t buf[1];
    HAL_SPI_TransmitReceive(&hspi2, (uint8_t*)&byte, buf, 1, 10);
    return *buf;
}
//上面这一部分是SPI读写寄存器
//*****************************************************************************************************
//从这一部分开始就是数据采集和数据处理过程
Lis3dData Lis3dGetData(void)
{
  Lis3dData Data3D;
  Lis3dSourceData SourceData;

  SourceData.Dx_L = LIS3DH_SPI_RD(0x28);
  SourceData.Dy_L = LIS3DH_SPI_RD(0x2A);
  SourceData.Dz_L = LIS3DH_SPI_RD(0x2C);

  SourceData.Dx_H = LIS3DH_SPI_RD(0x29);
  SourceData.Dy_H = LIS3DH_SPI_RD(0x2B);
  SourceData.Dz_H = LIS3DH_SPI_RD(0x2D);

  Data3D.Dx = (int16_t)((((int16_t)SourceData.Dx_L << 8)) | (int16_t)SourceData.Dx_H);
  Data3D.Dy = (int16_t)((((int16_t)SourceData.Dy_L << 8)) | (int16_t)SourceData.Dy_H);
  Data3D.Dz = (int16_t)((((int16_t)SourceData.Dz_L << 8)) | (int16_t)SourceData.Dz_H);
  return Data3D;
}

char Lis3dCouter(Lis3dData *data)
{
  static uint16_t caonima[5] = {0};
  static uint16_t avarage[3] = {0};
  static unsigned char i = 0, j = 0;
  uint32_t buf = 0;

  data->Dx = data->Dx >> 4;
  data->Dy = data->Dy >> 4;
  data->Dz = data->Dz >> 4;
  buf += (data->Dx)*(data->Dx);
  buf += (data->Dy)*(data->Dy);
  buf += (data->Dz)*(data->Dz);
  avarage[2] = (uint16_t)sqrt(buf);             //加速度3轴求和
  caonima[4] = (avarage[0] + avarage[1] + avarage[2]) / 3; //滑动窗口取3次平均赋值

  // 取波峰逻辑判定
  if (
    //加入波形检测
    (caonima[0] <= caonima[1] && caonima[1] <= caonima[2])
    &&
    (caonima[2] >= caonima[3] && caonima[3] >= caonima[4])

    //加入幅度检测
    &&
    (caonima[2] - caonima[0] >= 45 || caonima[2] - caonima[4] >= 45)
    )
  {
    i++;
  }

  // Shift
  avarage[0] = avarage[1];
  avarage[1] = avarage[2];
  caonima[0] = caonima[1];
  caonima[1] = caonima[2];
  caonima[2] = caonima[3];
  caonima[3] = caonima[4];

  //Make sure
  if (i)
  {
    j++;
    if (i > 2)
    {
      i = 0;
      j = 0;
      return 0;
    }
    if (j >= 5) //&& i <= 2
    {
      i = 0;
      j = 0;
      return 1;
    }
  }
  return 0;
}
