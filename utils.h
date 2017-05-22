#include <syslog.h>

#ifndef UTILS_H_
#define UTILS_H_

#ifndef EXIT_SUCCESS
  #define EXIT_FAILURE 1
  #define EXIT_SUCCESS 0
#endif

#ifndef TRUE
  #define TRUE 1
  #define FALSE 0
#endif

typedef enum
{
  false = FALSE, true = TRUE
} boolean;

void daemonise ( char *pidFile, void (*main_function)(void) );
//void debugPrint (char *format, ...);
void displayLastSystemError (const char *on_what);
void logMessage(int level, char *format, ...);
int count_characters(const char *str, char character);
void run_external(char *command, int state);
//void readCfg (char *cfgFile);


//#ifndef _UTILS_C_
  extern boolean _daemon_;
  extern boolean _debuglog_;
  extern boolean _debug2file_;
//#endif

#endif /* UTILS_H_ */
