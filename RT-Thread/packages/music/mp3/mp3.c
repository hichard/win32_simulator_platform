/****************************************Copyright (c)****************************************************
**                             �� �� �� �� �� �� �� �� �� �� �� ˾
**                                http://www.6lowpanworld.com
**                                http://hichard.taobao.com
**
**
**
**--------------File Info---------------------------------------------------------------------------------
** File Name:           mp3.c
** Last modified Date:  2018-09-03
** Last Version:        V1.00
** Description:         mp3��ʽ��Ƶ�ļ��������
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
#include <string.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <dfs_posix.h>

#include "music/mp3/pub/mp3dec.h"

/*********************************************************************************************************
** ��������궨��
*********************************************************************************************************/
#define MP3_DEBUG   0

#if (MP3_DEBUG > 0)
#define mp3_trace(fmt, ...)           rt_kprintf(fmt, ##__VA_ARGS__)
#else
#define mp3_trace(fmt, ...)
#endif
/*********************************************************************************************************
** ����ṹ�壬������ȷʶ��wav��ʽ��Ƶ��ͷ��
*********************************************************************************************************/
//  ����mp3���봦������
#define MP3_AUDIO_BUF_SZ        (5 * 1024)

/*********************************************************************************************************
** ����һ��mp3���ŵĽṹ
*********************************************************************************************************/
struct mp3_decoder
{
  /* mp3 information */
  HMP3Decoder decoder;
  MP3FrameInfo frame_info;
  uint32_t frames;
  
  /* mp3 file descriptor */
  rt_size_t (*fetch_data)(void* parameter, rt_uint8_t *buffer, rt_size_t length);
  void* fetch_parameter;
  
  /* mp3 read session */
  rt_uint8_t *read_buffer, *read_ptr;
  rt_int32_t  read_offset;
  rt_uint32_t bytes_left, bytes_left_before_decoding;
  
  /* audio device */
  rt_device_t snd_device;
};

/*********************************************************************************************************
** ȫ�ֱ�������
*********************************************************************************************************/
static uint8_t *mp3_fd_buffer;
static int current_sample_rate = 0;
static int current_bit_per_sample = 0;
static int32_t current_offset = 0;

/*********************************************************************************************************
** Function name:       mp3_decoder_tx_done
** Descriptions:        ���������鲥����ɺ���
** input parameters:    dev:    �������
**                      buffer: ���Ż�����
** output parameters:   NONE
** Returned value:      RT_EOK�������� �������쳣��
*********************************************************************************************************/
static rt_err_t mp3_decoder_tx_done(rt_device_t dev, void *buffer)
{
  /* release memory block */
  rt_device_control(dev, AUDIO_CTL_FREEBUFFER, buffer);
  return RT_EOK;
}

/*********************************************************************************************************
** Function name:       mp3_decoder_init
** Descriptions:        mp3���빦�ܳ�ʼ��
** input parameters:    decoder: mp3��������ṹ
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void mp3_decoder_init(struct mp3_decoder* decoder)
{
  RT_ASSERT(decoder != RT_NULL);
  
  /* init read session */
  decoder->read_ptr = RT_NULL;
  decoder->bytes_left_before_decoding = decoder->bytes_left = 0;
  decoder->frames = 0;
  
  decoder->read_buffer = &mp3_fd_buffer[0];
  if (decoder->read_buffer == RT_NULL) return;
  
  decoder->decoder = MP3InitDecoder();
  
  /* open audio device */
  decoder->snd_device = rt_device_find("sound0");
  if (decoder->snd_device != RT_NULL)
  {
    /* set tx complete call back function */
    rt_device_set_tx_complete(decoder->snd_device, mp3_decoder_tx_done);
    rt_device_open(decoder->snd_device, RT_DEVICE_OFLAG_WRONLY);
  }
}

/*********************************************************************************************************
** Function name:       mp3_decoder_detach
** Descriptions:        ����mp3����
** input parameters:    decoder: mp3��������ṹ
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void mp3_decoder_detach(struct mp3_decoder* decoder)
{
  RT_ASSERT(decoder != RT_NULL);
  
  /* close audio device */
  if (decoder->snd_device != RT_NULL)
    rt_device_close(decoder->snd_device);
  
  /* release mp3 decoder */
  MP3FreeDecoder(decoder->decoder);
}

/*********************************************************************************************************
** Function name:       mp3_decoder_create
** Descriptions:        ����mp3����ṹ������ʼ��
** input parameters:    NONE
** output parameters:   NONE
** Returned value:     ������mp3_decoder
*********************************************************************************************************/
struct mp3_decoder* mp3_decoder_create(void)
{
  struct mp3_decoder* decoder;
  
