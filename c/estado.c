#include "estado.h"

#include <stdio.h>
#include <string.h>

static int slot(int x, int y, int z) {
    return (x > 0) | ((y > 0) << 1) | ((z > 0) << 2);
}

/* eixo, camada (+1 ou -1), tipo do giro, mapa das cores. */
static const int SPEC[12][6] = {
    {1,  1, 0, 2, 1, 0}, {1,  1, 1, 2, 1, 0},
    {0,  1, 2, 0, 2, 1}, {0,  1, 3, 0, 2, 1},
    {2,  1, 4, 1, 0, 2}, {2,  1, 5, 1, 0, 2},
    {1, -1, 1, 2, 1, 0}, {1, -1, 0, 2, 1, 0},
    {0, -1, 3, 0, 2, 1}, {0, -1, 2, 0, 2, 1},
    {2, -1, 5, 1, 0, 2}, {2, -1, 4, 1, 0, 2}
};

static const char *NOMES[12] = {
    "U", "U'", "R", "R'", "F", "F'", "D", "D'", "L", "L'", "B", "B'"
};

static const char LETRAS[] = "LRABZV";

Estado estado_resolvido(void) {
    Estado e;
    int x, y, z;
    for (x = -1; x <= 1; x += 2)
        for (y = -1; y <= 1; y += 2)
            for (z = -1; z <= 1; z += 2) {
                int s = slot(x, y, z);
                e.cor[s][0] = (uint8_t)(x > 0 ? VERMELHO : LARANJA);
                e.cor[s][1] = (uint8_t)(y > 0 ? BRANCO : AMARELO);
                e.cor[s][2] = (uint8_t)(z > 0 ? VERDE : AZUL);
            }
    return e;
}

static void destino(int tipo, int x, int y, int z, int *nx, int *ny, int *nz) {
    int X[6] = {-z, z, x, x, y, -y};
    int Y[6] = {y, y, z, -z, -x, x};
    int Z[6] = {x, -x, -y, y, z, z};
    *nx = X[tipo];
    *ny = Y[tipo];
    *nz = Z[tipo];
}

Estado funcao_sucessora(Estado estado, int movimento) {
    Estado saida = estado;
    int eixo = SPEC[movimento][0];
    int camada = SPEC[movimento][1];
    int tipo = SPEC[movimento][2];
    int mapa0 = SPEC[movimento][3];
    int mapa1 = SPEC[movimento][4];
    int mapa2 = SPEC[movimento][5];
    int mapa[3];
    int x, y, z;
    mapa[0] = mapa0;
    mapa[1] = mapa1;
    mapa[2] = mapa2;

    for (x = -1; x <= 1; x += 2)
        for (y = -1; y <= 1; y += 2)
            for (z = -1; z <= 1; z += 2) {
                int valor = (eixo == 0) ? x : (eixo == 1) ? y : z;
                int nx, ny, nz, a, de, para;
                if (valor != camada) continue;
                destino(tipo, x, y, z, &nx, &ny, &nz);
                de = slot(x, y, z);
                para = slot(nx, ny, nz);
                for (a = 0; a < 3; a++)
                    saida.cor[para][a] = estado.cor[de][mapa[a]];
            }
    return saida;
}

int funcao_avaliadora(Estado estado) {
    Estado alvo = estado_resolvido();
    return memcmp(&estado, &alvo, sizeof estado) == 0;
}

int funcao_heuristica(Estado estado, int tipo) {
    Estado alvo = estado_resolvido();
    int s, a;
    if (tipo == HEURISTICA_ZERO) return 0;
    if (tipo == HEURISTICA_ADESIVOS) {
        int errados = 0;
        for (s = 0; s < 8; s++)
            for (a = 0; a < 3; a++)
                if (estado.cor[s][a] != alvo.cor[s][a]) errados++;
        return (errados + 11) / 12;
    }
    {
        int cantos = 0;
        for (s = 0; s < 8; s++) {
            int ok = 1;
            for (a = 0; a < 3; a++)
                if (estado.cor[s][a] != alvo.cor[s][a]) ok = 0;
            if (!ok) cantos++;
        }
        return (cantos + 3) / 4;
    }
}

Chave estado_chave(Estado estado) {
    Chave c;
    int k = 0, s, a;
    c.a = 0;
    c.b = 0;
    for (s = 0; s < 8; s++) {
        for (a = 0; a < 3; a++, k++) {
            uint64_t pedaco = estado.cor[s][a];
            if (k < 12) c.a |= pedaco << (k * 3);
            else c.b |= pedaco << ((k - 12) * 3);
        }
    }
    return c;
}

const char *nome_movimento(int movimento) {
    if (movimento < 0 || movimento >= NUM_MOVIMENTOS) return "?";
    return NOMES[movimento];
}

const char *nome_heuristica(int tipo) {
    if (tipo == HEURISTICA_ADESIVOS) return "adesivos / 12";
    if (tipo == HEURISTICA_ZERO) return "h = 0";
    return "cantos / 4";
}

