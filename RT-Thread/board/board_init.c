/****************************************Copyright (c)****************************************************
**                             成 都 世 纪 华 宁 科 技 有 限 公 司
**                                http://www.6lowpanworld.com
**                                http://hichard.taobao.com
**
**
**--------------File Info---------------------------------------------------------------------------------
** File Name:           board_init.c
** Last modified Date:  2014-12-23
** Last Version:        v1.00
** Description:         目标板初始化文件
**--------------------------------------------------------------------------------------------------------
** Created By:          Renhaibo
** Created date:        2014-12-23
** Version:             v1.00
** Descriptions:
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
** Version:
** Description:
*********************************************************************************************************/
#include <stdlib.h>
#include <rthw.h>
#include <rtthread.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "uart_console.h"

/*********************************************************************************************************
**  定义heap大小
*********************************************************************************************************/
#define RT_HEAP_SIZE     1024*1024

/*********************************************************************************************************
  全局变量声明
*********************************************************************************************************/
static rt_uint8_t *heap;

/*********************************************************************************************************
  外部函数定义
*********************************************************************************************************/

/*********************************************************************************************************
** Function name:       rt_hw_sram_init
** Descriptions:        分配堆区内存
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      堆区地址
*********************************************************************************************************/
static rt_uint8_t *rt_hw_sram_init(void)
{
    rt_uint8_t *heap;
    heap = malloc(RT_HEAP_SIZE);
    if (heap == RT_NULL)
    {
        rt_kprintf("there is no memory in pc.");
#ifdef _WIN32
        _exit(1);
#else
        exit(1);
#endif
    }
    return heap;
}

/*********************************************************************************************************
** Function name:       rt_hw_win32_low_cpu
** Descriptions:        设置CPU进入休眠，可以用作tick的hook函数
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void rt_hw_win32_low_cpu(void)
{
#ifdef _WIN32
    /* in windows */
    Sleep(1000);
#else
    /* in linux */
    sleep(1);
#endif
}

#ifdef _MSC_VER
#ifndef _CRT_TERMINATE_DEFINED
#define _CRT_TERMINATE_DEFINED
_CRTIMP __declspec(noreturn) void __cdecl exit(__in int _Code);
_CRTIMP __declspec(noreturn) void __cdecl _exit(__in int _Code);
_CRTIMP void __cdecl abort(void);
#endif
#endif

/*********************************************************************************************************
** Function name:       rt_hw_exit
** Descriptions:        退出程序前的调用函数
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void rt_hw_exit(void)
{
    rt_kprintf("RT-Thread, bye\n");
#if !defined(_WIN32) && defined(__GNUC__)
    /* *
     * getchar reads key from buffer, while finsh need an non-buffer getchar
     * in windows, getch is such an function, in linux, we had to change
     * the behaviour of terminal to get an non-buffer getchar.
     * in usart_sim.c, set_stty is called to do this work
     * */
    {
        extern void restore_stty(void);
        restore_stty();
    }
#endif
    exit(0);
}

#if defined(RT_USING_FINSH)
#include <finsh.h>
FINSH_FUNCTION_EXPORT_ALIAS(rt_hw_exit, exit, exit rt - thread);
FINSH_FUNCTION_EXPORT_ALIAS(rt_hw_exit, __cmd_quit, exit rt-thread);
#endif /* RT_USING_FINSH */

/*********************************************************************************************************
** Function name:       rt_hw_board_init
** Descriptions:        初始化目标主板
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void rt_hw_board_init(void)
{
  /**
  * Step1, 初始化堆空间
  */
#ifdef RT_USING_HEAP
  /* init memory system */
  heap = rt_hw_sram_init();
  rt_system_heap_init((void *)heap, (void *)&heap[RT_HEAP_SIZE - 1]);
#endif /* RT_USING_HEAP */

  /**
  * Step2, 初始化板级驱动组件
  */
#ifdef RT_USING_COMPONENTS_INIT
  rt_components_board_init();
#endif
  uart_console_init();


  /**
  * Step3, 设置控制台及shell的串口
  */
#ifdef RT_USING_CONSOLE
  rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
#endif

  /**
  * Step4, 设置idle hook
  */
#ifdef _WIN32
    rt_thread_idle_sethook(rt_hw_win32_low_cpu);
#endif
}

/*********************************************************************************************************
END FILE
*********************************************************************************************************/
