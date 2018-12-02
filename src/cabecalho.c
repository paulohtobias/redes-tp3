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

void segmento_corrigir_endianness(mpw_segmento_t *segmento, bool leitura) {
	uint32_t (*convert_long)(uint32_t) = htonl;
	uint32_t (*convert_short)(uint32_t) = htons;

	if (leitura) {
		convert_long = ntohl;
		convert_short = ntohs;
	}
	
	segmento->cabecalho.socket = convert_long(segmento->cabecalho.socket);
	segmento->cabecalho.ip_origem = convert_long(segmento->cabecalho.ip_origem);
	segmento->cabecalho.ip_origem = convert_short(segmento->cabecalho.porta_origem);
	segmento->cabecalho.flags = convert_short(segmento->cabecalho.flags);
	segmento->cabecalho.tamanho_dados = convert_short(segmento->cabecalho.tamanho_dados);
	segmento->cabecalho.checksum = convert_short(segmento->cabecalho.checksum);
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

	__mpw_write(sfd, &segmento);
}

void __mpw_write(int sfd, mpw_segmento_t *segmento) {
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

	segmento_corrigir_endianness(segmento, false);

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = segmento->cabecalho.ip_origem;
	addr.sin_port = segmento->cabecalho.porta_origem;

	sendto(sfd, segmento, sizeof *segmento, 0, (struct sockaddr *) &addr, sizeof addr);
}