int movimento_inverso(int movimento) {
    return movimento ^ 1;
}

void formatar_notacao(const int *passos, int n, char *saida, int tamanho) {
    int i = 0, pos = 0;
    if (n <= 0) {
        snprintf(saida, (size_t)tamanho, "(ja resolvido)");
        return;
    }
    saida[0] = 0;
    while (i < n && pos < tamanho - 8) {
        int j = i + 1;
        int vezes, base, anti, escrito;
        while (j < n && passos[j] == passos[i] && (j - i) < 3) j++;
        vezes = j - i;
        base = passos[i] & ~1;
        anti = passos[i] & 1;
        if (pos > 0) saida[pos++] = ' ';
        escrito = snprintf(saida + pos, (size_t)(tamanho - pos), "%s", NOMES[base]);
        if (escrito < 0) break;
        pos += escrito;
        if (vezes == 2) saida[pos++] = '2';
        else if (vezes == 3) {
            if (!anti) saida[pos++] = '\'';
        } else if (anti) {
            saida[pos++] = '\'';
        }
        saida[pos] = 0;
        i = j;
    }
}

static void selo(Estado e, int x, int y, int z, int eixo) {
    printf("%c ", LETRAS[e.cor[slot(x, y, z)][eixo]]);
}

void imprimir_estado(Estado e) {
    printf("\n    ");
    selo(e, -1, 1, -1, 1); selo(e, 1, 1, -1, 1); printf("\n    ");
    selo(e, -1, 1, 1, 1); selo(e, 1, 1, 1, 1); printf("  cima (U)\n");
    selo(e, -1, 1, -1, 0); selo(e, -1, 1, 1, 0);
    selo(e, -1, 1, 1, 2); selo(e, 1, 1, 1, 2);
    selo(e, 1, 1, 1, 0); selo(e, 1, 1, -1, 0);
    selo(e, 1, 1, -1, 2); selo(e, -1, 1, -1, 2); printf("\n");
    selo(e, -1, -1, -1, 0); selo(e, -1, -1, 1, 0);
    selo(e, -1, -1, 1, 2); selo(e, 1, -1, 1, 2);
    selo(e, 1, -1, 1, 0); selo(e, 1, -1, -1, 0);
    selo(e, 1, -1, -1, 2); selo(e, -1, -1, -1, 2); printf("\n    ");
    selo(e, -1, -1, 1, 1); selo(e, 1, -1, 1, 1); printf("\n    ");
    selo(e, -1, -1, -1, 1); selo(e, 1, -1, -1, 1); printf("  baixo (D)\n");
    printf("L laranja  R vermelho  A amarelo  B branco  Z azul  V verde\n");
    printf("Ordem do meio: esquerda, frente, direita, tras\n");
}

/* Mersenne Twister. A mesma seed produz sempre a mesma sequencia. */
static uint32_t mt[624];
static int mt_i = 624;

static void mt_semente(uint32_t semente) {
    int i;
    mt[0] = semente;
    for (i = 1; i < 624; i++)
        mt[i] = 1812433253u * (mt[i - 1] ^ (mt[i - 1] >> 30)) + (uint32_t)i;
    mt_i = 624;
}

static uint32_t mt_proximo(void) {
    uint32_t y;
    int i;
    if (mt_i >= 624) {
        for (i = 0; i < 624; i++) {
            y = (mt[i] & 0x80000000u) | (mt[(i + 1) % 624] & 0x7fffffffu);
            mt[i] = mt[(i + 397) % 624] ^ (y >> 1) ^ ((y & 1u) ? 0x9908b0dfu : 0u);
        }
        mt_i = 0;
    }
    y = mt[mt_i++];
    y ^= y >> 11;
    y ^= (y << 7) & 0x9d2c5680u;
    y ^= (y << 15) & 0xefc60000u;
    y ^= y >> 18;
    return y;
}

/* Igual a std::uniform_int_distribution<int>(0, 11) do g++. */
static int aleatorio(void) {
    const uint32_t escala = 4294967295u / 12u;
    const uint32_t limite = 12u * escala;
    uint32_t n;
    do n = mt_proximo();
    while (n >= limite);
    return (int)(n / escala);
}

Estado embaralhar(int passos, uint32_t seed, int *sequencia) {
    Estado e = estado_resolvido();
    int anterior = -1;
    int feitos = 0;
    mt_semente(seed);
    while (feitos < passos) {
        int m = aleatorio();
        if (anterior >= 0 && (anterior ^ 1) == m) continue;
        e = funcao_sucessora(e, m);
        if (sequencia) sequencia[feitos] = m;
        anterior = m;
        feitos++;
    }
    return e;
}

Estado aplicar_sequencia(Estado estado, const int *passos, int n) {
    int i;
    for (i = 0; i < n; i++) estado = funcao_sucessora(estado, passos[i]);
    return estado;
}

