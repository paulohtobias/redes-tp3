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
