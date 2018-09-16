/****************************************Copyright (c)****************************************************
**                             成 都 世 纪 华 宁 科 技 有 限 公 司
**                                http://www.6lowpanworld.com
**                                http://hichard.taobao.com
**
**
**--------------File Info---------------------------------------------------------------------------------
** File Name:           sim868.c
** Last modified Date:  2018-06-02
** Last Version:        v1.00
** Description:         Simcom的GPRS模块SIM868应用接口
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
** 全局变量定义
*********************************************************************************************************/

/*********************************************************************************************************
** Function name:       at_command
** Descriptions:        AT指令处理
** input parameters:    handle: 使用的串口句柄
*                       buffer: 处理缓冲区
**                      at：    AT命令
**                      ack：   应答标志
** output parameters:   无
** Returned value:      0:   AT指令正确应答
**                      -1： AT指令错误应答
**                      -2： AT指令无应答
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
** Descriptions:        sim868进入mux模式
** Input parameters:    handle:   串口句柄
**                      buffer:   处理数据缓冲区
** Output parameters:   None 无
** Returned value:      0:  进入mux模式失败
**                      1： 进入mux成功
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
** Descriptions:        sim868拨号上网
** Input parameters:    handle:   串口句柄
**                      buffer:   处理数据缓冲区
** Output parameters:   None 无
** Returned value:      -1: at指令都不能响应
**                      0:  拨号失败
**                      1： 拨号成功
*********************************************************************************************************/
int sim868_at_dial(rt_device_t handle, uint8_t *buffer)
{
  uint32_t u32Len;
  
  /*
  ** Step 1, 一般命令，设置应答超时为5秒，接收时间间隔为1秒
  */
  u32Len = 5000;
  rt_device_control(handle, RT_SERIAL_TIMEOUT_SET, &u32Len);
  u32Len = 1000;
  rt_device_control(handle, RT_SERIAL_TIME_INTERVAL_SET, &u32Len);
    
  /*
  ** Step 2, AT指令测试
  */
  if(at_command(handle,(char *)buffer, "AT\r\n", "OK") < 0) {
    return -1;
  }
  
  /*
  ** Step 3, 关闭回显
  */
  if(at_command(handle,(char *)buffer, "ATE0\r\n", "OK") < 0) {
    return -1;
  }
  
  /*
  ** Step 4, 检测模块是否注册上网
  */
  if(at_command(handle,(char *)buffer, "AT+CREG?\r\n", "OK") < 0) {
    return 0;
  }
  
  /*
  ** Step 5, 查询基站信息
  */
  if(at_command(handle,(char *)buffer, "AT+CGREG=?\r\n", "OK") < 0) {
    return 0;
  }
  
  /*
  ** Step 6, 查看GPRS网络帮助信息
  */
  if(at_command(handle,(char *)buffer, "AT+CGDCONT=?\r\n", "OK") < 0) {
    return 0;
  }
  
  /*
  ** Step 7,  Before active, use this command to set PDP context.此后的命令可能等待较长
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
** Descriptions:        获取GPRS模块的信号强度
** input parameters:    无
** output parameters:   无
** Returned value:      无
*********************************************************************************************************/
rt_uint16_t gprs_rssi_get(void)
{
  return 0;
}

/*********************************************************************************************************
** Function name:       gprs_rssi_get
** Descriptions:        获取GPRS模块的信号强度
** input parameters:    无
** output parameters:   无
** Returned value:      ccid长度
*********************************************************************************************************/
int gprs_ccid_get(char *ccid)
{
  return 0;
}
/*********************************************************************************************************
END FILE
*********************************************************************************************************/
