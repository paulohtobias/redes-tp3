#include "opcoes.h"
#include "mpw.h"

int main(int argc, char *argv[]) {
	unsigned int semente = (unsigned) time(NULL);
	srand(semente);

	void *mensagem;
	size_t tamanho_mensagem = 0;
	char *nome_arquivo;
	char *endereco;
	in_port_t porta;

	qpacotes_corrompidos = 0;
	qpacotes_perdidos = 0;
	qpacotes_enviados = 0;
	qacks_corrompidos = 0;
	qpacotes_reenviados = 0;

	opcao_t opcoes[] = {
		OPCAO_INIT('q', tipo_bool, &gquiet, "0", "Suprime todas as mensagens da saída padrão"),
		OPCAO_INIT('R', tipo_int, &gestimated_rtt, "RTT=100", "Estimated RTT em milissegundos"),
		OPCAO_INIT('C', tipo_double, &probabilidade_corromper, "PROB=0", "Probabilidade de corromper pacotes"),
		OPCAO_INIT('D', tipo_double, &probabilidade_descartar, "PROB=0", "Probabilidade de descartar pacotes"),
		OPCAO_INIT('A', tipo_double, &probabilidade_atrasar, "PROB=0", "Probabilidade de atrasar pacotes"),
		OPCAO_INIT('m', tipo_int, &max_conexoes, "MAX=3", "Número máximo de conexões simultâneas"),
		OPCAO_INIT('B', tipo_int, &gfila_conexoes.tamanho_maximo, "BACKLOG=5", "Quantidade de conexões enfileiradas"),
		OPCAO_INIT('s', tipo_str(0), &mensagem, "MSG=", "Mensagem a ser enviada"),
		OPCAO_INIT('f', tipo_str(0), &nome_arquivo, "NOME=", "Nome do arquivo a ser enviado"),
		OPCAO_INIT('i', tipo_str(0), &endereco, "IP=0.0.0.0", "Endereço ip para enviar/receber mensagens"),
		OPCAO_INIT('p', tipo_int, &porta, "PORTA=9999", "Porta usada pelo processo"),
		OPCAO_INIT('t', tipo_int, &tamanho_mensagem, "TAM", "Tamanho da mensagem a ser enviada/recebida")
	};

	parse_args(argc, argv, opcoes, sizeof opcoes / sizeof(opcao_t));

	int retval;

	// Cria o socket mpw
	int sfd = mpw_socket();
	if (sfd == -1) {
		fprintf(stderr, "Erro ao criar um socket MPW.\n");
		return 1;
	}

	// Endereço para enviar/receber as mensagens.
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(porta);
	retval = inet_pton(AF_INET, endereco, &addr.sin_addr.s_addr);
	if (retval < 1) {
		if (retval == 0) {
			fprintf(stderr, "Erro ao converter endereço %s\n", endereco);
			return 1;
		}

		handle_error(errno, "inet_pton");
	}

	bool modo_enviar = false;

	// Definição da mensagem a ser enviada.
	if (strlen(nome_arquivo) > 0) {
		free(mensagem);

		// Carrega o arquivo para o buffer.
		FILE *in = fopen(nome_arquivo, "r");
		if (in == NULL) {
			handle_error(errno, "fopen");
		}
		tamanho_mensagem = fread(mensagem, 1, tamanho_mensagem, in);
		fclose(in);

		modo_enviar = true;
	} else {
		// Se o usuário passou uma string, então ele quer enviar arquivos.
		if (strlen(mensagem) > 0) {
			tamanho_mensagem = MIN(tamanho_mensagem, strlen(mensagem));
			((char *) mensagem)[tamanho_mensagem] = '\0';
			modo_enviar = true;
		}
	}

	if (modo_enviar) {
		// Se conecta ao endereço indicado.
		int retval = mpw_connect(sfd, (struct sockaddr *) &addr, sizeof(addr));
		if (retval == -1) {
			handle_error(errno, "criar_socket_servidor-bind");
		}

		// Envia a mensagem.
		retval = enviar(sfd, mensagem, tamanho_mensagem);
		if (retval == -1) {
			fprintf(stderr, "Erro ao enviar a mensagem\n");
			return 1;
		}
	}  else {
		// Faz o bind no endereço indicado.
		int retval = mpw_bind(sfd, (struct sockaddr *) &addr, sizeof(addr));
		if (retval == -1) {
			handle_error(errno, "criar_socket_servidor-bind");
		}

		// Escuta requisições no endereço indicado.
		retval = mpw_accept(sfd);
		if (retval == -1) {
			fprintf(stderr, "Não foi possível se conectar\n");
			return 1;
		}


		if (1 || !gquiet) {
			printf("tamanho: %lu\n", tamanho_mensagem);
		}

		
		// Recebe a mensagem.
		free(mensagem);
		size_t buffer_cru_tamanho = tamanho_mensagem;
		mensagem = malloc(tamanho_mensagem);
		void *buffer_cru = malloc(buffer_cru_tamanho);
		retval = receber(sfd, mensagem, tamanho_mensagem, &buffer_cru, &buffer_cru_tamanho);
		if (retval == -1) {
			fprintf(stderr, "Erro ao receber a mensagem\n");
			return 1;
		} else if (retval == 0) {
			fprintf(stderr, "Conexão foi fechada durante a leitura.\n");
		}

		// Escreve os dois buffers em disco.
		FILE *out = fopen("mensagem", "w");
		if (out == NULL) {
			handle_error(errno, "fopen(mensagem)");
		}
		fwrite(mensagem, 1, tamanho_mensagem, out);
		fclose(out);

		out = fopen("mensagem_crua", "w");
		if (out == NULL) {
			handle_error(errno, "fopen(mensagem)");
		}
		fwrite(buffer_cru, 1, buffer_cru_tamanho, out);
		fclose(out);

		printf(
			"Mensagem original: %lu bytes\n"
			"Total de dados recebidos: %lu bytes\n",
			tamanho_mensagem, buffer_cru_tamanho
		);
	}
	printf ("e:: %ld;%ld;%ld;%ld.\n", qpacotes_enviados, qpacotes_reenviados, qpacotes_perdidos, qpacotes_corrompidos);
	printf ("e:: Quantidade de pacotes enviados ao todo: %ld.\n", qpacotes_enviados);
	printf ("e:: Quantidade de pacotes reenviados ao todo: %ld.\n", qpacotes_reenviados);
	printf ("e:: Quantidade de pacotes perdidos: %ld.\n", qpacotes_perdidos);
	printf ("e:: Quantidade de pacotes corrompidos: %ld.\n", qpacotes_corrompidos);
	return 0;
}
