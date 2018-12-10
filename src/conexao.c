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

	// Remove um pedido de conexão da fila de conexões. É bloqueante.
	remover_fila(&gfila_conexoes, &segmento);

	// Procura por um socket fd válido.
	int sfd_cliente;
	for (sfd_cliente = 0; sfd_cliente < max_conexoes && !gconexoes[sfd_cliente].ativo; sfd_cliente++);

	if (sfd_cliente == max_conexoes) {
		return -1;
	}

	// Adiciona a nova conexão no vetor de conexões.
	mpw_conexao_t *conexao = &gconexoes[sfd_cliente];
	conexao->ativo = true;
	conexao->bytes_lidos = 0;
	conexao->offset = 0;
	conexao->tem_dado = 0;
	conexao->id = segmento.cabecalho.socket;

	// Confirma a conexão.
	segmento.cabecalho.flags = (C_INIT | ACK_1);
	segmento.cabecalho.socket = sfd_cliente;
	pthread_mutex_lock(&conexao->mutex);
	__mpw_write(sfd, &segmento);

	// Espera confirmação do cliente.
	int tentativas = 3;
	while (!conexao->tem_dado) {
		retval = pthread_cond_timedwait(&conexao->cond, &conexao->mutex, estimated_rtt);

		// Se estourar o temporizador.
		if (retval == ETIMEDOUT) {
			tentativas--;

			// Marca que não há dados.
			conexao->tem_dado = 0;
		} else {
			// Verifica se houve um despertar falso da thread.
			if (!conexao->tem_dado) {
				continue;
			}

			// Se os dados chegaram normalmente.
			if (retval == 0 && segmento_valido(&conexao->segmento, ACK_2) && CONEXAO_CONFIRMADA(conexao->segmento.cabecalho.flags)) {
				break;
			} else {
				tentativas--;

				// Marca que não há dados.
				conexao->tem_dado = 0;
			}
		}

		// As tentativas acabaram e não foi possível estabelecer a conexão.
		if (tentativas == 0) {
			conexao->ativo = false;
			sfd_cliente = -1;
			break;
		} else {
			// Reenvia o ack.
			__mpw_write(sfd, &segmento);
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
	conexao->bytes_lidos = 0;
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
			if (retval == 0 && segmento_valido(&conexao->segmento, ACK_1) && ACEITOU_CONEXAO(conexao->segmento.cabecalho.flags)) {
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
	segmento.cabecalho.flags = (C_INIT & ACK_2);
	__mpw_write(sfd, &segmento);

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
	ssize_t bytes_recebidos;
	unsigned int indice;

	while (1) {
		if (remover_fila(&gfila_mensagens, &segmento)) {
			// Copia os dados lidos para o segmento correto.
			indice = ntohl(segmento.cabecalho.socket);
			mpw_conexao_t *mensagem = &gconexoes[segmento.cabecalho.socket];
			memcpy(&mensagem->segmento, &segmento, sizeof segmento);
			mensagem->bytes_lidos = bytes_recebidos;

			// Avisa para a função de leitura que há novos dados.
			pthread_mutex_lock(&mensagem->mutex);
			mensagem->tem_dado = 1;
			pthread_cond_signal(&mensagem->cond);
			pthread_mutex_unlock(&mensagem->mutex);
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

		// Verifica o tipo da mensagem e insere na fila correta.
		if (INICIAR_CONEXAO(segmento.cabecalho.flags)) {
			//TODO: verificar se não é duplicata
			inserir_fila(&gfila_conexoes, &segmento);
		} else {
			inserir_fila(&gfila_mensagens, &segmento);
		}
	}

	return NULL;
}
