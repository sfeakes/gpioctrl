#ifndef _lpd8806WORKER_H_
#define _lpd8806WORKER_H_

//boolean lpd8806worker(struct LPD8806cfg *lpddevice, int pattern, int r, int g, int b);
boolean lpd8806worker(struct LPD8806cfg *lpddevice, uint8_t *pattern, uint8_t *r, uint8_t *g, uint8_t *b);
void lpd880led_cleanup();

#endif