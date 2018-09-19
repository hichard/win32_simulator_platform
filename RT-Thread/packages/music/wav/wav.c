/****************************************Copyright (c)****************************************************
**                             成 都 世 纪 华 宁 科 技 有 限 公 司
**                                http://www.6lowpanworld.com
**                                http://hichard.taobao.com
**
**
**
**--------------File Info---------------------------------------------------------------------------------
** File Name:           wav.c
** Last modified Date:  2018-09-03
** Last Version:        V1.00
** Description:         wav格式音频文件播放软件
**--------------------------------------------------------------------------------------------------------
** Created By:          Renhaibo
** Created date:        2018-09-03
** Version:             V1.00
** Descriptions:
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
** Version:
** Description:
*********************************************************************************************************/
#include <stdint.h>
#include <rtthread.h>
#include <dfs_posix.h>
#include <rtdevice.h>

/*********************************************************************************************************
** 定义结构体，用于正确识别wav格式音频的头部
*********************************************************************************************************/
#pragma pack(1)
//  RIFF头部
struct RIFF_HEADER_DEF
{
  char riff_id[4];     // 'R','I','F','F'
  uint32_t riff_size;
  char riff_format[4]; // 'W','A','V','E'
};

//  fmt 头部
struct WAVE_FORMAT_DEF
{
  uint16_t FormatTag;
  uint16_t Channels;
  uint32_t SamplesPerSec;
  uint32_t AvgBytesPerSec;
  uint16_t BlockAlign;
  uint16_t BitsPerSample;
};

struct FMT_BLOCK_DEF
{
  char fmt_id[4];    // 'f','m','t',' '
  uint32_t fmt_size;
  struct WAVE_FORMAT_DEF wav_format;
};
#pragma pack()

/*********************************************************************************************************
** Function name:       wav_tx_done
** Descriptions:        单个语音块播放完成函数
** input parameters:    dev:    声卡句柄
**                      buffer: 播放缓冲区
** output parameters:   NONE
** Returned value:      RT_EOK：正常； 其它：异常码
*********************************************************************************************************/
static rt_err_t wav_tx_done(rt_device_t dev, void *buffer)
{
  /* release memory block */
  rt_device_control(dev, AUDIO_CTL_FREEBUFFER, buffer);
  return RT_EOK;
}

