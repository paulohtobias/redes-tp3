#ifndef CONEXAO_H
#define CONEXAO_H

#include "nucleo.h"

typedef struct mpw_conexao_t {
	int id;
	uint8_t tem_dado;
	ssize_t bytes_lidos;
	size_t offset;
	mpw_segmento_t segmento;

	bool ativo;

	pthread_mutex_t mutex;
	pthread_cond_t cond;
} mpw_conexao_t;

/// Variáveis globais
size_t max_conexoes;
mpw_conexao_t *gconexoes;
unsigned int estimated_rtt;

/// Funções
void init_conexao();

int mpw_accept(int sfd);

int mpw_connect(int sfd, const struct sockaddr *addr, socklen_t addrlen);

void enviar_ack(int sfd, mpw_cabecalho_t cabecalho, int ack);

void __mpw_write(int sfd, mpw_segmento_t *segmento);

void *__mpw_read(void *args);

#endif //CONEXAO_H