  /* allocate object */
  decoder = (struct mp3_decoder*) rt_malloc (sizeof(struct mp3_decoder));
  
  if (decoder != RT_NULL)
  {
    mp3_decoder_init(decoder);
  }
  
  return decoder;
}

/*********************************************************************************************************
** Function name:       mp3_decoder_delete
** Descriptions:        ������ɾ��mp3����ṹ
** input parameters:    decoder: mp3��������ṹ
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void mp3_decoder_delete(struct mp3_decoder* decoder)
{
  RT_ASSERT(decoder != RT_NULL);
  
  /* de-init mp3 decoder object */
  mp3_decoder_detach(decoder);
  /* release this object */
  rt_free(decoder);
}

/*********************************************************************************************************
** Function name:       mp3_decoder_fill_buffer
** Descriptions:        ��ȡһЩ�µ����ݵ�������
** input parameters:    decoder: mp3��������ṹ
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static rt_int32_t mp3_decoder_fill_buffer(struct mp3_decoder* decoder)
{
  rt_size_t bytes_read;
  rt_size_t bytes_to_read;
  
  if (decoder->bytes_left > 0) {
    // better: move unused rest of buffer to the start
    rt_memmove(decoder->read_buffer, decoder->read_ptr, decoder->bytes_left);
  }
  
  bytes_to_read = (MP3_AUDIO_BUF_SZ - decoder->bytes_left) & ~(512 - 1);
  
  bytes_read = decoder->fetch_data(decoder->fetch_parameter,
                                   (rt_uint8_t *)(decoder->read_buffer + decoder->bytes_left),
                                   bytes_to_read);
  
  if (bytes_read > 0)
  {
    decoder->read_ptr = decoder->read_buffer;
    decoder->read_offset = 0;
    decoder->bytes_left = decoder->bytes_left + bytes_read;
    return 0;
  }
  else
  {
    //mp3_trace("can't read more data\n");
    return -1;
  }
}

/*********************************************************************************************************
** Function name:       mp3_decoder_run
** Descriptions:        mp3���룬ͬʱ��������wav������������
** input parameters:    decoder: mp3��������ṹ
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
int mp3_decoder_run(struct mp3_decoder* decoder)
{
  int err;
  rt_uint32_t  delta;
  struct rt_audio_buf_desc desc;
  
  RT_ASSERT(decoder != RT_NULL);
  
  if ((decoder->read_ptr == RT_NULL) || decoder->bytes_left < 2*MAINBUF_SIZE)
  {
    if(mp3_decoder_fill_buffer(decoder) != 0)
      return -1;
  }
  
  decoder->read_offset = MP3FindSyncWord(decoder->read_ptr, decoder->bytes_left);
  if (decoder->read_offset < 0)
  {
    /* discard this data */
    mp3_trace("outof sync, byte left: %d\n", decoder->bytes_left);
    decoder->bytes_left = 0;
    return 0;
  }
  
  decoder->read_ptr += decoder->read_offset;
  delta = decoder->read_offset;
  decoder->bytes_left -= decoder->read_offset;
  
  if (decoder->bytes_left < 1024)
  {
    /* fill more data */
    if(mp3_decoder_fill_buffer(decoder) != 0)
      return -1;
  }
  
  /* get a decoder buffer */
  rt_device_control(decoder->snd_device, AUDIO_CTL_ALLOCBUFFER, &desc);
  decoder->bytes_left_before_decoding = decoder->bytes_left;
  err = MP3Decode(decoder->decoder, &decoder->read_ptr,
                  (int*)&decoder->bytes_left, (short*)desc.data_ptr, 0);
  delta += (decoder->bytes_left_before_decoding - decoder->bytes_left);
  
  current_offset += delta;
  
  decoder->frames++;
  
  if (err != ERR_MP3_NONE) {
    switch (err) {
    case ERR_MP3_INDATA_UNDERFLOW:
      mp3_trace("ERR_MP3_INDATA_UNDERFLOW\n");
      decoder->bytes_left = 0;
      if(mp3_decoder_fill_buffer(decoder) != 0) {
        /* release this memory block */
        mp3_trace("mp3_decoder_fill_buffer != 0\r\n");
        rt_device_control(decoder->snd_device, AUDIO_CTL_FREEBUFFER, desc.data_ptr);
        return -1;
      }
      break;
      
    case ERR_MP3_MAINDATA_UNDERFLOW:
      /* do nothing - next call to decode will provide more mainData */
      mp3_trace("ERR_MP3_MAINDATA_UNDERFLOW\n");
      break;
      
    default:
      mp3_trace("unknown error: %d, left: %d\n", err, decoder->bytes_left);
      
      if (decoder->bytes_left > 0) {
        decoder->bytes_left --;
        decoder->read_ptr ++;
      }
      else {
        RT_ASSERT(0);
      }
      break;
    }
    rt_device_control(decoder->snd_device, AUDIO_CTL_FREEBUFFER, desc.data_ptr);
  }
  else {  
    int outputSamps;
    /* no error */
    MP3GetLastFrameInfo(decoder->decoder, &decoder->frame_info);
    
    /* set sample rate */
    if ((decoder->frame_info.samprate != current_sample_rate) || (decoder->frame_info.bitsPerSample != current_bit_per_sample))
    {
      struct rt_audio_caps caps;
      current_sample_rate = decoder->frame_info.samprate;
      caps.main_type = AUDIO_TYPE_OUTPUT;
      caps.sub_type = AUDIO_DSP_SAMPLERATE;
      caps.udata.value = current_sample_rate;
      rt_device_control(decoder->snd_device, AUDIO_CTL_CONFIGURE, &caps);
    
      current_bit_per_sample = decoder->frame_info.bitsPerSample;
      caps.main_type = AUDIO_TYPE_OUTPUT;
      caps.sub_type = AUDIO_DSP_FMT;
      caps.udata.value = current_bit_per_sample;
      rt_device_control(decoder->snd_device, AUDIO_CTL_CONFIGURE, &caps);
    }
    
    /* write to sound device */
    outputSamps = decoder->frame_info.outputSamps;
    if (outputSamps > 0)
    {
      if (decoder->frame_info.nChans == 1)
      {
        int i;
        for (i = outputSamps - 1; i >= 0; i--)
        {
          desc.data_ptr[i * 2] = desc.data_ptr[i];
          desc.data_ptr[i * 2 + 1] = desc.data_ptr[i];
        }
        outputSamps *= 2;
      }
      
      rt_device_write(decoder->snd_device, 0, desc.data_ptr, outputSamps * sizeof(rt_uint16_t));
    }
    else
    {
      /* no output */
      rt_device_control(decoder->snd_device, AUDIO_CTL_FREEBUFFER, desc.data_ptr);
    }
  }
  
  return 0;
}

