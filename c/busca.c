#include "busca.h"

#include "estado.h"
#include "fronteira.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define HCAP (1 << 20)

typedef struct {
    uint64_t a, b;
    int g;
} Casa;

static int guarda(No **nos, int *n, int *cap, No no) {
    if (*n == *cap) {
        *cap = *cap ? *cap * 2 : 4096;
        *nos = (No *)realloc(*nos, (size_t)(*cap) * sizeof(No));
        if (!*nos) {
            perror("memoria");
            exit(1);
        }
    }
    (*nos)[*n] = no;
    return (*n)++;
}

static uint64_t mistura(uint64_t a, uint64_t b) {
    return a ^ (b * 0x9E3779B97F4A7C15ull);
}

static int memoria_achar(Casa *tab, uint64_t a, uint64_t b, int *g) {
    uint64_t i = mistura(a, b) & (HCAP - 1);
    int passos = 0;
    while (tab[i].g != -1) {
        if (tab[i].a == a && tab[i].b == b) {
            *g = tab[i].g;
            return 1;
        }
        i = (i + 1) & (HCAP - 1);
        if (++passos > HCAP) return 0;
    }
    return 0;
}

static void memoria_por(Casa *tab, uint64_t a, uint64_t b, int g) {
    uint64_t i = mistura(a, b) & (HCAP - 1);
    while (tab[i].g != -1 && !(tab[i].a == a && tab[i].b == b))
        i = (i + 1) & (HCAP - 1);
    tab[i].a = a;
    tab[i].b = b;
    tab[i].g = g;
}

static double agora_ms(clock_t inicio) {
    return 1000.0 * (double)(clock() - inicio) / (double)CLOCKS_PER_SEC;
}

static void mostrar_solucao(const ResultadoBusca *r, long long visitados, double tempo) {
    int i;
    char notacao[180];
    printf("\nSolucao encontrada.\n");
    printf("Quantidade de movimentos: %d (cada giro de 90 graus conta 1)\n", r->npassos);
    printf("Estados visitados: %lld\n", visitados);
    if (tempo < 0.5) printf("Tempo: < 1 ms\n");
    else printf("Tempo: %.0f ms\n", tempo);
    printf("Passos:");
    for (i = 0; i < r->npassos; i++) printf(" %d.%s", i + 1, nome_movimento(r->passos[i]));
    printf("\n");
    formatar_notacao(r->passos, r->npassos, notacao, (int)sizeof notacao);
    printf("Notacao: %s\n", notacao);
}

