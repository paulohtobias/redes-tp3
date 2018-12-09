#include "conexao.h"

void init_conexao() {
	iniciar_fila(&gfila_conexoes, sizeof(mpw_segmento_t), true);
	iniciar_fila(&gfila_segmentos, sizeof(mpw_mensagem_t), true);
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

void *__mpw_read(void* args) {

	int sfd = *(int *) args;
	mpw_segmento_t segmento;
	ssize_t bytes_recebidos;

	int indice;
	while (1) {
		bytes_recebidos = recvfrom(sfd, &segmento, sizeof segmento, 0, NULL, NULL);

		
		// Identifica o tipo do segmento e insere na fila correspondente.
		if (INICIAR_CONEXAO(segmento.cabecalho.flags)) {
			inserir_fila(&gfila_conexoes, &segmento);
		}
		
		
		// Copia os dados lidos para o segmento correto.
		/*indice = ntohl(segmento.cabecalho.socket);
		mpw_mensagem_t *conexao = &gconexoes[segmento.cabecalho.socket];
		memcpy(&conexao->segmento, &segmento, sizeof segmento);
		conexao->bytes_lidos = bytes_recebidos;

		// Avisa para a função de leitura que há novos dados.
		pthread_mutex_lock(&conexao->mutex);
		conexao->tem_dado = 1;
		pthread_cond_signal(&conexao->cond);
		pthread_mutex_unlock(&conexao->mutex);
		*/
	}
	return NULL;
}
