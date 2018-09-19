/****************************************Copyright (c)****************************************************
**                             成 都 世 纪 华 宁 科 技 有 限 公 司
**                                http://www.6lowpanworld.com
**                                http://hichard.taobao.com
**
**
**--------------File Info---------------------------------------------------------------------------------
** File Name:           audio_codec_win32.c
** Last modified Date:  2018-09-19
** Last Version:        v1.00
** Description:         windows下的声卡驱动
**--------------------------------------------------------------------------------------------------------
** Created By:          Renhaibo
** Created date:        2018-09-19
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
#include <rtlibc.h>

#include <windows.h>
/*********************************************************************************************************
** 驱动结构相关定义
*********************************************************************************************************/
struct codec_device
{
  /* wavein 相关，用于录音  */


  /* waveout 相关，用于播放音频  */
  WAVEFORMATEX WaveoutFormat;   // 播放音频数据格式结构
  LPHWAVEOUT hWaveOut;          // 播放设备句柄
  LPWAVEHDR  lpWaveoutHdr;      // 播放数据结构
  uint8_t    WaveoutVolume;     // 播放音量
};

/*********************************************************************************************************
** 全局变量定义
*********************************************************************************************************/
static struct codec_device      __g_win32_codec;
static struct rt_audio_device   __g_audio_device;


/*********************************************************************************************************
** Function name:       waveout_volume_set
** Descriptions:        设置播放声音大小
** Input parameters:    v：声音大小，0-100的数
** Output parameters:   NONE
** Returned value:      >=0: 设置成功; 其它：设置失败
*********************************************************************************************************/
int waveout_volume_set(struct codec_device *icodec, uint16_t v)
{
    DWORD temp;

    if(icodec->hWaveOut == NULL) {
       return -1;
    }
    if(v > 100) {
        v = 100;
    }
    temp = v;
    temp = (65535 * temp) / 100;
    temp = temp | (temp << 16);
    waveOutSetVolume(icodec->hWaveOut, temp);
}

/*********************************************************************************************************
** Function name:       codec_audio_waveout_callback
** Descriptions:        waveout播放音频回调函数
** Input parameters:    NONE
** Output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void codec_audio_waveout_callback(
   HWAVEOUT  hwo,
   UINT      uMsg,
   DWORD_PTR dwInstance,
   DWORD_PTR dwParam1,
   DWORD_PTR dwParam2)
{
    struct codec_device *icodec = (struct codec_device *)dwInstance;

    if(uMsg == WOM_DONE ) {
       if(__g_win32_codec.hWaveOut != NULL ) {
           waveOutUnprepareHeader(__g_win32_codec.hWaveOut,__g_win32_codec.lpWaveoutHdr,sizeof(WAVEHDR));
           rt_audio_tx_complete(&__g_audio_device, __g_win32_codec.lpWaveoutHdr->lpData);
           rt_free(__g_win32_codec.lpWaveoutHdr);
           __g_win32_codec.lpWaveoutHdr = NULL;
       }
    }
}

/*********************************************************************************************************
**   Audio device， 以下是按照框架的声卡驱动
*********************************************************************************************************/

