#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>

#include <pthread.h>

#include <netinet/in.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#define handle_error(cod, msg)\
	fprintf(stderr, "%3d: ", errno); perror(msg); if (cod) {exit(cod);}

#define false 0
#define true 1
typedef char bool;

extern int gquiet;

// Funções
	// Realiza a verificação do RTT, 
	// 0 se sucesso, erro caso contrário
	int mpw_rtt(pthread_cond_t *cond, pthread_mutex_t *mutex, int tempo_ms);

#endif //UTILS_H
