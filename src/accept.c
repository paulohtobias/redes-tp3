#include "accept.h"

int mpw_accept(int sfd){
	
	struct sockaddr_in cliente_addr;
	socklen_t len = sizeof cliente_addr;
	mpw_segmento_t segmento;
	// Leitura dos dados do socket principal.
    int retval = recvfrom(sfd, &segmento, MPW_MAX_SS, 0, (struct sockaddr *) &cliente_addr, &len);
    // Se falhou a leitura.
    if (retval == -1) {
        return retval;
    }
    if (!INICIAR_CONEXAO (segmento.cabecalho.flags)){
        return -1;
    }
    
}