#ifndef BUSCA_H
#define BUSCA_H

#include "cubo.h"

#define MAX_PASSOS 40

typedef struct {
    int encontrou;
    int interrompida;
    long long visitados;
    double milissegundos;
    int passos[MAX_PASSOS];
    int npassos;
} ResultadoBusca;

typedef struct {
    int limite;
    int heuristica;
    int relatorio;
    int anunciar_falha;
    long long visitados_base;
    double tempo_base;
    long long max_visitados;
} OpcoesBusca;

/* Laco unico. A fronteira ja vem pronta: fila, pilha ou heap. */
ResultadoBusca busca_generica(Estado inicial, int tipo_fronteira, OpcoesBusca opcoes);

ResultadoBusca busca_largura(Estado inicial, OpcoesBusca opcoes);
ResultadoBusca busca_profundidade(Estado inicial, OpcoesBusca opcoes);
ResultadoBusca busca_estrela(Estado inicial, OpcoesBusca opcoes);

#endif
