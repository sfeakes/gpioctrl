

/*
text/html	.html
text/plain	.txt
application/javascript	.js
application/json	.json
image/jpeg	.jpeg, .jpg
mage/gif	.gif
image/jpeg	.jpeg, .jpg
application/xml	.xml
*/

#define NOTFOUND_404 "<html><head><title>404 Not Found</title></head><body><h1>Not Found</h1><p>The requested URL was not found on this server.</p><hr></body></html>"


#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>

#include <fcntl.h>

#include <wiringPi.h>
#include "httpd.h"
#include "utils.h"
#include "config.h"
#include "lpd8806worker.h"

// Need to put this in headder
//extern int readw1(char *device, char *buff);
extern int readw1(struct ONEWcfg *w1device, char *rtnbuff);
extern int readw1_for_mh(struct ONEWcfg *w1device, int *rtnbuff);

#define ISspace(x) isspace((int)(x))

#define SERVER_STRING "Server: gpioctrld/0.1.0\r\n"

#define READ_BUFFER_SIZE 265

//void accept_request(int);
//int startup(u_short *);

void bad_request(int);
void cat(int, FILE *);
void cannot_execute(int);
//void error_die(const char *);
void execute_cgi(int, const char *, const char *, const char *);
int get_line(int, char *, int);
void headers(int, const char *);
void not_found(int);
void serve_file(int, const char *);
void serve_gpio_request(int, char *);
void serve_led_request(int, char *);
void serve_meteohub_request(int client, char *query);
void unimplemented(int);

/**********************************************************************/
/* A request has caused a call to accept() on the server port to
* return.  Process the request appropriately.
* Parameters: the socket connected to the client */
/**********************************************************************/
void accept_request(int client)
{
  char buf[1024];
  int numchars;
  char method[255];
  char url[255];
  char path[512];
  size_t i, j;
  struct stat st;
  //int cgi = 0;      /* becomes true if server decides this is a CGI program */
  char *query_string = NULL;

  numchars = get_line(client, buf, sizeof(buf));
  i = 0; j = 0;
  while (!ISspace(buf[j]) && (i < sizeof(method) - 1))
  {
    method[i] = buf[j];
    i++; j++;
  }
  method[i] = '\0';

  if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
  {
    unimplemented(client);
    close(client);
    return;
  }
/*
  if (strcasecmp(method, "POST") == 0)
  cgi = 1;
*/
  i = 0;
  while (ISspace(buf[j]) && (j < sizeof(buf)))
    j++;
  while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf)))
  {
    url[i] = buf[j];
    i++; j++;
  }
  url[i] = '\0';

  if (strcasecmp(method, "GET") == 0)
  {
    query_string = url;
    while ((*query_string != '?') && (*query_string != '\0'))
    query_string++;
    if (*query_string == '?')
    {
//      cgi = 1;
      *query_string = '\0';
      query_string++;
    }
  }
/*
  serve_gpio_request(client, NULL);
  */
  
  logMessage (LOG_DEBUG,"HTTPD processing %s %s\n",url, query_string);
  
  if ( strcmp(url, "/gpio") == 0 || strcmp(url, "/gpio/") == 0) {
    serve_gpio_request(client, query_string);
  } 
  else if ( strcmp(url, "/led") == 0 || strcmp(url, "/led/") == 0) 
  {
    serve_led_request(client, query_string);
  }
  else if ( strcmp(url, "/mh") == 0 || strcmp(url, "/mh/") == 0) 
  {
    serve_meteohub_request(client, query_string);
  }
  else
  {
    if ( _gpioconfig_.docroot != 0 )
      sprintf(path, "%s/%s", _gpioconfig_.docroot, url);
    else
      sprintf(path, "htdocs%s", url);
      
    if (path[strlen(path) - 1] == '/')
      strcat(path, "index.html");
 
    if (stat(path, &st) == -1) {
      while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
        numchars = get_line(client, buf, sizeof(buf));
      not_found(client);
    }
    else
    {
      if ((st.st_mode & S_IFMT) == S_IFDIR)
      {
        while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
          numchars = get_line(client, buf, sizeof(buf));
        not_found(client);
      } else {
        serve_file(client, path);
      }
      /*
      put cgi in here in the future
      */
    }
  }

  close(client);
}

