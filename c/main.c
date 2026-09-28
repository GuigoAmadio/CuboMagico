#include "busca.h"
#include "estado.h"
#include "tela.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

/* Cubo magico 2x2x2 — Grupo T1
   Integrantes: ________________________________ */

#ifdef _WIN32
static void preparar_console(void) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}
#else
static void preparar_console(void) {}
#endif

static void aparar(char *s) {
    char *a = s;
    char *b;
    size_t n;
    while (*a == ' ' || *a == '\t' || *a == '\r') a++;
    n = strlen(a);
    b = a + n;
    while (b > a && (b[-1] == ' ' || b[-1] == '\t' || b[-1] == '\r' || b[-1] == '\n')) b--;
    *b = 0;
    if (a != s) memmove(s, a, (size_t)(b - a) + 1);
}

static void minusculas(char *s) {
    for (; *s; s++)
        if (*s >= 'A' && *s <= 'Z') *s = (char)(*s - 'A' + 'a');
}

static unsigned seed_atual = 42;
static int passos_atual = 5;
static int heuristica = HEURISTICA_CANTOS;
static Estado cubo;
static int mistura[16];
static int nmistura;
static int historico[80];
static int nhist;
static int solucao[MAX_PASSOS];
static int nsolucao;
static Chave chave_solucao;
static int tem_solucao;

static void mostrar_ajuda(void) {
    printf(
        "\nComo jogar\n"
        "  O desenho e a planificacao do cubo. Cada letra e um adesivo.\n"
        "  U cima    D baixo    F frente    B tras    R direita    L esquerda\n"
        "  ' = anti-horario     2 = meia volta (dois giros de 90 graus)\n"
        "  Exemplos: R     U'     F2     R U F'     direita     frente'\n\n"
        "Comandos\n"
        "  embaralhar              gera de novo o problema da seed atual\n"
        "  seed 42                 fixa a seed e embaralha\n"
        "  passos 5                quantidade de giros do embaralhamento (1 a 7)\n"
        "  heuristica 1|2|3        1 cantos/4   2 adesivos/12   3 h=0\n"
        "  ia largura              busca em largura\n"
        "  ia profundidade         busca em profundidade limitada iterativa\n"
        "  ia estrela              busca A*\n"
        "  ia todas                compara as tres na posicao atual\n"
        "  aplicar                 executa a ultima solucao no cubo\n"
        "  desfazer                desfaz o ultimo giro\n"
        "  reiniciar               volta ao cubo resolvido\n"
        "  explicar                texto para a arguicao\n"
        "  ajuda                   esta tela\n"
        "  sair\n\n"
        "A solucao da IA nao mexe no cubo. Rode as tres buscas na mesma\n"
        "posicao e use 'aplicar' se quiser executar os giros.\n");
}

static void mostrar_explicacao(void) {
    printf(
        "\nO que mostrar na arguicao\n"
        "\n1. Estado (c/cubo.h e c/estado.c)\n"
        "   Oito cantos. Cada um guarda a cor que aponta para X, Y e Z.\n"
        "   Resolvido: branco em cima, amarelo embaixo, verde na frente,\n"
        "   azul atras, vermelho na direita, laranja na esquerda.\n"
        "\n2. Funcao sucessora (funcao_sucessora)\n"
        "   Recebe um estado e um giro de 90 graus e devolve o proximo.\n"
        "   Ha 12 sucessores. Meia volta sao dois sucessores.\n"
        "\n3. Funcao avaliadora (funcao_avaliadora)\n"
        "   No laco, avaliar e perguntar se o estado e o cubo resolvido.\n"
        "   No A*, a fila de prioridade ordena por f(n) = g(n) + h(n).\n"
        "\n4. O laco e unico (busca_generica, em c/busca.c)\n"
        "   Adicionar estado na estrutura\n"
        "   Enquanto a estrutura nao estiver vazia:\n"
        "     Remover proximo estado\n"
        "     Avaliar\n"
        "     Se for final, mostrar a solucao e encerrar a busca\n"
        "     Adicionar os estados seguintes\n"
        "   Se a estrutura esvaziar: \"Sem solucao\"\n"
        "   O laco nao muda. O que muda e a fronteira (c/fronteira.c):\n"
        "     fila (FIFO)              -> busca em largura, solucao otima\n"
        "     pilha (LIFO) + limite    -> profundidade limitada\n"
        "     o limite sobe 0, 1, 2... -> profundidade iterativa, sem recursao\n"
        "     heap por f               -> A*\n"
        "\n5. Heuristica: teto(cantos incorretos / 4)\n"
        "   Admissivel: um giro mexe em 4 cantos.\n"
        "   Consistente: h cai no maximo 1 e cada giro custa 1.\n"
        "\n6. Seed\n"
        "   'seed 42' e 'passos 5' refazem sempre o mesmo embaralhamento.\n"
        "   As tres buscas sao otimas: o numero de movimentos coincide.\n"
        "   O A* com a heuristica dos cantos visita menos estados.\n");
}

