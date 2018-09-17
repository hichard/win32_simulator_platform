/****************************************Copyright (c)****************************************************
**                             成 都 世 纪 华 宁 科 技 有 限 公 司
**                                http://www.6lowpanworld.com
**                                http://hichard.taobao.com
**
**
**--------------File Info---------------------------------------------------------------------------------
** File Name:           rtconfig.h
** Last modified Date:  2015-01-09
** Last Version:        v1.00
** Description:         RT-Thread config file
**--------------------------------------------------------------------------------------------------------
** Created By:          Renhaibo
** Created date:        2015-01-09
** Version:             v1.00
** Descriptions:
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
** Version:
** Description:
*********************************************************************************************************/
#ifndef __RTTHREAD_CFG_H__
#define __RTTHREAD_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************************************
**  操作系统相关配置
*********************************************************************************************************/
/* RT_NAME_MAX*/
#define RT_NAME_MAX	8

/* RT_ALIGN_SIZE*/
#define RT_ALIGN_SIZE	4

/* PRIORITY_MAX */
#define RT_THREAD_PRIORITY_MAX	32

/* Tick per Second */
#define RT_TICK_PER_SECOND	200

/* SECTION: RT_DEBUG */
/* Thread Debug */
#define RT_DEBUG
#define RT_USING_OVERFLOW_CHECK

/* Using Hook */
#define RT_USING_HOOK

/* Using Software Timer */
//#define RT_USING_TIMER_SOFT
#define RT_TIMER_THREAD_PRIO		4
#define RT_TIMER_THREAD_STACK_SIZE	512
#define RT_TIMER_TICK_PER_SECOND	10

/* SECTION: IPC */
/* Using Semaphore*/
#define RT_USING_SEMAPHORE

/* Using Mutex */
#define RT_USING_MUTEX

/* Using Event */
#define RT_USING_EVENT

/* Using MailBox */
#define RT_USING_MAILBOX

/* Using Message Queue */
#define RT_USING_MESSAGEQUEUE

/* SECTION: Memory Management */
/* Using Memory Pool Management*/
#define RT_USING_MEMPOOL

/* Using Dynamic Heap Management */
#define RT_USING_HEAP

/* Using MemHeap as Dynamic Heap Management */
//#define RT_USING_MEMHEAP
//#define RT_USING_MEMHEAP_AS_HEAP

/* Using Small MM */
#define RT_USING_SMALL_MEM

/*********************************************************************************************************
**  组件相关配置
*********************************************************************************************************/
// <bool name="RT_USING_COMPONENTS_INIT" description="Using RT-Thread components initialization" default="true" />
#define RT_USING_COMPONENTS_INIT

/*********************************************************************************************************
**  使用用户独立的main函数
*********************************************************************************************************/
//#define RT_USING_USER_MAIN
//#define RT_MAIN_THREAD_STACK_SIZE   2048
//#define RT_MAIN_THREAD_PRIO         0

/*********************************************************************************************************
**  IO设备相关配置
*********************************************************************************************************/
/* SECTION: Device System */
/* Using Device System */
#define RT_USING_DEVICE
// <bool name="RT_USING_DEVICE_IPC" description="Using device communication" default="true" />
#define RT_USING_DEVICE_IPC
// <bool name="RT_USING_SERIAL" description="Using Serial" default="true" />
#define RT_USING_SERIAL

/*********************************************************************************************************
**  使用rtc的话请定义
*********************************************************************************************************/
#define RT_USING_RTC
//#define RT_USING_SOFT_RTC

/*********************************************************************************************************
**  使用I2C总线设备
*********************************************************************************************************/
//#define RT_USING_I2C
//#define RT_USING_I2C_BITOPS

/*********************************************************************************************************
**  使用mtd框架
*********************************************************************************************************/
//#define RT_USING_MTD_NAND     // mtd nand框架使能
//#define RT_USING_MTD_NOR      // mtd nor框架使能

/*********************************************************************************************************
**  控制台相关配置
*********************************************************************************************************/
/* SECTION: Console options */
#define RT_USING_CONSOLE
/* the buffer size of console*/
#define RT_CONSOLEBUF_SIZE	        128
// <string name="RT_CONSOLE_DEVICE_NAME" description="The device name for console" default="uart1" />
#define RT_CONSOLE_DEVICE_NAME	    "console"

/*********************************************************************************************************
**  shell工具相关配置
*********************************************************************************************************/
#define RT_USING_FINSH
#define FINSH_THREAD_NAME               "tshell"
#define FINSH_USING_HISTORY
#define FINSH_HISTORY_LINES             5
#define FINSH_USING_SYMTAB
#define FINSH_USING_DESCRIPTION
#define FINSH_THREAD_PRIORITY           20
#define FINSH_THREAD_STACK_SIZE         4096
#define FINSH_CMD_SIZE                  80
#define FINSH_USING_MSH
#define FINSH_USING_MSH_DEFAULT
#define FINSH_USING_MSH_ONLY
#define FINSH_ARG_MAX                   10

