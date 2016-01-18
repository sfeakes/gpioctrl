/* daemon.c */

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <syslog.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef _UTILS_C_
#define _UTILS_C_
#endif

#include "utils.h"

#define MAXCFGLINE 265

/*  _var = local
    _var_ = global
*/

boolean _daemon_ = false;
boolean _debuglog_ = false;

/*
* This function reports the error and
* exits back to the shell:
*/
void displayLastSystemError (const char *on_what)
{
  fputs (strerror (errno), stderr);
  fputs (": ", stderr);
  fputs (on_what, stderr);
  fputc ('\n', stderr);

  if (_daemon_ == TRUE)
  {
    logMessage (LOG_ERR, "%d : %s", errno, on_what);
    closelog ();
  }
}

//LOG_INFO
//LOG_ERR
//LOG_DEBUG
//LOG_WARNING
void logMessage(int level, char *format, ...)
{
  if (_debuglog_ == FALSE && level == LOG_DEBUG)
    return;
  
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsprintf (buffer, format, args);
  va_end(args);

  if (_daemon_ == TRUE)
  {
    syslog (level, "%s", buffer);
    closelog ();
  } 
  
  if (_daemon_ != TRUE || level == LOG_ERR)
  {
    char *strLevel = NULL;
    switch(level) {
    case LOG_ERR:
      strLevel = "Error";
      break;
    case LOG_DEBUG:
      strLevel = "Debug";
      break;
    case LOG_WARNING:
      strLevel = "Warning";
      break;
    case LOG_INFO:
    default:
      strLevel = "Info";
      break;
    }
    if ( buffer[strlen(buffer)-1] == '\n') { 
      buffer[strlen(buffer)-1] = '\0';
    }
    
    if (level != LOG_ERR)
      printf ("%s: %s\n", strLevel, buffer);
    else
      fprintf (stderr, "%s: %s\n", strLevel, buffer);
  }

}

void daemonise (char *pidFile, void (*main_function) (void))
{
  FILE *fp = NULL;
  pid_t process_id = 0;
  pid_t sid = 0;

  _daemon_ = true;
  
  /* Check we are root */
  if (getuid() != 0)
  {
    logMessage(LOG_ERR,"Can only be run as root\n");
    exit(EXIT_FAILURE);
  }

  int pid_file = open (pidFile, O_CREAT | O_RDWR, 0666);
  int rc = flock (pid_file, LOCK_EX | LOCK_NB);
  if (rc)
  {
    if (EWOULDBLOCK == errno)
    ; // another instance is running
    //fputs ("\nAnother instance is already running\n", stderr);
    logMessage(LOG_ERR,"\nAnother instance is already running\n");
    exit (EXIT_FAILURE);
  }

  process_id = fork ();
  // Indication of fork() failure
  if (process_id < 0)
  {
    displayLastSystemError ("fork failed!");
    // Return failure in exit status
    exit (EXIT_FAILURE);
  }
  // PARENT PROCESS. Need to kill it.
  if (process_id > 0)
  {
    fp = fopen (pidFile, "w");

    if (fp == NULL)
    logMessage(LOG_ERR,"can't write to PID file %s",pidFile);
    else
    fprintf(fp, "%d", process_id);

    fclose (fp);
    logMessage (LOG_DEBUG, "process_id of child process %d \n", process_id);
    // return success in exit status
    exit (EXIT_SUCCESS);
  }
  //unmask the file mode
  umask (0);
  //set new session
  sid = setsid ();
  if (sid < 0)
  {
    // Return failure
    displayLastSystemError("Failed to fork process");
    exit (EXIT_FAILURE);
  }
  // Change the current working directory to root.
  chdir ("/");
  // Close stdin. stdout and stderr
  close (STDIN_FILENO);
  close (STDOUT_FILENO);
  close (STDERR_FILENO);

  // this is the first instance
  (*main_function) ();

  return;
}

int count_characters(const char *str, char character)
{
    const char *p = str;
    int count = 0;

    do {
        if (*p == character)
            count++;
    } while (*(p++));

    return count;
}
/*
void readCfg (char *cfgFile)
{
  FILE * fp ;
  char bufr[MAXCFGLINE];
  const char delim[2] = ";";
  char *token;
  int line = 1;

  if( (fp = fopen(cfgFile, "r")) != NULL){
    while(! feof(fp)){
      if (fgets(bufr, MAXCFGLINE, fp) != NULL)
      {
        if (bufr[0] != '#' && bufr[0] != ' ' && bufr[0] != '\n')
        {
          token = strtok(bufr, delim);
          while( token != NULL ) {
            if ( token[(strlen(token)-1)] == '\n') { token[(strlen(token)-1)] = '\0'; }
            printf( "Line %d - Token %s\n", line, token );
            token = strtok(NULL, delim);
          }
          line++;
        }
      }
    }
    fclose(fp);
  } else {
    displayLastSystemError(cfgFile);
    exit (EXIT_FAILURE);
  }
}
*/