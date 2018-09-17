/****************************************Copyright (c)****************************************************
**                             成 都 世 纪 华 宁 科 技 有 限 公 司
**                                http://www.6lowpanworld.com
**                                http://hichard.taobao.com
**
**
**
**--------------File Info---------------------------------------------------------------------------------
** File Name:           Main.c
** Last modified Date:  2014-12-23
** Last Version:        V1.00
** Description:         主函数模板，系统启动执行文件
**--------------------------------------------------------------------------------------------------------
** Created By:          Renhaibo
** Created date:        2014-12-23
** Version:             V1.00
** Descriptions:
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
** Version:
** Description:
*********************************************************************************************************/
#include "includes.h"

/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/

/*********************************************************************************************************
  本地函数声明
*********************************************************************************************************/

/*********************************************************************************************************
** Function name:       win_main
** Descriptions:        应用程序主函数入口
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      0：正确返回，无任何意义
*********************************************************************************************************/
int win_main(void)
{
  tap_netif_hw_init();
#if 0
  if_set_dhcp("e0", 1);
#else
  set_if("e0","192.168.3.135","192.168.3.1","255.255.255.0");
  set_dns(0, "223.5.5.5");
  //set_dns(1, ip);
#endif
  return 0;
}

/*********************************************************************************************************
END FILE
*********************************************************************************************************/
