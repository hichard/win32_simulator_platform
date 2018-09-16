/****************************************Copyright (c)****************************************************
**                             成 都 世 纪 华 宁 科 技 有 限 公 司
**                                http://www.6lowpanworld.com
**                                http://hichard.taobao.com
**
**
**
**--------------File Info---------------------------------------------------------------------------------
** File Name:           music.c
** Last modified Date:  2018-09-03
** Last Version:        v1.0
** Description:         音乐播放控制总文件
**
**--------------------------------------------------------------------------------------------------------
** Created By:          Renhaibo任海波
** Created date:        2018-09-03
** Version:             v1.0
** Descriptions:        音乐播放控制总文件
**
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
** Version:
** Description:
**
*********************************************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>

/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/

/*********************************************************************************************************
** Function name:       audio_set_vol
** Descriptions:        设置声卡音量
** input parameters:    vol：待设置的音量
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void audio_set_vol(uint16_t vol)
{
  struct rt_audio_caps caps;
  rt_device_t handle;
  
  handle = rt_device_find("sound0");
  if(handle != RT_NULL) {
    caps.main_type = AUDIO_TYPE_MIXER;
    caps.sub_type  = AUDIO_MIXER_VOLUME;
    caps.udata.value = vol;
    rt_device_control(handle, AUDIO_CTL_CONFIGURE, &caps);
  }
}

/*********************************************************************************************************
** Function name:       audio_get_vol
** Descriptions:        获取声卡音量
** input parameters:    vol：待设置的音量
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
uint16_t audio_get_vol(void)
{
  uint16_t vol = 0;
  struct rt_audio_caps caps;
  rt_device_t handle;
  
  handle = rt_device_find("sound0");
  if(handle != RT_NULL) {
    caps.main_type = AUDIO_TYPE_MIXER;
    caps.sub_type  = AUDIO_MIXER_VOLUME;
    rt_device_control(handle, AUDIO_CTL_GETCAPS, &caps);
    vol = caps.udata.value;
  }
  
  return vol;
}

///*********************************************************************************************************
//** Function name:       audio_set_rate
//** Descriptions:        配置声卡采样率
//** input parameters:    sample_rate: 音频文件采样率
//** output parameters:   NONE
//** Returned value:      NONE
//*********************************************************************************************************/
//void audio_set_rate(int sample_rate)
//{
//  struct rt_audio_caps caps;
//  rt_device_t handle;
//  
//  handle = rt_device_find("sound0");
//  if(handle != RT_NULL) {
//    caps.main_type = AUDIO_TYPE_OUTPUT;
//    caps.sub_type  = AUDIO_DSP_SAMPLERATE;
//    caps.udata.value = sample_rate;
//    rt_device_control(handle, AUDIO_CTL_CONFIGURE, &caps);
//  }
//}


/*********************************************************************************************************
** Function name:       cmd_vol
** Descriptions:        设置声卡音量
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      0：正确返回，无任何意义
*********************************************************************************************************/
int cmd_vol(int argc, char **argv)
{
    int vol;
    if (argc == 2)
    {
        vol = atoi(argv[1]);
        if(vol < 0) {
          vol = 0;
        }
        if(vol > 100) {
          vol =100;
        }
        audio_set_vol(vol);
        rt_kprintf("Set audio volume ok\r\n");
    }
    else
    {
        rt_kprintf("bad parameter! e.g: vol 60\r\n");
    }

    return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(cmd_vol, __cmd_vol,set the audio volume);
/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
