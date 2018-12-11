#ifndef CONEXAO_H
#define CONEXAO_H

#include "nucleo.h"

typedef struct mpw_conexao_t {
	/// Guarda o estado da conexão.
	enum {MPW_CONEXAO_INATIVA, MPW_CONEXAO_CONECTANDO, MPW_CONEXAO_ESTABELECIDA} estado;
	
	/// Atributos para identificar o remetente.
	int id; //meu id do lado de lá. Uso pra me identificar ao mandar as mensagens. TODO: apagar
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
size_t max_conexoes;
mpw_conexao_t *gconexoes;
fila_t gfila_conexoes;
fila_t gfila_mensagens;

// Tempo em nanosegundos
unsigned int gestimated_rtt;

/// Funções
void init_conexoes(int sfd);

int mpw_accept(int sfd);

int mpw_connect(int sfd, const struct sockaddr *addr, socklen_t addrlen);

void enviar_ack(int sfd, mpw_cabecalho_t cabecalho, int ack);

void __mpw_write(int sfd, mpw_segmento_t *segmento);

void *__mpw_read(void *args);

#endif //CONEXAO_H
