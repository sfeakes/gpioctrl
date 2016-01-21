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

void *pattern_rainbow(void *threadlpddevice);
void *pattern_christmas(void *threadlpddevice);
void *pattern_fillandempty(void *threadlpddevice);
void *pattern_runners(void *threadlpddevice);
void *pattern_random(void *threadlpddevice);
void *pattern_randomrunners(void *threadlpddevice);

typedef void *(*PatternFunction)(void *prms);
PatternFunction patternFunctions[] = {&pattern_rainbow, 
                                      &pattern_christmas,
                                      &pattern_fillandempty,
                                      &pattern_runners,
                                      &pattern_randomrunners,
                                      &pattern_random
                                      };

void HSVtoRGB(double h, double s, double v, double *r, double *g, double *b);

#define d0 0
#define d1 255
/* Remember order is BRG below */
//                      {Blue,Red,Green)
/*
lpd8806_color redLED      = {d0,d1,d0};
lpd8806_color greenLED    = {d0,d0,d1};
lpd8806_color blueLED     = {d1,d0,d0};
lpd8806_color whiteLED    = {d1,d1,d1};
lpd8806_color cyanLED     = {d1,d0,d1};
lpd8806_color yellowLED   = {d0,d1,d1};
lpd8806_color magentaLED  = {d1,d1,d0};
*/

lpd8806_color primaryLEDs[7]={
/* Remember order is BRG below */
//             {Blue,Red,Green)
/* redLED      */ {d0,d1,d0},
/* greenLED    */ {d0,d0,d1},
/* blueLED     */ {d1,d0,d0},
/* cyanLED     */ {d1,d0,d1},
/* yellowLED   */ {d0,d1,d1},
/* magentaLED  */ {d1,d1,d0},
/* whiteLED    */ {d1,d1,d1}
};

void print_colors()
{
  int i;
  for(i=0;i<7;i++) {
    switch(i) {
      case 0:;printf("Red");break;
      case 1:;printf("Green");break;
      case 2:;printf("Blue");break;
      case 3:;printf("Cyan");break;
      case 4:;printf("Yellow");break;
      case 5:;printf("Magenta");break;
      case 6:;printf("White");break;
    }
    printf(" r:%d, g:%d, b:%d\n",primaryLEDs[i].red,primaryLEDs[i].green,primaryLEDs[i].blue);
  }
}

void lpd880led_cleanup()
{
  int i;
  
  for (i=0; i < _gpioconfig_.lpd8806devices; i++)
  {
    if (_gpioconfig_.lpd8806cfg[i].thread_id != 0) {
      logMessage (LOG_DEBUG, "Killing LED pattern thread\n");
      pthread_kill(_gpioconfig_.lpd8806cfg[i].thread_id, SIGTERM); // request thread terminate
      _gpioconfig_.lpd8806cfg[i].thread_id = 0;
    }

    if (_gpioconfig_.lpd8806cfg[i].buf.buffer != NULL) {
      logMessage (LOG_DEBUG, "free LED buffer\n");
      lpd8806_free(&_gpioconfig_.lpd8806cfg[i].buf);
    }
    
    if (_gpioconfig_.lpd8806cfg[i].fd != 0)
      close(_gpioconfig_.lpd8806cfg[i].fd);
      
    pthread_mutex_destroy(&_gpioconfig_.lpd8806cfg[i].t_mutex);
    
    _gpioconfig_.lpd8806cfg[i].red = 0;
    _gpioconfig_.lpd8806cfg[i].green = 0;
    _gpioconfig_.lpd8806cfg[i].blue = 0;
    _gpioconfig_.lpd8806cfg[i].pattern = 0;
  }
}

