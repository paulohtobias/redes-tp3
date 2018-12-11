#include "enviar.h"

void __init_enviar(){
	
}

int enviar(int sockfd, void *dados, size_t tamanho) {
	//mpw_cabecalho_t ack;
	mpw_segmento_t pacote = (mpw_segmento_t){0};
	mpw_conexao_t *conexao = &gconexoes[sockfd];
	int indx = 0;
	enum MPW_FLAGS seq_num = SEQ_1;
	int bytes_escritos = 0;

	pacote.cabecalho.socket = conexao->id;
	pacote.cabecalho.ip_origem = conexao->ip_origem;
	pacote.cabecalho.porta_origem = conexao->porta_origem;
	
	if(!gquiet){printf("Antes do while\n");}
	while(tamanho > 0){
		// Inicialização do pacote
		int tamanho_pacote;

		if(tamanho >= MPW_MAX_DADOS){
			tamanho_pacote = MPW_MAX_DADOS;
		}else{
			tamanho_pacote = tamanho;
			seq_num = 0;
		}

		if(!gquiet){printf("Definindo o pacote\n");}
		pacote.cabecalho.tamanho_dados = tamanho_pacote;
		pacote.cabecalho.flags = seq_num;

		// Adiciona os dados ao pacote
		memcpy(pacote.dados, dados + indx, tamanho_pacote);
		indx += tamanho_pacote;

		// Realiza o envio do pacote
		if(!gquiet){printf("Enviando o pacote\n");}
		__mpw_write(&pacote);

		// Esperando por um ACK
		
		// Realiza a tentativa de enviar o dado
		pthread_mutex_lock(&(conexao->mutex));
		while (!conexao->tem_dado) {
			if(!gquiet){printf("Inicio loop\n");}
			int retval = mpw_rtt(&conexao->cond, &conexao->mutex, gestimated_rtt);

			// Se estourar o temporizador.
			if (retval == ETIMEDOUT) {
				if(!gquiet){printf("Olha o timeout\n");}
				conexao->tem_dado = 0;
				__mpw_write(&pacote);
			} else {
				// Se os dados chegaram normalmente.
				// SEQ_1 >> 2 == ACK_1; SEQ_2 >> 2 == ACK_2
				// && segmento_valido(&conexao->segmento, seq_num >> 2
				if (retval == 0) {
					if(!gquiet){printf("Segmento valido\n");}
					break;
				} else {
					conexao->tem_dado = 0;
					__mpw_write(&pacote);
				}
			}
		}
		conexao->tem_dado = 0;
		pthread_mutex_unlock(&conexao->mutex);

		seq_num = (SEQ_1 | SEQ_2) - seq_num;
		tamanho -= tamanho_pacote;
		bytes_escritos += tamanho_pacote;
	}

	if(!gquiet){printf("Finalizou o envio\n");}
	return bytes_escritos;
}

