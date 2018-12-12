#include "utils.h"

int gquiet = 0;

int mpw_rtt(pthread_cond_t *cond, pthread_mutex_t *mutex, int tempo_ms){
	struct timeval tv;
    struct timespec ts;

	// Pega o valor absoluto e salva na struct time
    gettimeofday(&tv, NULL);
    
    // Pegando o valor em segundos do timeInMS e adicionando
    ts.tv_sec = time(NULL) + tempo_ms / 1000;
    
    // Pegando o valor em microsegundos e transformando em nanossegundos
    // Pegando o valor restante do timeInMs e transformando em nanossegundos
    ts.tv_nsec = tv.tv_usec * 1000 + 1000 * 1000 * (tempo_ms % 1000);
    
    // Pegando o excesso de nanossegundos e transformando em segundos
    ts.tv_sec += ts.tv_nsec / (1000 * 1000 * 1000);
    
    // Pegando o valor restante
    ts.tv_nsec %= (1000 * 1000 * 1000);

    return pthread_cond_timedwait(cond, mutex, &ts);
}

void *carregar_arquivo(FILE *in, size_t *tamanho_arquivo) {
	if (in == NULL) {
		handle_error(0, "carregar_arquivo-abrir");
		*tamanho_arquivo = 0;
		return NULL;
	}
	uint8_t *dados = NULL;

	fseek(in, 0, SEEK_END);
	*tamanho_arquivo = ftell(in);
	rewind(in);
	dados = malloc(*tamanho_arquivo);
	if (dados == NULL) {
		handle_error(0, "carregar_arquivo-malloc");
		*tamanho_arquivo = 0;
		fclose(in);
		return NULL;
	}
	fread(dados, 1, *tamanho_arquivo, in);
	fclose(in);

	return dados;
}