boolean lpd880led_init(struct LPD8806cfg *lpddevice)
{
  pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
  char devPath[128];
 
 //print_colors();
 
  sprintf(devPath, "/dev/%s", lpddevice->device);
  
 //logMessage (LOG_DEBUG, "opening device %s\n", devPath);
 
  /* Open the device file using Low-Level IO */
  lpddevice->fd = open(devPath,O_WRONLY);
  if(lpddevice->fd < 0) {
    displayLastSystemError("Opening device SPIdevice");
    return(FALSE);
  }

  /* Initialize the SPI bus for Total Control Lighting */
  if(spi_init(lpddevice->fd)<0) {
    displayLastSystemError("SPI initialization error");
    return(FALSE);
  }

  /* Initialize pixel buffer */
  if(lpd8806_init(&lpddevice->buf,lpddevice->buf.leds)<0) {
    logMessage (LOG_ERR, "Insufficient memory for pixel buffer.\n");
    return(FALSE);
  }
 
  /* Set the gamma correction factors for each color */
  set_gamma(2.2,2.2,2.2);
  
  /* set global available mutex */  
  lpddevice->t_mutex = mut;

  return(TRUE);
}

boolean lpd8806worker(struct LPD8806cfg *lpddevice, uint8_t *pattern, uint8_t *option, uint8_t *r, uint8_t *g, uint8_t *b) 
{
logMessage (LOG_DEBUG, "lpd8806worker %s %s pattern:%d r:%d g:%d b:%d\n",lpddevice->name,lpddevice->device,*pattern,*r,*g,*b);

  if ( lpddevice->buf.buffer == NULL) {
    if ( lpd880led_init(lpddevice) != TRUE) {
      logMessage (LOG_ERR, "initilizing LEDs\n");
      return FALSE;
    }
  }
  
  // Stop and pattern thread that's running
  if (lpddevice->thread_id != 0)
  {
    pthread_cancel(lpddevice->thread_id); // request thread terminate
    pthread_join(lpddevice->thread_id , NULL); // wait for thread to terminate
    lpddevice->thread_id = 0;
    logMessage (LOG_DEBUG, "Canceled LED Pattern thread.\n");
    lpddevice->red = lpddevice->green = lpddevice->blue = lpddevice->pattern = 0;
  }
  
  if (*pattern > 0)
  {
    pthread_attr_t attr;
    /* Initialize and set thread detached attribute */
    pthread_attr_init(&attr);
    //pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    
    if (*pattern < 1 || *pattern > (sizeof (patternFunctions) / sizeof (PatternFunction)) )
      *pattern=1;
      
    if( pthread_create( &lpddevice->thread_id , NULL ,  patternFunctions[*pattern-1] , (void*) lpddevice) < 0)
    {
      logMessage (LOG_ERR, "could not create thread\n");
      return FALSE;
    }
    
    *r = *g = *b = 0;
  } else {
    int i;  
    logMessage (LOG_DEBUG, "sending LED's color r:%d,g:%d,b:%d\n",*r,*g,*b);

    if ( pthread_mutex_lock(&lpddevice->t_mutex) != 0 )
      logMessage (LOG_WARNING, "Could not lock LED thread mutex %d\n",lpddevice->t_mutex);
//logMessage (LOG_DEBUG, "Mutex locked\n");
    
    for(i=0; i < lpddevice->buf.leds; i++) {
      write_gamma_color(&lpddevice->buf.pixels[i],*r,*g,*b); 
    }
//logMessage (LOG_DEBUG, "writing buffer\n");
    send_buffer((int)lpddevice->fd,&lpddevice->buf);
    
    if ( pthread_mutex_unlock(&lpddevice->t_mutex) != 0 )
      logMessage (LOG_WARNING, "Could not unlock LED thread mutex\n");
      
//logMessage (LOG_DEBUG, "Mutex unlocked\n");
  }
  
  lpddevice->red = *r;
  lpddevice->green = *g;
  lpddevice->blue = *b;
  lpddevice->pattern = *pattern;
  
  return TRUE;   
}

