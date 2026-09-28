#ifndef FRONTEIRA_H
#define FRONTEIRA_H

#include "cubo.h"

/* A busca so usa estas tres operacoes.
   O tipo escolhe fila (largura), pilha (profundidade) ou heap (A*). */
enum {
    TIPO_FILA = 0,
    TIPO_PILHA = 1,
    TIPO_HEAP = 2
};

typedef struct {
    int tipo;
    int *v;
    int n;
    int cap;
    int ini;
    No *nos; /* o heap le g e h aqui */
} Fronteira;

void fronteira_iniciar(Fronteira *f, int tipo, No *nos);
void fronteira_liberar(Fronteira *f);
void fronteira_adicionar(Fronteira *f, int indice);
int fronteira_remover(Fronteira *f);
int fronteira_vazia(const Fronteira *f);

#endif
