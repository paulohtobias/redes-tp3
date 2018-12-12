#include "receber.h"

ssize_t receber(int fd, void *buffer, size_t buffer_tamanho, void **buffer_cru, size_t *buffer_cru_tamanho) {
	if (fd >= max_conexoes) {
		fprintf(stderr, "Socket inválido\n");
		return -1;
	}

	mpw_conexao_t *conexao = &gconexoes[fd];

	if (conexao->estado != MPW_CONEXAO_ESTABELECIDA) {
		fprintf(stderr, "Conexão não foi estabilecida.\n");
		return -1;
	}

	//size_t tamanho_segmento = sizeof (mpw_segmento_t);
	mpw_segmento_t segmento_ack;
	int seq_esperado = 1;
	int seq_recebido;
	int ack = ACK_1;
	conexao->offset = 0;
	ssize_t bytes_lidos_total = 0;
	size_t buffer_cru_offset = 0;

	// Inicializa o segmento para enviar os ACKS.
	segmento_ack.cabecalho.id = conexao->id;
	segmento_ack.cabecalho.ip_origem = conexao->ip_origem;
	segmento_ack.cabecalho.porta_origem = conexao->porta_origem;
	segmento_ack.cabecalho.tamanho_dados = 0;

	int terminou = 0;
	while (!terminou) {
		// Espera por novos segmentos.
		if (!gquiet) {
			printf("Espera por novos segmentos.\n");
		}
		pthread_mutex_lock(&conexao->mutex);
		while (!conexao->tem_dado) {
			pthread_cond_wait(&conexao->cond, &conexao->mutex);
		}
		conexao->tem_dado = 0;
		pthread_mutex_unlock(&conexao->mutex);

		// Verifica se a conexão foi fechada no meio da transmissão.
		if (!segmento_corrompido(&conexao->segmento) && CHECAR_FLAG(conexao->segmento, TERMINAR_CONEXAO)) {
		if (!gquiet) {
			printf("A conexão foi fechada no meio da transmissão.\n");
		}
			return 0;
		}

		// Tem dados novos.
		if (!gquiet) {
			printf("Tem dados novos.\n");
		}
		if (buffer_cru != NULL) {
			// Verifica se os novos bytes não extrapolam o buffer cru.
			if (*buffer_cru != NULL && buffer_cru_offset + conexao->segmento.cabecalho.tamanho_dados > *buffer_cru_tamanho) {
				if (!gquiet) {
					printf("Os novos bytes extrapolam o buffer cru.\n");
				}

				*buffer_cru_tamanho = buffer_cru_offset + conexao->segmento.cabecalho.tamanho_dados;
				*buffer_cru = realloc(*buffer_cru, *buffer_cru_tamanho);

				// Verifica se o realloc falhou.
				if (*buffer_cru == NULL) {
					if (!gquiet) {
						printf("realloc falhou.\n");
					}
					fprintf(stderr, "receber: não foi possível realocar buffer_cru.\n");
				}
			}

			// Copia os novos dados para o buffer.
			if (*buffer_cru != NULL) {
				if (!gquiet) {
					printf("Copia os novos dados para o buffer.\n");
				}

				memcpy(*buffer_cru + buffer_cru_offset, &conexao->segmento.dados, conexao->segmento.cabecalho.tamanho_dados);
				buffer_cru_offset += conexao->segmento.cabecalho.tamanho_dados;
			}
		}


		seq_recebido = GET_SEQ(conexao->segmento);
		if (!segmento_corrompido(&conexao->segmento) && (seq_recebido == seq_esperado || seq_recebido == -1)) {
			// Verifica se os novos bytes não extrapolam o buffer.
			if (!gquiet) {
				printf("Verifica se os novos bytes (%ld) não extrapolam o buffer (%ld).\n", bytes_lidos_total + conexao->segmento.cabecalho.tamanho_dados, buffer_tamanho);
			}
			if (bytes_lidos_total + conexao->segmento.cabecalho.tamanho_dados <= buffer_tamanho) {
				// Copia os novos dados para o buffer.
				if (!gquiet) {
					printf("Copia os novos dados para o buffer.\n");
				}
				memcpy(buffer + conexao->offset, &conexao->segmento.dados, conexao->segmento.cabecalho.tamanho_dados);

				conexao->offset += conexao->segmento.cabecalho.tamanho_dados;
				bytes_lidos_total += conexao->segmento.cabecalho.tamanho_dados;

				// Define o valor do ACK.
				ack = seq_esperado;
				seq_esperado = 3 - seq_esperado;
				if (!gquiet) {
					printf("Define o valor do ACK: %d\n.", ack);
				}

				// Verifica se todos os bytes foram enviados.
				if (seq_recebido == -1) {
					if (!gquiet) {
						printf("Todos os bytes foram enviados.\n");
					}
					terminou = 1;
				}
			} else {
				// Não cabem mais dados no buffer. Avisa para o remente que a recepção será encerrada.
				if (!gquiet) {
					printf("Não cabem mais dados no buffer. Avisa para o remente que a recepção será encerrada.\n");
				}
				ack = BUFFER_CHEIO;
				terminou = 1;
			}
		} else { // Se for um pacote não esperado.
			ack = 3 - seq_esperado;
			// e:: Acrescenta a quantidade de pacotes reenviados.
			qpacotes_reenviados++;
		}
		enviar_ack(segmento_ack, ack);
	}

	return bytes_lidos_total;
}

ssize_t ler(int fd, void *buffer, size_t tamanho_maximo) {
	return receber(fd, buffer, tamanho_maximo, NULL, NULL);
}
