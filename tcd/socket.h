#ifndef TCD_SOCKET_H
#define TCD_SOCKET_H

typedef void (*PeriodicFunc)();

int     opensocket( char *hostname, int port );
int fgetsock( char* s, int size, int socket, PeriodicFunc func );
 
#endif
