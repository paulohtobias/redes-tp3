#include "receber.h"

void __init_receber(){
	// gconexoes = calloc(gconexoes_tamanho, sizeof(mpw_conexao_t));
	// gsegmentos = calloc(gsegmentos_tamanho, sizeof(mpw_segmento_t));
}

ssize_t receber(int fd, void *buffer, size_t buffer_tamanho, void **buffer_cru, size_t *buffer_cru_tamanho) {
	if (fd >= max_conexoes) {
		return -1;
	}

	mpw_conexao_t *conexao = &gconexoes[fd];

	if (conexao->estado != MPW_CONEXAO_ESTABELECIDA) {
		return -1;
	}

	//size_t tamanho_segmento = sizeof (mpw_segmento_t);
	int seq_esperado = 1;
	int ack = 2;
	conexao->offset = 0;
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
		if (CHECHAR_FLAG(conexao->segmento, TERMINAR_CONEXAO)) {
			conexao->segmento.cabecalho.flags += ACK_1;

			__mpw_write(fd, &conexao->segmento);

			//to-do: retirar a conexão do vetor global de conexões.
			//conexao->ativo = false;

			return 0;
		}

		// Tem dados novos.
		if (buffer_cru != NULL) {
			// Verifica se os novos bytes não extrapolam o buffer cru.
			if (*buffer_cru != NULL && buffer_cru_offset + conexao->segmento.cabecalho.tamanho_dados >= *buffer_cru_tamanho) {
				*buffer_cru_tamanho  = buffer_cru_offset + conexao->segmento.cabecalho.tamanho_dados + 1;
				*buffer_cru = realloc(*buffer_cru, *buffer_cru_tamanho);

				// Verifica se o realloc falhou.
				if (*buffer_cru == NULL) {
					fprintf(stderr, "receber: não foi possível realocar buffer_cru.\n");
				}
			}

			// Copia os novos dados para o buffer.
			if (*buffer_cru != NULL) {
				memcpy(*buffer_cru + buffer_cru_offset, &conexao->segmento, conexao->segmento.cabecalho.tamanho_dados);
				buffer_cru_offset += conexao->segmento.cabecalho.tamanho_dados;
			}
		}

		if (segmento_valido(&conexao->segmento, seq_esperado)) {
			// Verifica se os novos bytes não extrapolam o buffer.
			if (bytes_lidos_total + conexao->segmento.cabecalho.tamanho_dados < buffer_tamanho) {
				// Copia os novos dados para o buffer.
				memcpy(buffer + conexao->offset, &conexao->segmento, conexao->segmento.cabecalho.tamanho_dados);

				conexao->offset += conexao->segmento.cabecalho.tamanho_dados;
				bytes_lidos_total += conexao->segmento.cabecalho.tamanho_dados;

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
	return bytes_lidos_total;
}

ssize_t ler(int fd, void *buffer, size_t tamanho_maximo) {
	return receber(fd, buffer, tamanho_maximo, NULL, NULL);
}