/*********************************************************************************************************
** Function name:       icodec_getcaps
** Descriptions:        驱动函数--->获取声卡配置
** Input parameters:    audio: 音频驱动结构
** Output parameters:   NONE
** Returned value:      执行结果
*********************************************************************************************************/
static rt_err_t icodec_getcaps(struct rt_audio_device *audio,struct rt_audio_caps *caps)
{
  rt_err_t result = RT_EOK;
  struct codec_device *icodec = (struct codec_device *)audio->parent.user_data;

  switch (caps->main_type)
  {
  case AUDIO_TYPE_QUERY: /* qurey the types of hw_codec device */
    {
      switch (caps->sub_type)
      {
      case AUDIO_TYPE_QUERY:
        caps->udata.mask = AUDIO_TYPE_OUTPUT | AUDIO_TYPE_MIXER;
        break;
      default:
        result = -RT_ERROR;
        break;
      }

      break;
    }
  case AUDIO_TYPE_OUTPUT: /* Provide capabilities of OUTPUT unit */
    switch (caps->sub_type)
    {
    case AUDIO_DSP_PARAM:
      if (audio->replay == NULL)
      {
        result = -RT_ERROR;
        break;
      }
      caps->udata.config.channels     = icodec->WaveoutFormat.nChannels;
      caps->udata.config.samplefmt    = icodec->WaveoutFormat.wBitsPerSample;
      caps->udata.config.samplerate   = icodec->WaveoutFormat.nSamplesPerSec;
      caps->udata.config.samplefmts   = icodec->WaveoutFormat.wFormatTag;
      break;
    case AUDIO_DSP_SAMPLERATE:
      caps->udata.value = icodec->WaveoutFormat.nSamplesPerSec;
      break;
    case AUDIO_DSP_FMT:
      caps->udata.value = icodec->WaveoutFormat.wBitsPerSample;
      break;
    default:
      result = -RT_ERROR;
      break;
    }
    break;
  case AUDIO_TYPE_MIXER: /* report the Mixer Units */
    switch (caps->sub_type)
    {
    case AUDIO_MIXER_QUERY:
      caps->udata.mask = AUDIO_MIXER_VOLUME | AUDIO_MIXER_DIGITAL | AUDIO_MIXER_LINE;
      break;
    case AUDIO_MIXER_VOLUME:
      caps->udata.value = icodec->WaveoutVolume;
      break;
    case AUDIO_MIXER_DIGITAL:
      break;
    case AUDIO_MIXER_LINE:
      break;
    default:
      result = -RT_ERROR;
      break;
    }
    break;
  default:
    result = -RT_ERROR;
    break;
  }

  return result;
}

/*********************************************************************************************************
** Function name:       icodec_configure
** Descriptions:        驱动函数--->配置声卡
** Input parameters:    audio: 音频驱动结构
** Output parameters:   NONE
** Returned value:      执行结果
*********************************************************************************************************/
static rt_err_t icodec_configure(struct rt_audio_device *audio,struct rt_audio_caps *caps)
{
  rt_err_t result = RT_EOK;
  struct codec_device *icodec = (struct codec_device *)audio->parent.user_data;

  switch (caps->main_type)
  {
  case AUDIO_TYPE_MIXER:
    {
      switch (caps->sub_type)
      {
      case AUDIO_MIXER_VOLUME:
        {
          icodec->WaveoutVolume = caps->udata.value;
          waveout_volume_set(icodec, caps->udata.value);
        }
        break;
      default:
        {
          result = -RT_ERROR;
        }
        break;
      }
    }
    break;
  case AUDIO_TYPE_OUTPUT:
    {
      switch (caps->sub_type)
      {
      case AUDIO_DSP_PARAM:
        {

        } break;
      case AUDIO_DSP_SAMPLERATE:
        {
          icodec->WaveoutFormat.nSamplesPerSec = caps->udata.value;
          icodec->WaveoutFormat.nBlockAlign = (icodec->WaveoutFormat.wBitsPerSample * icodec->WaveoutFormat.nChannels) >> 3;
          icodec->WaveoutFormat.nAvgBytesPerSec = icodec->WaveoutFormat.nBlockAlign * icodec->WaveoutFormat.nSamplesPerSec;
          if(icodec->hWaveOut != NULL) {
              waveOutOpen(&icodec->hWaveOut, (UINT)icodec->hWaveOut, &icodec->WaveoutFormat,
                        (DWORD_PTR)codec_audio_waveout_callback, icodec, CALLBACK_FUNCTION);
          }
        }
        break;
      case AUDIO_DSP_FMT:
        {
          icodec->WaveoutFormat.wBitsPerSample= caps->udata.value;
          icodec->WaveoutFormat.nBlockAlign = (icodec->WaveoutFormat.wBitsPerSample * icodec->WaveoutFormat.nChannels) >> 3;
          icodec->WaveoutFormat.nAvgBytesPerSec = icodec->WaveoutFormat.nBlockAlign * icodec->WaveoutFormat.nSamplesPerSec;
          if(icodec->hWaveOut != NULL) {
              waveOutOpen(&icodec->hWaveOut, (UINT)icodec->hWaveOut, &icodec->WaveoutFormat,
                        (DWORD_PTR)codec_audio_waveout_callback, icodec, CALLBACK_FUNCTION);
          }
        }
        break;
      default:
        {
          result = -RT_ERROR;
        }
        break;
      }
    }
    break;
  default:
    result = -RT_ERROR;
    break;
  }

  return result;
}

