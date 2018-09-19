/****************************************Copyright (c)****************************************************
**                             �� �� �� �� �� �� �� �� �� �� �� ˾
**                                http://www.6lowpanworld.com
**                                http://hichard.taobao.com
**
**
**--------------File Info---------------------------------------------------------------------------------
** File Name:           audio_codec_win32.c
** Last modified Date:  2018-09-19
** Last Version:        v1.00
** Description:         windows�µ���������
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
** �����ṹ��ض���
*********************************************************************************************************/
struct codec_device
{
  /* wavein ��أ�����¼��  */


  /* waveout ��أ����ڲ�����Ƶ  */
  WAVEFORMATEX WaveoutFormat;   // ������Ƶ���ݸ�ʽ�ṹ
  LPHWAVEOUT hWaveOut;          // �����豸���
  LPWAVEHDR  lpWaveoutHdr;      // �������ݽṹ
  uint8_t    WaveoutVolume;     // ��������
};

/*********************************************************************************************************
** ȫ�ֱ�������
*********************************************************************************************************/
static struct codec_device      __g_win32_codec;
static struct rt_audio_device   __g_audio_device;


/*********************************************************************************************************
** Function name:       waveout_volume_set
** Descriptions:        ���ò���������С
** Input parameters:    v��������С��0-100����
** Output parameters:   NONE
** Returned value:      >=0: ���óɹ�; ����������ʧ��
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
** Descriptions:        waveout������Ƶ�ص�����
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
}

/*********************************************************************************************************
**   Audio device�� �����ǰ��տ�ܵ���������
*********************************************************************************************************/

/*********************************************************************************************************
** Function name:       icodec_getcaps
** Descriptions:        ��������--->��ȡ��������
** Input parameters:    audio: ��Ƶ�����ṹ
** Output parameters:   NONE
** Returned value:      ִ�н��
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
** Descriptions:        ��������--->��������
** Input parameters:    audio: ��Ƶ�����ṹ
** Output parameters:   NONE
** Returned value:      ִ�н��
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
          //sample_rate(icodec, icodec->SampleRate);
        }
        break;
      case AUDIO_DSP_FMT:
        {
          icodec->WaveoutFormat.wBitsPerSample= caps->udata.value;
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
** Descriptions:        ��������--->��ʼ��
** Input parameters:    audio: ��Ƶ�����ṹ
** Output parameters:   NONE
** Returned value:      ִ�н��
*********************************************************************************************************/
static rt_err_t icodec_init(struct rt_audio_device *audio)
{
  //struct codec_device *icodec = (struct codec_device *)audio->parent.user_data;

  return RT_EOK;
}

/*********************************************************************************************************
** Function name:       icodec_shutdown
** Descriptions:        ��������--->�ر�����
** Input parameters:    audio: ��Ƶ�����ṹ
** Output parameters:   NONE
** Returned value:      ִ�н��
*********************************************************************************************************/
static rt_err_t icodec_shutdown(struct rt_audio_device *audio)
{
  struct codec_device *icodec = (struct codec_device *)audio->parent.user_data;

  waveOutClose(icodec->hWaveOut);
  return RT_EOK;
}

/*********************************************************************************************************
** Function name:       icodec_shutdown
** Descriptions:        ��������--->��������
** Input parameters:    audio: ��Ƶ�����ṹ
** Output parameters:   NONE
** Returned value:      ִ�н��
*********************************************************************************************************/
rt_err_t icodec_start(struct rt_audio_device *audio,int stream)
{
//     WAVEFORMATEX wfx;
//
//    wfx.wFormatTag = WAVE_FORMAT_PCM;
//    wfx.nChannels = nChannels;
//    wfx.nSamplesPerSec = nSamplesPerSec;
//    wfx.wBitsPerSample = wBitsPerSample;
//    wfx.cbSize = 0;
//    wfx.nBlockAlign = wfx.wBitsPerSample * wfx.nChannels / 8;
//    wfx.nAvgBytesPerSec = wfx.nChannels * wfx.nSamplesPerSec * wfx.wBitsPerSample / 8;
//
//    /* 'waveOutOpen' will call 'SetEvent'. */
//    if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, (DWORD_PTR)hEventPlay, 0, CALLBACK_EVENT))
//    {
//      return -1;
//    }

    return RT_EOK;
}

/*********************************************************************************************************
** Function name:       icodec_stop
** Descriptions:        ��������--->ֹͣ����
** Input parameters:    audio: ��Ƶ�����ṹ
** Output parameters:   NONE
** Returned value:      ִ�н��
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
** Descriptions:        ��������--->��ͣ����������������
** Input parameters:    audio: ��Ƶ�����ṹ
** Output parameters:   NONE
** Returned value:      ִ�н��
*********************************************************************************************************/
static rt_err_t icodec_suspend(struct rt_audio_device *audio,int stream)
{
  struct codec_device *icodec = (struct codec_device *)audio->parent.user_data;

  waveOutPause(icodec->hWaveOut);
  return RT_EOK;
}