/**********************************************************************/
/* Inform the client that a request it has made has a problem.
* Parameters: client socket */
/**********************************************************************/
void bad_request(int client)
{
  char buf[1024];

  sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
  send(client, buf, sizeof(buf), 0);
  sprintf(buf, "Content-type: text/html\r\n");
  send(client, buf, sizeof(buf), 0);
  sprintf(buf, "\r\n");
  send(client, buf, sizeof(buf), 0);
  sprintf(buf, "<P>Your browser sent a bad request, ");
  send(client, buf, sizeof(buf), 0);
  sprintf(buf, "such as a POST without a Content-Length.\r\n");
  send(client, buf, sizeof(buf), 0);
}

/**********************************************************************/
/* Put the entire contents of a file out on a socket.  This function
* is named after the UNIX "cat" command, because it might have been
* easier just to do something like pipe, fork, and exec("cat").
* Parameters: the client socket descriptor
*             FILE pointer for the file to cat */
/**********************************************************************/
void cat(int client, FILE *resource)
{
  char buf[READ_BUFFER_SIZE];
  int n;

  while (!feof(resource)) 
  {
    n = fread(buf, 1, READ_BUFFER_SIZE, resource);
    send(client, buf, n, 0);
  }
  /*
  fgets(buf, sizeof(buf), resource);
  while (!feof(resource))
  {
    send(client, buf, strlen(buf), 0);
    fgets(buf, sizeof(buf), resource);
  }
  */
}

/**********************************************************************/
/* Inform the client that a CGI script could not be executed.
* Parameter: the client socket descriptor. */
/**********************************************************************/
void cannot_execute(int client)
{
  char buf[1024];

  sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "Content-type: text/html\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
  send(client, buf, strlen(buf), 0);
}



/**********************************************************************/
/* Get a line from a socket, whether the line ends in a newline,
* carriage return, or a CRLF combination.  Terminates the string read
* with a null character.  If no newline indicator is found before the
* end of the buffer, the string is terminated with a null.  If any of
* the above three line terminators is read, the last character of the
* string will be a linefeed and the string will be terminated with a
* null character.
* Parameters: the socket descriptor
*             the buffer to save the data in
*             the size of the buffer
* Returns: the number of bytes stored (excluding null) */
/**********************************************************************/
int get_line(int sock, char *buf, int size)
{
  int i = 0;
  char c = '\0';
  int n;

  while ((i < size - 1) && (c != '\n'))
  {
    n = recv(sock, &c, 1, 0);
    /* DEBUG printf("%02X\n", c); */
    if (n > 0)
    {
      if (c == '\r')
      {
        n = recv(sock, &c, 1, MSG_PEEK);
        /* DEBUG printf("%02X\n", c); */
        if ((n > 0) && (c == '\n'))
        recv(sock, &c, 1, 0);
        else
        c = '\n';
      }
      buf[i] = c;
      i++;
    }
    else
    c = '\n';
  }
  buf[i] = '\0';

  return(i);
}

/**********************************************************************/
/* Return the informational HTTP headers about a file. */
/* Parameters: the socket to print the headers on
*             the name of the file */
/**********************************************************************/


#define CACHE "cache-control: public, max-age=86400\r\n"
#define NOCACHE "cache-control: private, max-age=0, no-cache, no-store\r\n"

void headers(int client, const char *filename)
{
  char buf[1024];
  char *ext;
  (void)filename;  /* could use filename to determine file type */
  
  strcpy(buf, "HTTP/1.0 200 OK\r\n");
  send(client, buf, strlen(buf), 0);
  strcpy(buf, SERVER_STRING);
  send(client, buf, strlen(buf), 0);
 
  if (filename == NULL) {
	 sprintf(buf, "Content-Type: text/html\r\n%s", (_gpioconfig_.webcache==TRUE?CACHE:NOCACHE) );
  } else {
    ext=strrchr(filename,'.');
    if ( (strcasecmp(ext, ".png")) == 0) {
      sprintf(buf, "Content-Type: image/png\r\n%s", (_gpioconfig_.webcache==TRUE?CACHE:NOCACHE));
    } else if ( (strcasecmp(ext, ".json")) == 0 || (strcasecmp(ext, ".jsn")) == 0) {
      sprintf(buf, "Content-Type: application/json\r\n%s", NOCACHE);
    } else if ( (strcasecmp(ext, ".jpg")) == 0 || (strcasecmp(ext, ".jpeg")) == 0) {
      sprintf(buf, "Content-Type: image/jpeg\r\n%s", (_gpioconfig_.webcache==TRUE?CACHE:NOCACHE));
    } else {
      sprintf(buf, "Content-Type: text/html\r\n%s", (_gpioconfig_.webcache==TRUE?CACHE:NOCACHE));
    }
  }
  send(client, buf, strlen(buf), 0);
    
  strcpy(buf, "\r\n");
  send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Give a client a 404 not found status message. */
/**********************************************************************/
void not_found(int client)
{
  char buf[1024];

  sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, SERVER_STRING);
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "Content-type: text/html\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "%s\r\n", NOTFOUND_404);
  send(client, buf, strlen(buf), 0);
}