void *pattern_random(void *threadlpddevice) 
{
  int i;
  struct LPD8806cfg *lpddevice = (struct LPD8806cfg *) threadlpddevice;
  //lpd8806_color ledcolor;
  
  logMessage (LOG_DEBUG, "LED Pattern Random starting.\n");
  
  /* Blank the pixels */
  if ( pthread_mutex_lock(&lpddevice->t_mutex) != 0 )
  logMessage (LOG_WARNING, "Could not lock LED thread mutex %d\n",lpddevice->t_mutex);
  
  for(i=0;i < lpddevice->buf.leds;i++) {
    write_gamma_color(&lpddevice->buf.pixels[i],0x00,0x00,0x00);
  }
  send_buffer((int)lpddevice->fd,&lpddevice->buf);
  
  if ( pthread_mutex_unlock(&lpddevice->t_mutex) != 0 )
  logMessage (LOG_WARNING, "Could not unlock LED thread mutex\n");


  while(1) {
    //ledcolor = whiteLED;
    //ledcolor = primaryLEDs[rand() % 7];
    
    for(i=0;i < lpddevice->buf.leds;i++) {
      /*
      ledcolor = primaryLEDs[rand() % 7];
      write_gamma_color(&lpddevice->buf.pixels[i],ledcolor.red,ledcolor.green,ledcolor.blue);
      */
      write_gamma_color(&lpddevice->buf.pixels[i],rand()%d1,rand()%d1,rand()%d1);
    }
    send_buffer((int)lpddevice->fd,&lpddevice->buf);
  
    usleep((rand() % 100) * 10000);
    //sleep(1);
  }
  
  return 0;
}

void *pattern_randomrunners(void *threadlpddevice) 
{
  int i, j;
  int ledtail = 4;
  struct LPD8806cfg *lpddevice = (struct LPD8806cfg *) threadlpddevice;
  lpd8806_color ledcolor;
  
  logMessage (LOG_DEBUG, "LED Pattern random runners starting.\n");
  
  /* Blank the pixels */
  if ( pthread_mutex_lock(&lpddevice->t_mutex) != 0 )
  logMessage (LOG_WARNING, "Could not lock LED thread mutex %d\n",lpddevice->t_mutex);
  
  for(i=0;i < lpddevice->buf.leds;i++) {
    write_gamma_color(&lpddevice->buf.pixels[i],0x00,0x00,0x00);
  }
  send_buffer((int)lpddevice->fd,&lpddevice->buf);
  
  if ( pthread_mutex_unlock(&lpddevice->t_mutex) != 0 )
  logMessage (LOG_WARNING, "Could not unlock LED thread mutex\n");


  while(1) {
    //ledcolor = whiteLED;
    ledcolor = primaryLEDs[rand() % 7];
    int shade;
    
    // Random direction
    if (rand() % 2 == 1) {
     for(i=lpddevice->buf.leds-1; i >= 0-ledtail-1; i--)
     {
      for (j=i; j < i+ledtail; j++)
      {
        shade=(j-i)*(d1/ledtail);
        if (j >= 0 && j < lpddevice->buf.leds)
          write_gamma_color(&lpddevice->buf.pixels[j],(ledcolor.red>d0)?ledcolor.red-shade:d0,
                                                      (ledcolor.green>d0)?ledcolor.green-shade:d0,
                                                      (ledcolor.blue>d0)?ledcolor.blue-shade:d0);   
        if (j+ledtail < lpddevice->buf.leds+1)
          write_gamma_color(&lpddevice->buf.pixels[j+ledtail+1],0x00,0x00,0x00);
      } 
      send_buffer((int)lpddevice->fd,&lpddevice->buf);
      usleep(10000);
     }
    } else {
     for(i=0; i < lpddevice->buf.leds+ledtail+1; i++)
     {
      for (j=i; j > i-ledtail; j--)
      {
        shade=(i-j)*(d1/ledtail);

        if (j >= 0 && j < lpddevice->buf.leds-1)
          write_gamma_color(&lpddevice->buf.pixels[j],(ledcolor.red>d0)?ledcolor.red-shade:d0,
                                                      (ledcolor.green>d0)?ledcolor.green-shade:d0,
                                                      (ledcolor.blue>d0)?ledcolor.blue-shade:d0); 
        if (j-ledtail > 0)
          write_gamma_color(&lpddevice->buf.pixels[j-ledtail-1],0x00,0x00,0x00);
      }
      send_buffer((int)lpddevice->fd,&lpddevice->buf);
      usleep(10000);
     }
    }
    
    usleep((rand() % 100) * 10000);
  }
  
  return 0;
}

