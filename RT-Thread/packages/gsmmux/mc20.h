/****************************************Copyright (c)****************************************************
**                             �� �� �� �� �� �� �� �� �� �� �� ˾
**                                http://www.6lowpanworld.com
**                                http://hichard.taobao.com
**
**
**--------------File Info---------------------------------------------------------------------------------
** File Name:           mc20.h
** Last modified Date:  2018-06-02
** Last Version:        v1.00
** Description:         �Ϻ���Զͨ�ŵ�GPRSģ��MC20Ӧ�ýӿ�ͷ�ļ�
**--------------------------------------------------------------------------------------------------------
** Created By:          Renhaibo
** Created date:        2018-06-02
** Version:             v1.00
** Descriptions:        
**--------------------------------------------------------------------------------------------------------
** Modified by:         
** Modified date:       
** Version:             
** Description:         
*********************************************************************************************************/
#ifndef __MC20_H__
#define __MC20_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <rtthread.h>
#include <rtdevice.h>

/*********************************************************************************************************
**  ʵ�ֵ��ⲿ��������
*********************************************************************************************************/
extern int mc20_enter_mux(rt_device_t handle, uint8_t *buffer);
extern int mc20_at_dial(rt_device_t handle, uint8_t *buffer);


#ifdef __cplusplus
    }
#endif      // __cplusplus
    
#endif      // __MC20_H__
/*********************************************************************************************************
END FILE
*********************************************************************************************************/