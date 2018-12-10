#include "conexao.h"

fila_t gfila_conexoes;
fila_t gfila_mensagens;

void init_conexao() {
	int i;
	iniciar_fila(&gfila_conexoes, sizeof(mpw_segmento_t), true);
	iniciar_fila(&gfila_mensagens, sizeof(mpw_segmento_t), true);
	gconexoes = calloc(max_conexoes, sizeof(mpw_conexao_t));
	for (i = 0; i < max_conexoes; i++) {
		pthread_mutex_init(& gconexoes[i].mutex, NULL);
		pthread_cond_init(& gconexoes[i].cond, NULL);
	}
}

int mpw_accept(int sfd) {
	int retval;
	struct sockaddr_in cliente_addr;
	socklen_t len = sizeof cliente_addr;
	mpw_segmento_t segmento;

	do {
		// Remove um pedido de conexão da fila de conexões. É bloqueante.
		remover_fila(&gfila_conexoes, &segmento);
	
	// Repete enquanto houver problema com o segmento.
	} while (segmento_corrompido(&segmento) || !CHECHAR_FLAG(segmento, INICIAR_CONEXAO));

	// Procura por um socket fd válido.
	int sfd_cliente;
	for (sfd_cliente = 0; sfd_cliente < max_conexoes && !gconexoes[sfd_cliente].ativo; sfd_cliente++);

	if (sfd_cliente == max_conexoes) {
		return -1;
	}

	// Adiciona a nova conexão no vetor de conexões.
	mpw_conexao_t *conexao = &gconexoes[sfd_cliente];
	conexao->ativo = true;
	conexao->offset = 0;
	conexao->tem_dado = 0;
	conexao->id = segmento.cabecalho.socket;

	// Confirma a conexão.
	DEFINIR_FLAG(segmento, ACEITOU_CONEXAO);
	segmento.cabecalho.socket = sfd_cliente;
	pthread_mutex_lock(&conexao->mutex);
	__mpw_write(sfd, &segmento);

	// Espera confirmação do cliente.
	//TODO: otimizar
	while (!conexao->tem_dado) {
		retval = pthread_cond_timedwait(&conexao->cond, &conexao->mutex, estimated_rtt);

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
	struct sockaddr_in servidor_addr;
	socklen_t len = sizeof servidor_addr;
	mpw_segmento_t segmento;

	// Procura por uma posição vazia na lista de conexões.
	int id;
	for (id = 0; id < max_conexoes && !gconexoes[id].ativo; id++);

	if (id == max_conexoes) {
		return -1;
	}

	// Adiciona a conexão no vetor de conexões.
	mpw_conexao_t *conexao = &gconexoes[id];
	conexao->ativo = true;
	conexao->offset = 0;
	conexao->tem_dado = 0;

	// Montando o cabeçalho para solicitar a conexão.
	segmento.cabecalho.socket = id;
	segmento.cabecalho.ip_origem = ((struct sockaddr_in *) addr)->sin_addr.s_addr;
	segmento.cabecalho.porta_origem = ((struct sockaddr_in *) addr)->sin_port;
	segmento.cabecalho.flags = C_INIT;
	segmento.cabecalho.tamanho_dados = 0;

	// Solicita a abertura de conexão.
	pthread_mutex_lock(&conexao->mutex);
	__mpw_write(sfd, &segmento);


	// Espera mensagem de conexão aceita.
	while (!conexao->tem_dado) {
		retval = pthread_cond_timedwait(&conexao->cond, &conexao->mutex, estimated_rtt);

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
		sleep(estimated_rtt);
	}

	segmento_corrigir_endianness(segmento, false);

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = segmento->cabecalho.ip_origem;
	addr.sin_port = segmento->cabecalho.porta_origem;

	sendto(sfd, segmento, sizeof *segmento, 0, (struct sockaddr *) &addr, sizeof addr);
}

void *__processar_mensagens(void *args) {
	int sfd = *(int *) args;
	mpw_segmento_t segmento;
	unsigned int indice;

	while (1) {
		if (remover_fila(&gfila_mensagens, &segmento)) {

			// Copia os dados lidos para o segmento correto.
			indice = ntohl(segmento.cabecalho.socket);
			mpw_conexao_t *conexao = &gconexoes[segmento.cabecalho.socket];
			memcpy(&conexao->segmento, &segmento, sizeof segmento);

			// Verifica confirmação do accept.
			if (!segmento_corrompido(&conexao->segmento) && CHECHAR_FLAG(conexao->segmento, ACEITOU_CONEXAO)) {
				// Seta o id da conexão certa.
				segmento.cabecalho.socket = conexao->id;

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

void *__mpw_read(void* args) {
	int sfd = *(int *) args;
	mpw_segmento_t segmento;
	ssize_t bytes_recebidos;

	int indice;
	while (1) {
		bytes_recebidos = recvfrom(sfd, &segmento, sizeof segmento, 0, NULL, NULL);

		if (bytes_recebidos == -1) {
			handle_error(0, "__mpw_read");
		} else {
			// Verifica o tipo da mensagem e insere na fila correta.
			if (CHECHAR_FLAG(segmento, INICIAR_CONEXAO)) {
				//TODO: verificar se não é duplicata
				inserir_fila(&gfila_conexoes, &segmento);
			} else {
				inserir_fila(&gfila_mensagens, &segmento);
			}
		}
	}

	return NULL;
}