static int token_movimento(char *tok, int *saida, int *n, int max) {
    char sufixo = 0;
    int base = -1;
    size_t ntok = strlen(tok);
    minusculas(tok);
    if (ntok >= 3 && (unsigned char)tok[ntok - 3] == 0xE2 &&
        (unsigned char)tok[ntok - 2] == 0x80 &&
        ((unsigned char)tok[ntok - 1] == 0x99 || (unsigned char)tok[ntok - 1] == 0xB2)) {
        sufixo = '\'';
        tok[ntok - 3] = 0;
    } else if (ntok > 0 && (tok[ntok - 1] == '\'' || tok[ntok - 1] == '2')) {
        sufixo = tok[ntok - 1];
        tok[ntok - 1] = 0;
    }
    if (strcmp(tok, "u") == 0 || strcmp(tok, "cima") == 0) base = U;
    else if (strcmp(tok, "d") == 0 || strcmp(tok, "baixo") == 0) base = D;
    else if (strcmp(tok, "f") == 0 || strcmp(tok, "frente") == 0) base = F;
    else if (strcmp(tok, "b") == 0 || strcmp(tok, "tras") == 0) base = B;
    else if (strcmp(tok, "r") == 0 || strcmp(tok, "direita") == 0) base = R;
    else if (strcmp(tok, "l") == 0 || strcmp(tok, "esquerda") == 0) base = L;
    else return 0;
    if (sufixo == '\'') {
        if (*n >= max) return 0;
        saida[(*n)++] = base + 1;
    } else if (sufixo == '2') {
        if (*n + 1 >= max) return 0;
        saida[(*n)++] = base;
        saida[(*n)++] = base;
    } else if (sufixo == 0) {
        if (*n >= max) return 0;
        saida[(*n)++] = base;
    } else return 0;
    return 1;
}

static int linha_movimentos(char *linha, int *saida, int *n) {
    char copia[256];
    char *tok;
    int algum = 0;
    snprintf(copia, sizeof copia, "%s", linha);
    *n = 0;
    tok = strtok(copia, " \t");
    if (!tok) return 0;
    while (tok) {
        if (!token_movimento(tok, saida, n, 40)) return 0;
        algum = 1;
        tok = strtok(NULL, " \t");
    }
    return algum;
}

static void gerar_mistura(void) {
    char nota[180];
    cubo = embaralhar(passos_atual, seed_atual, mistura);
    nmistura = passos_atual;
    memcpy(historico, mistura, (size_t)nmistura * sizeof(int));
    nhist = nmistura;
    tem_solucao = 0;
    formatar_notacao(mistura, nmistura, nota, (int)sizeof nota);
    printf("Embaralhamento (seed %u, %d giros): %s\n", seed_atual, passos_atual, nota);
    printf("A mesma seed refaz esta posicao. Da para comparar as tres IAs.\n");
    if (passos_atual >= 6)
        printf("Aviso: com %d giros a largura e a profundidade ficam lentas. 5 e um bom padrao.\n",
               passos_atual);
}

static OpcoesBusca opcoes_de(void) {
    OpcoesBusca op;
    memset(&op, 0, sizeof op);
    op.limite = 14;
    op.heuristica = heuristica;
    op.relatorio = 1;
    op.anunciar_falha = 1;
    op.max_visitados = 2000000;
    return op;
}

static void guardar_se_achou(ResultadoBusca r) {
    if (!r.encontrou) return;
    memcpy(solucao, r.passos, (size_t)r.npassos * sizeof(int));
    nsolucao = r.npassos;
    chave_solucao = estado_chave(cubo);
    tem_solucao = 1;
}