uint8_t toRGB(char *snum)
{
  int rtn = atoi(snum);

  rtn>255?rtn=255:rtn;
  rtn<0?rtn=0:rtn;
  
  return rtn;
}

void serve_meteohub_request(int client, char *query)
{
  char buf[1024];
  int value = 0;
  int numchars = 1;
  int i;
  
  buf[0] = 'A'; buf[1] = '\0';
  while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
  {
    numchars = get_line(client, buf, sizeof(buf));
  }
  
  for (i=0; i < _gpioconfig_.onewiredevices; i++)
  {
    if (readw1_for_mh(&_gpioconfig_.onewcfg[i], &value) )
    {
      sprintf(buf,"%d|%s|%s\r\n",value,_gpioconfig_.onewcfg[i].name,_gpioconfig_.onewcfg[i].device);
      send(client, buf, strlen(buf), 0);
    }
  }
}

void serve_led_request(int client, char *query)
{
  char buf[1024];
  int numchars = 1;
  char *token;
  char *split;
  int r = -1;
  int g = -1;
  int b = -1;
  int pattern = 0;
  int option = 0;
  
  buf[0] = 'A'; buf[1] = '\0';
  while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
  {
    numchars = get_line(client, buf, sizeof(buf));
  }
  
  token = strtok(query, "&");
  while( token != NULL ) {
	  split=strchr(token,'=');
	  switch(token[0]) {
  	  case 'r':
	      r = toRGB(split+1);
	    break;
  	  case 'g':
	      g = toRGB(split+1);
	    break;
  	  case 'b':
	      b = toRGB(split+1);
	    break;
      case 'p':
	      pattern = toRGB(split+1);
	    break;
      case 'o':
	      option = toRGB(split+1);
	    break;
	  }
    token = strtok(NULL, "&");
  }
  
  if ( (r == -1 || g == -1 || b == -1) && pattern == 0) {
    logMessage (LOG_DEBUG,"Received BAD LED query:- Red=%d Green=%d, Blue=%d\n",r,g,b);
    cannot_execute(client);
    return;
  } else {
    int index = 0;
    lpd8806worker(&_gpioconfig_.lpd8806cfg[index], (uint8_t*)&pattern, (uint8_t*)&option, (uint8_t*)&r, (uint8_t*)&g, (uint8_t*)&b );
    
    sprintf(buf, "{ \"title\" : \"%s\",\r\n\"name\" : \"%s\",\r\n", _gpioconfig_.name, _gpioconfig_.name);
	  send(client, buf, strlen(buf), 0);
    sprintf(buf, "\"led\" : {\"r\" : %d, \"g\" : %d, \"b\" : %d, \"p\" : %d}\r\n}\r\n", r,g,b,pattern);
    /* This one is more accurate and should be used over the above next debug session */
    /*
    sprintf(buf, "\"led\" : {\"name\" : \"%s\", \"r\" : %d, \"g\" : %d, \"b\" : %d, \"p\" : %d}\r\n}\r\n",
                             _gpioconfig_.lpd8806cfg[index].name,
                             _gpioconfig_.lpd8806cfg[index].red,
                             _gpioconfig_.lpd8806cfg[index].green,
                             _gpioconfig_.lpd8806cfg[index].blue,
                             _gpioconfig_.lpd8806cfg[index].pattern);*/
	  send(client, buf, strlen(buf), 0);
  }
}