/*********************************************************************************************************
** Function name:       fd_fetch
** Descriptions:        ��ȡmp3��Ƶ�ļ�����
** input parameters:    parameter: �ļ�ָ�룬Ҳ�����ǻ�����ָ��
**                      buffer:    ���ݶ�ȡ������
**                      length:    ��ȡ���ݳ��� 
** output parameters:   NONE
** Returned value:      ʵ�ʶ�ȡ�����ݳ���
*********************************************************************************************************/
rt_size_t fd_fetch(void* parameter, rt_uint8_t *buffer, rt_size_t length)
{
  int fd = (int)parameter;
  int read_bytes;
  
  read_bytes = read(fd, (char*)buffer, length);
  if (read_bytes <= 0) return 0;
  
  return read_bytes;
}

/*********************************************************************************************************
** Function name:       mp3
** Descriptions:        mp3���ֲ��ź���
** input parameters:    filename: ���ŵ�MP3�ļ���
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void mp3(char* filename)
{
  int fd;
  struct mp3_decoder* decoder;
  
  if(mp3_fd_buffer == RT_NULL) {
    mp3_fd_buffer = rt_malloc(MP3_AUDIO_BUF_SZ);
    if(mp3_fd_buffer == RT_NULL) {
      mp3_trace("malloc mp3_fd_buffer failed\n");
      return;
    }
  }
  
  fd = open(filename, O_RDONLY, 0);
  if (fd >= 0)
  {
    decoder = mp3_decoder_create();
    if (decoder != RT_NULL)
    {
      rt_device_control(decoder->snd_device, AUDIO_CTL_START, filename);
      decoder->fetch_data = fd_fetch;
      decoder->fetch_parameter = (void*)fd;
      
      current_offset = 0;
      while (mp3_decoder_run(decoder) != -1);
      
      /* delete decoder object */
      mp3_decoder_delete(decoder);
    }
    
    rt_free(mp3_fd_buffer);
    mp3_fd_buffer = RT_NULL;
    close(fd);
  }
}
/*********************************************************************************************************
END FILE
*********************************************************************************************************/
