#include "mpw.h"
#include "opcoes.h"
#include "fila.h"

int main(int argc, char *argv[]) {
	opcao_t opcoes[] = {
		//OPCAO_INIT('c', tipo_int, &, "CONEXOES=100", "Número máximo de conexões simultâneas")
		OPCAO_INIT('R', tipo_int, &estimated_rtt, "RTT=1000000", "Estimated RTT em nanossegundos")
	};

	parse_args(argc, argv, opcoes, sizeof opcoes / sizeof(opcao_t));

	mpw_init();

	return 0;
}

int main2(int argc, char *argv[]){
	printf("%d\n", sizeof(mpw_cabecalho_t));
	printf("%d\n", sizeof(mpw_segmento_t));
	int retval;
	int sfd = socket(AF_INET, SOCK_DGRAM, 0);

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(2020);

	if (argc == 1) {
		retval = bind(sfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
		if (retval == -1) {
			handle_error(errno, "criar_socket_servidor-bind");
		}
		struct sockaddr_in cliente_addr;
		socklen_t len = sizeof cliente_addr;
		char buffer[40960];
		retval = recvfrom(sfd, buffer, sizeof buffer, 0, (struct sockaddr *) &cliente_addr, &len);
		printf("'%s'\n%d bytes", buffer, retval);
	} else {
		printf("Enviando '%s'...\n", argv[1]);
		retval = sendto(sfd, argv[1], strlen(argv[1]) + 1, 0, (struct sockaddr *) &server_addr, sizeof server_addr);
		printf("%d bytes escritos no socket\n", retval);
	}
	return 0;
}
