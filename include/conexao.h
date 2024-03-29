#ifndef CONEXAO_H
#define CONEXAO_H

#include <signal.h>
#include "nucleo.h"

typedef struct mpw_conexao_t {
	/// Guarda o estado da conexão.
	enum {MPW_CONEXAO_INATIVA, MPW_CONEXAO_CONECTANDO, MPW_CONEXAO_ESTABELECIDA} estado;
	
	/// Atributos para identificar o remetente.
	int id;
	in_addr_t ip_origem;
	in_port_t porta_origem;

	/// Indica que há dados novos para serem lidos.
	uint8_t tem_dado;

	/// Guarda o offset dos dados durante a escrita e leitura.
	size_t offset;

	/// Segmento lido.
	mpw_segmento_t segmento;

	/// Sincronização de threads.
	pthread_mutex_t mutex;
	pthread_cond_t cond;
} mpw_conexao_t;

/// Variáveis globais
extern int __socket_real;
size_t max_conexoes;
mpw_conexao_t *gconexoes;
fila_t gfila_conexoes;

// Tempo em nanosegundos
unsigned int gestimated_rtt;

/// Funções
void init_conexoes();

int mpw_socket();

int mpw_bind(int sfd, struct sockaddr *server_addr, size_t len);

int mpw_accept(int sfd);

int mpw_connect(int sfd, const struct sockaddr *addr, socklen_t addrlen);

int mpw_close(int sfd);

void enviar_ack(mpw_segmento_t segmento, int ack);

void *__mpw_read(void *args);

typedef struct{bool inconsistencia;} __mpw_write_args;
#define __mpw_write(segmento, ...)\
	_v_mpw_write(segmento, (__mpw_write_args){__VA_ARGS__})

// Funções com parâmetros variados
void _v_mpw_write(mpw_segmento_t *segmento, __mpw_write_args in);

#endif //CONEXAO_H
