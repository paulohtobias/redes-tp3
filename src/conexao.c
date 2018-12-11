#include "conexao.h"

pthread_mutex_t mutex_conexoes = PTHREAD_MUTEX_INITIALIZER;

// Threads
pthread_t thread_processar_conexoes;
pthread_t thread_processar_mensagens;
pthread_t thread_read;

void *__processar_mensagens(void *args);

void init_conexoes(int sfd) {
	int i;
	iniciar_fila(&gfila_conexoes, sizeof(mpw_segmento_t), true);
	iniciar_fila(&gfila_mensagens, sizeof(mpw_segmento_t), true);
	gconexoes = calloc(max_conexoes, sizeof(mpw_conexao_t));
	for (i = 0; i < max_conexoes; i++) {
		pthread_mutex_init(& gconexoes[i].mutex, NULL);
		pthread_cond_init(& gconexoes[i].cond, NULL);
	}

	// Cria as threads operárias.
	//pthread_create(&thread_processar_conexoes, NULL, processar_conexoes, &sfd);
	pthread_create(&thread_processar_mensagens, NULL, __processar_mensagens, &sfd);
	pthread_create(&thread_read, NULL, __mpw_read, &sfd);
}

int mpw_accept(int sfd) {
	int retval;
	mpw_segmento_t segmento;

	do {
		// Remove um pedido de conexão da fila de conexões. É bloqueante.
		remover_fila(&gfila_conexoes, &segmento);
	
	// Repete enquanto houver problema com o segmento.
	} while (segmento_corrompido(&segmento) || !CHECHAR_FLAG(segmento, INICIAR_CONEXAO));

	// Procura por um socket fd válido.
	// TODO: otimizar este for
	pthread_mutex_lock(&mutex_conexoes);
	int sfd_cliente;
	for (sfd_cliente = 0; sfd_cliente < max_conexoes && gconexoes[sfd_cliente].estado != MPW_CONEXAO_INATIVA; sfd_cliente++);

	if (sfd_cliente == max_conexoes) {
		return -1;
	}

	// Adiciona a nova conexão no vetor de conexões.
	mpw_conexao_t *conexao = &gconexoes[sfd_cliente];
	conexao->estado = MPW_CONEXAO_CONECTANDO;
	conexao->offset = 0;
	conexao->tem_dado = 0;
	conexao->id = segmento.cabecalho.socket;
	conexao->ip_origem = segmento.cabecalho.ip_origem;
	conexao->porta_origem = segmento.cabecalho.porta_origem;
	
	pthread_mutex_unlock(&mutex_conexoes);

	// Confirma a conexão.
	DEFINIR_FLAG(segmento, ACEITOU_CONEXAO);
	segmento.cabecalho.socket = sfd_cliente;
	pthread_mutex_lock(&conexao->mutex);
	__mpw_write(sfd, &segmento);

	// Espera confirmação do cliente.
	//TODO: otimizar
	while (!conexao->tem_dado) {
		retval = mpw_rtt(&conexao->cond, &conexao->mutex, gestimated_rtt);

		// Se estourar o temporizador.
		if (retval == ETIMEDOUT) {

			// Marca que não há dados.
			conexao->tem_dado = 0;

			// Reenvia o ack.
			__mpw_write(sfd, &segmento);
		} else {
			// Verifica se houve um despertar falso da thread.
			if (!conexao->tem_dado) {
				continue;
			}

			// Se os dados chegaram normalmente.
			if (retval == 0 && !segmento_corrompido(&segmento) && CHECHAR_FLAG(conexao->segmento, CONEXAO_CONFIRMADA)) {
				break;
			} else {

				// Marca que não há dados.
				conexao->tem_dado = 0;

				// Reenvia o ack.
				__mpw_write(sfd, &segmento);
			}
		}
	}
	conexao->tem_dado = 0;
	pthread_mutex_unlock(&conexao->mutex);

	return sfd_cliente;
}

