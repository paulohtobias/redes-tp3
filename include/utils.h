#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <pthread.h>

#include <netinet/in.h>
#include <netinet/udp.h>

#define handle_error(cod, msg)\
	fprintf(stderr, "%3d: ", errno); perror(msg); if (cod) {exit(cod);}

extern int gquiet;

#endif //UTILS_H
