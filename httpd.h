#ifndef HTTPD_H_
#define HTTPD_H_

#define HTTPD_FAILURE -1

int startup(u_short *port);
void accept_request(int client);

#endif /* HTTPD_H_ */