int mpw_connect(int sfd, const struct sockaddr *addr, socklen_t addrlen) {
	int retval;
	mpw_segmento_t segmento;

	// Procura por uma posição vazia na lista de conexões.
	// TODO: otimizar este for
	pthread_mutex_lock(&mutex_conexoes);
	int id;
	for (id = 0; id < max_conexoes && gconexoes[id].estado != MPW_CONEXAO_INATIVA; id++);

	if (id == max_conexoes) {
		return -1;
	}

	// Adiciona a conexão no vetor de conexões.
	mpw_conexao_t *conexao = &gconexoes[id];
	conexao->estado = MPW_CONEXAO_CONECTANDO;
	conexao->tem_dado = 0;
	conexao->offset = 0;

	pthread_mutex_unlock(&mutex_conexoes);

	// Montando o cabeçalho para solicitar a conexão.
	segmento.cabecalho.socket = id;
	segmento.cabecalho.ip_origem = ((struct sockaddr_in *) addr)->sin_addr.s_addr;
	segmento.cabecalho.porta_origem = ((struct sockaddr_in *) addr)->sin_port;
	segmento.cabecalho.flags = INICIAR_CONEXAO;
	segmento.cabecalho.tamanho_dados = 0;

	// Solicita a abertura de conexão.
	pthread_mutex_lock(&conexao->mutex);
	__mpw_write(sfd, &segmento);


	// Espera mensagem de conexão aceita.
	while (!conexao->tem_dado) {
		retval = mpw_rtt(&conexao->cond, &conexao->mutex, gestimated_rtt);

		// Se estourar o temporizador.
		if (retval == ETIMEDOUT) {

			// Marca que não há dados.
			conexao->tem_dado = 0;

			// Solicita novamente a abertura de conexão.
			__mpw_write(sfd, &segmento);
		} else {
			// Verifica se houve um despertar falso da thread.
			if (!conexao->tem_dado) {
				continue;
			}

			// Se os dados chegaram normalmente.
			if (retval == 0 && segmento_valido(&conexao->segmento, ACK_1) && CHECHAR_FLAG(conexao->segmento, ACEITOU_CONEXAO)) {
				// Marca a conexão como ativa e define os atributos de multiplexação.
				pthread_mutex_lock(&mutex_conexoes);
				conexao->estado = MPW_CONEXAO_ESTABELECIDA;
				pthread_mutex_unlock(&mutex_conexoes);
				conexao->id = conexao->segmento.cabecalho.socket;
				conexao->ip_origem = conexao->segmento.cabecalho.ip_origem;
				conexao->porta_origem = conexao->segmento.cabecalho.porta_origem;
				break;
			} else {
				// Marca que não há dados.
				conexao->tem_dado = 0;

				// Solicita novamente a abertura de conexão.
				__mpw_write(sfd, &segmento);
			}
		}
	}
	conexao->tem_dado = 0;
	pthread_mutex_unlock(&conexao->mutex);

	// Confirma a conexão
	//segmento.cabecalho.flags = (C_INIT & ACK_2);
	//__mpw_write(sfd, &segmento);

	return 0;
}

void enviar_ack(int sfd, mpw_cabecalho_t cabecalho, int ack) {
	cabecalho.flags = ack;

	mpw_segmento_t segmento = (mpw_segmento_t){0};
	segmento.cabecalho = cabecalho;

	__mpw_write(sfd, &segmento);
}

