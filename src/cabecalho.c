#include "cabecalho.h"

uint16_t __soma_checksum(uint16_t op1, uint16_t op2) {
	//to-do
	return 0;
}

uint16_t calcular_checksum(const mpw_segmento_t *segmento) {
	//to-do
	return 0;
}

int segmento_valido(const mpw_segmento_t *segmento, int ack_esperado) {
	// Testa se o segmento está corrompido usando a soma de verificação.
	if (calcular_checksum(segmento) != segmento->cabecalho.checksum) {
		return 0;
	}

	int flags = segmento->cabecalho.flags;
	if (CORROMPIDO(flags) || ATRASADO(flags) || DESCARTADO(flags)) {
		return 0;
	}

	if (IS_ACK(segmento->cabecalho.flags) != ack_esperado) {
		return 0;
	}

	return 1;
}
