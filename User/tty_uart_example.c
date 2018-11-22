/****************************************Copyright (c)****************************************************
**                             成 都 世 纪 华 宁 科 技 有 限 公 司
**                                http://www.6lowpanworld.com
**                                http://hichard.taobao.com
**
**
**--------------File Info---------------------------------------------------------------------------------
** File Name:           gps_app.cpp
** Last modified Date:  2017-01=11
** Last Version:        V1.00
** Description:         gps模块应用
**--------------------------------------------------------------------------------------------------------
** Created By:          Renhaibo
** Created date:        2017-01-11
** Version:             V1.00
** Descriptions:
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
** Version:
** Description:
*********************************************************************************************************/
#include <stdio.h>
#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>

/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/

/*********************************************************************************************************
** Function name:       gps_open_tty
** Descriptions:        打开tty串口
** input parameters:    device:	ppp网络接口结构
** output parameters:   NONE
** Returned value:      打开的串口句柄
*********************************************************************************************************/
static rt_device_t gps_open_tty(const char *name)
{
  rt_device_t handle;

  while(1)
  {
    handle = rt_device_find(name);
    if(handle != RT_NULL) {
      if(handle->ref_count == 0) {
        rt_device_open(handle,RT_DEVICE_OFLAG_RDWR);
      }
      return handle;
    }
    rt_thread_delay(RT_TICK_PER_SECOND);
  }
}

/*********************************************************************************************************
** Function name:       tty_test_thread
** Descriptions:        串口测试线程
** input parameters:    *p_arg
** output parameters:   NONE无
** Returned value:      NONE无
*********************************************************************************************************/
static void tty_test_thread(void *parg)
{
  static char buffer[1024];
  uint32_t len;
  rt_device_t handle;
  struct serial_configure cfg = RT_SERIAL_CONFIG_DEFAULT;

  handle = rt_device_find("ttyS0");
  if(handle == RT_NULL) {
    printf("Can't find ttyS0\r\n");
    return;
  }
  rt_device_open(handle,RT_DEVICE_OFLAG_RDWR);
  cfg.baud_rate = 115200;
  rt_device_control(handle, RT_DEVICE_CTRL_CONFIG_SET, &cfg);

  for(;;)
  {
      len = rt_device_read(handle, 0, buffer, 1024);
      if(len > 0) {
         rt_device_write(handle, 0, buffer, len);
      }
  }
}

/*********************************************************************************************************
** Function name:       tty_example_init
** Descriptions:        串口应用例子初始化
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void tty_example_init(void)
{
  rt_thread_t tid;

  tid = rt_thread_create("tty", tty_test_thread, RT_NULL, 2048, 8, 20);

  if (tid != RT_NULL) {
    rt_thread_startup(tid);
  }
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
