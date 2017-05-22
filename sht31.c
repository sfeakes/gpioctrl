#include <linux/i2c-dev.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <elf.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include "utils.h"
#include "config.h"

#define SHT31_INTERFACE_ADDR       1
#define SHT31_DEFAULT_ADDR         0x44
#define SHT31_READ_SERIALNO        0x3780
#define SHT31_MEAS_HIGHREP_STRETCH 0x2C06 // Doesn't work on PI
#define SHT31_MEAS_MEDREP_STRETCH  0x2C0D // Seems to work on PI but shouldn't
#define SHT31_MEAS_LOWREP_STRETCH  0x2C10 // Seems to work on PI but shouldn't
#define SHT31_MEAS_HIGHREP         0x2400 // Doesn't work on PI
#define SHT31_MEAS_MEDREP          0x240B
#define SHT31_MEAS_LOWREP          0x2416
#define SHT31_READSTATUS           0xF32D
#define SHT31_CLEARSTATUS          0x3041
#define SHT31_SOFTRESET            0x30A2
#define SHT31_HEATER_ENABLE        0x306D
#define SHT31_HEATER_DISABLE       0x3066

void delay (unsigned int howLong)
{
  struct timespec sleeper, dummy ;

  sleeper.tv_sec  = (time_t)(howLong / 1000) ;
  sleeper.tv_nsec = (long)(howLong % 1000) * 1000000 ;

  nanosleep (&sleeper, &dummy) ;
}

uint8_t crc8(const uint8_t *data, int len)
{
  const uint8_t POLYNOMIAL = 0x31;
  uint8_t crc = 0xFF;
  int j;
  int i;
  
  for (j = len; j; --j ) {
    crc ^= *data++;

    for ( i = 8; i; --i ) {
      crc = ( crc & 0x80 )
            ? (crc << 1) ^ POLYNOMIAL
            : (crc << 1);
    }
  }
  return crc;
}

boolean writeandread(int fd, uint16_t sndword, uint8_t *buffer, int readsize)
{
  int rtn;
  uint8_t snd[3];
  
  //snd[0]= SHT31_DEFAULT_ADDR << 1;
  snd[0] = 0x00;
  if (write(fd, snd, 1) != 1)
    printf("ERROR sending header\n");
  
  // Split the 16bit word into two 8 bits that are flipped.
  snd[0]=(sndword >> 8) & 0xff;
  snd[1]=sndword & 0xff;

  rtn = write(fd, snd, 2);
  if ( rtn != 2 ) {
    logMessage (LOG_ERR,"Write to sht31 failed\n");
    return false;
  } 
  
  if (readsize > 0) {
    delay(10);
    rtn = read(fd, buffer, readsize);
    if ( rtn < readsize) {
      logMessage (LOG_ERR,"Read from sht31 failed\n");
      return false;
    }
  }
  
  return true;
}

int readsht31_raw(struct SHT31cfg *sht31device, boolean force) 
{
  int file;
  char filename[20];
  time_t rawtime;
  
  time (&rawtime);
  
  if ( force == true || (rawtime - sht31device->last_event_time) > sht31device->pollsec || sht31device->last_value_t == -1 || sht31device->last_value_h == -1)
  {
    snprintf(filename, 19, "/dev/%s", sht31device->device);
    file = open(filename, O_RDWR);
    if (file < 0) {
      logMessage (LOG_ERR,"Couldn't open the i2c device %s\n", filename);
      return FALSE;
    }

    if (ioctl(file, I2C_SLAVE, SHT31_DEFAULT_ADDR) < 0) {
      logMessage (LOG_ERR,"Couldn't connect to sht31 I2C address 0x%02hhx\n", SHT31_DEFAULT_ADDR);
      return FALSE;
    }
  
    uint8_t buf[10];
    int rtn;
  
    rtn = writeandread(file, SHT31_MEAS_MEDREP, buf, 6);
  
    if (rtn != true || buf[2] != crc8(buf, 2) || buf[5] != crc8(buf+3, 2) )
    { // Try one more time
      rtn = writeandread(file, SHT31_MEAS_MEDREP, buf, 6);
      if (rtn != true || buf[2] != crc8(buf, 2) || buf[5] != crc8(buf+3, 2) ) {
        logMessage (LOG_ERR,"Bad read, write or CRC check from sht31\n");
        return false;
      }
    }
  
    close(file);
    
    uint16_t ST, SRH;
    ST = buf[0];
    ST <<= 8;
    ST |= buf[1];
    
    SRH = buf[3];
    SRH <<= 8;
    SRH |= buf[4];
    
    sht31device->last_event_time = rawtime;
    sht31device->last_value_t = ST;
    sht31device->last_value_h = SRH;
    
    logMessage (LOG_DEBUG,"Read t:%f & h:%f from %s\n",sht31device->last_value_t,sht31device->last_value_h,sht31device->device);
  } else {
    logMessage (LOG_DEBUG,"Reading %s FROM CACHE\n",sht31device->device);
  } 
  
  return true;
}

//int readsht31_for_mh(struct SHT31cfg *sht31device, int *rtnbuff)
int readsht31_for_mh(struct SHT31cfg *sht31device, int *trtnbuff, int *hrtnbuff)
{
  if (!readsht31_raw(sht31device, true))
    return FALSE;
  
  *trtnbuff = (int)((-45.0 + (175.0 * ((float) sht31device->last_value_t / (float) 0xFFFF)))*10)+0.5f;
  *hrtnbuff = (int)(( 100.0 * ((float) sht31device->last_value_h / (float) 0xFFFF)))+0.5f;
  
  return true;
}

int readsht31(struct SHT31cfg *sht31device, char *rtnbuff) 
{

  if (!readsht31_raw(sht31device, false))
    return FALSE;

  sprintf(rtnbuff, "\"sht31\" : { \"name\" : \"%s\", \"pin\" : \"i2c\", \"temperature\" :  %.2f, \"humidity\" :  %.2f}\r\n",
                     sht31device->name,
                     (-45.0 + (175.0 * ((float) sht31device->last_value_t / (float) 0xFFFF))),
                     ( 100.0 * ((float) sht31device->last_value_h / (float) 0xFFFF)) );

  return TRUE;
  
}
