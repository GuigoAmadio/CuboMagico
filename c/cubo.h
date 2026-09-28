#ifndef CUBO_H
#define CUBO_H

#include <stdint.h>

/* Cores. O cubo fica fixo no espaço. */
enum {
    LARANJA = 0, /* esquerda */
    VERMELHO,    /* direita */
    AMARELO,     /* baixo */
    BRANCO,      /* cima */
    AZUL,        /* tras */
    VERDE        /* frente */
};

/* Giros de 90 graus. O par (par, impar) e sempre (horario, anti-horario). */
enum {
    U = 0, U_PRIMA,
    R, R_PRIMA,
    F, F_PRIMA,
    D, D_PRIMA,
    L, L_PRIMA,
    B, B_PRIMA,
    NUM_MOVIMENTOS
};

enum {
    HEURISTICA_CANTOS = 1,
    HEURISTICA_ADESIVOS = 2,
    HEURISTICA_ZERO = 3
};

/* ESTADO: oito cantos. cor[canto][eixo], eixo 0 = X, 1 = Y, 2 = Z. */
typedef struct {
    uint8_t cor[8][3];
} Estado;

typedef struct {
    uint64_t a, b;
} Chave;

/* No da arvore de busca. O caminho volta pelo indice do pai. */
typedef struct {
    Estado estado;
    Chave chave;
    int pai;
    int movimento;
    int g;
    int h;
} No;

#endif
