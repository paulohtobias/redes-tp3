#ifndef ENVIAR_H
#define ENVIAR_H

#include "conexao.h"

ssize_t enviar(int sockfd, void *dados, size_t tamanho);

#endif //ENVIAR_H
