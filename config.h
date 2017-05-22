#ifndef CONFIG_H_
#define CONFIG_H_

#include <pthread.h>
//#include <linux/types.h>
#include <stdint.h>
//#include <stdlib.h>
#include "lpd8806led.h"

#define PIN_CFGS 10
#define W1_CFGS 2
#define LPD8806_CFGS 2
#define SHT31_CFGS 2

#define DEFAULT_LEDS 50;  // if not passed on commant line

#define CFGFILE     "./config.cfg"

struct GPIOcfg {
  int pin;
  int input_output;
  int set_pull_updown;
  int receive_mode;  // INT_EDGE_RISING  Mode to pass to wiringPi function wiringPiISR
  int receive_state;  // LOW | HIGH  State from wiringPi function digitalRead (PIN)
  int output_pin;
  int output_state; // 0 or 1
  int last_event_state; // 0 or 1
  long last_event_time; 
  char *name;
  char *ext_cmd;
};

struct ONEWcfg {
  float last_value; 
  long last_event_time;
  int pollsec;
  char *name;
  char *mh_name;
  char *device;
};

struct SHT31cfg {
  float last_value_t;
  float last_value_h;   
  long last_event_time;
  int pollsec;
  int address;
  char *name;
  char *mh_name;
  char *device;
};

struct LPD8806cfg {

  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint8_t pattern;
  uint8_t option;

  //int leds;
  char *device;
  char *name;
  pthread_t thread_id;
  pthread_mutex_t t_mutex;
  lpd8806_buffer buf;
  int fd;
};

struct GPIOCONTROLcfg {
  int port;
  char *name;
  char *gpslocation;
  char *docroot;
  boolean webcache;
  //char *onewiredevice;
  int onewiredevices;
  int lpd8806devices;
  int sht31devices;
  int pinscfgs;
  struct ONEWcfg onewcfg[W1_CFGS];
  struct LPD8806cfg lpd8806cfg[LPD8806_CFGS];
  struct SHT31cfg sht31cfg[SHT31_CFGS];
  struct GPIOcfg gpiocfg[PIN_CFGS];
};

//struct GPIOcfg _gpioconfig_[NUM_CFGS];
extern struct GPIOCONTROLcfg _gpioconfig_;
//struct HTTPDcfg _httpdconfig_;

void readCfg (char *cfgFile);

// Few states not definen in wiringPI
#define TOGGLE 2
#define BOTH 3
#define NONE -1

#endif /* CONFIG_H_ */
