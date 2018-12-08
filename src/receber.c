#include "receber.h"

/// Vetor global onde serão armazenadas as informações das conexões.
size_t gconexoes_tamanho;
mpw_conexao_t *gconexoes;

unsigned int gsegmentos_inicio = 0;
unsigned int gsegmentos_fim = 0;
size_t gsegmentos_tamanho;
mpw_segmento_t *gsegmentos;

void __init_receber(){
	gconexoes = calloc(gconexoes_tamanho, sizeof(mpw_conexao_t));
	gsegmentos = calloc(gsegmentos_tamanho, sizeof(mpw_segmento_t));
}

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
		conexao->bytes_lidos = bytes_recebidos;

		// Avisa para a função de leitura que há novos dados.
		pthread_mutex_lock(&conexao->mutex);
		conexao->tem_dado = 1;
		pthread_cond_signal(&conexao->cond);
		pthread_mutex_unlock(&conexao->mutex);
	}
}

ssize_t receber(int fd, void *buffer, size_t buffer_tamanho, void **buffer_cru, size_t *buffer_cru_tamanho) {
	//to-do: error-checking
	mpw_conexao_t *conexao = &gconexoes[fd];

	size_t tamanho_segmento = sizeof (mpw_segmento_t);
	int seq_esperado = 1;
	int ack = 2;
	conexao->offset = 0;
	conexao->bytes_lidos = 0;
	ssize_t bytes_lidos_total = 0;
	size_t buffer_cru_offset = 0;

	int terminou = 0;
	while (!terminou) {
		// Espera por novos segmentos.
		pthread_mutex_lock(&conexao->mutex);
		while (!conexao->tem_dado) {
			pthread_cond_wait(&conexao->cond, &conexao->mutex);
		}
		conexao->tem_dado = 0;
		pthread_mutex_unlock(&conexao->mutex);

		// Verifica se a conexão foi fechada no meio da transmissão.
		if (TERMINAR_CONEXAO(conexao->segmento.cabecalho.flags)) {
			conexao->segmento.cabecalho.flags += ACK_1;

			__mpw_write(fd, &conexao->segmento);

			//to-do: retirar a conexão do vetor global de conexões.

			return 0;
		}

		// Tem dados novos.
		if (buffer_cru != NULL) {
			// Verifica se os novos bytes não extrapolam o buffer cru.
			if (*buffer_cru != NULL && buffer_cru_offset + conexao->bytes_lidos >= *buffer_cru_tamanho) {
				*buffer_cru_tamanho  = buffer_cru_offset + conexao->bytes_lidos + 1;
				*buffer_cru = realloc(*buffer_cru, buffer_cru_tamanho);

				// Verifica se o realloc falhou.
				if (*buffer_cru == NULL) {
					fprintf(stderr, "receber: não foi possível realocar buffer_cru.\n");
				}
			}
			
			// Copia os novos dados para o buffer.
			if (*buffer_cru != NULL) {
				memcpy(*buffer_cru + buffer_cru_offset, &conexao->segmento, conexao->bytes_lidos);
				buffer_cru_offset += conexao->bytes_lidos;
			}
		}

		if (segmento_valido(&conexao->segmento, seq_esperado)) {
			// Verifica se os novos bytes não extrapolam o buffer.
			if (bytes_lidos_total + conexao->bytes_lidos < buffer_tamanho) {
				// Copia os novos dados para o buffer.
				memcpy(buffer + conexao->offset, &conexao->segmento, conexao->bytes_lidos);

				conexao->offset += conexao->bytes_lidos;
				bytes_lidos_total += conexao->bytes_lidos;

				// Define o valor do ACK.
				ack = 3 - seq_esperado;

				// Verifica se todos os bytes foram enviados.
				if (GET_SEQ(conexao->segmento.cabecalho.flags) == -1) {
					terminou = 1;
				}
			} else {
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
	return receber(fd, buffer, tamanho_maximo, NULL, NULL);
}
