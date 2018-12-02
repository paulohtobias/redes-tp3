#include "receber.h"

/// Vetor global onde serão armazenadas as informações das conexões.
mpw_conexao_t gconexoes[MPW_MAX_CONEXOES] = {0};
mpw_segmento_t gsegmentos[100];

void *__le_principal(void *args) {
	int sfd = *((int *) args);
	mpw_segmento_t segmento;
	ssize_t bytes_recebidos;

	int indice;
	while (1) {
		bytes_recebidos = recvfrom(sfd, &segmento, sizeof segmento, 0, NULL, NULL);

		// Copia os dados lidos para o segmento correto.
		indice = ntohl(segmento.cabecalho.socket);
		mpw_conexao_t *conexao = &gconexoes[segmento.cabecalho.socket];
		memcpy(&conexao->segmento, &segmento, sizeof segmento);
		conexao->bytes_lidos += bytes_recebidos;

		// Avisa para a função de leitura que há novos dados.
		pthread_mutex_lock(&conexao->mutex);
		conexao->tem_dado = 1;
		pthread_cond_signal(&conexao->cond);
		pthread_mutex_unlock(&conexao->mutex);
	}
}

int segmento_corrigir_endianness(mpw_segmento_t *segmento) {
	segmento->cabecalho.socket = ntohl(segmento->cabecalho.socket);
	segmento->cabecalho.tamanho_dados = ntohs(segmento->cabecalho.tamanho_dados);
	segmento->cabecalho.checksum = ntohs(segmento->cabecalho.checksum);
	segmento->cabecalho.flags = ntohs(segmento->cabecalho.flags);
}

ssize_t receber(int fd, void *buffer, void **buffer_cru, size_t tamanho_maximo) {
	//to-do: error-checking
	mpw_conexao_t *conexao = &gconexoes[fd];

	size_t tamanho_segmento = sizeof (mpw_segmento_t);
	int seq_esperado = 1;
	int ack;
	conexao->offset = 0;
	conexao->bytes_lidos = 0;

	int terminou = 0;
	while (!terminou) {
		// Espera por novos segmentos.
		pthread_mutex_lock(&conexao->mutex);
		while (!conexao->tem_dado) {
			pthread_cond_wait(&conexao->cond, &conexao->mutex);
		}
		conexao->tem_dado = 0;
		pthread_mutex_unlock(&conexao->mutex);

		// Tem dados novos.
		if (buffer_cru != NULL) {
			//memcpy(raw_buffer[rbo], conexao->segmento.);
		}

		if (segmento_valido(&conexao->segmento, seq_esperado)) {
			memcpy(buffer + conexao->offset, &conexao->segmento, tamanho_segmento);
			conexao->offset += tamanho_segmento;

			ack = 3 - seq_esperado;

			if (GET_SEQ(conexao->segmento.cabecalho.flags) == -1) {
				terminou = 1;
			}
		} else {
			ack = seq_esperado;
		}

		enviar_ack(fd, conexao->segmento.cabecalho, ack);
	}
	return conexao->bytes_lidos;
}

ssize_t ler(int fd, void *buffer, size_t tamanho_maximo) {
	return receber(fd, buffer, NULL, tamanho_maximo);
}