/*********************************************************************************************************
**  标准C库相关配置
*********************************************************************************************************/
#define RT_USING_LIBC
#define RT_USING_NEWLIB

/*********************************************************************************************************
**  文件系统相关配置
*********************************************************************************************************/
///* SECTION: device filesystem */
//#define RT_USING_DFS
//
////#define RT_USING_DFS_DEVFS
//
//  /* Use Using working directory */
//#define DFS_USING_WORKDIR
//
//  /* the max number of mounted filesystem */
//#define DFS_FILESYSTEMS_MAX             1
//  /* the max number of opened files       */
//#define DFS_FD_MAX                      8

/*********************************************************************************************************
**  elm fat相关配置
*********************************************************************************************************/
//#define RT_USING_DFS_ELMFAT
///* Use exFat, support fat8, fat16, fat32, fat64.  */
//#define RT_DFS_ELM_USE_EXFAT
///* Reentrancy (thread safe) of the FatFs module.  */
//#define RT_DFS_ELM_REENTRANT
///* Number of volumes (logical drives) to be used. */
//#define RT_DFS_ELM_DRIVES               1
//#define RT_DFS_ELM_USE_LFN              3
////#define RT_DFS_ELM_LFN_UNICODE
//#define RT_DFS_ELM_CODE_PAGE            437
//// <bool name="RT_DFS_ELM_CODE_PAGE_FILE" description="Using OEM code page file" default="false" />
//#define RT_DFS_ELM_CODE_PAGE_FILE
//#define RT_DFS_ELM_MAX_LFN              128
///* Maximum sector size to be handled. */
//#define RT_DFS_ELM_MAX_SECTOR_SIZE      512

/*********************************************************************************************************
**  uffs相关配置
*********************************************************************************************************/
///* configration for uffs, more to see dfs_uffs.h and uffs_config.h */
//#define RT_USING_DFS_UFFS
//#define RT_CONFIG_UFFS_ECC_MODE  UFFS_ECC_HW_AUTO
///* enable this ,you need provide a mark_badblock/check_block funciton */
///* #define RT_UFFS_USE_CHECK_MARK_FUNCITON */


/*********************************************************************************************************
**  网络相关配置
*********************************************************************************************************/
/* SECTION: lwip, a lighwight TCP/IP protocol stack */
#define RT_USING_LWIP
/* LwIP uses RT-Thread Memory Management */
//#define RT_LWIP_USING_RT_MEM
/* the number of simulatenously active TCP connections*/
//#define RT_LWIP_TCP_PCB_NUM	5

/*********************************************************************************************************
**  以太网支持
*********************************************************************************************************/
#define RT_USE_ETHERNET
#define RT_LWIP_NO_TX_THREAD
//#define RT_LWIP_NO_RX_THREAD
/* ethernet if thread options */
#define RT_LWIP_ETHTHREAD_PRIORITY		2
#define RT_LWIP_ETHTHREAD_MBOX_SIZE		16
#define RT_LWIP_ETHTHREAD_STACKSIZE		512

/*********************************************************************************************************
**  6LoWPAN支持
*********************************************************************************************************/
//#define RT_USE_6LOWPAN
//#define RT_LWIP_6LOWPANTHREAD_PRIORITY		3
//#define RT_LWIP_6LOWPANTHREAD_MBOX_SIZE		16
//#define RT_LWIP_6LOWPANTHREAD_STACKSIZE		512

/*********************************************************************************************************
**  RT GUI相关配置
*********************************************************************************************************/
///* SECTION: RT-Thread/GUI */
///* #define RT_USING_RTGUI */
//
///* name length of RTGUI object */
//#define RTGUI_NAME_MAX		12
///* support 16 weight font */
//#define RTGUI_USING_FONT16
///* support Chinese font */
//#define RTGUI_USING_FONTHZ
///* use DFS as file interface */
//#define RTGUI_USING_DFS_FILERW
///* use font file as Chinese font */
//#define RTGUI_USING_HZ_FILE
///* use Chinese bitmap font */
//#define RTGUI_USING_HZ_BMP
///* use small size in RTGUI */
//#define RTGUI_USING_SMALL_SIZE
///* use mouse cursor */
///* #define RTGUI_USING_MOUSE_CURSOR */
///* default font size in RTGUI */
//#define RTGUI_DEFAULT_FONT_SIZE	16
//
///* image support */
///* #define RTGUI_IMAGE_XPM */
///* #define RTGUI_IMAGE_BMP */

/*********************************************************************************************************
**  包管理器的一些配置
*********************************************************************************************************/
#define PKG_NETUTILS_PING

#ifdef __cplusplus
    }
#endif      // __cplusplus

#endif		 // __RTTHREAD_CFG_H__
/*********************************************************************************************************
END FILE
*********************************************************************************************************/
