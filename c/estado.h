#ifndef ESTADO_H
#define ESTADO_H

#include "cubo.h"

Estado estado_resolvido(void);

/* Funcao sucessora: estado seguinte depois de um giro de 90 graus. */
Estado funcao_sucessora(Estado estado, int movimento);

/* Funcao avaliadora: 1 somente se o cubo esta resolvido. */
int funcao_avaliadora(Estado estado);

/* h(n) do A*. As tres opcoes sao admissiveis. */
int funcao_heuristica(Estado estado, int tipo);

Chave estado_chave(Estado estado);

const char *nome_movimento(int movimento);
const char *nome_heuristica(int tipo);
int movimento_inverso(int movimento);

void formatar_notacao(const int *passos, int n, char *saida, int tamanho);
const char *texto_heuristica(int tipo);
void imprimir_estado(Estado estado);
Estado embaralhar(int passos, uint32_t seed, int *sequencia);
Estado aplicar_sequencia(Estado estado, const int *passos, int n);
int pecas_validas(Estado estado);

/* Devolve NULL se os giros estao corretos. Senao, a mensagem do erro. */
const char *motor_erro(void);

#endif
