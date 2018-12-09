#include "fila.h"

void iniciar_fila(fila_t *fila, size_t qtd_bytes_elemento, bool bloqueante) {
	fila->inicio = 0;
	fila->tamanho = 0;
	fila->qtd_bytes_elemento = qtd_bytes_elemento;
	fila->elementos = malloc(fila->qtd_bytes_elemento * fila->qtd_bytes_elemento);
	fila->bloqueante = bloqueante;

	pthread_mutex_init(&fila->protetor, NULL);
	pthread_cond_init(&fila->sinal, NULL);		
}

bool inserir_fila(fila_t *fila, void *elemento) {
	
    bool inseriu = false;
	
	//! Inicio da região crítica
	pthread_mutex_lock(&fila->protetor);
    // Se ainda há espaço para inserir
    if(fila->tamanho < fila->tamanho_maximo){
        // Realiza a inserção do elemento
        memcpy(
            fila->elementos +
			(((fila->inicio + fila->tamanho) % 
			fila->tamanho_maximo) * fila->qtd_bytes_elemento), 
            elemento, 
            fila->qtd_bytes_elemento
        );
        
        // Se for bloqueante envia sinais ao consumidor
        if(fila->bloqueante){
            pthread_cond_signal(&fila->sinal);
        }
        inseriu = true;
        fila->tamanho++; // Aumenta o tamanho da fila
    }
	//! Fim da região crítica
	pthread_mutex_unlock(&fila->protetor);
	return inseriu;
}

bool remover_fila(fila_t *fila, void *elemento) {
    bool removeu = false;

	// Criando região crítica.
    pthread_mutex_lock(&fila->protetor);
	// Se a fila estiver vazia e for bloqueante.
	while (fila->tamanho == 0 && fila->bloqueante) {
		pthread_cond_wait(&fila->sinal, &fila->protetor);		
	}
	if (fila->tamanho > 0) {
		// Copiando objeto para parâmetro de retorno.
		memcpy (elemento, fila->elementos + (fila->inicio * fila->qtd_bytes_elemento),
			fila->qtd_bytes_elemento);

		// Se o ponteiro atingir o tamanho da fila.    
		if (fila->inicio + 1 == fila->tamanho_maximo) {
			// Retorne-o para o início.
			fila->inicio = 0;
		}else{ // Se  não estiver no limite do vetor.
			// Avance uma casa.
			fila->inicio++;
		}
        // Decremente o tamanho da fila.
        fila->tamanho--;

		removeu = true;
	}
	// Liberando região crítica.
	pthread_mutex_unlock(&fila->protetor);

	return removeu;
}

void destruir_fila(fila_t *fila) {
	pthread_mutex_destroy(&fila->protetor);
	pthread_cond_destroy(&fila->sinal);
    free(fila->elementos);
}

int fila_teste() {
	fila_t fila;
	fila.tamanho_maximo = 2;

	iniciar_fila(&fila, sizeof(int), false);

	for (int i = 0; i < 3; i++) {
		printf("%d: %d\n", i, inserir_fila(&fila, &i));		
	}

	for (int i = 0; i < 3; i++) {
		int j;
		bool r = remover_fila(&fila, &j);
		printf("%d: %d\n", j, r);
	}

	return 0;
}
