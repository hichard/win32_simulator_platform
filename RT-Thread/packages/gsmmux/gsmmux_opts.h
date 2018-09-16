/****************************************Copyright (c)****************************************************
**                             成 都 世 纪 华 宁 科 技 有 限 公 司
**                                http://www.6lowpanworld.com
**                                http://hichard.taobao.com
**
**
**--------------File Info---------------------------------------------------------------------------------
** File Name:           gsmmux_opts.h
** Last modified Date:  2017-11-11
** Last Version:        v1.00
** Description:         gsmmux配置文件
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
#ifndef __GSMMUX_OPTS_H__
#define __GSMMUX_OPTS_H__

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************************************
**  GSM0710包格式协议支持,该软件包支持一种模式，下面三个配置只能有一个为1
*********************************************************************************************************/
#define GSM0710_BASIC                   1       // 基本模式
#define GSM0710_ADV_WITHOUT_ERR         0       // Advanced无错误恢复模式
#define GSM0710_ADV_WITH_ERR            0       // Advanced带错误恢复模式
  
#if (!((GSM0710_BASIC > 0 ) && (GSM0710_ADV_WITHOUT_ERR == 0) && (GSM0710_ADV_WITH_ERR == 0)) || \
    ((GSM0710_BASIC == 0 ) && (GSM0710_ADV_WITHOUT_ERR > 0) && (GSM0710_ADV_WITH_ERR == 0)) ||   \
    ((GSM0710_BASIC == 0 ) && (GSM0710_ADV_WITHOUT_ERR == 0) && (GSM0710_ADV_WITH_ERR > 0)))
#error "This software only support one frame format"
#endif

/*********************************************************************************************************
**  虚拟串口的数量(控制通道除外)，必须大于或等于2.如果为1，则不需要开启mux功能
*********************************************************************************************************/
#define GSM0710_PORT_NUM                2
#if (GSM0710_PORT_NUM < 2)
#error "This gsm0710 logic channel number must bigger than 2"
#endif

/*********************************************************************************************************
**  发送和接收串口缓冲区大小
*********************************************************************************************************/
#define GSM0710_BUFFER_NUM             1500

/*********************************************************************************************************
**  上电等待modem准备好的时间、能响应AT指令，进入mux模式，则视为准备好，以秒为单位
*********************************************************************************************************/
#define GSM0710_MODEM_WAIT_TIME        60*10

/*********************************************************************************************************
**  是否使能调试输出
*********************************************************************************************************/
#define GSMMUX_BUFFER_DEBUG             1     // 使能缓冲区管理调试输出
#define GSMMUX_0710_DEBUG               1     // 使能0710协议处理调试输出

#ifdef __cplusplus
    }
#endif      // __cplusplus
    
#endif      // __GSMMUX_OPTS_H__
/*********************************************************************************************************
END FILE
*********************************************************************************************************/
