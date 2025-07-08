#ifndef __SPI_H
#define __SPI_H
#include "sys.h"
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK NANO STM32F4������
//SPI��������	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2019/4/23
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2019-2029
//All rights reserved									  
//////////////////////////////////////////////////////////////////////////////////  	

extern SPI_HandleTypeDef SPI1_Handler;  //SPI���

void SPI1_Init(void);
void SPI1_SetSpeed(u8 SPI_BaudRatePrescaler);
u8 SPI1_ReadWriteByte(u8 TxData);
u8 SPI1_WriteData(u8 *data, u16 size);
#endif
