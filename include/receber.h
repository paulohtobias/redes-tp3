#ifndef RECEBER_H
#define RECEBER_H

#include "cabecalho.h"

typedef struct mpw_conexao_t {
	in_addr_t ip_origem;
	in_port_t porta_origem;
	uint8_t tem_dado;
	ssize_t bytes_lidos;
	size_t offset;
	mpw_segmento_t segmento;

	//contadores aqui? corrompidos, duplicados, perdidos...

	pthread_mutex_t mutex;
	pthread_cond_t cond;
} mpw_conexao_t;

void *__le_principal(void *args);

ssize_t receber(int fd, void *buffer, void **buffer_cru, size_t tamanho_maximo);

ssize_t ler(int fd, void *buffer, size_t tamanho_maximo);

#endif //RECEBER_H