/*********************************************************************************************************
** Function name:       wav
** Descriptions:        wav音乐播放函数，该函数对外 提供
** input parameters:    filename： 播放的音频文件文件名
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void wav(char* filename)
{
  char *buf;
  int fd;
  struct rt_audio_buf_desc desc;
  struct FMT_BLOCK_DEF   fmt_block;


  //打开文件
  fd = open(filename, O_RDONLY, 0);
  if (fd >= 0)
  {
    //rt_uint8_t* buf;
    int 	len;
    uint32_t ChunkSize;
    rt_device_t device;
    char riff_chunk[4];

    /* wav format check */
    do
    {
      len = read(fd, riff_chunk, sizeof(riff_chunk));
      if(len != sizeof(riff_chunk))
      {
        rt_kprintf("read riff chunk fail!\r\n");
        close(fd);
        return;
      }

      if(strncmp(riff_chunk, "RIFF", sizeof(riff_chunk)) == 0)
      {
        struct RIFF_HEADER_DEF riff_header;

        /* read riff header */
        len = read(fd, (void*)((uint32_t)&riff_header + sizeof(riff_chunk)),
                   sizeof(struct RIFF_HEADER_DEF) - sizeof(riff_chunk));
        if(strncmp(riff_header.riff_format, "WAVE", 4) != 0)
        {
          rt_kprintf("RIFF format error:%-4s\r\n", riff_header.riff_format);
          close(fd);
          return;
        }
      }
      else if(strncmp(riff_chunk, "fmt ", sizeof(riff_chunk)) == 0)
      {
        /* read riff format block */
        len = read(fd, (void*)((uint32_t)&fmt_block + sizeof(riff_chunk)),
                   sizeof(struct FMT_BLOCK_DEF) - sizeof(riff_chunk));
        if(len != sizeof(struct FMT_BLOCK_DEF) - sizeof(riff_chunk))
        {
          rt_kprintf("read riff format block fail!\r\n");
          close(fd);
          return;
        }

        if(fmt_block.fmt_size != 16)
        {
          char tmp[2];
          read(fd, tmp, fmt_block.fmt_size - 16);
        }

        if(fmt_block.wav_format.Channels != 2)
        {
          rt_kprintf("[err] only support stereo!\r\n");
          close(fd);
          return;
        }
      } else if(strncmp(riff_chunk, "data", 4) == 0) {
        break;
      } else {
        len = read(fd, &ChunkSize, 4);
        if(len != 4) {
          rt_kprintf("read else block fail!\r\n");
          close(fd);
        }
        lseek(fd, ChunkSize, SEEK_CUR);
      }
    } while(1);

    /* get data size */
    {
      rt_size_t size;
      len = read(fd, &size, 4);
      if(len != 4)
      {
        rt_kprintf("read data size fail!\r\n");
        close(fd);
        return;
      }

      /* print play time */
      {
        uint32_t hour, min, sec;

        hour = min = 0;
        sec = size / fmt_block.wav_format.AvgBytesPerSec;

        if(sec / (60*60))
        {
          hour = sec / (60*60);
          sec -= hour * (60*60);
        }

        if(sec / 60)
        {
          min = sec / 60;
          sec -= min * 60;
        }

        /* dump wav info, (only in finsh) */
        if(strncmp(rt_thread_self()->name, "tshell", sizeof("tshell") -1) == 0)
        {
          rt_kprintf("wav info:\r\n");
          rt_kprintf("Channels:%d ", fmt_block.wav_format.Channels);
          rt_kprintf("SamplesPerSec:%d ", fmt_block.wav_format.SamplesPerSec);
          rt_kprintf("BitsPerSample:%d\r\n", fmt_block.wav_format.BitsPerSample);
          rt_kprintf("play time: %02d:%02d:%02d\r\n", hour, min, sec);
        }
      }
    } /* get data size */

    /* open audio device and set tx done call back */
    device = rt_device_find("sound0");
    if(device == RT_NULL)
    {
      rt_kprintf("audio device not found!\r\n");
      close(fd);
      return;
    }
    //设置发送完成回调函数,让DAC数据发完时执行wav_tx_done函数释放空间.
    rt_device_set_tx_complete(device, wav_tx_done);
    rt_device_open(device, RT_DEVICE_OFLAG_WRONLY);

    /* set samplerate */
    {
      int SamplesPerSec = fmt_block.wav_format.SamplesPerSec;
      struct rt_audio_caps caps;
      caps.main_type = AUDIO_TYPE_OUTPUT;
      caps.sub_type = AUDIO_DSP_SAMPLERATE;
      caps.udata.value = SamplesPerSec;
      if(rt_device_control(device, AUDIO_CTL_CONFIGURE, &caps) != RT_EOK) {
        rt_kprintf("audio device doesn't support this sample rate: %d\r\n",
                   SamplesPerSec);
        close(fd);
        rt_device_close(device);
        return;
      }

      int BitsPerSample = fmt_block.wav_format.BitsPerSample;
      caps.main_type = AUDIO_TYPE_OUTPUT;
      caps.sub_type = AUDIO_DSP_FMT;
      caps.udata.value = BitsPerSample;
      rt_device_control(device, AUDIO_CTL_CONFIGURE, &caps);
    }

    rt_device_control(device, AUDIO_CTL_START, filename);

    do
    {
      rt_device_control(device, AUDIO_CTL_ALLOCBUFFER, &desc);
      if(desc.data_ptr == RT_NULL) {
        rt_thread_delay(10);
        continue;
      }
      len = read(fd, desc.data_ptr, 4096);
      if(len > 0) {
        rt_device_write(device, 0, desc.data_ptr, len);
      } else {
        rt_device_control(device, AUDIO_CTL_FREEBUFFER, desc.data_ptr);
      }
    } while (len > 0);

    /* close device and file */
    rt_device_close(device);
    close(fd);
  }
}

#ifdef RT_USING_FINSH
#include <finsh.h>
int cmd_wav(int argc, char **argv)
{
    rt_device_t tid;

    if (argc == 2)
    {
        wav(argv[1]);
    } else {
        rt_kprintf("bad parameter! e.g: wav /test.wav\r\n");
    }

    return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(cmd_wav, __cmd_wav, play wav music);
#endif
/*********************************************************************************************************
END FILE
*********************************************************************************************************/
