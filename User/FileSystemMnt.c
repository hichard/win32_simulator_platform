/****************************************Copyright (c)****************************************************
**                             成 都 世 纪 华 宁 科 技 有 限 公 司
**                                http://www.6lowpanworld.com
**                                http://hichard.taobao.com
**
**
**
**--------------File Info---------------------------------------------------------------------------------
** File Name:           FileSystemMnt.h
** Last modified Date:  2015-06-08
** Last Version:        V1.00
** Description:         文件系统挂载功能实现
**--------------------------------------------------------------------------------------------------------
** Created By:          Renhaibo
** Created date:        2015-06-08
** Version:             V1.00
** Descriptions:
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
** Version:
** Description:
*********************************************************************************************************/
#include <rtdef.h>
#include <rtthread.h>
#include <rtdevice.h>

#ifdef RT_USING_DFS

#include <dfs_fs.h>
/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/

/*********************************************************************************************************
  本地函数声明
*********************************************************************************************************/

/*********************************************************************************************************
** Function name:       mnt_init
** Descriptions:        文件系统挂载
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void mnt_init(void)
{
    extern int dfs_win32_init(void);
    extern rt_err_t rt_win_sharedir_init(const char *name);

    dfs_win32_init();
    rt_win_sharedir_init("wshare");
    rt_hw_sdcard_init();

    if (dfs_mount("wshare", "/", "wdir", 0, 0) == 0)
    {
        rt_kprintf("File System on root initialized!\n");
    }
    else
    {
        rt_kprintf("File System on root initialization failed!\n");
    }

    if (dfs_mount("sd0", "/sd", "elm", 0, 0) == 0)
    {
        rt_kprintf("File System on sd initialized!\n");
    }
    else
    {
        dfs_mkfs("elm", "sd0");
        rt_kprintf("File System on sd initialization failed!\n");
    }
}
#endif   // end of RT_USING_DFS
/*********************************************************************************************************
END FILE
*********************************************************************************************************/
