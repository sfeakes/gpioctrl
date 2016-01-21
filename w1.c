#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "utils.h"
#include "config.h"

int readw1_raw(struct ONEWcfg *w1device, boolean force) 
{
  char devPath[128]; // Path to device
  char buf[256];     // Data from device
  char tmpData[8];   // Temp C * 1000 reported by device 
  char path[] = "/sys/bus/w1/devices";
  ssize_t numRead;
  time_t rawtime;
  
  time (&rawtime);
  
  if ( force == true || (rawtime - w1device->last_event_time) > w1device->pollsec || w1device->last_value == -1)
  {
    sprintf(devPath, "%s/%s/w1_slave", path, w1device->device);
    w1device->last_event_time = rawtime;
    int fd = open(devPath, O_RDONLY);
    if(fd == -1)
    {
      logMessage (LOG_ERR,"Couldn't open the w1 device %s\n", devPath);
      return FALSE;   
    }
    while((numRead = read(fd, buf, 256)) > 0) 
    {
      strncpy(tmpData, strstr(buf, "t=") + 2, 5); 
      w1device->last_value = strtof(tmpData, NULL);
      logMessage (LOG_DEBUG,"Read %f from %s\n",w1device->last_value,w1device->device);
    }
    close(fd);
  } else {
    logMessage (LOG_DEBUG,"Reading %s FROM CACHE\n",w1device->device);
  } 
    
  return TRUE;
}

int readw1_for_mh(struct ONEWcfg *w1device, int *rtnbuff) 
{
  if (!readw1_raw(w1device, true))
    return FALSE;
  
  *rtnbuff = (int)(w1device->last_value / 100)+0.5f;
  
  return true;
}

int readw1(struct ONEWcfg *w1device, char *rtnbuff) 
{

  if (!readw1_raw(w1device, false))
    return FALSE;
  
  sprintf(rtnbuff, "\"%s\" : { \"name\" : \"%s\", \"pin\" : \"1w1\", \"value\" :  %.2f}\r\n",
                   w1device->device,w1device->name,(w1device->last_value / 1000) * 9 / 5 + 32 );
                   
  return TRUE;
  
}


int readw1_OLD(struct ONEWcfg *w1device, char *rtnbuff) {
  char devPath[128]; // Path to device
  char buf[256];     // Data from device
  char tmpData[6];   // Temp C * 1000 reported by device 
  char path[] = "/sys/bus/w1/devices"; 
  ssize_t numRead;

  time_t rawtime;
  time (&rawtime);
 
 //float tempC;
 
 if ((rawtime - w1device->last_event_time) > 5 || w1device->last_value == -1)
 {
    sprintf(devPath, "%s/%s/w1_slave", path, w1device->device);
    w1device->last_event_time = rawtime;

    int fd = open(devPath, O_RDONLY);
    if(fd == -1)
    {
      w1device->last_value = -1;
      logMessage (LOG_ERR,"Couldn't open the w1 device %s\n", devPath);
      return FALSE;   
    }
    while((numRead = read(fd, buf, 256)) > 0) 
    {
      strncpy(tmpData, strstr(buf, "t=") + 2, 5); 
      w1device->last_value = strtof(tmpData, NULL);
      logMessage (LOG_DEBUG,"Read %f from %s\n",w1device->last_value,w1device->device);
    }
    close(fd);
  } else {
    logMessage (LOG_DEBUG,"Reading %s FROM CACHE\n",w1device->device);
  }
  
  sprintf(rtnbuff, "\"%s\" : { \"name\" : \"%s\", \"pin\" : \"1w1\", \"value\" :  %.2f}\r\n",
                   w1device->device,w1device->name,(w1device->last_value / 1000) * 9 / 5 + 32 );

  return TRUE; 
}
/*
int readw1_old(char *device, char *rtnbuff) {
 DIR * dir;
 struct dirent *dirent;
 char dev[16];      // Dev ID
 char devPath[128]; // Path to device
 char buf[256];     // Data from device
 char tmpData[6];   // Temp C * 1000 reported by device 
 char path[] = "/sys/bus/w1/devices"; 
 ssize_t numRead;
 
 if (device != NULL ) {
   sprintf(devPath, "%s/%s/w1_slave", path, device);
   sprintf(dev, device);
 } else {
   dir = opendir (path);
   if (dir != NULL)
   {
    while ((dirent = readdir (dir)))
     // 1-wire devices are links beginning with 28-
     if (dirent->d_type == DT_LNK && strstr(dirent->d_name, "28-") != NULL) { 
       strcpy(dev, dirent->d_name);
       logMessage (LOG_DEBUG,"Found 1 wire device: %s\n", dev);
     }
     (void) closedir (dir);
   }
   else
   {
    //perror ("Couldn't open the w1 devices directory");
    logMessage (LOG_ERR,"Couldn't open the w1 devices directory\n");
    return FALSE;
   }
           // Assemble path to OneWire device
   sprintf(devPath, "%s/%s/w1_slave", path, dev);
 }

 time_t rawtime;
 time (&rawtime);
 
 //float tempC;
 
 if ((rawtime - _gpioconfig_.onewcfg.last_event_time) > 5 )
 {
   _gpioconfig_.onewcfg.last_event_time = rawtime;

   int fd = open(devPath, O_RDONLY);
   if(fd == -1)
   {
     logMessage (LOG_ERR,"Couldn't open the w1 device %s\n", devPath);
     return FALSE;   
   }
   while((numRead = read(fd, buf, 256)) > 0) 
   {
     strncpy(tmpData, strstr(buf, "t=") + 2, 5); 
    //float tempC = strtof(tmpData, NULL);
     _gpioconfig_.onewcfg.last_value = strtof(tmpData, NULL);
     logMessage (LOG_DEBUG,"Read %f from %s\n",_gpioconfig_.onewcfg.last_value,dev);
    //sprintf(rtnbuff, "\"%s\" : { \"name\" : \"%s\", \"pin\" : \"1w1\", \"value\" :  %.2f}\r\n",dev,dev,(tempC / 1000) * 9 / 5 + 32 );
    //logMessage (LOG_DEBUG,"Read %f from %s\n",tempC,dev);
   }
   close(fd);
 } else {
   logMessage (LOG_DEBUG,"Reading %s FROM CACHE\n",dev);
 }
  
 //float tempC = _gpioconfig_.onewcfg.last_value;
 sprintf(rtnbuff, "\"%s\" : { \"name\" : \"%s\", \"pin\" : \"1w1\", \"value\" :  %.2f}\r\n",dev,dev,(_gpioconfig_.onewcfg.last_value / 1000) * 9 / 5 + 32 );
 //logMessage (LOG_DEBUG,"Read %f from %s\n",tempC,dev);
  
 //} 
 
 return TRUE;
}
*/