static void tempo_ms(double v, char *saida, int tam) {
    if (v < 0.5) snprintf(saida, (size_t)tam, "< 1");
    else snprintf(saida, (size_t)tam, "%.0f", v);
}

static void comparar(void) {
    OpcoesBusca op = opcoes_de();
    ResultadoBusca largura, prof, estrela;
    char t1[16], t2[16], t3[16];
    char nota[180];
    printf("\n=== Comparacao na mesma posicao ===\n");
    if (nmistura > 0) {
        formatar_notacao(mistura, nmistura, nota, (int)sizeof nota);
        printf("Ultimo embaralhamento (seed %u): %s\n", seed_atual, nota);
    }
    printf("Heuristica do A*: %s\n", nome_heuristica(heuristica));
    printf("%s", texto_heuristica(heuristica));

    printf("\n--- Largura ---\n");
    largura = busca_largura(cubo, op);
    printf("\n--- Profundidade limitada iterativa ---\n");
    prof = busca_profundidade(cubo, op);
    printf("\n--- A* ---\n");
    printf("h(estado atual) = %d\n", funcao_heuristica(cubo, heuristica));
    estrela = busca_estrela(cubo, op);

    tempo_ms(largura.milissegundos, t1, (int)sizeof t1);
    tempo_ms(prof.milissegundos, t2, (int)sizeof t2);
    tempo_ms(estrela.milissegundos, t3, (int)sizeof t3);
    printf("\nResumo (mesma posicao)\n");
    printf("  Largura        movimentos %d   estados %lld   tempo %s ms\n",
           largura.encontrou ? largura.npassos : -1, largura.visitados, t1);
    printf("  Profundidade   movimentos %d   estados %lld   tempo %s ms\n",
           prof.encontrou ? prof.npassos : -1, prof.visitados, t2);
    printf("  A*             movimentos %d   estados %lld   tempo %s ms\n",
           estrela.encontrou ? estrela.npassos : -1, estrela.visitados, t3);
    if (largura.encontrou && prof.encontrou && estrela.encontrou &&
        largura.npassos == prof.npassos && largura.npassos == estrela.npassos) {
        printf("As tres acharam solucao otima com %d movimentos.\n", largura.npassos);
        guardar_se_achou(largura);
    }
}

static int comando_ia(const char *qual) {
    if (strcmp(qual, "largura") == 0 || strcmp(qual, "bfs") == 0 || strcmp(qual, "fila") == 0) {
        printf("\n=== Busca em largura (fila) ===\n");
        guardar_se_achou(busca_largura(cubo, opcoes_de()));
        return 1;
    }
    if (strcmp(qual, "profundidade") == 0 || strcmp(qual, "prof") == 0 ||
        strcmp(qual, "iddfs") == 0 || strcmp(qual, "iterativa") == 0 ||
        strcmp(qual, "pilha") == 0 || strcmp(qual, "dfs") == 0) {
        printf("\n=== Busca em profundidade limitada iterativa (pilha) ===\n");
        guardar_se_achou(busca_profundidade(cubo, opcoes_de()));
        return 1;
    }
    if (strcmp(qual, "estrela") == 0 || strcmp(qual, "a*") == 0 || strcmp(qual, "a") == 0 ||
        strcmp(qual, "astar") == 0) {
        printf("\n=== Busca A* (fila de prioridade) ===\n");
        printf("%s", texto_heuristica(heuristica));
        printf("h(estado atual) = %d\n", funcao_heuristica(cubo, heuristica));
        guardar_se_achou(busca_estrela(cubo, opcoes_de()));
        return 1;
    }
    if (strcmp(qual, "todas") == 0 || strcmp(qual, "comparar") == 0 || strcmp(qual, "compare") == 0) {
        comparar();
        return 1;
    }
    return 0;
}

static void aplicar_movimentos(const int *movs, int n) {
    int i;
    for (i = 0; i < n; i++) {
        cubo = funcao_sucessora(cubo, movs[i]);
        if (nhist < 80) historico[nhist++] = movs[i];
    }
    tem_solucao = 0;
    if (funcao_avaliadora(cubo)) printf("Cubo resolvido. Parabens!\n");
}