const char *texto_heuristica(int tipo) {
    if (tipo == HEURISTICA_ADESIVOS)
        return
            "h(n) = teto(adesivos fora da cor / 12).\n"
            "Um giro move 12 adesivos, entao nao corrige mais que 12.\n"
            "E admissivel, porem mais fraca que a dos cantos.\n";
    if (tipo == HEURISTICA_ZERO)
        return
            "h(n) = 0. Admissivel, mas nao orienta a busca.\n"
            "O A* vira busca de custo uniforme, parecida com a largura.\n";
    return
        "h(n) = teto(cantos fora do lugar / 4).\n"
        "Um giro mexe em 4 cantos, entao nao corrige mais que 4.\n"
        "h nunca passa do custo real: e admissivel e consistente.\n"
        "No A*, f(n) = g(n) + h(n). A primeira solucao retirada e otima.\n";
}

static int par_x(int c) { return c == LARANJA || c == VERMELHO; }
static int par_y(int c) { return c == AMARELO || c == BRANCO; }
static int par_z(int c) { return c == AZUL || c == VERDE; }

int pecas_validas(Estado e) {
    int visto[8] = {0};
    int s;
    for (s = 0; s < 8; s++) {
        int xs = 0, ys = 0, zs = 0;
        int xcor = -1, ycor = -1, zcor = -1;
        int a, x, y, z, id;
        for (a = 0; a < 3; a++) {
            int c = e.cor[s][a];
            if (par_x(c)) { xs++; xcor = c; }
            else if (par_y(c)) { ys++; ycor = c; }
            else if (par_z(c)) { zs++; zcor = c; }
            else return 0;
        }
        if (xs != 1 || ys != 1 || zs != 1) return 0;
        x = xcor == VERMELHO ? 1 : -1;
        y = ycor == BRANCO ? 1 : -1;
        z = zcor == VERDE ? 1 : -1;
        id = slot(x, y, z);
        if (visto[id]) return 0;
        visto[id] = 1;
    }
    return 1;
}

static int contar_cor(Estado e, int cor) {
    int n = 0, s, a;
    for (s = 0; s < 8; s++)
        for (a = 0; a < 3; a++)
            if (e.cor[s][a] == cor) n++;
    return n;
}

const char *motor_erro(void) {
    Estado base = estado_resolvido();
    int m;
    if (!funcao_avaliadora(base)) return "o cubo resolvido nao passou na funcao avaliadora";
    if (!pecas_validas(base)) return "o cubo resolvido tem pecas invalidas";

    for (m = 0; m < NUM_MOVIMENTOS; m++) {
        Estado e = base;
        Estado ida, volta;
        int k, cor;
        for (k = 0; k < 4; k++) e = funcao_sucessora(e, m);
        if (memcmp(&e, &base, sizeof e) != 0)
            return "um giro quatro vezes nao voltou a identidade";
        ida = funcao_sucessora(base, m);
        volta = funcao_sucessora(ida, movimento_inverso(m));
        if (memcmp(&volta, &base, sizeof volta) != 0)
            return "um giro nao tem inverso";
        if (!pecas_validas(ida)) return "um giro quebrou uma peca";
        for (cor = 0; cor < 6; cor++)
            if (contar_cor(ida, cor) != 4) return "um giro nao conservou as cores";
    }

    {
        Estado u = funcao_sucessora(base, U);
        int ufl = slot(-1, 1, 1);
        if (u.cor[ufl][0] != VERDE || u.cor[ufl][1] != BRANCO || u.cor[ufl][2] != VERMELHO)
            return "orientacao do movimento U esta incorreta";
    }
    {
        Estado r = funcao_sucessora(base, R);
        int ubr = slot(1, 1, -1);
        if (r.cor[ubr][0] != VERMELHO || r.cor[ubr][1] != VERDE || r.cor[ubr][2] != BRANCO)
            return "orientacao do movimento R esta incorreta";
    }
    {
        Estado f = funcao_sucessora(base, F);
        int dfr = slot(1, -1, 1);
        if (f.cor[dfr][0] != BRANCO || f.cor[dfr][1] != VERMELHO || f.cor[dfr][2] != VERDE)
            return "orientacao do movimento F esta incorreta";
    }

    {
        int seq[30], a[8], b[8], i;
        Estado misturado = embaralhar(30, 12345u, seq);
        Estado desfaz = misturado;
        if (!pecas_validas(misturado)) return "embaralhamento gerou peca invalida";
        for (i = 29; i >= 0; i--) desfaz = funcao_sucessora(desfaz, movimento_inverso(seq[i]));
        if (!funcao_avaliadora(desfaz)) return "inverter o embaralhamento nao resolveu";
        embaralhar(8, 99u, a);
        embaralhar(8, 99u, b);
        if (memcmp(a, b, sizeof a) != 0) return "a mesma seed nao repetiu o embaralhamento";
    }
    return NULL;
}
