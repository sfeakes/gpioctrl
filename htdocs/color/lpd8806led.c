/*
 * lpd8806led.c
 *
 * Copyright 2012 Christopher De Vries
 * This program is distributed under the Artistic License 2.0, a copy of which
 * is included in the file LICENSE.txt
 */
#include "lpd8806led.h"
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <errno.h>
#include <math.h>

void write_frame(lpd8806_color *p, uint8_t red, uint8_t green, uint8_t blue);
uint8_t make_flag(uint8_t red, uint8_t greem, uint8_t blue);
ssize_t write_all(int filedes, const void *buf, size_t size);

static uint8_t gamma_table_red[256];
static uint8_t gamma_table_green[256];
static uint8_t gamma_table_blue[256];

int lpd8806_init(lpd8806_buffer *buf, int leds) { 
  buf->leds = leds;
  buf->size = (leds+3)*sizeof(lpd8806_color);
  buf->buffer = (lpd8806_color*)malloc(buf->size);
  
  if(buf->buffer==NULL) {
    return -1;
  }

//printf("buf Pointer=%d size=%d strut=%d\n",buf->buffer,buf->size,sizeof(lpd8806_color));

  buf->pixels = buf->buffer;
  ++buf->pixels;

//printf("pxls=%d pxls[0]=%d pxls[1]=%d pslx[50]=%d\n",buf->pixels,&buf->pixels[0],&buf->pixels[1],&buf->pixels[leds]);

  write_frame(buf->buffer,0x00,0x00,0x00);
  write_frame(&buf->pixels[leds],0x00,0x00,0x00);
  write_frame(&buf->pixels[leds+1],0x00,0x00,0x00);
  //write_frame(buf->pixels+leds,0x00,0x00,0x00);
  //write_frame(buf->pixels+leds+1,0x00,0x00,0x00);

  return 0;
}

int spi_init(int filedes) {
  int ret;
  const uint8_t mode = SPI_MODE_0;
  const uint8_t bits = 8;
  const uint32_t speed = 15000000;

  ret = ioctl(filedes,SPI_IOC_WR_MODE, &mode);
  if(ret==-1) {
    return -1;
  }

  ret = ioctl(filedes,SPI_IOC_WR_BITS_PER_WORD, &bits);
  if(ret==-1) {
    return -1;
  }

  ret = ioctl(filedes,SPI_IOC_WR_MAX_SPEED_HZ,&speed);
  if(ret==-1) {
    return -1;
  }

  return 0;
}

void write_color(lpd8806_color *p, uint8_t red, uint8_t green, uint8_t blue) {
  // uint8_t flag;

  // flag = make_flag(red,green,blue);
  write_frame(p,red,green,blue);
}

void print_actual_buf(lpd8806_buffer *buf)
{
  int i;
  
  for (i=-1; i< buf->leds+2; i++)
  {
    printf("LED %d = r:%d,g:%d,b:%d",i+1,buf->pixels[i].red,buf->pixels[i].green,buf->pixels[i].blue);
    if (i == -1)
      printf(" initilization\n");
    else if (i >= buf->leds)
      printf(" latch\n");
    else
      printf("\n");
  }
}

void print_ascii_buf(lpd8806_buffer *buf)
{
  int i;
  
  for (i=-1; i< buf->leds+2; i++)
  {
    if (i == -1)
      printf("_");
    else if (i >= buf->leds)
      printf("_");
    else { /*
      111 w
      000 k
      100 r
      010 g
      001 b
      110 y
      101 m
      011 c */
      if ( buf->pixels[i].red > 128 && buf->pixels[i].green > 128 && buf->pixels[i].blue > 128)
        printf("w");
      else if ( buf->pixels[i].red <= 128 && buf->pixels[i].green <= 128 && buf->pixels[i].blue <= 128)
        printf("k");
      else if ( buf->pixels[i].red > 128 && buf->pixels[i].green <= 128 && buf->pixels[i].blue <= 128)
        printf("r");
      else if ( buf->pixels[i].red <= 128 && buf->pixels[i].green > 128 && buf->pixels[i].blue <= 128)
        printf("g");
      else if ( buf->pixels[i].red <= 128 && buf->pixels[i].green <= 128 && buf->pixels[i].blue > 128)
        printf("b");
      else if ( buf->pixels[i].red > 128 && buf->pixels[i].green > 128 && buf->pixels[i].blue <= 128)
        printf("y");
      else if ( buf->pixels[i].red > 128 && buf->pixels[i].green <= 128 && buf->pixels[i].blue > 128)
        printf("m");
      else if ( buf->pixels[i].red <= 128 && buf->pixels[i].green > 128 && buf->pixels[i].blue > 128)
        printf("c");
      else
        printf("x");
    }
  }
  printf("\n");
}


int send_buffer(int filedes, lpd8806_buffer *buf) {
  int ret;

  //print_actual_buf(buf);
  //print_ascii_buf(buf);

  ret = (int)write_all(filedes,buf->buffer,buf->size);
  return ret;
}

void lpd8806_free(lpd8806_buffer *buf) {
  free(buf->buffer);
  buf->buffer=NULL;
  buf->pixels=NULL;
  //buf=NULL;
}

void write_frame(lpd8806_color *p, uint8_t red, uint8_t green, uint8_t blue) {
  // p->flag=flag;
  p->blue=blue;
  p->green=green;
  p->red=red;
  // printf ("index : %i %i %i %i\n",p, p->red  , p->green, p->blue);
}

uint8_t make_flag(uint8_t red, uint8_t green, uint8_t blue) {
  uint8_t flag;

  flag =  (red&0xc0)>>6;
  flag |= (green&0xc0)>>4;
  flag |= (blue&0xc0)>>2;

  return ~flag;
}

ssize_t write_all(int filedes, const void *buf, size_t size) {
  ssize_t buf_len = (ssize_t)size;
  size_t attempt = size;
  ssize_t result;

  while(size>0) {
    result = write(filedes,buf,attempt);
    if(result<0) {
      if(errno==EINTR) continue;
      else if(errno==EMSGSIZE) {
        attempt = attempt/2;
        result = 0;
      }
      else {
        return result;
      }
    }
    buf+=result;
    size-=result;
    if(attempt>size) attempt=size;
  }

  return buf_len;
}

void set_gamma(double gamma_red, double gamma_green, double gamma_blue) {
  int i;
  
  for(i=0;i<256;i++) {
    gamma_table_red[i] = (0x80 | (int)(pow(((float)i / 255.0), gamma_red) * 127.0 + 0.5));//(uint8_t)(pow(i/255.0,gamma_red)*255.0+0.5);
    gamma_table_green[i] = (0x80 | (int)(pow(((float)i / 255.0), gamma_green) * 127.0 + 0.5));//(uint8_t)(pow(i/255.0,gamma_green)*255.0+0.5);
    gamma_table_blue[i] = (0x80 | (int)(pow(((float)i / 255.0), gamma_blue) * 127.0 + 0.5));//(uint8_t)(pow(i/255.0,gamma_blue)*255.0+0.5);
  }
}

void write_gamma_color(lpd8806_color *p, uint8_t red, uint8_t green, uint8_t blue) {
  //uint8_t flag;
  uint8_t gamma_corrected_red = gamma_table_red[red];
  uint8_t gamma_corrected_green = gamma_table_green[green];
  uint8_t gamma_corrected_blue = gamma_table_blue[blue];

  // flag = make_flag(gamma_corrected_red,gamma_corrected_green,gamma_corrected_blue);
  write_frame(p,gamma_corrected_red,gamma_corrected_green,gamma_corrected_blue);
}
