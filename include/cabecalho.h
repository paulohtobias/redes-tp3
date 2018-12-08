#ifndef CABECALHO_H
#define CABECALHO_H

#include "utils.h"

/// Variáveis para controle de erros.
extern int8_t probabilidade_corromper;
extern int8_t probabilidade_descartar;
extern int8_t probabilidade_atrasar;

/// Número máximo de conexões simultâneas.
#define MPW_MAX_CONEXOES 1024
/*
 * to-do: calcular este valor em relação ao tamanho máximo real de um
 * datagrama udp
 */
/// Tamanho máximo de um segmento (incluindo o cabeçalho).
#define MPW_MAX_SS 1024
#define MPW_MAX_DADOS (MPW_MAX_SS - sizeof(mpw_cabecalho_t))

enum MPW_FLAGS {
	ACK_1 = 0x1,
	ACK_2 = 0x2,
	SEQ_1 = 0x4,
	SEQ_2 = 0x8,
	C_INIT = 0x10,
	C_TERM = 0x20,
	D_BIT = 0x40
};

/**
 *  Estrutua do cabeçalho, RFC-2154:
 * 
 *  <----------- 32 bits ---------->
 * +--------------------------------+
 * |             socket             |
 * +--------------------------------+
 * |           ip origem            |
 * +----------------+---------------+
 * |      porta     |     flags     |
 * +----------------+---------------+
 * |     tamanho    |    checksum   |
 * +----------------+---------------+
 * 
 */

typedef struct mpw_cabecalho_t {
	int socket;
	in_addr_t ip_origem;
	in_port_t porta_origem;
	uint16_t flags;
	uint16_t tamanho_dados;
	uint16_t checksum;
} mpw_cabecalho_t;

typedef struct mpw_segmento_t {
	mpw_cabecalho_t cabecalho;
	uint8_t dados[MPW_MAX_DADOS];
} mpw_segmento_t;

/// Macros para demultiplexar as flags.
#define IS_ACK(flags) ((flags & ACK_1) ? 1 : (flags & ACK_2) ? 2 : 0)
#define GET_SEQ(flags) ((flags & SEQ_1) ? 1 : (flags & SEQ_2) ? 2 : -1)
#define INICIAR_CONEXAO(flags) (flags & C_INIT)
#define ACEITOU_CONEXAO(flags) (flags & (C_INIT | ACK_1))
#define TERMINAR_CONEXAO(flags) (flags & C_TERM)
#define CONFIRMOU_TERMINO(flags) (flags & (C_TERM | ACK1))
#define DIRTY_BIT(flags) (flags & D_BIT)

/// Variáveis globais
unsigned int estimated_rtt;

int segmento_valido(const mpw_segmento_t *segmento, int ack_esperado);

void enviar_ack(int sfd, mpw_cabecalho_t cabecalho, int ack);

void __mpw_write(int sfd, mpw_segmento_t *segmento);

#endif //CABECALHO_H
