#include "enviar.h"
#include "receber.h"

int main(int argc, char *argv[]){
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
		char buffer[20];
		retval = recvfrom(sfd, buffer, sizeof buffer, 0, (struct sockaddr *) &cliente_addr, &len);
		printf("recebi %d bytes: '%s'\n", retval, buffer);
	} else {
		printf("Enviando '%s'...\n", argv[1]);
		retval = sendto(sfd, argv[1], strlen(argv[1]) + 1, 0, (struct sockaddr *) &server_addr, sizeof server_addr);
		printf("%d bytes escritos no socket\n", retval);
	}

	return 0;
}
