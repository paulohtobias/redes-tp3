#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>

pthread_mutex_t fakeMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t fakeCond = PTHREAD_COND_INITIALIZER;

void mywait(int timeInMs)
{
    struct timeval tv;
    struct timespec ts;

	// Pega o valor absoluto e salva na struct time
    gettimeofday(&tv, NULL);
    
    // Pegando o valor em segundos do timeInMS e adicionando
    ts.tv_sec = time(NULL) + timeInMs / 1000;
    
    // Pegando o valor em microsegundos e transformando em nanossegundos
    // Pegando o valor restante do timeInMs e transformando em nanossegundos
    ts.tv_nsec = tv.tv_usec * 1000 + 1000 * 1000 * (timeInMs % 1000);
    
    // Pegando o excesso de nanossegundos e transformando em segundos
    ts.tv_sec += ts.tv_nsec / (1000 * 1000 * 1000);
    
    // Pegando o valor restante
    ts.tv_nsec %= (1000 * 1000 * 1000);

    int n = pthread_cond_timedwait(&fakeCond, &fakeMutex, &ts);
    if (n == 0){
        printf("Acordei antes :(\n");
	}else{
  		printf("Acordado :D\n");
	}
}

void* fun(void* arg)
{
    printf("\nIn thread\n");
    mywait(10000);
}

int main()
{
    pthread_t thread;
    void *ret;

    pthread_create(&thread, NULL, fun, NULL);
    pthread_join(thread,&ret);
}

// https://stackoverflow.com/questions/1486833/pthread-cond-timedwait
