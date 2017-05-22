#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <libgen.h>
#include <netinet/in.h>
#include <time.h>
#include <pthread.h>
#include <arpa/inet.h> //inet_addr
//#include <sys/socket.h>
//#include <netdb.h>
// We want a success/failure return value from 'wiringPiSetup()'
#define WIRINGPI_CODES      1
#include <wiringPi.h>

#include "utils.h"
#include "config.h"
#include "httpd.h"
#include "lpd8806worker.h"

//#define PIDFILE     "/var/run/fha_daemon.pid"
#define PIDLOCATION "/var/run/"
//#define CFGFILE     "./config.cfg"
//#define HTTPD_PORT 80

// Use threads to server http requests
#define PTHREAD

/* Function prototypes */
void Daemon_Stop (int signum);
void main_loop (void);
void event_trigger (struct GPIOcfg *);
void intHandler(int signum);

//extern _gpioconfig_;

/* BS functions due to limitations in wiringpi */
void event_trigger_0 (void) { event_trigger (&_gpioconfig_.gpiocfg[0]) ; }
void event_trigger_1 (void) { event_trigger (&_gpioconfig_.gpiocfg[1]) ; }
void event_trigger_2 (void) { event_trigger (&_gpioconfig_.gpiocfg[2]) ; }
void event_trigger_3 (void) { event_trigger (&_gpioconfig_.gpiocfg[3]) ; }
void event_trigger_4 (void) { event_trigger (&_gpioconfig_.gpiocfg[4]) ; }
void event_trigger_5 (void) { event_trigger (&_gpioconfig_.gpiocfg[5]) ; }
void event_trigger_6 (void) { event_trigger (&_gpioconfig_.gpiocfg[6]) ; }
void event_trigger_7 (void) { event_trigger (&_gpioconfig_.gpiocfg[7]) ; }
void event_trigger_8 (void) { event_trigger (&_gpioconfig_.gpiocfg[8]) ; }
void event_trigger_9 (void) { event_trigger (&_gpioconfig_.gpiocfg[9]) ; }

typedef void (*FunctionCallback)();
FunctionCallback callbackFunctions[] = {&event_trigger_0, &event_trigger_1, &event_trigger_2, 
                                        &event_trigger_3, &event_trigger_4, &event_trigger_5, 
                                        &event_trigger_6, &event_trigger_7, &event_trigger_8, 
                                        &event_trigger_9};

static int server_sock = -1;
 

#ifdef PTHREAD
void *connection_handler(void *socket_desc)
{
    //logMessage (LOG_DEBUG, "connection_handler()");
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    accept_request(sock);
    close(sock);
    
    pthread_exit(NULL);
  //return 0;
}
#endif
 
/* ------------------ Start Here ----------------------- */
int main (int argc, char *argv[])
{
  int i;
  char *cfg = CFGFILE;
  int port = -1;
  
  // Default daemon to true to logging works correctly before we even start deamon process.
  _daemon_ = true;
  //_gpioconfig_.port = HTTPD_PORT;
  
  for (i = 1; i < argc; i++)
  {
    if (strcmp (argv[i], "-h") == 0)
    {
      printf ("%s (options)\n -d do NOT run as daemon\n -v verbose\n -c [cfg file name]\n -f debug 2 file\n -h this", argv[0]);
      exit (EXIT_SUCCESS);
    }
    else if (strcmp (argv[i], "-d") == 0)
      _daemon_ = false;
    else if (strcmp (argv[i], "-v") == 0)
      _debuglog_ = true;
    else if (strcmp (argv[i], "-f") == 0)
      _debug2file_ = true;
    else if (strcmp (argv[i], "-c") == 0)
      cfg = argv[++i];
    else if (strcmp (argv[i], "-p") == 0)
      port = atoi(argv[++i]);
  }

  /* Check we are root */
  if (getuid() != 0)
  {
    logMessage(LOG_ERR,"%s Can only be run as root\n",argv[0]);
    exit(EXIT_FAILURE);
  }
  
  readCfg(cfg);
  
  if (port != -1)
    _gpioconfig_.port = port; 
  
  logMessage (LOG_DEBUG, "Running %s with options :- daemon=%d, verbose=%d, httpdport=%d, debug2file=%d configfile=%s\n", 
                         argv[0], _daemon_, _debuglog_, _gpioconfig_.port, _debug2file_, cfg);

  if (_daemon_ == false)
  {
    main_loop ();
  }
  else
  {
    char pidfile[256];
    sprintf(pidfile, "%s/%s.pid",PIDLOCATION, basename(argv[0]));
    daemonise (pidfile, main_loop);
  }

  exit (EXIT_SUCCESS);
}

