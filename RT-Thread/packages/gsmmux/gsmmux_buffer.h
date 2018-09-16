/****************************************Copyright (c)****************************************************
**                             �� �� �� �� �� �� �� �� �� �� �� ˾
**                                http://www.6lowpanworld.com
**                                http://hichard.taobao.com
**
**
**--------------File Info---------------------------------------------------------------------------------
** File Name:           gsmmux_buffer.h
** Last modified Date:  2017-11-11
** Last Version:        v1.00
** Description:         gsmmuxʵ���ڴ����
**--------------------------------------------------------------------------------------------------------
** Created By:          Renhaibo
** Created date:        2017-11-11
** Version:             v1.00
** Descriptions:        
**--------------------------------------------------------------------------------------------------------
** Modified by:         
** Modified date:       
** Version:             
** Description:         
*********************************************************************************************************/
#ifndef __GSMMUX_BUFFER_H__
#define __GSMMUX_BUFFER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <rtthread.h>
  
/*********************************************************************************************************
**  ����������ṹ����
*********************************************************************************************************/
typedef struct gsmmux_buffer
{
  struct rt_mutex lock;
  struct rt_completion Comp;                //  ������������
  
  uint8_t *buffer;
  uint16_t size;
  uint16_t readp;
  uint16_t writep;
} gsmmux_buffer_t;

/*********************************************************************************************************
**  ʵ�ֵ��ⲿ��������
*********************************************************************************************************/
extern void gsmmux_buffer_init(gsmmux_buffer_t *gsm_buffer, uint8_t *buffer, uint16_t size);
extern uint16_t gsmmux_buffer_write(gsmmux_buffer_t *gsm_buffer, const uint8_t *buffer, uint16_t length);
extern uint16_t gsmmux_buffer_read(gsmmux_buffer_t *gsm_buffer, 
                            uint8_t *buffer, uint16_t size, uint32_t timeout, uint32_t interval);
extern uint8_t gsmmux_make_fcs(const uint8_t *input, uint32_t length, uint8_t crc8);

#ifdef __cplusplus
    }
#endif      // __cplusplus
    
#endif      // __GSMMUX_BUFFER_H__
/*********************************************************************************************************
END FILE
*********************************************************************************************************/