static void loop_texto(void) {
    char linha[256];
    cubo = estado_resolvido();
    printf(
        "================================\n"
        "  Cubo magico 2x2x2   Grupo T1\n"
        "================================\n"
        "Modo texto. A janela 3D abre com .\\cubo.exe sem argumentos.\n"
        "Cubo resolvido. Digite 'embaralhar' (seed %u, %d giros).\n"
        "Digite 'ajuda' para ver os movimentos e as IAs.\n\n",
        seed_atual, passos_atual);
    imprimir_estado(cubo);

    while (1) {
        char cmd[64], arg[160];
        int movs[40], nmov = 0;
        printf("\n[seed %u | passos %d | h %s]\n> ", seed_atual, passos_atual, nome_heuristica(heuristica));
        fflush(stdout);
        if (!fgets(linha, (int)sizeof linha, stdin)) break;
        aparar(linha);
        if (!linha[0]) {
            imprimir_estado(cubo);
            continue;
        }
        if (linha_movimentos(linha, movs, &nmov)) {
            aplicar_movimentos(movs, nmov);
            imprimir_estado(cubo);
            continue;
        }
        cmd[0] = 0;
        arg[0] = 0;
        sscanf(linha, "%63s %159[^\n]", cmd, arg);
        aparar(arg);
        minusculas(cmd);
        minusculas(arg);

        if (strcmp(cmd, "sair") == 0 || strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) break;
        else if (strcmp(cmd, "ajuda") == 0 || strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) mostrar_ajuda();
        else if (strcmp(cmd, "explicar") == 0) mostrar_explicacao();
        else if (strcmp(cmd, "mostrar") == 0 || strcmp(cmd, "cubo") == 0) imprimir_estado(cubo);
        else if (strcmp(cmd, "embaralhar") == 0 || strcmp(cmd, "misturar") == 0) {
            gerar_mistura();
            imprimir_estado(cubo);
        } else if (strcmp(cmd, "reiniciar") == 0 || strcmp(cmd, "reset") == 0) {
            cubo = estado_resolvido();
            nhist = 0;
            nmistura = 0;
            tem_solucao = 0;
            printf("Cubo resolvido.\n");
            imprimir_estado(cubo);
        } else if (strcmp(cmd, "desfazer") == 0 || strcmp(cmd, "undo") == 0) {
            if (nhist <= 0) printf("Nada para desfazer.\n");
            else {
                int m = historico[--nhist];
                cubo = funcao_sucessora(cubo, movimento_inverso(m));
                tem_solucao = 0;
                printf("Desfeito: %s\n", nome_movimento(m));
                imprimir_estado(cubo);
            }
        } else if (strcmp(cmd, "seed") == 0 || strcmp(cmd, "semente") == 0) {
            char *fim = NULL;
            unsigned long v = strtoul(arg, &fim, 10);
            if (!arg[0] || (fim && *fim)) printf("Use: seed 42\n");
            else {
                seed_atual = (unsigned)v;
                gerar_mistura();
                imprimir_estado(cubo);
            }
        } else if (strcmp(cmd, "passos") == 0) {
            int n = atoi(arg);
            if (n < 1 || n > 7) printf("Escolha de 1 a 7.\n");
            else {
                passos_atual = n;
                gerar_mistura();
                imprimir_estado(cubo);
            }
        } else if (strcmp(cmd, "heuristica") == 0) {
            if (strcmp(arg, "1") == 0 || strcmp(arg, "cantos") == 0) heuristica = HEURISTICA_CANTOS;
            else if (strcmp(arg, "2") == 0 || strcmp(arg, "adesivos") == 0) heuristica = HEURISTICA_ADESIVOS;
            else if (strcmp(arg, "3") == 0 || strcmp(arg, "zero") == 0) heuristica = HEURISTICA_ZERO;
            else if (arg[0]) {
                printf("Use: heuristica 1, 2 ou 3\n");
                continue;
            }
            printf("Heuristica atual: %s\n", nome_heuristica(heuristica));
            printf("%s", texto_heuristica(heuristica));
        } else if (strcmp(cmd, "ia") == 0 || strcmp(cmd, "resolver") == 0 || strcmp(cmd, "busca") == 0) {
            if (!arg[0]) printf("Use: ia largura | ia profundidade | ia estrela | ia todas\n");
            else if (!comando_ia(arg)) printf("IA desconhecida. Use largura, profundidade, estrela ou todas.\n");
        } else if (strcmp(cmd, "aplicar") == 0) {
            Chave agora = estado_chave(cubo);
            if (!tem_solucao) printf("Ainda nao ha solucao. Rode uma IA antes.\n");
            else if (agora.a != chave_solucao.a || agora.b != chave_solucao.b)
                printf("O cubo mudou desde a ultima solucao. Rode a IA de novo.\n");
            else {
                char nota[180];
                formatar_notacao(solucao, nsolucao, nota, (int)sizeof nota);
                printf("Aplicando: %s\n", nota);
                aplicar_movimentos(solucao, nsolucao);
                imprimir_estado(cubo);
            }
        } else {
            printf("Comando nao reconhecido. Digite 'ajuda'.\n");
        }
    }
}