void *pattern_runners(void *threadlpddevice) 
{
  int i, j;
  int ledtail = 4;
  struct LPD8806cfg *lpddevice = (struct LPD8806cfg *) threadlpddevice;
  lpd8806_color ledcolor;
  
  logMessage (LOG_DEBUG, "LED Pattern runners starting.\n");
  
  /* Blank the pixels */
  if ( pthread_mutex_lock(&lpddevice->t_mutex) != 0 )
  logMessage (LOG_WARNING, "Could not lock LED thread mutex %d\n",lpddevice->t_mutex);
  
  for(i=0;i < lpddevice->buf.leds;i++) {
    write_gamma_color(&lpddevice->buf.pixels[i],0x00,0x00,0x00);
  }
  send_buffer((int)lpddevice->fd,&lpddevice->buf);
  
  if ( pthread_mutex_unlock(&lpddevice->t_mutex) != 0 )
  logMessage (LOG_WARNING, "Could not unlock LED thread mutex\n");


  while(1) {
    //ledcolor = whiteLED;
    ledcolor = primaryLEDs[rand() % 7];
    
    int shade;
    for(i=0; i < lpddevice->buf.leds+ledtail+1; i++)
    {
      for (j=i; j > i-ledtail; j--)
      {
        //shade=d1-((i-j)*(d1/ledtail));
        shade=(i-j)*(d1/ledtail);

        if (j >= 0 && j < lpddevice->buf.leds-1)
          write_gamma_color(&lpddevice->buf.pixels[j],(ledcolor.red>d0)?ledcolor.red-shade:d0,
                                                      (ledcolor.green>d0)?ledcolor.green-shade:d0,
                                                      (ledcolor.blue>d0)?ledcolor.blue-shade:d0);
        
        if (j-ledtail > 0)
          write_gamma_color(&lpddevice->buf.pixels[j-ledtail-1],0x00,0x00,0x00);
      }
      
      send_buffer((int)lpddevice->fd,&lpddevice->buf);
      usleep(10000);
    }
    usleep((rand() % 100) * 10000);
    //sleep(1);
  }
  
  return 0;
}

void *pattern_fillandempty(void *threadlpddevice) 
{
  int i;
  int endat;
  int startat;
  struct LPD8806cfg *lpddevice = (struct LPD8806cfg *) threadlpddevice;
  
  logMessage (LOG_DEBUG, "LED Pattern fill and empty starting.\n");
  
  /* Blank the pixels */
  if ( pthread_mutex_lock(&lpddevice->t_mutex) != 0 )
      logMessage (LOG_WARNING, "Could not lock LED thread mutex %d\n",lpddevice->t_mutex);
      
  for(i=0;i < lpddevice->buf.leds;i++) {
    write_gamma_color(&lpddevice->buf.pixels[i],0x00,0x00,0x00);
  }
  send_buffer((int)lpddevice->fd,&lpddevice->buf);
  
  if ( pthread_mutex_unlock(&lpddevice->t_mutex) != 0 )
      logMessage (LOG_WARNING, "Could not unlock LED thread mutex\n");


  while(1) {
    // Outer loop to move each LED to the fill point
    for(endat=lpddevice->buf.leds;endat > 0;endat--) {
      // move an LED from 0 to the endat point
      for(i=0; i < endat; i++)
      {
         write_gamma_color(&lpddevice->buf.pixels[i],255,255,255);
         if (i > 0)
           write_gamma_color(&lpddevice->buf.pixels[i-1],0x00,0x00,0x00);

        send_buffer((int)lpddevice->fd,&lpddevice->buf);
        usleep(10000);
      }
    }
    sleep(3);
    for(startat=(lpddevice->buf.leds-1);startat >= 0;startat--) {
      for(i=startat; i < lpddevice->buf.leds; i++)
      {
        write_gamma_color(&lpddevice->buf.pixels[i],0x00,0x00,0x00);
        if (i < (lpddevice->buf.leds-1))
          write_gamma_color(&lpddevice->buf.pixels[i+1],255,255,255);

        send_buffer((int)lpddevice->fd,&lpddevice->buf);
        usleep(10000);
      }
    }
    sleep(3);
  }
  
  return 0;
}

