#include "nucleo.h"

fila_t gfila_segmentos;
fila_t gfila_conexoes;

int8_t probabilidade_corromper = 0;
int8_t probabilidade_descartar = 0;
int8_t probabilidade_atrasar = 0;

typedef union pu32_t {
	uint32_t val;
	uint16_t b[2];
} pu32_t;

typedef union pu16_t {
	uint16_t val;
	uint8_t b[2];
} pu16_t;

uint16_t __soma_checksum(uint16_t op1, uint16_t op2) {
	uint32_t soma = op1 + op2;
	soma += (soma > (uint16_t) -1);
	return (uint16_t) soma;
}

uint16_t calcular_checksum(const mpw_segmento_t *segmento) {
	uint16_t checksum = 0;

	pu32_t sockfd;
	pu32_t ip_origem;

	sockfd.val = segmento->cabecalho.id;
	ip_origem.val = segmento->cabecalho.ip_origem;

	checksum = __soma_checksum(checksum, sockfd.b[0]);
	checksum = __soma_checksum(checksum, sockfd.b[1]);
	checksum = __soma_checksum(checksum, ip_origem.b[0]);
	checksum = __soma_checksum(checksum, ip_origem.b[1]);
	checksum = __soma_checksum(checksum, segmento->cabecalho.porta_origem);
	checksum = __soma_checksum(checksum, segmento->cabecalho.flags);
	checksum = __soma_checksum(checksum, segmento->cabecalho.tamanho_dados);

	int i;
	pu16_t dado;
	for (i = 0; i < segmento->cabecalho.tamanho_dados; i += 2) {
		dado.b[0] = segmento->dados[i];
		dado.b[1] = (i + 1 < segmento->cabecalho.tamanho_dados) ? segmento->dados[i + 1] : 0;
		checksum = __soma_checksum(checksum, dado.val);
	}

	// Iverte os bits.
	checksum = ~checksum;

	return checksum;
}

void segmento_corrigir_endianness(mpw_segmento_t *segmento, bool leitura) {
	uint32_t (*convert_long)(uint32_t) = htonl;
	uint16_t (*convert_short)(uint16_t) = htons;

	if (leitura) {
		convert_long = ntohl;
		convert_short = ntohs;
	}
	
	segmento->cabecalho.id = convert_long(segmento->cabecalho.id);
	segmento->cabecalho.ip_origem = convert_long(segmento->cabecalho.ip_origem);
	segmento->cabecalho.porta_origem = convert_short(segmento->cabecalho.porta_origem);
	segmento->cabecalho.flags = convert_short(segmento->cabecalho.flags);
	segmento->cabecalho.tamanho_dados = convert_short(segmento->cabecalho.tamanho_dados);
	segmento->cabecalho.checksum = convert_short(segmento->cabecalho.checksum);
}

int segmento_corrompido(const mpw_segmento_t *segmento) {
	return calcular_checksum(segmento) != segmento->cabecalho.checksum;
}