void main_loop ()
{
  int i;
  //int server_sock = -1;
  u_short http_port;
  int client_sock = -1;
  struct sockaddr_in client_name;
  int client_name_len = sizeof(client_name);
  
  /* Make sure the file '/usr/local/bin/gpio' exists */
  struct stat filestat;
  if (stat ("/usr/local/bin/gpio", &filestat) == -1)
  {
    logMessage(LOG_ERR,"The program '/usr/local/bin/gpio' is missing, exiting");
    exit (EXIT_FAILURE);
  }

  /* Initialize 'wiringPi' library */
  if (wiringPiSetup () == -1)
  {
    displayLastSystemError ("'wiringPi' library couldn't be initialized, exiting");
    exit (EXIT_FAILURE);
  }

  logMessage(LOG_INFO, "Setting up GPIO\n");
  
  //for (i=0; _gpioconfig_.gpiocfg[i].pin > -1 ; i++)
  for (i=0; i < _gpioconfig_.pinscfgs ; i++)
  {
    logMessage (LOG_DEBUG, "Setting up pin %d\n", _gpioconfig_.gpiocfg[i].pin);
    
    pinMode (_gpioconfig_.gpiocfg[i].pin, _gpioconfig_.gpiocfg[i].input_output);    
    logMessage (LOG_DEBUG, "Set pin %d set to %s\n", _gpioconfig_.gpiocfg[i].pin,(_gpioconfig_.gpiocfg[i].input_output==OUTPUT?"OUTPUT":"INPUT") );
    
    if (_gpioconfig_.gpiocfg[i].input_output == OUTPUT) {
      digitalWrite(_gpioconfig_.gpiocfg[i].pin, 1);
      logMessage (LOG_DEBUG, "Set pin %d set to high/off\n", _gpioconfig_.gpiocfg[i].pin);
    }
    
    if (_gpioconfig_.gpiocfg[i].set_pull_updown != NONE) {
      logMessage (LOG_DEBUG, "Set pin %d set pull up/down resistor to %d\n", _gpioconfig_.gpiocfg[i].pin, _gpioconfig_.gpiocfg[i].set_pull_updown );
      pullUpDnControl (_gpioconfig_.gpiocfg[i].pin, _gpioconfig_.gpiocfg[i].set_pull_updown);
    }

    if ( _gpioconfig_.gpiocfg[i].receive_mode != NONE) {
      if (wiringPiISR (_gpioconfig_.gpiocfg[i].pin, _gpioconfig_.gpiocfg[i].receive_mode, callbackFunctions[i]) == -1)
      {
        displayLastSystemError ("Unable to set interrupt handler for specified pin, exiting");
        exit (EXIT_FAILURE);
      }
      logMessage (LOG_DEBUG, "Set pin %d for trigger with rising/falling mode %d\n", _gpioconfig_.gpiocfg[i].pin, _gpioconfig_.gpiocfg[i].receive_mode );
      
      // Reset output mode if we are triggering on an output pin, as last call re-sets state for some reason
      if (_gpioconfig_.gpiocfg[i].input_output == OUTPUT) {
        pinMode (_gpioconfig_.gpiocfg[i].pin, _gpioconfig_.gpiocfg[i].input_output);
        logMessage (LOG_DEBUG, "ReSet pin %d set to %s\n", _gpioconfig_.gpiocfg[i].pin,(_gpioconfig_.gpiocfg[i].input_output==OUTPUT?"OUTPUT":"INPUT") );
      }
    }
  }

  logMessage (LOG_DEBUG, "Pin setup complete\n");

  logMessage (LOG_DEBUG, "Starting HTTPD\n");
  
  // NSF, need to work out why the cast of _gpioconfig_.port fails in startup function, but just copy var for the moment.
  http_port = _gpioconfig_.port;
  if ( (server_sock = startup(&http_port)) == HTTPD_FAILURE )
  {
    logMessage (LOG_ERR, "Error starting HTTPD\n");
    exit (EXIT_FAILURE);
  }
  
  logMessage (LOG_INFO, "httpd running on port %d\n", _gpioconfig_.port);
  signal(SIGINT, intHandler);
  
  
  
#ifdef PTHREAD
  pthread_t thread_id;
  pthread_attr_t attr;
  /* Initialize and set thread detached attribute */
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  //pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  
  while( (client_sock = accept(server_sock, (struct sockaddr *)&client_name, (socklen_t*)&client_name_len)) )
  {
    if (client_sock == -1) // Timeout
      continue;
    //printf("- %d -\n",client_sock);
    logMessage (LOG_DEBUG, "Client IP: %s\n", inet_ntoa(client_name.sin_addr));

    if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) &client_sock) < 0)
    {
      logMessage (LOG_ERR, "could not create thread\n");
      return;
    }
    
    //ONLY USE WITH PTHREAD_CREATE_JOINABLE. Now join the thread , so that we dont terminate before the thread
    pthread_join( thread_id , NULL);
  }
