#include "mpw.h"
#include "opcoes.h"
#include "fila.h"
#include <arpa/inet.h>

int maina(int argc, char *argv[]) {
	srand(1);
	opcao_t opcoes[] = {
		OPCAO_INIT('R', tipo_int, &gestimated_rtt, "RTT=2000", "Estimated RTT em milissegundos"),
		OPCAO_INIT('C', tipo_double, &probabilidade_corromper, "PROB=0", "Probabilidade de corromper pacotes"),
		OPCAO_INIT('D', tipo_double, &probabilidade_descartar, "PROB=0", "Probabilidade de descartar pacotes"),
		OPCAO_INIT('A', tipo_double, &probabilidade_atrasar, "PROB=0", "Probabilidade de atrasar pacotes"),
		OPCAO_INIT('m', tipo_int, &max_conexoes, "MAX=3", "Número máximo de conexões simultâneas"),
		OPCAO_INIT('B', tipo_int, &gfila_conexoes.tamanho_maximo, "BACKLOG=5", "Quantidade de conexões enfileiradas"),
		OPCAO_INIT('b', tipo_int, &gfila_mensagens.tamanho_maximo, "MAX=100", "Tamanho do buffer para mensagens recebidas")
	};

	parse_args(argc, argv, opcoes, sizeof opcoes / sizeof(opcao_t));

	mpw_init();
	
	int sfd = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(8080);

	int retval = bind(sfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
	if (retval == -1) {
		handle_error(errno, "criar_socket_servidor-bind");
	}

	printf("sfd: %d\n", sfd);

	init_conexoes(sfd);

	//
	int csfd = mpw_accept(sfd);

	return 0;
}

int mainc(int argc, char *argv[]) {
	srand(1);
	opcao_t opcoes[] = {
		OPCAO_INIT('R', tipo_int, &gestimated_rtt, "RTT=2000", "Estimated RTT em milissegundos"),
		OPCAO_INIT('C', tipo_double, &probabilidade_corromper, "PROB=0", "Probabilidade de corromper pacotes"),
		OPCAO_INIT('D', tipo_double, &probabilidade_descartar, "PROB=0", "Probabilidade de descartar pacotes"),
		OPCAO_INIT('A', tipo_double, &probabilidade_atrasar, "PROB=0", "Probabilidade de atrasar pacotes"),
		OPCAO_INIT('m', tipo_int, &max_conexoes, "MAX=3", "Número máximo de conexões simultâneas"),
		OPCAO_INIT('B', tipo_int, &gfila_conexoes.tamanho_maximo, "BACKLOG=5", "Quantidade de conexões enfileiradas"),
		OPCAO_INIT('b', tipo_int, &gfila_mensagens.tamanho_maximo, "MAX=100", "Tamanho do buffer para mensagens recebidas")
	};

	parse_args(argc, argv, opcoes, sizeof opcoes / sizeof(opcao_t));

	mpw_init();
	int sfd = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	//inet_pton(AF_INET, "localhost", &server_addr.sin_addr.s_addr);
	server_addr.sin_port = htons(8080);

	init_conexoes(sfd);

	mpw_connect(sfd, (const struct sockaddr*)&server_addr, sizeof(struct sockaddr_in));

	return 0;
}

int main2(int argc, char *argv[]);

int main(int argc, char *argv[]) {
	//return main2(argc, argv);
	if (argc > 1) {
		return maina(argc, argv);
	} else {
		return mainc(argc, argv);
	}
}

int main2(int argc, char *argv[]){
	int retval;
	int sfd = socket(AF_INET, SOCK_DGRAM, 0);

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(2000);

	if (argc == 1) {
		retval = bind(sfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
		if (retval == -1) {
			handle_error(errno, "criar_socket_servidor-bind");
		}
		struct sockaddr_in cliente_addr;
		socklen_t len = sizeof cliente_addr;
		char buffer[40960];
		retval = recvfrom(sfd, buffer, sizeof buffer, 0, (struct sockaddr *) &cliente_addr, &len);
		printf("'%s'\n%d bytes\n", buffer, retval);
	} else {
		printf("Enviando '%s'...\n", argv[1]);
		retval = sendto(sfd, argv[1], strlen(argv[1]) + 1, 0, (struct sockaddr *) &server_addr, sizeof server_addr);
		printf("%d bytes escritos no socket\n", retval);
	}
	return 0;
}
