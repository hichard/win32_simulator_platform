/****************************************Copyright (c)****************************************************
**                             成 都 世 纪 华 宁 科 技 有 限 公 司
**                                http://www.6lowpanworld.com
**                                http://hichard.taobao.com
**
**
**--------------File Info---------------------------------------------------------------------------------
** File Name:           music_player.c
** Last modified Date:  2015-11-14
** Last Version:        v1.0
** Description:         音乐播放器
**
**--------------------------------------------------------------------------------------------------------
** Created By:          Renhaibo任海波
** Created date:        2015-11-14
** Version:             v1.0
** Descriptions:        The original version 初始版本
**
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
** Version:
** Description:
**
*********************************************************************************************************/
#include <includes.h>


/*********************************************************************************************************
** 全局变量
*********************************************************************************************************/
static rt_mailbox_t mboxMp3;

/*********************************************************************************************************
** Function name:       rt_music_play_thread
** Descriptions:        音乐播放线程
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void rt_music_play_thread(void *parg)
{
   char *pFile;
   char *name;
   rt_thread_delay(RT_TICK_PER_SECOND * 5);
   for(;;)
   {
     if(rt_mb_recv(mboxMp3, (rt_uint32_t*)&name, RT_WAITING_FOREVER) == RT_EOK) {
        pFile = strrchr(name, '.');
        if(pFile == RT_NULL) {
          rt_thread_delay(RT_TICK_PER_SECOND * 2);
          continue;
        }

        pFile+= 1;
//        if(strncmp(pFile, "mp3",3) == 0) {
//          mp3(name);
//        }  else
          if(strncmp(pFile, "wav",3) == 0) {
          wav(name);
        }
         printf("The %s play End\r\n",name);
     }

//      mp3("/music/my.mp3");
//      rt_thread_delay(RT_TICK_PER_SECOND * 3);
   }
}


#ifdef RT_USING_FINSH
#include <finsh.h>
int cmd_music(int argc, char **argv)
{
    rt_device_t tid;

    if (argc == 1)
    {
        mboxMp3 = rt_mb_create("mp3", 16, RT_IPC_FLAG_FIFO);
        tid = rt_thread_create("music", rt_music_play_thread, RT_NULL, 1024, 10, 20);

        if (tid != RT_NULL) {
            rt_thread_startup(tid);
        }
    }

    return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(cmd_music, __cmd_music, create music play thread);

int cmd_play(int argc, char **argv)
{
    if (argc == 1)
    {
        rt_kprintf("Please a file name for play wav\r\n");
    }
    else if (argc == 2)
    {
        rt_mb_send_wait(mboxMp3, (rt_uint32_t)(argv[1]), RT_WAITING_FOREVER);

    }
    else
    {
        rt_kprintf("bad parameter! e.g: wav /music/my.mp3\r\n");
    }

    return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(cmd_play, __cmd_play, play a music of mp3 or wmv);
#endif
/*********************************************************************************************************
** End of File
*********************************************************************************************************/