void __mpw_write(int sfd, mpw_segmento_t *segmento) {
	// Calcular checksum
	segmento->cabecalho.checksum = calcular_checksum(segmento);

	// Calcula probabilidade de um pacote ser descartado (simula perda de
	// pacote por descarte de roteadores).
	if (rand() % 101 < probabilidade_descartar) {
		return;
	}

	// Calcula probabilidade de um pacote ser corrompido durante o envio.
	if (rand() % 101 < probabilidade_corromper) {
		segmento->cabecalho.checksum++;
	}

	// Calcula a probabilidade de um pacote demorar para chegar ao destino.
	// Utilizado para simular o estouro de temporizador.
	if (rand() % 101 < probabilidade_atrasar) {
		sleep(gestimated_rtt);
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = segmento->cabecalho.ip_origem;
	addr.sin_port = segmento->cabecalho.porta_origem;

	segmento_corrigir_endianness(segmento, false);

	int bytes_escritos = sendto(sfd, segmento, sizeof *segmento, 0, (struct sockaddr *) &addr, sizeof addr);

	if (!gquiet) {
		printf("%s: bytes_escritos: %ld\n", __FUNCTION__, bytes_escritos);
	}
}

void *__processar_mensagens(void *args) {
	int sfd = *(int *) args;
	mpw_segmento_t segmento;
	unsigned int indice;

	while (1) {
		if (remover_fila(&gfila_mensagens, &segmento)) {

			// Copia os dados lidos para o segmento correto.
			indice = ntohl(segmento.cabecalho.socket);
			mpw_conexao_t *conexao = &gconexoes[indice];
			memcpy(&conexao->segmento, &segmento, sizeof segmento);

			// Verifica confirmação do accept.
			if (!segmento_corrompido(&conexao->segmento) && CHECHAR_FLAG(conexao->segmento, ACEITOU_CONEXAO)) {
				// Enviar a confirmação da conexão.
				DEFINIR_FLAG(segmento, CONEXAO_CONFIRMADA);
				__mpw_write(sfd, &segmento);
			}

			// Avisa para a função de leitura que há novos dados.
			pthread_mutex_lock(&conexao->mutex);
			conexao->tem_dado = 1;
			pthread_cond_signal(&conexao->cond);
			pthread_mutex_unlock(&conexao->mutex);
		}
	}
}

bool processar_conexoes(){
	//TODO: mover pra outra thread e otimizar a busca e a região crítica.
	pthread_mutex_lock(&mutex_conexoes);
	int i, id = 0;
	mpw_segmento_t segmento;
	//int i, id = segmento.cabecalho.socket;
	for (i = 0; i < max_conexoes; i++) {
		if (gconexoes[i].estado == MPW_CONEXAO_CONECTANDO && 
			gconexoes[i].id == id && 
			gconexoes[i].ip_origem == segmento.cabecalho.ip_origem && 
			gconexoes[i].porta_origem == segmento.cabecalho.porta_origem) {
			
			pthread_mutex_unlock(&mutex_conexoes);
			continue;
		}
	}
	pthread_mutex_unlock(&mutex_conexoes);
	return true;
}

void *__mpw_read(void* args) {
	int sfd = *(int *) args;
	mpw_segmento_t segmento;
	struct sockaddr_in addr;
	socklen_t addr_len = sizeof addr;
	ssize_t bytes_recebidos;
	
	if (!gquiet) {
		printf("Thread leitura %d\n", sfd);
	}

	char buffer[1024];

	int i, id;
	while (1) {
		bytes_recebidos = recvfrom(sfd, buffer, sizeof buffer, 0, (struct sockaddr *) &addr, &addr_len);
		//bytes_recebidos = recvfrom(sfd, &segmento, sizeof segmento, 0, (struct sockaddr *) &addr, &addr_len);

		if (!gquiet) {
			printf("%s: bytes recebidos: %ld\n", __FUNCTION__, bytes_recebidos);
		}

		return NULL;

		if (bytes_recebidos == -1) {
			handle_error(0, "__mpw_read");
		} else {
			// Define ip e porta de origem do segmento.
			segmento.cabecalho.ip_origem = addr.sin_addr.s_addr;
			segmento.cabecalho.porta_origem = addr.sin_port;

			// Verifica o tipo da mensagem e insere na fila correta.
			if (CHECHAR_FLAG(segmento, INICIAR_CONEXAO)) {
				// Se a conexão já foi iniciada (e ainda não foi estabelecida), não a coloque na fila de conexões.
				//processar_conexoes();

				//TODO: APAGAR ISSO AQUI E CHAMAR A processar_conexões quando ela tiver pronta
				pthread_mutex_lock(&mutex_conexoes);
				id = segmento.cabecalho.socket;
				for (i = 0; i < max_conexoes; i++) {
					if (gconexoes[i].estado == MPW_CONEXAO_CONECTANDO && 
						gconexoes[i].id == id && 
						gconexoes[i].ip_origem == segmento.cabecalho.ip_origem && 
						gconexoes[i].porta_origem == segmento.cabecalho.porta_origem) {
						
						pthread_mutex_unlock(&mutex_conexoes);
						continue;
					}
				}
				pthread_mutex_unlock(&mutex_conexoes);

				//TROCAR:
				inserir_fila(&gfila_conexoes, &segmento);
				//inserir_fila(&gfila_filanova_millas_AQUI, &segmento);
			} else {
				inserir_fila(&gfila_mensagens, &segmento);
			}
		}
	}

	return NULL;
}
