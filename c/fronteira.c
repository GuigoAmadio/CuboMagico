#include "fronteira.h"

#include <stdio.h>
#include <stdlib.h>

static void garantir(Fronteira *f) {
    if (f->n < f->cap) return;
    f->cap = f->cap ? f->cap * 2 : 1024;
    f->v = (int *)realloc(f->v, (size_t)f->cap * sizeof(int));
    if (!f->v) {
        perror("memoria");
        exit(1);
    }
}

/* Menor que zero: o indice a deve sair antes de b.
   Ordem: menor f, empate no maior g, empate no menor indice. */
static int comparar(const Fronteira *f, int a, int b) {
    int fa = f->nos[a].g + f->nos[a].h;
    int fb = f->nos[b].g + f->nos[b].h;
    if (fa != fb) return fa - fb;
    if (f->nos[a].g != f->nos[b].g) return f->nos[b].g - f->nos[a].g;
    return a - b;
}

static void heap_subir(Fronteira *f, int i) {
    while (i > 0) {
        int pai = (i - 1) / 2;
        int tmp;
        if (comparar(f, f->v[i], f->v[pai]) >= 0) break;
        tmp = f->v[pai];
        f->v[pai] = f->v[i];
        f->v[i] = tmp;
        i = pai;
    }
}

static void heap_descer(Fronteira *f, int i) {
    for (;;) {
        int esq = i * 2 + 1;
        int dir = esq + 1;
        int menor = i;
        int tmp;
        if (esq < f->n && comparar(f, f->v[esq], f->v[menor]) < 0) menor = esq;
        if (dir < f->n && comparar(f, f->v[dir], f->v[menor]) < 0) menor = dir;
        if (menor == i) break;
        tmp = f->v[i];
        f->v[i] = f->v[menor];
        f->v[menor] = tmp;
        i = menor;
    }
}

void fronteira_iniciar(Fronteira *f, int tipo, No *nos) {
    f->tipo = tipo;
    f->v = NULL;
    f->n = 0;
    f->cap = 0;
    f->ini = 0;
    f->nos = nos;
}

void fronteira_liberar(Fronteira *f) {
    free(f->v);
    f->v = NULL;
    f->n = 0;
    f->cap = 0;
    f->ini = 0;
}

void fronteira_adicionar(Fronteira *f, int indice) {
    if (f->tipo == TIPO_FILA) {
        garantir(f);
        f->v[f->n++] = indice;
        return;
    }
    if (f->tipo == TIPO_PILHA) {
        garantir(f);
        f->v[f->n++] = indice;
        return;
    }
    garantir(f);
    f->v[f->n] = indice;
    heap_subir(f, f->n);
    f->n++;
}

int fronteira_remover(Fronteira *f) {
    if (f->tipo == TIPO_FILA) {
        return f->v[f->ini++];
    }
    if (f->tipo == TIPO_PILHA) {
        f->n--;
        return f->v[f->n];
    }
    {
        int melhor = f->v[0];
        f->n--;
        if (f->n > 0) {
            f->v[0] = f->v[f->n];
            heap_descer(f, 0);
        }
        return melhor;
    }
}

int fronteira_vazia(const Fronteira *f) {
    if (f->tipo == TIPO_FILA) return f->ini >= f->n;
    return f->n == 0;
}