static int falhas = 0;

static void checar(int ok, const char *msg) {
    if (ok) printf("ok  %s\n", msg);
    else {
        printf("FALHOU  %s\n", msg);
        falhas++;
    }
}

static int resolve_e_confere(Estado inicio, ResultadoBusca r) {
    if (!r.encontrou || r.interrompida) return 0;
    return funcao_avaliadora(aplicar_sequencia(inicio, r.passos, r.npassos));
}

static OpcoesBusca op_teste(int h) {
    OpcoesBusca op;
    memset(&op, 0, sizeof op);
    op.limite = 14;
    op.heuristica = h;
    op.max_visitados = 2000000;
    return op;
}

static void bater(unsigned s, int npassos) {
    int seq[16];
    Estado e = embaralhar(npassos, s, seq);
    int h = funcao_heuristica(e, HEURISTICA_CANTOS);
    ResultadoBusca b = busca_largura(e, op_teste(HEURISTICA_ZERO));
    ResultadoBusca p = busca_profundidade(e, op_teste(HEURISTICA_ZERO));
    ResultadoBusca a1 = busca_estrela(e, op_teste(HEURISTICA_CANTOS));
    ResultadoBusca a2 = busca_estrela(e, op_teste(HEURISTICA_ADESIVOS));
    ResultadoBusca a0 = busca_estrela(e, op_teste(HEURISTICA_ZERO));
    int ok = resolve_e_confere(e, b) && resolve_e_confere(e, p) &&
             resolve_e_confere(e, a1) && resolve_e_confere(e, a2) && resolve_e_confere(e, a0);
    char msg[220];
    ok = ok && b.npassos == p.npassos && b.npassos == a1.npassos &&
         b.npassos == a2.npassos && b.npassos == a0.npassos;
    ok = ok && b.npassos <= npassos && h <= b.npassos;
    snprintf(msg, sizeof msg,
             "seed %u passos %d  solucao %d  visitados L/P/A %lld/%lld/%lld  h %d",
             s, npassos, b.npassos, b.visitados, p.visitados, a1.visitados, h);
    checar(ok, msg);
}

static int executar_testes(void) {
    const char *erro;
    printf("Testes do cubo 2x2x2\n");
    erro = motor_erro();
    checar(erro == NULL, erro ? erro : "motor de movimentos");
    checar(funcao_heuristica(estado_resolvido(), HEURISTICA_CANTOS) == 0, "h(resolvido) = 0");
    checar(funcao_avaliadora(estado_resolvido()), "avaliadora no cubo resolvido");
    {
        ResultadoBusca r = busca_largura(estado_resolvido(), op_teste(HEURISTICA_ZERO));
        checar(r.encontrou && r.npassos == 0 && r.visitados == 1,
               "cubo resolvido: largura visita 1 estado e 0 movimentos");
    }
    bater(42, 1);
    bater(42, 2);
    bater(42, 3);
    bater(7, 4);
    bater(42, 5);
    {
        int seq[4];
        Estado e = embaralhar(1, 1u, seq);
        ResultadoBusca r = busca_largura(e, op_teste(HEURISTICA_ZERO));
        checar(r.encontrou && r.npassos == 1, "um giro se desfaz em um movimento");
    }
    if (falhas) {
        printf("%d teste(s) falharam.\n", falhas);
        return 1;
    }
    printf("Todos os testes passaram.\n");
    return 0;
}

int main(int argc, char **argv) {
    const char *erro;
    preparar_console();
    erro = motor_erro();
    if (erro) {
        printf("Erro no motor do cubo: %s\n", erro);
        return 1;
    }
    if (argc > 1 && strcmp(argv[1], "--teste") == 0) return executar_testes();
    if (argc > 1 && strcmp(argv[1], "--console") == 0) {
        loop_texto();
        return 0;
    }
    executar_interface_3d();
    return 0;
}
