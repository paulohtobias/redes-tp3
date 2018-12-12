#include "enviar.h"

ssize_t enviar(int sockfd, void *dados, size_t tamanho) {
	mpw_segmento_t pacote = (mpw_segmento_t){0};
	mpw_conexao_t *conexao = &gconexoes[sockfd];
	int indx = 0;
	int seq_num = SEQ_1;
	int ack_esp = ACK_1;
	ssize_t bytes_escritos = 0;

	pacote.cabecalho.id = conexao->id;
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

		if(!gquiet){printf("Definindo o pacote: %d\n", tamanho_pacote);}
		pacote.cabecalho.tamanho_dados = tamanho_pacote;
		pacote.cabecalho.flags = seq_num;

		// Adiciona os dados ao pacote
		memcpy(pacote.dados, dados + indx, tamanho_pacote);
		indx += tamanho_pacote;

		// Realiza o envio do pacote
		if(!gquiet){printf("Enviando o pacote\nFlags: %d\n", pacote.cabecalho.flags);}
		pthread_mutex_lock(&(conexao->mutex));
		
		conexao->tem_dado = 0;
		__mpw_write(&pacote, true);
		// Realiza a tentativa de enviar o dado
		while (!conexao->tem_dado) {
			if(!gquiet){printf("Inicio loop %s\n", __func__);}
			int retval = mpw_rtt(
					&conexao->cond, 
					&conexao->mutex, 
					gestimated_rtt);

			// Se estourar o temporizador.
			if (retval == ETIMEDOUT) {
				if(!gquiet){printf("Timeout estourado.\n");}
				conexao->tem_dado = 0;
				__mpw_write(&pacote, true);
			} else {
				if(!conexao->tem_dado){
					continue;
				}
				conexao->tem_dado = 0;
				
				// Se os dados chegaram normalmente.				
				if (retval == 0 && 
					!segmento_corrompido(&conexao->segmento)
				){
					// Pedido de finalizacao de conexao
					if(CHECAR_FLAG_EXCLUSIVO(conexao->segmento, TERMINAR_CONEXAO)){
						conexao->tem_dado = 0;
						pthread_mutex_unlock(&conexao->mutex);
						if(!gquiet){printf("\033[0;34m############## TERMINEI A CONEXAO ##############\033[0m\n");}
						return bytes_escritos;
					}else 
					// Verifica se é um ACK válido	
					if(ack_esp == IS_ACK(conexao->segmento)){
						break;
					}else if(CHECAR_FLAG_EXCLUSIVO(conexao->segmento, BUFFER_CHEIO)){
						ack_esp = 1;
						conexao->tem_dado = 0;
					}else{
						conexao->tem_dado = 0;
						__mpw_write(&pacote, true);
					}
				}
				// Se chegou qualquer outro pacote não ACK esperado
				else {
					// Buffer no destinatário encheu, não é possível alocar mais
					if(!gquiet){
						printf("################################################\n");
						printf("############        Corrompido        ##########\n");
						printf("################################################\n");
					}
					conexao->tem_dado = 0;
					__mpw_write(&pacote, true);
				}
			}
		}
		conexao->tem_dado = 0;
		pthread_mutex_unlock(&conexao->mutex);

		seq_num = (SEQ_1 | SEQ_2) - seq_num;
		ack_esp = seq_num/SEQ_1;
		tamanho -= tamanho_pacote;
		bytes_escritos += tamanho_pacote;
	}

	if(!gquiet){printf("Finalizou o envio\n");}
	return bytes_escritos;
}