/*********************************************************************************************************
** Function name:       icodec_init
** Descriptions:        驱动函数--->初始化
** Input parameters:    audio: 音频驱动结构
** Output parameters:   NONE
** Returned value:      执行结果
*********************************************************************************************************/
static rt_err_t icodec_init(struct rt_audio_device *audio)
{
    struct codec_device *icodec = (struct codec_device *)audio->parent.user_data;

    icodec->WaveoutFormat.nSamplesPerSec = 44100;
    icodec->WaveoutFormat.wBitsPerSample = 16;
    icodec->WaveoutFormat.nChannels = 2;
    icodec->WaveoutFormat.cbSize = 0;
    icodec->WaveoutFormat.wFormatTag = WAVE_FORMAT_PCM;
    icodec->WaveoutFormat.nBlockAlign = (icodec->WaveoutFormat.wBitsPerSample * icodec->WaveoutFormat.nChannels) >> 3;
    icodec->WaveoutFormat.nAvgBytesPerSec = icodec->WaveoutFormat.nBlockAlign * icodec->WaveoutFormat.nSamplesPerSec;

    return RT_EOK;
}

/*********************************************************************************************************
** Function name:       icodec_shutdown
** Descriptions:        驱动函数--->关闭声卡
** Input parameters:    audio: 音频驱动结构
** Output parameters:   NONE
** Returned value:      执行结果
*********************************************************************************************************/
static rt_err_t icodec_shutdown(struct rt_audio_device *audio)
{
  struct codec_device *icodec = (struct codec_device *)audio->parent.user_data;

  waveOutPause(icodec->hWaveOut);
  return RT_EOK;
}

/*********************************************************************************************************
** Function name:       icodec_shutdown
** Descriptions:        驱动函数--->启动声卡
** Input parameters:    audio: 音频驱动结构
** Output parameters:   NONE
** Returned value:      执行结果
*********************************************************************************************************/
rt_err_t icodec_start(struct rt_audio_device *audio,int stream)
{
    struct codec_device *icodec = (struct codec_device *)audio->parent.user_data;

    if(icodec->hWaveOut == NULL) {
        if (waveOutOpen(&icodec->hWaveOut, WAVE_MAPPER, &icodec->WaveoutFormat,
                        (DWORD_PTR)codec_audio_waveout_callback, icodec, CALLBACK_FUNCTION))
        {
          return -1;
        }
    } else {
        if (waveOutOpen(&icodec->hWaveOut, (UINT)icodec->hWaveOut, &icodec->WaveoutFormat,
                        (DWORD_PTR)codec_audio_waveout_callback, icodec, CALLBACK_FUNCTION))
        {
          return -1;
        }
    }

    icodec->WaveoutVolume = 50;
    waveout_volume_set(icodec, icodec->WaveoutVolume);

    return RT_EOK;
}

/*********************************************************************************************************
** Function name:       icodec_stop
** Descriptions:        驱动函数--->停止声卡
** Input parameters:    audio: 音频驱动结构
** Output parameters:   NONE
** Returned value:      执行结果
*********************************************************************************************************/
rt_err_t icodec_stop(struct rt_audio_device *audio,int stream)
{
  struct codec_device *icodec = (struct codec_device *)audio->parent.user_data;

  waveOutClose(icodec->hWaveOut);
  icodec->hWaveOut = NULL;
  return RT_EOK;
}

/*********************************************************************************************************
** Function name:       icodec_suspend
** Descriptions:        驱动函数--->暂停声卡，即挂起声卡
** Input parameters:    audio: 音频驱动结构
** Output parameters:   NONE
** Returned value:      执行结果
*********************************************************************************************************/
static rt_err_t icodec_suspend(struct rt_audio_device *audio,int stream)
{
  struct codec_device *icodec = (struct codec_device *)audio->parent.user_data;

  waveOutPause(icodec->hWaveOut);
  return RT_EOK;
}

