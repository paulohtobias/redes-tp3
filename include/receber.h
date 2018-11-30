#ifndef RECEBER_H
#define RECEBER_H

#include "cabecalho.h"

typedef struct mpw_conexao_t {
	in_addr_t ip_origem;
	in_port_t porta_origem;
	uint8_t tem_dado;
	uint8_t segmento;

	//contadores aqui? corrompidos, duplicados, perdidos...

	pthread_mutex_t mutex;
	pthread_cond_t cond;
} mpw_conexao_t;

void *__le_principal(void *args);

ssize_t receber(int fd, void *buffer, size_t tamanho_maximo);

#endif //RECEBER_H
