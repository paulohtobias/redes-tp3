#ifndef RECEBER_H
#define RECEBER_H

#include "cabecalho.h"

extern unsigned int gsegmentos_inicio;
extern unsigned int gsegmentos_fim;
extern size_t gconexoes_tamanho;
extern size_t gsegmentos_tamanho;

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

void __init_receber();

void *__le_principal(void *args);

/// se buffer_cru não for NULL, precisa ser *obrigatoriamente* alocado dinamicamente.
ssize_t receber(int fd, void *buffer, size_t buffer_tamanho, void **buffer_cru, size_t *buffer_cru_tamanho);

ssize_t ler(int fd, void *buffer, size_t tamanho_maximo);

#endif //RECEBER_H
