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

#define DEBUGFILE     "/var/log/gpioctrld.log"

/*  _var = local
    _var_ = global
*/

boolean _daemon_ = false;
boolean _debuglog_ = false;
boolean _debug2file_ = false;

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

char *elevel2text(int level) 
{
  switch(level) {
    case LOG_ERR:
      return "Error:";
      break;
    case LOG_DEBUG:
      return "Debug:";
      break;
    case LOG_WARNING:
      return "Warning:";
      break;
    case LOG_INFO:
    default:
      return "Info:";
      break;
  }
  
  return "";
}

void logMessage(int level, char *format, ...)
{
  if (_debuglog_ == FALSE && level == LOG_DEBUG)
    return;
  
  char buffer[512];
  va_list args;
  va_start(args, format);
  strncpy(buffer, "         ", 8);
  vsprintf (&buffer[8], format, args);
  va_end(args);

  if (_daemon_ == TRUE)
  {
    syslog (level, "%s", &buffer[8]);
    closelog ();
  } 
  
  if (_daemon_ != TRUE || level == LOG_ERR || _debug2file_ == TRUE)
  {
    char *strLevel = elevel2text(level);

    strncpy(buffer, strLevel, strlen(strLevel));
    if ( buffer[strlen(buffer)-1] != '\n') { 
      buffer[strlen(buffer)+1] = '\0';
      buffer[strlen(buffer)] = '\n';
    }
    
    if (_debug2file_ == TRUE) {
      int fp = open(DEBUGFILE, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
      if (fp != -1) {
        write(fp, buffer, strlen(buffer)+1 );
        close(fp);
      } else {
        fprintf (stderr, "Can't open debug log\n %s", buffer);
      }
    } else if (level != LOG_ERR) {
      printf ("%s", buffer);
    }
    
    if (level == LOG_ERR)
      fprintf (stderr, "%s", buffer);
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

char * replace(char const * const original, char const * const pattern, char const * const replacement) 
{
  size_t const replen = strlen(replacement);
  size_t const patlen = strlen(pattern);
  size_t const orilen = strlen(original);

  size_t patcnt = 0;
  const char * oriptr;
  const char * patloc;

  // find how many times the pattern occurs in the original string
  for ( (oriptr = original); (patloc = strstr(oriptr, pattern)); (oriptr = patloc + patlen))
  {
    patcnt++;
  }

  {
    // allocate memory for the new string
    size_t const retlen = orilen + patcnt * (replen - patlen);
    char * const returned = (char *) malloc( sizeof(char) * (retlen + 1) );

    if (returned != NULL)
    {
      // copy the original string, 
      // replacing all the instances of the pattern
      char * retptr = returned;
      for ( (oriptr = original); (patloc = strstr(oriptr, pattern)); (oriptr = patloc + patlen))
      {
        size_t const skplen = patloc - oriptr;
        // copy the section until the occurence of the pattern
        strncpy(retptr, oriptr, skplen);
        retptr += skplen;
        // copy the replacement 
        strncpy(retptr, replacement, replen);
        retptr += replen;
      }
      // copy the rest of the string.
      strcpy(retptr, oriptr);
    }
    return returned;
  }
}

void run_external(char *command, int state)
{

  char *cmd = replace(command, "%STATE%", (state==1?"1":"0"));
  system(cmd);
  logMessage (LOG_DEBUG, "Ran command '%s'\n", cmd);
  free(cmd);
    
  return;
}

void run_external_OLD(char *command, int state)
{
  //int status;
  // By calling fork(), a child process will be created as a exact duplicate of the calling process.
    // Search for fork() (maybe "man fork" on Linux) for more information.
  if(fork() == 0){ 
    // Child process will return 0 from fork()
    //printf("I'm the child process.\n");
    char *cmd = replace(command, "%STATE%", (state==1?"1":"0"));
    system(cmd);
    logMessage (LOG_DEBUG, "Ran command '%s'\n", cmd);
    free(cmd);
    exit(0);
  }else{
    // Parent process will return a non-zero value from fork()
    //printf("I'm the parent.\n");
    
  }

  return;
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