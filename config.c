#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "utils.h"
#include "config.h"


#define MAXCFGLINE 256


struct GPIOCONTROLcfg _gpioconfig_;


char *trimwhitespace(char *str)
{
  char *end;

  // Trim leading space
  while(isspace(*str)) str++;

  if(*str == 0)  // All spaces?
  return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace(*end)) end--;

  // Write new null terminator
  *(end+1) = 0;

  return str;
}

char *cleanalloc(char*str)
{
  char *result;
  str = trimwhitespace(str);
  
  result = (char*)malloc(strlen(str)+1);
  strcpy ( result, str );
  //printf("Result=%s\n",result);
  return result;
}

char *cleanallocindex(char*str, int index)
{
  char *result;
  int i;
  int found = 1;
  int loc1=0;
  int loc2=strlen(str);
  
  for(i=0;i<loc2;i++) {
    if ( str[i] == ';' ) {
      found++;
      if (found == index)
        loc1 = i;
      else if (found == (index+1))
        loc2 = i;
    }
  }
  
  if (found < index)
    return NULL;

  // Trim leading & trailing spaces
  loc1++;
  while(isspace(str[loc1])) loc1++;
  loc2--;
  while(isspace(str[loc2])) loc2--;
  
  // Allocate and copy
  result = (char*)malloc(loc2-loc1+2*sizeof(char));
  strncpy ( result, &str[loc1], loc2-loc1+1 );
  result[loc2-loc1+1] = '\0';

  return result;
}

int cleanint(char*str)
{
  if (str == NULL)
    return 0;
    
  str = trimwhitespace(str);
  return atoi(str);
}

boolean text2bool(char *str)
{
  str = trimwhitespace(str);
  if (strcasecmp (str, "YES") == 0 || strcasecmp (str, "ON") == 0)
    return TRUE;
  else
    return FALSE;
}


