/****************************************Copyright (c)****************************************************
**                             �� �� �� �� �� �� �� �� �� �� �� ˾
**                                http://www.6lowpanworld.com
**                                http://hichard.taobao.com
**
**
**
**--------------File Info---------------------------------------------------------------------------------
** File Name:           Main.c
** Last modified Date:  2014-12-23
** Last Version:        V1.00
** Description:         ������ģ�壬ϵͳ����ִ���ļ�
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
  ȫ�ֱ�������
*********************************************************************************************************/

/*********************************************************************************************************
  ���غ�������
*********************************************************************************************************/

/*********************************************************************************************************
** Function name:       win_main
** Descriptions:        Ӧ�ó������������
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      0����ȷ���أ����κ�����
*********************************************************************************************************/
int win_main(void)
{
  tap_netif_hw_init();
#if 1
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