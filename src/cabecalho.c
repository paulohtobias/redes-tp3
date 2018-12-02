#include "cabecalho.h"

int8_t probabilidade_corromper = 0;
int8_t probabilidade_descartar = 0;
int8_t probabilidade_atrasar = 0;

uint16_t __soma_checksum(uint16_t op1, uint16_t op2) {
	//to-do
	return 0;
}

uint16_t calcular_checksum(const mpw_segmento_t *segmento) {
	//to-do
	return 0;
}

int segmento_valido(const mpw_segmento_t *segmento, int seq_esperado) {
	// Testa se o segmento está corrompido usando a soma de verificação.
	if (calcular_checksum(segmento) != segmento->cabecalho.checksum) {
		return 0;
	}

	int flags = segmento->cabecalho.flags;
	if (GET_SEQ(segmento->cabecalho.flags) != seq_esperado) {
		return 0;
	}

	return 1;
}

void enviar_ack(int sfd, mpw_cabecalho_t cabecalho, int ack) {
	cabecalho.flags = ack;

	mpw_segmento_t segmento = (mpw_segmento_t){0};
	segmento.cabecalho = cabecalho;

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = cabecalho.ip_origem;
	addr.sin_port = cabecalho.porta_origem;

	__mpw_sendto(sfd, &segmento, (struct sockaddr *) &addr, sizeof addr);
}

void __mpw_sendto(int sfd, mpw_segmento_t *segmento, const struct sockaddr *dest_addr, socklen_t addrlen) {
	// Calcular checksum
	segmento->cabecalho.checksum = calcular_checksum(segmento);

	if (rand() % 101 < probabilidade_descartar) {
		return;
	}

	if (rand() % 101 < probabilidade_corromper) {
		segmento->cabecalho.checksum++;
	}

	if (rand() % 101 < probabilidade_atrasar) {
		sleep(estimated_rtt);
	}

	sendto(sfd, segmento, sizeof *segmento, 0, dest_addr, addrlen);
}
