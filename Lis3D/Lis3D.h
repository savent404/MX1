#ifndef _LIS3D_H_
#define _LIS3D_H_
#include "stdint.h"
#include "math.h"
typedef struct Source
{
  uint8_t Dx_L;
  uint8_t Dy_L;
  uint8_t Dz_L;
  uint8_t Dx_H;
  uint8_t Dy_H;
  uint8_t Dz_H;
}
Lis3dSourceData;

typedef struct Data
{
  int16_t Dx;
  int16_t Dy;
  int16_t Dz;
}
Lis3dData;

typedef struct _Lis3dConfig
{
  uint16_t MD;
  uint16_t MT;
  uint16_t CD;
  uint16_t CT;
  uint16_t CL;
  uint16_t CW;
} Lis3dConfig;

static unsigned char LIS3DH_SPI_RD(unsigned char addr);
static void LIS3DH_SPI_WR(unsigned char addr,unsigned char wrdata);
/*
static void Delay_Spi(uint16_t nCount);
static void SPI1_Init(void);
*/
static void SPI1_CS_Low(void);
static void SPI1_CS_High(void);
static uint8_t SPI_LIS3DH_SendByte(uint8_t byte);


void Lis3d_Init(void);
void Lis3d_Set(Lis3dConfig *para);
uint8_t Lis3d_isClick(void);
uint8_t Lis3d_isMove(void);

#endif
