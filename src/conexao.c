#include "conexao.h"

pthread_mutex_t mutex_conexoes = PTHREAD_MUTEX_INITIALIZER;

// Threads
pthread_t thread_processar_conexoes;
pthread_t thread_processar_mensagens;
pthread_t thread_read;

// Socket interno.
int __socket_real = -1;


void __mpw_cleanup();
void INThandler(int sig);

void *__processar_mensagens(void *args);

#define prints(segmento) \
	printf(\
		"\tsocket: %d\n"\
		"\tip origem: %d"\
		"\tporta %d | flags: %d\n"\
		"\ttamanho: %d | checksum: %d\n\n",\
		(segmento).cabecalho.id,\
		(segmento).cabecalho.ip_origem,\
		(segmento).cabecalho.porta_origem, (segmento).cabecalho.flags,\
		(segmento).cabecalho.tamanho_dados, (segmento).cabecalho.checksum\
	);

void init_conexoes() {
	int i;
	atexit(__mpw_cleanup);
	signal(SIGINT, INThandler);
	iniciar_fila(&gfila_conexoes, sizeof(mpw_conexao_t), true);
	gconexoes = calloc(max_conexoes, sizeof(mpw_conexao_t));
	for (i = 0; i < max_conexoes; i++) {
		pthread_mutex_init(& gconexoes[i].mutex, NULL);
		pthread_cond_init(& gconexoes[i].cond, NULL);
	}

	// Cria as threads operárias.
	pthread_create(&thread_read, NULL, __mpw_read, NULL);
}

int mpw_socket() {
	// Cria o socket real.
	if (__socket_real == -1) {
		// Cria um socket UDP.
		__socket_real = socket(AF_INET, SOCK_DGRAM, 0);
		if (__socket_real == -1) {
			handle_error(errno, "mpw_socket-socket");
		}
		init_conexoes();
	}

	// Procura um id vazio.
	int sfd;
	for (sfd = 0; sfd < max_conexoes; sfd++) {
		pthread_mutex_lock(&mutex_conexoes);
		if (gconexoes[sfd].estado == MPW_CONEXAO_INATIVA) {
			// Reseta a conexão.
			memset(&gconexoes[sfd], 0, sizeof(mpw_conexao_t));

			pthread_mutex_unlock(&mutex_conexoes);
			return sfd;
		}
		pthread_mutex_unlock(&mutex_conexoes);
	}

	return -1;
}

