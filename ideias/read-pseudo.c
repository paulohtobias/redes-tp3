struct {
    uint8_t tem_dado;
    uint32_t ip_origem;
    uint16_t porta_origem;
    void *buffer;
    
    //AQUI
    pthead_cond_t cond;
    pthread_mutex_t mutex;
} soquete;

struct cabecalho {
    int socket;
	uint16_t flags;
    //vetor_dados;
}

//vetor global.
soquete soquetes[];

//thread separada.
le_principal(){
    Cabeçalho cabeçalho;
    recvfrom(sock_global, &cabecalho, &addr);

    int socket = cabecalho.socket;
    
    memcpy(soquetes[socket].buffer, cabecalho.dados);
    soquetes[socket].tem_dado = 1;
    cond_singal(soquetes[socket].cond);
}

void nosso_read(int socket, void *buffer, size_t len) {
    terminou = 0;
    while (!terminou) {
        while (!soquetes[socket].tem_dado) {
            ret = pthread_cond_wait_time(mutex, cond, tempo);
        }
        if (dados_validos(ret)) { //se não está corrompido, duplicado, perdido ou "perdido".
                                  // perdido: temporizador estourou.
                                  // "perdido": flag PERDIDO no cabecalho.
            //escreve na posição correta.
            memcpy(buffer + offset, soquetes[socket].buff);

            //enviar próximo ack
            
            terminou = verificar_fim();
        } else {
            //reenviar_ack
        }
        tem dado = 0;
    }
}

int main() {
    int socket_global = criar_socket_servidor();
    
    int socket_cliente = accept(socket_global);
    
    char buff[1024];
    while (1) {
        nosso_read(socket_cliente, buff);
    }
}