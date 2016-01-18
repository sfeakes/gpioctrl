#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// We want a success/failure return value from 'wiringPiSetup()'
#define WIRINGPI_CODES      1
#include <wiringPi.h>

#include "utils.h"
#include "config.h"


                                        
/* ------------------ Start Here ----------------------- */
int main (int argc, char *argv[])
{  
  int i;
  char *cfg = CFGFILE;
  char *name = NULL;
  int direction = -1;
  int state = -1;  
  
  for (i = 1; i < argc; i++)
  {
    if (strcmp (argv[i], "-h") == 0)
    {
      printf ("%s (options)\n -v verbose\n -w wait for [sunup|sundown]\n -c [cfg file name]\n -h this\n", argv[0]);
      exit (EXIT_SUCCESS);
    }
    else if (strcmp (argv[i], "-v") == 0)
      _debuglog_ = true;
    else if (strcmp (argv[i], "-c") == 0)
      cfg = argv[++i];
    else if (strcasecmp (argv[i], "read") == 0)
      direction = INPUT;
    else if (strcasecmp (argv[i], "write") == 0 || strcasecmp (argv[i], "send") == 0)
      direction = OUTPUT;
    else if (strcasecmp (argv[i], "off") == 0 || strcasecmp (argv[i], "high") == 0)
      state = HIGH;
    else if (strcasecmp (argv[i], "on") == 0 || strcasecmp (argv[i], "low") == 0)
      state = LOW;
    else
    {
      if (name != NULL) {
        logMessage (LOG_ERR, "Multiple names passed on commandline '%s' & '%s'\n",name, argv[i]);
        exit (EXIT_FAILURE);
      }
      name = argv[i];
    }
  }

  if (name == NULL) {
    logMessage (LOG_ERR, "no name found on command line\n",name);
    exit (EXIT_FAILURE);
  }
  logMessage (LOG_DEBUG, "Name = '%s' direction %d, state %d\n",name,direction, state);
  
  readCfg(cfg);
  
  for (i=0; _gpioconfig_.gpiocfg[i].pin > -1 ; i++)
  {
    if(strcasecmp(name, _gpioconfig_.gpiocfg[i].name) == 0)
      break;
  }
  
  if (_gpioconfig_.gpiocfg[i].pin == -1)
  {
    logMessage (LOG_ERR, "Couldn't find GPIO pin for name '%s'\n",name);
    exit (EXIT_FAILURE);
  }
  
  /* Initialize 'wiringPi' library */
  if (wiringPiSetup () == -1)
  {
    displayLastSystemError ("'wiringPi' library couldn't be initialized, exiting");
    exit (EXIT_FAILURE);
  }
  
  if ( direction == OUTPUT) {
    digitalWrite(_gpioconfig_.gpiocfg[i].pin, state);
  } else if ( direction == INPUT) {
    digitalRead(_gpioconfig_.gpiocfg[i].pin);
  }
  
  exit (EXIT_SUCCESS);
}