void readCfg (char *cfgFile)
{
  FILE * fp ;
  char bufr[MAXCFGLINE];
  //char w1bufr[MAXCFGLINE];
  const char delim[2] = ";";
  //const char delim = ';';
  char *token;
  int line = 0;
  int tokenindex = 0;
  char *b_ptr;

  // Reset whole cfg to // probably no need for this now, but need to check/debug
  for (tokenindex=0; tokenindex < PIN_CFGS; tokenindex++)
  {
    _gpioconfig_.gpiocfg[tokenindex].pin = -1;
    _gpioconfig_.gpiocfg[tokenindex].last_event_state = -1;
  }

  _gpioconfig_.webcache = TRUE;
  _gpioconfig_.onewiredevices = 0;
  _gpioconfig_.pinscfgs = 0;
  _gpioconfig_.lpd8806devices = 0;
  
  if( (fp = fopen(cfgFile, "r")) != NULL){
    while(! feof(fp)){
      if (fgets(bufr, MAXCFGLINE, fp) != NULL)
      {
        b_ptr = &bufr[0];
        char *indx;
        // Eat leading whitespace
        while(isspace(*b_ptr)) b_ptr++;
        //if (b_ptr[0] != '#' && b_ptr[0] != ' ' && b_ptr[0] != '\n' && b_ptr[0] != '\r' && line < PIN_CFGS)
        if ( b_ptr[0] != '\0' && b_ptr[0] != '#' && line < PIN_CFGS)
        {
          indx = strchr(b_ptr, '=');  
          if ( indx != NULL) 
          {
            if (strncasecmp (b_ptr, "PORT", 4) == 0) {
              _gpioconfig_.port = cleanint(indx+1);
              logMessage (LOG_DEBUG, "Config port = %d\n", _gpioconfig_.port);
            } else if (strncasecmp (b_ptr, "NAME", 4) == 0) {
              _gpioconfig_.name = cleanalloc(indx+1);
              logMessage (LOG_DEBUG, "Config name = %s\n", _gpioconfig_.name);
            } else if (strncasecmp (b_ptr, "GPSLOCATION", 11) == 0) {
              _gpioconfig_.gpslocation  = cleanalloc(indx+1);
              logMessage (LOG_DEBUG, "Config gpslocation = %s\n", _gpioconfig_.gpslocation);
            } else if (strncasecmp (b_ptr, "DOCUMENTROOT", 12) == 0) {
              _gpioconfig_.docroot = cleanalloc(indx+1);
              logMessage (LOG_DEBUG, "Config docroot = %s\n", _gpioconfig_.docroot);
            } else if (strncasecmp (b_ptr, "WEBCACHE", 8) == 0) {
              _gpioconfig_.webcache = text2bool(indx+1);
              logMessage (LOG_DEBUG, "Config Web cache = %d\n", _gpioconfig_.webcache);
            } else if (strncasecmp (b_ptr, "ONEWIREDEVICE", 13) == 0) {
              _gpioconfig_.onewcfg[_gpioconfig_.lpd8806devices].device = cleanallocindex(indx+1, 1);
              _gpioconfig_.onewcfg[_gpioconfig_.lpd8806devices].name = cleanallocindex(indx+1, 2);
              logMessage (LOG_DEBUG, "Config onewiredevice: '%s' = '%s'\n", 
                         _gpioconfig_.onewcfg[_gpioconfig_.lpd8806devices].device, 
                         _gpioconfig_.onewcfg[_gpioconfig_.lpd8806devices].name);
              _gpioconfig_.lpd8806devices++;
            } else if (strncasecmp (b_ptr, "LPD8806DEVICE", 13) == 0) {
              _gpioconfig_.lpd8806cfg[_gpioconfig_.onewiredevices].device = cleanallocindex(indx+1, 1);
              _gpioconfig_.lpd8806cfg[_gpioconfig_.onewiredevices].name = cleanallocindex(indx+1, 2);
              _gpioconfig_.lpd8806cfg[_gpioconfig_.onewiredevices].buf.leds = cleanint(cleanallocindex(indx+1, 3));
              if (_gpioconfig_.lpd8806cfg[_gpioconfig_.onewiredevices].buf.leds <= 0)
                {_gpioconfig_.lpd8806cfg[_gpioconfig_.onewiredevices].buf.leds=DEFAULT_LEDS;}
              logMessage (LOG_DEBUG, "Config lpd8806cfg: device='%s' name='%s' leds=%d\n", 
                          _gpioconfig_.lpd8806cfg[_gpioconfig_.onewiredevices].device, 
                          _gpioconfig_.lpd8806cfg[_gpioconfig_.onewiredevices].name,
                          _gpioconfig_.lpd8806cfg[_gpioconfig_.onewiredevices].buf.leds);
              _gpioconfig_.onewiredevices++;
            }
          } else {
            if (count_characters(b_ptr, delim[0]) < 6)
            {
              b_ptr[strlen(b_ptr)-1] = '\0';
              logMessage (LOG_WARNING, "Unknown line in cfg '%s', ignoring", b_ptr);
              continue;
            }
            tokenindex=0;
            token = strtok(b_ptr, delim);
            while( token != NULL ) {
              if ( token[(strlen(token)-1)] == '\n') { token[(strlen(token)-1)] = '\0'; }
              switch (tokenindex) {
              case 0:
                _gpioconfig_.gpiocfg[line].pin = atoi(token); 
                break;
              case 1:
                _gpioconfig_.gpiocfg[line].input_output = atoi(token); 
                break;
              case 2:
                _gpioconfig_.gpiocfg[line].set_pull_updown = atoi(token);
                break;
              case 3:
                _gpioconfig_.gpiocfg[line].receive_mode = atoi(token);
                break;
              case 4:
                _gpioconfig_.gpiocfg[line].receive_state = atoi(token);
                break;
              case 5:
                _gpioconfig_.gpiocfg[line].output_pin = atoi(token);
                break;
              case 6:
                _gpioconfig_.gpiocfg[line].output_state = atoi(token);
                break;
              case 7:
                _gpioconfig_.gpiocfg[line].name = (char*)malloc(strlen(token)+1);
                //strncpy ( _gpioconfig_.gpiocfg[line].name, token, 50 );
                strcpy ( _gpioconfig_.gpiocfg[line].name, trimwhitespace(token) );
                break;
              }
              token = strtok(NULL, delim);
              tokenindex++;
            }
            if (_gpioconfig_.gpiocfg[line].pin >= 0) {
              logMessage (LOG_DEBUG,"Config line %d : %s\n%25s : %d\n%25s : %d\n%25s : %d\n%25s : %d\n%25s : %d\n%25s : %d\n\%25s : %d\n",
              line, _gpioconfig_.gpiocfg[line].name,
              "PIN",_gpioconfig_.gpiocfg[line].pin,
              "inout/output", _gpioconfig_.gpiocfg[line].input_output, 
              "Set pull up/down", _gpioconfig_.gpiocfg[line].set_pull_updown, 
              "Receive mode", _gpioconfig_.gpiocfg[line].receive_mode, 
              "Receive state", _gpioconfig_.gpiocfg[line].receive_state, 
              "Trigger output pin", _gpioconfig_.gpiocfg[line].output_pin,
              "Trigger output state", _gpioconfig_.gpiocfg[line].output_state);
              
              _gpioconfig_.pinscfgs++;
            }
            line++;
          }
        }
      }
    }
    fclose(fp);
  } else {
    /* error processing, couldn't open file */
    displayLastSystemError(cfgFile);
    exit (EXIT_FAILURE);
  }
}
