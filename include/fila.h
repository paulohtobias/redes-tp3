#ifndef FILA_H
#define FILA_H

#include "utils.h"

typedef struct fila_t {
	// Posição do primeiro elemento da fila.
	unsigned int inicio;

	// Quantidade de elementos na fila.
	unsigned int tamanho;

	// Quantidade máxima de elementos que a fila comporta.
	unsigned int tamanho_maximo;

	// Tamanho em bytes de um elemento.
	size_t qtd_bytes_elemento;

	// Buffer.
	void *elementos;

	// Define se a remoção da fila é bloqueante.
	bool bloqueante;

	// Variáveis para sincronização.
	pthread_mutex_t protetor;
	pthread_cond_t sinal;
} fila_t;

void iniciar_fila(fila_t *fila, size_t qtd_bytes_elemento, bool bloqueante);

bool inserir_fila(fila_t *fila, void *elemento);

bool remover_fila(fila_t *fila, void *elemento);

void destruir_fila(fila_t *fila);

#endif