int mpw_bind(int sfd, struct sockaddr *server_addr, size_t len) {
	int enable = 1;
	int retval = setsockopt(__socket_real, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
	if (retval < 0) {
		handle_error(errno, "criar_socket_servidor-setsockopt(SO_REUSEADDR)");
	}

	return bind(__socket_real, (struct sockaddr *) server_addr, len);
}

int mpw_accept(int sfd) {
	int retval;
	mpw_conexao_t pedido_conexao;
	mpw_segmento_t *segmento = &pedido_conexao.segmento;

	do {
		// Remove um pedido de conexão da fila de conexões. É bloqueante.
		remover_fila(&gfila_conexoes, &pedido_conexao);
	
	// Repete enquanto houver problema com o segmento.
	} while (segmento_corrompido(segmento) || !CHECAR_FLAG_EXCLUSIVO(*segmento, INICIAR_CONEXAO));

	// Procura por um socket fd válido.
	int sfd_cliente = mpw_socket();
	
	if (sfd_cliente == max_conexoes) {
		return -1;
	}

	// Adiciona a nova conexão no vetor de conexões.
	mpw_conexao_t *conexao = &gconexoes[sfd_cliente];
	conexao->estado = MPW_CONEXAO_CONECTANDO;
	conexao->offset = 0;
	conexao->tem_dado = 0;
	conexao->id = segmento->cabecalho.id;
	conexao->ip_origem = pedido_conexao.ip_origem;
	conexao->porta_origem = pedido_conexao.porta_origem;
	
	pthread_mutex_unlock(&mutex_conexoes);

	if (!gquiet) {
		printf("accept: Enviando confirmação\n");
	}

	// Confirma a conexão.
	DEFINIR_FLAG(*segmento, ACEITOU_CONEXAO);
	segmento->cabecalho.id = sfd_cliente;
	segmento->cabecalho.ip_origem = pedido_conexao.ip_origem;
	segmento->cabecalho.porta_origem = pedido_conexao.porta_origem;
	pthread_mutex_lock(&conexao->mutex);
	__mpw_write(segmento);

	if (!gquiet) {
		printf("accept: esperando confirmação\n");
	}

	// Espera confirmação do cliente.
	while (!conexao->tem_dado) {
		retval = mpw_rtt(&conexao->cond, &conexao->mutex, gestimated_rtt);

		// Se estourar o temporizador.
		if (retval == ETIMEDOUT) {
			if (!gquiet) {
				printf("Estourou o temporizador.\n");
			}

			// Marca que não há dados.
			if (!gquiet) {
				printf("Marca que não há dados.\n");
			}
			conexao->tem_dado = 0;

			// Reenvia o ack.
			if (!gquiet) {
				printf("Reenvia o ack.\n");
			}
			__mpw_write(segmento);
		} else {
			// Verifica se houve um despertar falso da thread.
			if (!gquiet) {
				printf("Verifica se houve um despertar falso da thread.\n");
			}
			if (!conexao->tem_dado) {
				continue;
			}
			conexao->tem_dado = 0;

			// Se os dados chegaram normalmente.
			if (retval == 0 && !segmento_corrompido(segmento)) {
				if (!gquiet) {
					printf("Os dados chegaram normalmente. %d\n", conexao->segmento.cabecalho.flags);
				}
				if (CHECAR_FLAG_EXCLUSIVO(conexao->segmento, CONEXAO_CONFIRMADA)) {
					// Marca a conexão como estabelecida.
					if (!gquiet) {
						printf("Marca a conexão como estabelecida.\n");
					}
					conexao->estado = MPW_CONEXAO_ESTABELECIDA;
					break;
				}

				// Verifica se a conexão (remota) foi fechada prematuramente.
				if (!gquiet) {
					printf("Verifica se a conexão (remota) foi fechada prematuramente.\n");
				}
				if (CHECAR_FLAG(conexao->segmento, TERMINAR_CONEXAO)) {
					// Marca a conexão (local) como fechada.
					if (!gquiet) {
						printf("Marca a conexão (local) como fechada.\n");
					}
					conexao->estado = MPW_CONEXAO_INATIVA;
					sfd_cliente = -1;
					break;
				}
			} else {
				// Reenvia o ack.
				if (!gquiet) {
					printf("Reenvia o ack.\n");
				}
				__mpw_write(segmento);
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

	// Adiciona a conexão no vetor de conexões.
	pthread_mutex_lock(&mutex_conexoes);
	mpw_conexao_t *conexao = &gconexoes[sfd];
	conexao->estado = MPW_CONEXAO_CONECTANDO;
	conexao->tem_dado = 0;
	conexao->offset = 0;
	pthread_mutex_unlock(&mutex_conexoes);


	// Montando o cabeçalho para solicitar a conexão.
	segmento.cabecalho.id = sfd;
	segmento.cabecalho.ip_origem = ((struct sockaddr_in *) addr)->sin_addr.s_addr;
	segmento.cabecalho.porta_origem = ((struct sockaddr_in *) addr)->sin_port;
	segmento.cabecalho.flags = INICIAR_CONEXAO;
	segmento.cabecalho.tamanho_dados = 0;

	// Solicita a abertura de conexão.
	pthread_mutex_lock(&conexao->mutex);
	__mpw_write(&segmento);


	// Espera mensagem de conexão aceita.
	if(!gquiet){
		printf("Antes do while\n");
	}
	while (!conexao->tem_dado) {
		if(!gquiet){
			printf("Espera mensagem de conexão aceita.\n");
		}
		retval = mpw_rtt(&conexao->cond, &conexao->mutex, gestimated_rtt);

		// Se estourar o temporizador.
		if (retval == ETIMEDOUT) {

			// Marca que não há dados.
			conexao->tem_dado = 0;

			// Solicita novamente a abertura de conexão.
			__mpw_write(&segmento);
		} else {
			// Verifica se houve um despertar falso da thread.
			if (!conexao->tem_dado) {
				continue;
			}
			conexao->tem_dado = 0;

			// Se os dados chegaram normalmente.
			if (retval == 0 && !segmento_corrompido(&conexao->segmento)) {
				if (CHECAR_FLAG_EXCLUSIVO(conexao->segmento, ACEITOU_CONEXAO)) {
					if(!gquiet){
						printf("#### Os dados chegaram normalmente #####\n");
					}
					// Marca a conexão como ativa e define os atributos de multiplexação.
					pthread_mutex_lock(&mutex_conexoes);
					conexao->estado = MPW_CONEXAO_ESTABELECIDA;
					pthread_mutex_unlock(&mutex_conexoes);
					conexao->id = conexao->segmento.cabecalho.id;
					/*conexao->ip_origem = conexao->segmento.cabecalho.ip_origem;
					conexao->porta_origem = conexao->segmento.cabecalho.porta_origem;*/
					if(!gquiet){
						printf("Marca a conexão como ativa e define os atributos de multiplexação\n");
					}
					break;
				}

				// Verifica se a conexão (remota) foi fechada prematuramente.
				if (CHECAR_FLAG(conexao->segmento, TERMINAR_CONEXAO)) {
					// Marca a conexão (local) como fechada.
					conexao->estado = MPW_CONEXAO_INATIVA;
					
					pthread_mutex_unlock(&conexao->mutex);
					return -1;
				}
			} else {
				if(!gquiet){
					printf("Solicita novamente a abertura de conexão\n");
				}

				// Solicita novamente a abertura de conexão.
				__mpw_write(&segmento);
			}
		}
	}
	conexao->tem_dado = 0;
	pthread_mutex_unlock(&conexao->mutex);

	// Confirma a conexão
	//segmento.cabecalho.flags = (C_INIT & ACK_2);
	//__mpw_write(&segmento);

	if(!gquiet){
		printf("Finalizacao\n");
	}
	return 0;
}

void __mpw_cleanup() {
	int fd;
	for (fd = 0; fd < max_conexoes; fd++) {
		mpw_close(fd);
	}
}

void INThandler(int sig) {
	signal(sig, SIG_IGN);
	//TODO: iterar em todas as conexões abertas.
	int fd;
	for (fd = 0; fd < max_conexoes; fd++) {
		mpw_close(fd);
	}
	exit(0);
}

int mpw_close(int sfd) {
	if (sfd >= max_conexoes || gconexoes[sfd].estado == MPW_CONEXAO_INATIVA) {
		return -1;
	}
	if(!gquiet){
		printf("antes do %s lock\n", __func__);
	}
	pthread_mutex_lock(&mutex_conexoes);
	mpw_conexao_t *conexao = &gconexoes[sfd];
	conexao->estado = MPW_CONEXAO_INATIVA;
	mpw_segmento_t segmento;

	segmento.cabecalho.id = conexao->id;
	segmento.cabecalho.ip_origem = conexao->ip_origem;
	segmento.cabecalho.porta_origem = conexao->porta_origem;
	segmento.cabecalho.flags = TERMINAR_CONEXAO;
	segmento.cabecalho.tamanho_dados = 0;

	__mpw_write(&segmento);
	pthread_mutex_unlock(&mutex_conexoes);

	// Espera ACK de confirmação do fechamento da conexão.
	if(pthread_mutex_trylock(&conexao->mutex) == EBUSY){
		pthread_mutex_unlock(&conexao->mutex);
		pthread_mutex_lock(&conexao->mutex);
	}
	if (!gquiet) {
		printf("Travei regiao critica\n");
	}
	int retval;
	uint16_t heartbit = 1000;
	conexao->tem_dado = 0;
	while (!conexao->tem_dado && heartbit > 0) {
		retval = mpw_rtt(&conexao->cond, &conexao->mutex, gestimated_rtt);

		// Se estourou o temporizador.
		if (retval == ETIMEDOUT) {
			// Marca que não há dados.
			conexao->tem_dado = 0;

			// Reenvia o segmento.
			__mpw_write(&segmento);
		} else {
			// Verifica se houve um despertar falso da thread.
			if (!conexao->tem_dado) {
				continue;
			}

			// Se os dados chegaram normalmente.
			if (retval == 0 && !segmento_corrompido(&segmento) && CHECAR_FLAG(segmento, CONFIRMOU_TERMINO)) {
				break;
			} else {
				// Marca que não há dados.
				conexao->tem_dado = 0;

				// Reenvia o segmento.
				__mpw_write(&segmento);
			}
		}
		heartbit--;
	}

	pthread_mutex_unlock(&conexao->mutex);

	return 0;
}

void enviar_ack(mpw_segmento_t segmento, int ack) {
	segmento.cabecalho.flags = ack;

	__mpw_write(&segmento, true);
}

void _v_mpw_write(mpw_segmento_t *segmento, __mpw_write_args in) {
	// Calcular checksum
	bool inconsistencia = (in.inconsistencia)? true: false;
	segmento->cabecalho.checksum = calcular_checksum(segmento);
	
	// e:: Acrescenta a quantidade de pacotes corrompidos.
	qpacotes_enviados++;
	if(inconsistencia){
		// Calcula probabilidade de um pacote ser descartado (simula perda de
		// pacote por descarte de roteadores).
		if (rand() % 101 < probabilidade_descartar) {
			// e:: Acrescenta a quantidade de pacotes perdidos.
			qpacotes_perdidos++;
			return;
		}

		// Calcula probabilidade de um pacote ser corrompido durante o envio.
		if (rand() % 101 < probabilidade_corromper) {
			// e:: Acrescenta a quantidade de pacotes corrompidos.
			qpacotes_corrompidos++;
			segmento->cabecalho.checksum++;
		}

		// Calcula a probabilidade de um pacote demorar para chegar ao destino.
		// Utilizado para simular o estouro de temporizador.
		if (rand() % 101 < probabilidade_atrasar) {
			usleep(gestimated_rtt * 1000);
		}
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = segmento->cabecalho.ip_origem;
	addr.sin_port = segmento->cabecalho.porta_origem;

	mpw_segmento_t copia = *segmento;
	segmento_corrigir_endianness(&copia, false);

	sendto(__socket_real, &copia, sizeof copia, 0, (struct sockaddr *) &addr, sizeof addr);

	if (!gquiet) {
		printf("%lu: Enviando segmento para %s:%d\n", time(NULL), inet_ntoa(addr.sin_addr), addr.sin_port);
		prints(*segmento);
	}
}

void *__processar_mensagens(void *args) {
	mpw_conexao_t mensagem_conexao = *((mpw_conexao_t *) args);
	unsigned int indice;

	// Copia os dados lidos para o segmento correto.
	indice = ntohl(mensagem_conexao.segmento.cabecalho.id);
	mpw_conexao_t *conexao = &gconexoes[indice];
	pthread_mutex_lock(&conexao->mutex);
	memcpy(&conexao->segmento, &mensagem_conexao.segmento, sizeof mensagem_conexao.segmento);
	if (!segmento_corrompido(&conexao->segmento)) {
		conexao->ip_origem = mensagem_conexao.ip_origem;
		conexao->porta_origem = mensagem_conexao.porta_origem;
		
		// Verifica confirmação do accept.
		if (CHECAR_FLAG_EXCLUSIVO(conexao->segmento, ACEITOU_CONEXAO)) {
			// Enviar a confirmação da conexão.
			DEFINIR_FLAG(mensagem_conexao.segmento, CONEXAO_CONFIRMADA);
			mensagem_conexao.segmento.cabecalho.ip_origem = mensagem_conexao.ip_origem;
			mensagem_conexao.segmento.cabecalho.porta_origem = mensagem_conexao.porta_origem;
			__mpw_write(&mensagem_conexao.segmento);
		} else if (CHECAR_FLAG(conexao->segmento, TERMINAR_CONEXAO)) {
			// Enviar a confirmação do fechamento da conexão.
			DEFINIR_FLAG(mensagem_conexao.segmento, CONFIRMOU_TERMINO);
			mensagem_conexao.segmento.cabecalho.ip_origem = mensagem_conexao.ip_origem;
			mensagem_conexao.segmento.cabecalho.porta_origem = mensagem_conexao.porta_origem;
			
			// Marca a conexão como inativa.
			conexao->estado = MPW_CONEXAO_INATIVA;

			__mpw_write(&mensagem_conexao.segmento);
		}
	}


	// Avisa para a função de leitura que há novos dados.
	conexao->tem_dado = 1;
	pthread_cond_signal(&conexao->cond);
	pthread_mutex_unlock(&conexao->mutex);
	
	return NULL;
}

void *__mpw_read(void* args) {
	mpw_conexao_t conexao;
	struct sockaddr_in addr;
	socklen_t addr_len = sizeof addr;
	ssize_t bytes_recebidos;
	
	if (!gquiet) {
		printf("Thread leitura %d\n", __socket_real);
	}

	int i, id;
	while (1) {
		bytes_recebidos = recvfrom(__socket_real, &conexao.segmento, sizeof conexao.segmento, 0, (struct sockaddr *) &addr, &addr_len);

		if (!gquiet) {
			printf("%s: bytes recebidos: %ld\n", __FUNCTION__, bytes_recebidos);
		}

		if (bytes_recebidos == -1) {
			handle_error(0, "__mpw_read");
		} else {
			segmento_corrigir_endianness(&conexao.segmento, true);

			// Define ip e porta de origem do segmento.
			if (!segmento_corrompido(&conexao.segmento)) {
				conexao.ip_origem = addr.sin_addr.s_addr;
				conexao.porta_origem = addr.sin_port;
			}

			if (!gquiet) {
				printf("%lu: Segmento recebido de %s:%d\n", time(NULL), inet_ntoa(addr.sin_addr), conexao.segmento.cabecalho.porta_origem);
				prints(conexao.segmento);
			}

			// Verifica o tipo da mensagem e insere na fila correta.
			if (CHECAR_FLAG_EXCLUSIVO(conexao.segmento, INICIAR_CONEXAO)) {
				// Se a conexão já foi iniciada (e ainda não foi estabelecida), não a coloque na fila de conexões.
				//processar_conexoes();

				//TODO: APAGAR ISSO AQUI E CHAMAR A processar_conexões quando ela tiver pronta
				pthread_mutex_lock(&mutex_conexoes);
				id = conexao.segmento.cabecalho.id;
				for (i = 0; i < max_conexoes; i++) {
					if (gconexoes[i].estado == MPW_CONEXAO_CONECTANDO && 
						gconexoes[i].id == id && 
						gconexoes[i].ip_origem == conexao.segmento.cabecalho.ip_origem && 
						gconexoes[i].porta_origem == conexao.segmento.cabecalho.porta_origem) {
						
						pthread_mutex_unlock(&mutex_conexoes);
						continue;
					}
				}
				pthread_mutex_unlock(&mutex_conexoes);

				//TROCAR:
				inserir_fila(&gfila_conexoes, &conexao);
				//inserir_fila(&gfila_filanova_millas_AQUI, &conexao);
			} else {
				__processar_mensagens(&conexao);
			}
		}
	}

	return NULL;
}