/*********************************************************************************************************
** Function name:       icodec_resume
** Descriptions:        ��������--->������������
** Input parameters:    audio: ��Ƶ�����ṹ
** Output parameters:   NONE
** Returned value:      ִ�н��
*********************************************************************************************************/
static rt_err_t  icodec_resume(struct rt_audio_device *audio,int stream)
{
  struct codec_device *icodec = (struct codec_device *)audio->parent.user_data;

  waveOutRestart(icodec->hWaveOut);
  return RT_EOK;
}

/*********************************************************************************************************
** Function name:       icodec_control
** Descriptions:        ��������--->�������ƺ���
** Input parameters:    audio: ��Ƶ�����ṹ
** Output parameters:   NONE
** Returned value:      ִ�н��
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
** Descriptions:        ��������--->�������ݴ���
** Input parameters:    audio: ��Ƶ�����ṹ
** Output parameters:   NONE
** Returned value:      ִ�н��
*********************************************************************************************************/
static rt_size_t icodec_transmit(struct rt_audio_device *audio, const void *writeBuf, void *readBuf, rt_size_t size)
{
  struct codec_device *icodec = (struct codec_device *)audio->parent.user_data;

//  if(writeBuf != RT_NULL)
//  {
//    if((DMAIsEnabled(AUDIO_I2S_DMA_BASE, AUDIO_I2S_DMA_STREAM))) {
//      return 0;
//    }
//
//    icodec->send_buf = (uint8_t *)writeBuf;
//    //I2SDisable(CODEC_I2S_BASE);
//    DMADisable(AUDIO_I2S_DMA_BASE, AUDIO_I2S_DMA_STREAM);
//    DMAModeConfigSet(AUDIO_I2S_DMA_BASE, AUDIO_I2S_DMA_STREAM,
//                     AUDIO_I2S_DMA_CHANNEL |
//                       DMA_DIR_MemoryToPeripheral |
//                         DMA_PeripheralInc_Disable |
//                           DMA_MemoryInc_Enable |
//                             DMA_PeripheralDataSize_HalfWord |
//                               DMA_MemoryDataSize_HalfWord |
//                                 DMA_Mode_Normal |
//                                   DMA_Priority_High |
//                                     DMA_MemoryBurst_Single |
//                                       DMA_PeripheralBurst_Single);
//    DMAFIFOConfigSet(AUDIO_I2S_DMA_BASE, AUDIO_I2S_DMA_STREAM,
//                     DMA_FIFOMode_Disable | DMA_FIFOThreshold_Full);
//    //DMAFlowControllerConfig(AUDIO_I2S_DMA_BASE, AUDIO_I2S_DMA_STREAM, DMA_FlowCtrl_Peripheral);
//    DMAAddrSet(AUDIO_I2S_DMA_BASE, AUDIO_I2S_DMA_STREAM, (rt_uint32_t)writeBuf, (CODEC_I2S_BASE + SPI_DR));
//    DMABufferSizeSet(AUDIO_I2S_DMA_BASE, AUDIO_I2S_DMA_STREAM, size >> 1);
//
//    SPI_I2S_DMAEnable(CODEC_I2S_BASE,SPI_I2S_DMA_TX);
//    DMAIntEnable(AUDIO_I2S_DMA_BASE, AUDIO_I2S_DMA_STREAM, DMA_INT_CONFIG_TC/*DMA_INT_CONFIG_USUAL*/);
//    // ��ʼDMA����
//    DMAEnable(AUDIO_I2S_DMA_BASE, AUDIO_I2S_DMA_STREAM);
//    //I2SEnable(CODEC_I2S_BASE);
//
//    return size;
//  }

  return 0;
}

/*********************************************************************************************************
** ���������ṹ������׼��ע������
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
** Descriptions:        ע�������豸����
** Input parameters:    NONE
** Output parameters:   NONE
** Returned value:      ִ�н��
*********************************************************************************************************/
int rt_hw_codec_init(void)
{
  int result;
  struct rt_audio_device *audio   = &__g_audio_device;
  //struct codec_device *icodec = &__g_win32_codec;

  /**
  * Step1, ��ʼ����ر���
  */
  audio->ops = (struct rt_audio_ops*)&__g_audio_ops;
  memset(&__g_win32_codec, 0, sizeof(__g_win32_codec));

  /**
  * Step2, ע���豸����
  */
  result = rt_audio_register(audio,"sound0", RT_DEVICE_FLAG_WRONLY, &__g_win32_codec);

  return result;
}

/*********************************************************************************************************
** �����Զ���ʼ������
*********************************************************************************************************/
INIT_DEVICE_EXPORT(rt_hw_codec_init);

/*********************************************************************************************************
END FILE
*********************************************************************************************************/

