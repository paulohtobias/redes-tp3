#ifndef CONEXAO_H
#define CONEXAO_H

#include "nucleo.h"

typedef struct mpw_mensagem_t {
	in_addr_t ip_origem;
	in_port_t porta_origem;
	uint8_t tem_dado;
	ssize_t bytes_lidos;
	size_t offset;
	mpw_segmento_t segmento;

	pthread_mutex_t mutex;
	pthread_cond_t cond;
} mpw_mensagem_t;

/// Variáveis globais
fila_t gfila_conexoes;
fila_t gfila_segmentos;
unsigned int estimated_rtt;

void enviar_ack(int sfd, mpw_cabecalho_t cabecalho, int ack);

void __mpw_write(int sfd, mpw_segmento_t *segmento);

void *__mpw_read(void *args);

#endif //CONEXAO_H