void serve_gpio_request(int client, char *query)
{
  char buf[1024];
  int numchars = 1;
  int i;
  //const char delim[2] = ";";
  char *token;
  char *split;
  int action = -1;
  int pin = -1;
  int status = -1;
    
  buf[0] = 'A'; buf[1] = '\0';
  while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
  {
    numchars = get_line(client, buf, sizeof(buf));
  }
  
  headers(client, ".json");
  
  token = strtok(query, "&");
  while( token != NULL ) {
	split=strchr(token,'=');
	switch(token[0]) {
	  case 'a':
	    action = atoi(split+1);
	  break;
	  case 'p':
	    pin = atoi(split+1);
	  break;
	  case 's':
	    status = atoi(split+1);
	  break;
	} 
    token = strtok(NULL, "&");
  }
  
  logMessage (LOG_DEBUG,"Received query as Action=%d Pin=%d, status=%d\n",action,pin,status);
  
  if(action == 0) { // Maybe add title to the config in the future, for now just set to name
    sprintf(buf, "{ \"title\" : \"%s\",\r\n\"name\" : \"%s\",\r\n", _gpioconfig_.name, _gpioconfig_.name);
	  send(client, buf, strlen(buf), 0);
    //for (i=0; _gpioconfig_.gpiocfg[i].pin > -1 ; i++)
    for (i=0; i < _gpioconfig_.pinscfgs ; i++)
    {
      sprintf(buf, "\"pin%d\" : {\"name\" : \"%s\", \"pin\" : %d, \"status\" : %d, \"inputoutput\" : %d}%s\r\n", 
                   _gpioconfig_.gpiocfg[i].pin, _gpioconfig_.gpiocfg[i].name, _gpioconfig_.gpiocfg[i].pin,  digitalRead(_gpioconfig_.gpiocfg[i].pin), 
                   _gpioconfig_.gpiocfg[i].input_output, (_gpioconfig_.gpiocfg[i+1].pin != -1)?",":"");
	    send(client, buf, strlen(buf), 0);
    }
    // if W1
    for (i=0; i < _gpioconfig_.onewiredevices; i++)
    {
      if (readw1(&_gpioconfig_.onewcfg[i], buf) )
      {
        send(client, ",\r\n", 3, 0);
        send(client, buf, strlen(buf), 0);
      }
    }
    // if lpdled
    for (i=0; i < _gpioconfig_.lpd8806devices; i++)
    {
      sprintf(buf, "\"led\" : {\"name\" : \"%s\", \"r\" : %d, \"g\" : %d, \"b\" : %d, \"p\" : %d}\r\n}\r\n",
                             _gpioconfig_.lpd8806cfg[i].name,
                             _gpioconfig_.lpd8806cfg[i].red,
                             _gpioconfig_.lpd8806cfg[i].green,
                             _gpioconfig_.lpd8806cfg[i].blue,
                             _gpioconfig_.lpd8806cfg[i].pattern);
    }
    
    sprintf(buf, "}\r\n");
    send(client, buf, strlen(buf), 0);
  }
  else if(action == 1) {
	  sprintf(buf, "{\r\n\"pin%d\" : {\"pin\" : %d,\n\"status\" : %d}}\r\n", pin, pin, digitalRead(pin));
	  send(client, buf, strlen(buf), 0);
  }
  else if(action == 2) {
	  digitalWrite(pin, status);
    sprintf(buf, "{\r\n\"pin%d\" : {\"pin\" : %d,\n\"status\" : %d}}\r\n", pin, pin, digitalRead(pin));
	  send(client, buf, strlen(buf), 0);
  }

}

/**********************************************************************/
/* Send a regular file to the client.  Use headers, and report
* errors to client if they occur.
* Parameters: a pointer to a file structure produced from the socket
*              file descriptor
*             the name of the file to serve */
/**********************************************************************/
void serve_file(int client, const char *filename)
{
  FILE *resource = NULL;
  int numchars = 1;
  char buf[1024];

  buf[0] = 'A'; buf[1] = '\0';
  while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
    numchars = get_line(client, buf, sizeof(buf));

  resource = fopen(filename, "rb");
  if (resource == NULL) {
    not_found(client);
  } else {
    headers(client, filename);
    cat(client, resource);
  }
  fclose(resource);
}

/**********************************************************************/
/* This function starts the process of listening for web connections
* on a specified port.  If the port is 0, then dynamically allocate a
* port and modify the original port variable to reflect the actual
* port.
* Parameters: pointer to variable containing the port to connect on
* Returns: the socket */
/**********************************************************************/
int startup(u_short *port)
{
  int httpd = 0;
  struct sockaddr_in name;

  httpd = socket(PF_INET, SOCK_STREAM, 0);
  if (httpd == -1)
  {
    displayLastSystemError ("'socket' failure");
    return(EXIT_FAILURE);
  }
  
  int opt=1;    /* option is to be on/TRUE or off/FALSE */
  setsockopt(httpd,SOL_SOCKET,SO_REUSEADDR,(char *)&opt,sizeof(opt));	// Don't care if this fails, it's really only for restarting probram quickly

 
  memset(&name, 0, sizeof(name));
  name.sin_family = AF_INET;
  name.sin_port = htons(*port);
  name.sin_addr.s_addr = htonl(INADDR_ANY);

 
  int i=0;
  while (bind(httpd, (struct sockaddr *)&name, sizeof(name)) < 0)
  //if (bind(httpd, (struct sockaddr *)&name, sizeof(name)) < 0)
  {
    if (i < 5) {
      //displayLastSystemError ("'bind' failure, waiting!");
      logMessage (LOG_WARNING, "'bind failure, waiting!");
      sleep(2);
    } else {
      displayLastSystemError ("'bind' failure, giving up!");
      return(EXIT_FAILURE);
    }
    i++;
  }
 
  if (*port == 0)  /* if dynamically allocating a port */
  {
    int namelen = sizeof(name);
    if (getsockname(httpd, (struct sockaddr *)&name, (socklen_t *)&namelen) == -1)
    {
      displayLastSystemError ("'socketname' failure");
      return(EXIT_FAILURE);
    }
      
    *port = ntohs(name.sin_port);
  }
 
  if (listen(httpd, 5) < 0)
  {
    displayLastSystemError ("'listen' failure");
    return(EXIT_FAILURE);
  }

logMessage (LOG_DEBUG, "returning()");
  
  return(httpd);
}

