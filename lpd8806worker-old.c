/*
* blink_test.c
*
* Copyright 2012 Christopher De Vries
* This program is distributed under the Artistic License 2.0, a copy of which
* is included in the file LICENSE.txt
*/
#include "lpd8806led.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>
#include <pthread.h>

#include "utils.h"
#include "config.h"

static const int leds = 50;
static int continue_looping;

void stop_program(int sig);
void HSVtoRGB(double h, double s, double v, double *r, double *g, double *b);
void *pattern_rainbow(void *threadlpddevice);


boolean lpd880led_init(struct LPD8806cfg *lpddevice, lpd8806_buffer *buf, int *fd)
{
  char devPath[128];
  
  sprintf(devPath, "/dev/%s", lpddevice->device);
  /* Open the device file using Low-Level IO */
  *fd = open(devPath,O_WRONLY);
  if(*fd < 0) {
    //fprintf(stderr,"Error %d: %s\n",errno,strerror(errno));
    displayLastSystemError("Opening device SPIdevice");
    return(FALSE);
  }

  /* Initialize the SPI bus for Total Control Lighting */
  
  if(spi_init(*fd)<0) {
    //fprintf(stderr,"SPI initialization error %d: %s\n",errno, strerror(errno));
    displayLastSystemError("SPI initialization error");
    return(FALSE);
  }

  /* Initialize pixel buffer */
  if(lpd8806_init(buf,lpddevice->leds)<0) {
    logMessage (LOG_ERR, "Insufficient memory for pixel buffer.\n");
    return(FALSE);
  }

  /* Set the gamma correction factors for each color */
  set_gamma(2.2,2.2,2.2);
  
  return(TRUE);
}

boolean lpd8806worker(struct LPD8806cfg *lpddevice, int pattern, int r, int g, int b) 
{
  if (lpddevice->thread_id != 0)
  {
    pthread_kill(lpddevice->thread_id, SIGTERM); // request thread terminate
    pthread_join(lpddevice->thread_id , NULL); // wait for thread to terminate
    lpddevice->thread_id = 0;
    logMessage (LOG_DEBUG, "Killed LED Pattern thread.\n");
  }
  
  if (pattern > 0)
  {
    pthread_attr_t attr;
    /* Initialize and set thread detached attribute */
    pthread_attr_init(&attr);
    //pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    if( pthread_create( &lpddevice->thread_id , NULL ,  pattern_rainbow , (void*) lpddevice) < 0)
    {
      logMessage (LOG_ERR, "could not create thread\n");
      return FALSE;
    }
  } else {
    lpd8806_buffer buf;
    int fd;
    int i;
    
    logMessage (LOG_DEBUG, "Initilizing LED's and sending color\n");
    
    //if ( lpd880led_init(lpddevice, &buf, &fd) )
    lpd880led_init(lpddevice, &buf, &fd);
    
    for(i=0; i < lpddevice->leds; i++) {
      write_gamma_color(&buf.pixels[i],r,g,b); 
    }
    send_buffer(fd,&buf);

    /* Free the pixel buffer */
    lpd8806_free(&buf);

    /* Close the SPI device */
    close(fd);
  }
  
  return TRUE;
}

void *pattern_rainbow(void *threadlpddevice) 
{
  lpd8806_buffer buf;
  struct LPD8806cfg *lpddevice;
  int fd;
  lpd8806_color *p;
  int i;
  double h, r, g, b;
  
  logMessage (LOG_DEBUG, "LED Pattern rainbow starting.\n");
  
  lpddevice = (struct LPD8806cfg *) threadlpddevice;
  
  lpd880led_init(lpddevice, &buf, &fd);
  
  /* Blank the pixels */
  for(i=0;i < lpddevice->leds;i++) {
    write_gamma_color(&buf.pixels[i],0x00,0x00,0x00);
  }

  send_buffer(fd,&buf);

  
  continue_looping = 1;
  /* Prepare to receive ctrl-c to stop looping */
  //signal(SIGINT,stop_program);
  /* Prepare to receive terminate to stop looping */
  signal(SIGTERM,stop_program);
  h = 0.0;

  /* Loop while continue_looping is true */
  while(continue_looping) {
    h+=2.0;
    if(h>=360.0) h=0.0;
    memmove(buf.pixels+1,buf.pixels,sizeof(lpd8806_color)*(lpddevice->leds-2));
    HSVtoRGB(h,1.0,1.0,&r,&g,&b);
    p = &buf.pixels[0];
    write_gamma_color(p,(int)(r*255.0),(int)(g*255.0),(int)floor(b*255.0));
    send_buffer(fd,&buf);
    usleep(10000);
  }
  logMessage (LOG_DEBUG, "LED Pattern rainbow clearing LEDs.\n");
  for(i=0;i<lpddevice->leds;i++) {
    p = &buf.pixels[i];
    write_gamma_color(p,0x00,0x00,0x00);  
  }
  send_buffer(fd,&buf);
  lpd8806_free(&buf);
  close(fd);
  pthread_exit(0);
  logMessage (LOG_DEBUG, "LED Pattern rainbow terminated.\n");
}

void stop_program(int sig) {
  logMessage (LOG_DEBUG, "LED Pattern rainbow has been requested to terminate.\n");
  /* Ignore the signal */
  signal(sig,SIG_IGN);

  /* stop the looping */
  continue_looping = 0;

  /* Put the signal to default action in case something goes wrong */
  signal(sig,SIG_DFL);
}

/* Convert hsv values (0<=h<360, 0<=s<=1, 0<=v<=1) to rgb values (0<=r<=1, etc) */
void HSVtoRGB(double h, double s, double v, double *r, double *g, double *b) {
  int i;
  double f, p, q, t;

  if( s < 1.0e-6 ) {
    /* grey */
    *r = *g = *b = v;
    return;
  }

  h /= 60.0;			        /* Divide into 6 regions (0-5) */
  i = (int)floor( h );
  f = h - (double)i;			/* fractional part of h */
  p = v * ( 1.0 - s );
  q = v * ( 1.0 - s * f );
  t = v * ( 1.0 - s * ( 1.0 - f ) );

  switch( i ) {
  case 0:
    *r = v;
    *g = t;
    *b = p;
    break;
  case 1:
    *r = q;
    *g = v;
    *b = p;
    break;
  case 2:
    *r = p;
    *g = v;
    *b = t;
    break;
  case 3:
    *r = p;
    *g = q;
    *b = v;
    break;
  case 4:
    *r = t;
    *g = p;
    *b = v;
    break;
  default:		// case 5:
    *r = v;
    *g = p;
    *b = q;
    break;
  }
}