#else
  while (true)
  {
    //logMessage (LOG_DEBUG, "httpd waiting!\n");
    
    client_sock = accept(server_sock,(struct sockaddr *)&client_name,(socklen_t *)&client_name_len);
    if (client_sock == -1)
    {
      displayLastSystemError ("'accept' socket failure");
      exit (EXIT_FAILURE);
      // May want to exit program here
    }
    
    accept_request(client_sock);
    close(client_sock);
  }
  
  close(server_sock);
#endif
}

void Daemon_Stop (int signum)
{
  /* 'SIGTERM' was issued, system is telling this daemon to stop */
  //syslog (LOG_INFO, "Stopping daemon");
  logMessage (LOG_INFO, "Stopping daemon!\n");
  close(server_sock);
  logMessage (LOG_INFO, "Stopping LED threads!\n");
  lpd880led_cleanup();
  /*
#ifdef PTHREAD  
  logMessage (LOG_INFO, "Stopping httpd threads!\n"); 
  pthread_exit(NULL);
#endif
  */
  logMessage (LOG_INFO, "Exit!\n"); 
  exit (EXIT_SUCCESS);
}

void intHandler(int signum) {

  //syslog (LOG_INFO, "Stopping");
  logMessage (LOG_INFO, "Stopping!\n");
  close(server_sock);
  logMessage (LOG_INFO, "Stopping LED threads!\n");
  lpd880led_cleanup();
  /*
#ifdef PTHREAD
  logMessage (LOG_INFO, "Stopping httpd threads!\n"); 
  pthread_exit(NULL);
#endif
*/
  exit (EXIT_SUCCESS);
}

void event_trigger (struct GPIOcfg *gpioconfig)
{
  int out_state_toset; 
  int in_state_read;
  time_t rawtime;
  struct tm * timeinfo;
  char timebuffer[20];

  time (&rawtime);
  timeinfo = localtime (&rawtime);
  strftime (timebuffer,20,"%T",timeinfo);
 
 //printf("Received trigger %d - at %s - last trigger %d\n",digitalRead (gpioconfig->pin), timebuffer, gpioconfig->last_event_state);
 
  logMessage (LOG_DEBUG,"%s Received input change on pin %d - START\n",timebuffer, gpioconfig->pin);
  
  if ( (rawtime - gpioconfig->last_event_time) < 1 )
  {
    logMessage (LOG_DEBUG,"        ignoring, time between triggers too short (%d-%d)=%d\n",rawtime, gpioconfig->last_event_time, (rawtime - gpioconfig->last_event_time));
    return;
  }
  gpioconfig->last_event_time = rawtime;
  //logMessage (LOG_DEBUG,"Diff between last event %d (%d, %d)\n",rawtime - gpioconfig->last_event_time, rawtime, gpioconfig->last_event_time);
  
  /* Handle button pressed interrupts */
  
  in_state_read = digitalRead (gpioconfig->pin);
  logMessage (LOG_DEBUG, "        received state %d, previous state %d\n", in_state_read, gpioconfig->last_event_state);
  
  if ( gpioconfig->receive_state == BOTH || in_state_read == gpioconfig->receive_state )
  {
    gpioconfig->last_event_state = in_state_read;
    
    if (gpioconfig->output_pin != NONE) {
      if (gpioconfig->output_state == TOGGLE)
        out_state_toset = digitalRead (gpioconfig->output_pin) ^ 1;
      else
        out_state_toset = gpioconfig->output_state;
    
      digitalWrite(gpioconfig->output_pin, out_state_toset);
      logMessage (LOG_DEBUG, "        sent state %d to PIN %d\n", out_state_toset, gpioconfig->output_pin);
    }
    
    if (gpioconfig->ext_cmd != NULL) {
      //logMessage (LOG_DEBUG, "command '%s'\n", gpioconfig->ext_cmd);
      run_external(gpioconfig->ext_cmd, in_state_read);
    } 
    
    //sleep(1);
  } else {
    logMessage (LOG_DEBUG,"        ignoring, reseived state does not match cfg\n");
    return;
  }
  
  // Sleep for 1 second just to limit duplicte events
  //sleep(1);
  
  logMessage (LOG_DEBUG,"%s Receive input change on pin %d - END\n",timebuffer, gpioconfig->pin);
}