/**********************************************************************/
/* Inform the client that the requested web method has not been
* implemented.
* Parameter: the client socket */
/**********************************************************************/
void unimplemented(int client)
{
  char buf[1024];

  sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, SERVER_STRING);
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "Content-Type: text/html\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "</TITLE></HEAD>\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
  send(client, buf, strlen(buf), 0);
  sprintf(buf, "</BODY></HTML>\r\n");
  send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/*
int main(void)
{
  int server_sock = -1;
  u_short port = 80;
  int client_sock = -1;
  struct sockaddr_in client_name;
  int client_name_len = sizeof(client_name);

  server_sock = startup(&port);
  printf("httpd running on port %d\n", port);

  while (1)
  {
    client_sock = accept(server_sock,(struct sockaddr *)&client_name,(socklen_t *)&client_name_len);
    if (client_sock == -1)
      error_die("accept");
    
    accept_request(client_sock);
  }

  close(server_sock);

  return(0);
}
*/




/**********************************************************************/
/* Execute a CGI script.  Will need to set environment variables as
* appropriate.
* Parameters: client socket descriptor
*             path to the CGI script */
/**********************************************************************/
void execute_cgi(int client, const char *path,const char *method, const char *query_string)
{
  char buf[1024];
  int cgi_output[2];
  int cgi_input[2];
  pid_t pid;
  int status;
  int i;
  char c;
  int numchars = 1;
  int content_length = -1;

  buf[0] = 'A'; buf[1] = '\0';
  
  if (strcasecmp(method, "GET") == 0) 
  {
    while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
      numchars = get_line(client, buf, sizeof(buf));
  }
  else    /* POST */
  {
    numchars = get_line(client, buf, sizeof(buf));
    while ((numchars > 0) && strcmp("\n", buf))
    {
      buf[15] = '\0';
      if (strcasecmp(buf, "Content-Length:") == 0)
        content_length = atoi(&(buf[16]));
        
      numchars = get_line(client, buf, sizeof(buf));
    }
    if (content_length == -1) {
      bad_request(client);
      return;
    }
  }

  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  send(client, buf, strlen(buf), 0);

  if (pipe(cgi_output) < 0) {
    cannot_execute(client);
    return;
  }
  if (pipe(cgi_input) < 0) {
    cannot_execute(client);
    return;
  }

  if ( (pid = fork()) < 0 ) {
    cannot_execute(client);
    return;
  }
  if (pid == 0)  /* child: CGI script */
  {
    char meth_env[255];
    char query_env[255];
    char length_env[255];

    dup2(cgi_output[1], 1);
    dup2(cgi_input[0], 0);
    close(cgi_output[0]);
    close(cgi_input[1]);
    sprintf(meth_env, "REQUEST_METHOD=%s", method);
    putenv(meth_env);
    if (strcasecmp(method, "GET") == 0) {
      sprintf(query_env, "QUERY_STRING=%s", query_string);
      putenv(query_env);
    }
    else {   /* POST */
      sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
      putenv(length_env);
    }
    execl(path, path, NULL);
    exit(0);
  } else {    /* parent */
    close(cgi_output[1]);
    close(cgi_input[0]);
    if (strcasecmp(method, "POST") == 0)
    for (i = 0; i < content_length; i++) {
      recv(client, &c, 1, 0);
      write(cgi_input[1], &c, 1);
    }
    while (read(cgi_output[0], &c, 1) > 0)
    send(client, &c, 1, 0);

    close(cgi_output[0]);
    close(cgi_input[1]);
    waitpid(pid, &status, 0);
  }
}