/*********************************************************************************************************
** Function name:       icodec_resume
** Descriptions:        驱动函数--->声卡继续工作
** Input parameters:    audio: 音频驱动结构
** Output parameters:   NONE
** Returned value:      执行结果
*********************************************************************************************************/
static rt_err_t  icodec_resume(struct rt_audio_device *audio,int stream)
{
  struct codec_device *icodec = (struct codec_device *)audio->parent.user_data;

  waveOutRestart(icodec->hWaveOut);
  return RT_EOK;
}

/*********************************************************************************************************
** Function name:       icodec_control
** Descriptions:        驱动函数--->声卡控制函数
** Input parameters:    audio: 音频驱动结构
** Output parameters:   NONE
** Returned value:      执行结果
*********************************************************************************************************/
static rt_err_t icodec_control (struct rt_audio_device *audio, int cmd, void *args)
{
  rt_err_t result = RT_EOK;
  struct codec_device *icodec = (struct codec_device *)audio->parent.user_data;

  switch (cmd)
  {
  case AUDIO_CTL_HWRESET:
    break;
  default:
    result = -RT_ERROR;
    break;
  }

  return result;
}

/*********************************************************************************************************
** Function name:       icodec_transmit
** Descriptions:        驱动函数--->声卡数据传输
** Input parameters:    audio: 音频驱动结构
** Output parameters:   NONE
** Returned value:      执行结果
*********************************************************************************************************/
static rt_size_t icodec_transmit(struct rt_audio_device *audio, const void *writeBuf, void *readBuf, rt_size_t size)
{
  struct codec_device *icodec = (struct codec_device *)audio->parent.user_data;

  if(writeBuf != RT_NULL)
  {
    if(icodec->lpWaveoutHdr != NULL) {
        return 0;
    }

    icodec->lpWaveoutHdr = rt_malloc(sizeof(WAVEHDR));
    if(icodec->lpWaveoutHdr == NULL) {
        return 0;
    }
    ZeroMemory(icodec->lpWaveoutHdr, sizeof(WAVEHDR));
    icodec->lpWaveoutHdr->dwLoops = 1;
    icodec->lpWaveoutHdr->dwBufferLength = size;
    icodec->lpWaveoutHdr->lpData = writeBuf;
    waveOutPrepareHeader(icodec->hWaveOut, icodec->lpWaveoutHdr, sizeof(WAVEHDR));
    waveOutWrite(icodec->hWaveOut, icodec->lpWaveoutHdr, sizeof(WAVEHDR));

    return size;
  }

  return 0;
}

/*********************************************************************************************************
** 定义声卡结构变量，准备注册声卡
*********************************************************************************************************/
const struct rt_audio_ops       __g_audio_ops =
{
  .getcaps    = icodec_getcaps,
  .configure  = icodec_configure,

  .init       = icodec_init,
  .shutdown   = icodec_shutdown,
  .start      = icodec_start,
  .stop       = icodec_stop,
  .suspend    = icodec_suspend,
  .resume     = icodec_resume,
  .control    = icodec_control,

  .transmit   = icodec_transmit,
};

/*********************************************************************************************************
** Function name:       rt_hw_codec_init
** Descriptions:        注册声卡设备驱动
** Input parameters:    NONE
** Output parameters:   NONE
** Returned value:      执行结果
*********************************************************************************************************/
int rt_hw_codec_init(void)
{
  int result;
  struct rt_audio_device *audio   = &__g_audio_device;
  //struct codec_device *icodec = &__g_win32_codec;

  /**
  * Step1, 初始化相关变量
  */
  audio->ops = (struct rt_audio_ops*)&__g_audio_ops;
  memset(&__g_win32_codec, 0, sizeof(__g_win32_codec));

  /**
  * Step2, 注册设备驱动
  */
  result = rt_audio_register(audio,"sound0", RT_DEVICE_FLAG_WRONLY, &__g_win32_codec);

  return result;
}

/*********************************************************************************************************
** 加入自动初始化序列
*********************************************************************************************************/
INIT_DEVICE_EXPORT(rt_hw_codec_init);

/*********************************************************************************************************
END FILE
*********************************************************************************************************/

