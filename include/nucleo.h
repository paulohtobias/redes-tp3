#ifndef NUCLEO_H
#define NUCLEO_H

#include "utils.h"
#include "fila.h"

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
	ACK_1 =  0x1,
	ACK_2 =  0x2,
	SEQ_1 =  0x4,
	SEQ_2 =  0x8,
	C_INIT = 0x10,
	C_TERM = 0x20,
	D_BIT =  0x40,
	FULL_BUFF = 0x80
};

/**
 *  Estrutua do cabeçalho, RFC-2154:
 * 
 *  <─────────── 32 bits ──────────>
 * ┌────────────────────────────────┐
 * │             socket             │
 * ├────────────────────────────────┤
 * │           ip origem            │
 * ├────────────────┬───────────────┤
 * │      porta     │     flags     │
 * ├────────────────┼───────────────┤
 * │     tamanho    │    checksum   │
 * └────────────────┴───────────────┘
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
#define IS_ACK(segmento) (((segmento).cabecalho.flags & ACK_1) ? 1 : ((segmento).cabecalho.flags & ACK_2) ? 2 : 0)
#define GET_SEQ(segmento) (((segmento).cabecalho.flags & SEQ_1) ? 1 : ((segmento).cabecalho.flags & SEQ_2) ? 2 : -1)
#define BUFFER_CHEIO (FULL_BUFF)
#define INICIAR_CONEXAO (C_INIT)
#define ACEITOU_CONEXAO (C_INIT | ACK_1)
#define CONEXAO_CONFIRMADA (C_INIT | ACK_2)
#define TERMINAR_CONEXAO (C_TERM)
#define CONFIRMOU_TERMINO (C_TERM | ACK_1)
#define DIRTY_BIT (D_BIT)

#define CHECAR_FLAG(segmento, flag) ((segmento).cabecalho.flags & flag)
#define CHECAR_FLAG_EXCLUSIVO(segmento, flag) ((segmento).cabecalho.flags == flag)
#define DEFINIR_FLAG(segmento, flag) (segmento).cabecalho.flags = flag

// Funções
uint16_t calcular_checksum(const mpw_segmento_t *segmento);

void segmento_corrigir_endianness(mpw_segmento_t *segmento, bool leitura);

int segmento_corrompido(const mpw_segmento_t *segmento);

#endif //NUCLEO_H