ResultadoBusca busca_generica(Estado inicial, int tipo_fronteira, OpcoesBusca opcoes) {
    ResultadoBusca resultado;
    Fronteira fronteira;
    Casa *tab;
    No *nos = NULL;
    int n = 0, cap = 0;
    clock_t t0 = clock();
    No raiz;
    memset(&resultado, 0, sizeof resultado);
    if (opcoes.max_visitados <= 0) opcoes.max_visitados = 2000000;

    tab = (Casa *)malloc(HCAP * sizeof(Casa));
    if (!tab) {
        perror("memoria");
        exit(1);
    }
    memset(tab, 0xFF, HCAP * sizeof(Casa));

    memset(&raiz, 0, sizeof raiz);
    raiz.estado = inicial;
    raiz.chave = estado_chave(inicial);
    raiz.pai = -1;
    raiz.movimento = -1;
    raiz.g = 0;
    raiz.h = funcao_heuristica(inicial, opcoes.heuristica);
    guarda(&nos, &n, &cap, raiz);
    memoria_por(tab, raiz.chave.a, raiz.chave.b, 0);

    fronteira_iniciar(&fronteira, tipo_fronteira, nos);

    /* Adicionar estado na estrutura */
    fronteira_adicionar(&fronteira, 0);

    /* Enquanto a estrutura nao estiver vazia */
    while (!fronteira_vazia(&fronteira)) {
        int indice;
        No atual;
        int antigo = 0;
        int m;

        fronteira.nos = nos;
        indice = fronteira_remover(&fronteira); /* remover proximo */
        atual = nos[indice];

        if (memoria_achar(tab, atual.chave.a, atual.chave.b, &antigo) && atual.g > antigo)
            continue;

        resultado.visitados++;
        if (resultado.visitados >= opcoes.max_visitados) {
            resultado.interrompida = 1;
            resultado.milissegundos = agora_ms(t0);
            printf("\nBusca interrompida apos %lld estados. Use um embaralhamento mais curto.\n",
                   resultado.visitados);
            fronteira_liberar(&fronteira);
            free(nos);
            free(tab);
            return resultado;
        }

        /* Avaliar estado. Se for final, mostrar a solucao e encerrar. */
        if (funcao_avaliadora(atual.estado)) {
            int k = indice;
            int rev[MAX_PASSOS];
            int nr = 0;
            resultado.encontrou = 1;
            while (k >= 0 && nos[k].movimento >= 0 && nr < MAX_PASSOS) {
                rev[nr++] = nos[k].movimento;
                k = nos[k].pai;
            }
            resultado.npassos = nr;
            for (k = 0; k < nr; k++) resultado.passos[k] = rev[nr - 1 - k];
            resultado.milissegundos = agora_ms(t0);
            if (opcoes.relatorio) {
                mostrar_solucao(&resultado,
                                opcoes.visitados_base + resultado.visitados,
                                opcoes.tempo_base + resultado.milissegundos);
            }
            break;
        }

        if (atual.g >= opcoes.limite) continue;

        /* Adicionar estados seguintes */
        for (m = 0; m < NUM_MOVIMENTOS; m++) {
            No filho;
            int gant = 0;
            int idx;
            if (atual.movimento >= 0 && (atual.movimento ^ 1) == m) continue;
            filho.estado = funcao_sucessora(atual.estado, m);
            filho.chave = estado_chave(filho.estado);
            filho.pai = indice;
            filho.movimento = m;
            filho.g = atual.g + 1;
            filho.h = funcao_heuristica(filho.estado, opcoes.heuristica);
            if (memoria_achar(tab, filho.chave.a, filho.chave.b, &gant) && filho.g >= gant)
                continue;
            memoria_por(tab, filho.chave.a, filho.chave.b, filho.g);
            idx = guarda(&nos, &n, &cap, filho);
            fronteira.nos = nos;
            fronteira_adicionar(&fronteira, idx);
        }
    }

    if (!resultado.encontrou) {
        resultado.milissegundos = agora_ms(t0);
        if (opcoes.anunciar_falha) printf("Sem solução\n");
    }

    fronteira_liberar(&fronteira);
    free(nos);
    free(tab);
    return resultado;
}

ResultadoBusca busca_largura(Estado inicial, OpcoesBusca opcoes) {
    opcoes.heuristica = HEURISTICA_ZERO;
    opcoes.limite = 14;
    return busca_generica(inicial, TIPO_FILA, opcoes);
}

ResultadoBusca busca_estrela(Estado inicial, OpcoesBusca opcoes) {
    opcoes.limite = 14;
    return busca_generica(inicial, TIPO_HEAP, opcoes);
}

ResultadoBusca busca_profundidade(Estado inicial, OpcoesBusca opcoes) {
    ResultadoBusca total;
    long long visitados = 0;
    double tempo = 0;
    int limite;
    int limite_max = opcoes.limite > 0 ? opcoes.limite : 14;

    memset(&total, 0, sizeof total);
    if (opcoes.relatorio)
        printf("Profundidade limitada iterativa (limite 0, 1, 2, ... sem recursao):\n");

    for (limite = 0; limite <= limite_max; limite++) {
        OpcoesBusca op = opcoes;
        ResultadoBusca r;
        op.limite = limite;
        op.heuristica = HEURISTICA_ZERO;
        op.anunciar_falha = 0;
        op.visitados_base = visitados;
        op.tempo_base = tempo;
        r = busca_generica(inicial, TIPO_PILHA, op);
        visitados += r.visitados;
        tempo += r.milissegundos;
        if (r.interrompida) {
            total.interrompida = 1;
            total.visitados = visitados;
            total.milissegundos = tempo;
            return total;
        }
        if (r.encontrou) {
            total = r;
            total.visitados = visitados;
            total.milissegundos = tempo;
            return total;
        }
        if (opcoes.relatorio)
            printf("  Limite %d: %lld estados (nao resolvido neste limite)\n", limite, r.visitados);
    }

    total.visitados = visitados;
    total.milissegundos = tempo;
    if (opcoes.anunciar_falha) printf("Sem solução\n");
    return total;
}
