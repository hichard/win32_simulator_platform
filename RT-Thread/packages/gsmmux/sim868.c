/****************************************Copyright (c)****************************************************
**                             �� �� �� �� �� �� �� �� �� �� �� ˾
**                                http://www.6lowpanworld.com
**                                http://hichard.taobao.com
**
**
**--------------File Info---------------------------------------------------------------------------------
** File Name:           sim868.c
** Last modified Date:  2018-06-02
** Last Version:        v1.00
** Description:         Simcom��GPRSģ��SIM868Ӧ�ýӿ�
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
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>

#include "board_serial.h"

/*********************************************************************************************************
** ȫ�ֱ�������
*********************************************************************************************************/

/*********************************************************************************************************
** Function name:       at_command
** Descriptions:        ATָ���
** input parameters:    handle: ʹ�õĴ��ھ��
*                       buffer: ��������
**                      at��    AT����
**                      ack��   Ӧ���־
** output parameters:   ��
** Returned value:      0:   ATָ����ȷӦ��
**                      -1�� ATָ�����Ӧ��
**                      -2�� ATָ����Ӧ��
*********************************************************************************************************/
static int at_command(rt_device_t handle, char * buffer, const char *at, const char *ack)
{
  rt_int32_t  u32Len;
  
  sprintf(buffer, "%s", at);
  rt_device_write(handle, 0, buffer, strlen(buffer));
  u32Len = rt_device_read(handle, 0, buffer, 512);
  buffer[u32Len] = 0;
  if(u32Len < 2) {
    return -2;
  }
  
  if(strstr(buffer, ack) == RT_NULL) {
    return -1;
  }
  
  return 0;
}

/*********************************************************************************************************
** Function name:       sim868_enter_mux
** Descriptions:        sim868����muxģʽ
** Input parameters:    handle:   ���ھ��
**                      buffer:   �������ݻ�����
** Output parameters:   None ��
** Returned value:      0:  ����muxģʽʧ��
**                      1�� ����mux�ɹ�
*********************************************************************************************************/
int sim868_enter_mux(rt_device_t handle, uint8_t *buffer)
{
  if(at_command(handle, (char *)buffer, "AT\r\n", "OK") < 0) {
    return 0;
  }
  
  if(at_command(handle,(char *)buffer, "ATE0\r\n", "OK") < 0) {
    return 0;
  }
  
  if(at_command(handle, (char *)buffer, "AT+CMUX=0\r\n", "OK") < 0) {
    return 0;
  }
  return 1;
}

/*********************************************************************************************************
** Function name:       sim868_at_dial
** Descriptions:        sim868��������
** Input parameters:    handle:   ���ھ��
**                      buffer:   �������ݻ�����
** Output parameters:   None ��
** Returned value:      -1: atָ�������Ӧ
**                      0:  ����ʧ��
**                      1�� ���ųɹ�
*********************************************************************************************************/
int sim868_at_dial(rt_device_t handle, uint8_t *buffer)
{
  uint32_t u32Len;
  
  /*
  ** Step 1, һ���������Ӧ��ʱΪ5�룬����ʱ����Ϊ1��
  */
  u32Len = 5000;
  rt_device_control(handle, RT_SERIAL_TIMEOUT_SET, &u32Len);
  u32Len = 1000;
  rt_device_control(handle, RT_SERIAL_TIME_INTERVAL_SET, &u32Len);
    
  /*
  ** Step 2, ATָ�����
  */
  if(at_command(handle,(char *)buffer, "AT\r\n", "OK") < 0) {
    return -1;
  }
  
  /*
  ** Step 3, �رջ���
  */
  if(at_command(handle,(char *)buffer, "ATE0\r\n", "OK") < 0) {
    return -1;
  }
  
  /*
  ** Step 4, ���ģ���Ƿ�ע������
  */
  if(at_command(handle,(char *)buffer, "AT+CREG?\r\n", "OK") < 0) {
    return 0;
  }
  
  /*
  ** Step 5, ��ѯ��վ��Ϣ
  */
  if(at_command(handle,(char *)buffer, "AT+CGREG=?\r\n", "OK") < 0) {
    return 0;
  }
  
  /*
  ** Step 6, �鿴GPRS���������Ϣ
  */
  if(at_command(handle,(char *)buffer, "AT+CGDCONT=?\r\n", "OK") < 0) {
    return 0;
  }
  
  /*
  ** Step 7,  Before active, use this command to set PDP context.�˺��������ܵȴ��ϳ�
  */
  u32Len = 100000;
  rt_device_control(handle, RT_SERIAL_TIMEOUT_SET, &u32Len);
  if(at_command(handle,(char *)buffer, "AT+CGDCONT=1, \"IP\", \"CMNET\"\r\n", "OK") < 0) {
    return 0;
  }
  rt_thread_delay(RT_TICK_PER_SECOND * 3);
  
  /*
  ** Step 8, This command is to start PPP translation.
  */
  if(at_command(handle,(char *)buffer, "ATD*99***1#\r\n", "CONNECT") < 0) {
    at_command(handle,(char *)buffer, "+++", "OK");
    return 0;
  }
  
  return 1;
}

/*********************************************************************************************************
** Function name:       gprs_rssi_get
** Descriptions:        ��ȡGPRSģ����ź�ǿ��
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
rt_uint16_t gprs_rssi_get(void)
{
  return 0;
}

/*********************************************************************************************************
** Function name:       gprs_rssi_get
** Descriptions:        ��ȡGPRSģ����ź�ǿ��
** input parameters:    ��
** output parameters:   ��
** Returned value:      ccid����
*********************************************************************************************************/
int gprs_ccid_get(char *ccid)
{
  return 0;
}
/*********************************************************************************************************
END FILE
*********************************************************************************************************/