void *pattern_christmas(void *threadlpddevice) 
{
  int i;
  struct LPD8806cfg *lpddevice = (struct LPD8806cfg *) threadlpddevice;
  
  logMessage (LOG_DEBUG, "LED Pattern christmas starting.\n");
  
  /* Blank the pixels */
  if ( pthread_mutex_lock(&lpddevice->t_mutex) != 0 )
      logMessage (LOG_WARNING, "Could not lock LED thread mutex %d\n",lpddevice->t_mutex);
      
  for(i=0;i < lpddevice->buf.leds;i++) {
    write_gamma_color(&lpddevice->buf.pixels[i],0x00,0x00,0x00);
  }
  send_buffer((int)lpddevice->fd,&lpddevice->buf);
  
  if ( pthread_mutex_unlock(&lpddevice->t_mutex) != 0 )
      logMessage (LOG_WARNING, "Could not unlock LED thread mutex\n");

  int sr = 1; 
  while(1) {
    if ( pthread_mutex_lock(&lpddevice->t_mutex) != 0 )
      logMessage (LOG_WARNING, "Could not lock LED thread mutex %d\n",lpddevice->t_mutex);
      
    for(i=0;i < lpddevice->buf.leds;i++) {
      //logMessage (LOG_DEBUG, "i=%d sr=%d bitwise=%d\n",i,sr);
      if((i & 1) == sr)
        write_gamma_color(&lpddevice->buf.pixels[i],255,0x00,0x00);
      else
        write_gamma_color(&lpddevice->buf.pixels[i],0x00,255,0x00);     
    }
    send_buffer((int)lpddevice->fd,&lpddevice->buf);
    
    if ( pthread_mutex_unlock(&lpddevice->t_mutex) != 0 )
      logMessage (LOG_WARNING, "Could not unlock LED thread mutex\n");
    
    sr = !sr;
    //usleep(10000);
    sleep(1);
  }
  
  return 0;
}

void *pattern_rainbow(void *threadlpddevice) 
{
  lpd8806_color *p;
  //lpd8806_buffer *buf;
  int i;
  double h, r, g, b;
  struct LPD8806cfg *lpddevice = (struct LPD8806cfg *) threadlpddevice;
  
  logMessage (LOG_DEBUG, "LED Pattern rainbow starting.\n");
  
  /* Blank the pixels */
  if ( pthread_mutex_lock(&lpddevice->t_mutex) != 0 )
      logMessage (LOG_WARNING, "Could not lock LED thread mutex %d\n",lpddevice->t_mutex);
      
  for(i=0;i < lpddevice->buf.leds;i++) {
    write_gamma_color(&lpddevice->buf.pixels[i],0x00,0x00,0x00);
  }
  send_buffer((int)lpddevice->fd,&lpddevice->buf);
  
  if ( pthread_mutex_unlock(&lpddevice->t_mutex) != 0 )
      logMessage (LOG_WARNING, "Could not unlock LED thread mutex\n");

  h = 0.0;
  while(1) {
    h+=2.0;
    if(h>=360.0) h=0.0;
    
    if ( pthread_mutex_lock(&lpddevice->t_mutex) != 0 )
      logMessage (LOG_WARNING, "Could not lock LED thread mutex %d\n",lpddevice->t_mutex);

    memmove(lpddevice->buf.pixels+1,lpddevice->buf.pixels,sizeof(lpd8806_color)*(lpddevice->buf.leds-1));
    HSVtoRGB(h,1.0,1.0,&r,&g,&b);
    p = &lpddevice->buf.pixels[0];
    write_gamma_color(p,(int)(r*255.0),(int)(g*255.0),(int)floor(b*255.0));
    send_buffer((int)lpddevice->fd,&lpddevice->buf);
    
    if ( pthread_mutex_unlock(&lpddevice->t_mutex) != 0 )
      logMessage (LOG_WARNING, "Could not unlock LED thread mutex\n");
      
    usleep(10000);
  }
  
  // Shouldn't ever get here.
  logMessage (LOG_DEBUG, "LED Pattern rainbow clearing LEDs.\n");
  for(i=0;i < lpddevice->buf.leds;i++) {
    write_gamma_color(&lpddevice->buf.pixels[i],0x00,0x00,0x00);
  }
  send_buffer((int)lpddevice->fd,&lpddevice->buf);
  pthread_exit(0);
  logMessage (LOG_DEBUG, "LED Pattern rainbow terminated.\n");
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
