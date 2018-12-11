#ifndef RECEBER_H
#define RECEBER_H

#include "conexao.h"

/// se buffer_cru não for NULL, precisa ser *obrigatoriamente* alocado dinamicamente.
ssize_t receber(int fd, void *buffer, size_t buffer_tamanho, void **buffer_cru, size_t *buffer_cru_tamanho);

ssize_t ler(int fd, void *buffer, size_t tamanho_maximo);

#endif //RECEBER_H
