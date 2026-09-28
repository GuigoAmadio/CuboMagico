#include "tela.h"

#include "busca.h"
#include "estado.h"

#include "raylib.h"
#include "rlgl.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define FILA_MAX 64
#define HIST_MAX 64
#define LINHA_MAX 12
#define LINHA_TAM 180

static const Color FUNDO = {16, 18, 26, 255};
static const Color PAINEL = {28, 31, 42, 255};
static const Color BOTAO = {46, 52, 68, 255};
static const Color BOTAO_HOVER = {64, 74, 98, 255};
static const Color BOTAO_DESTAQUE = {52, 96, 168, 255};
static const Color TEXTO = {236, 238, 244, 255};
static const Color TEXTO_SUAVE = {168, 176, 196, 255};
static const Color PLASTICO = {12, 12, 14, 255};

typedef struct {
    int movimento;
    int anotar;
} Pedido;

typedef struct {
    Rectangle seed_menos, seed_mais;
    Rectangle passos_menos, passos_mais;
    Rectangle heuristica;
    Rectangle embaralhar, desfazer, reiniciar;
    Rectangle largura, profundidade, estrela, comparar;
    int x;
    int largura_px;
} Painel;

static Estado atual;
static int historico[HIST_MAX];
static int nhist;
static Pedido fila[FILA_MAX];
static int nfila;
static char linhas[LINHA_MAX][LINHA_TAM];
static int nlinhas;
static unsigned seed = 42;
static int passos = 5;
static int heuristica = HEURISTICA_CANTOS;
static int animando = 0;
static Pedido giro_atual;
static float anim_t = 0.f;
static float duracao = 0.22f;
static float yaw = 0.85f;
static float pitch = 0.42f;
static float distancia = 6.6f;

static int ocupado(void) {
    return animando || nfila > 0;
}

static void linhas_limpar(void) {
    nlinhas = 0;
}

static void linhas_por(const char *texto) {
    size_t n;
    if (!texto || !texto[0] || nlinhas >= LINHA_MAX) return;
    n = strlen(texto);
    if (n >= LINHA_TAM) n = LINHA_TAM - 1;
    memcpy(linhas[nlinhas], texto, n);
    linhas[nlinhas][n] = 0;
    nlinhas++;
}

static void enfileirar(int movimento, int anotar) {
    if (nfila >= FILA_MAX) return;
    fila[nfila].movimento = movimento;
    fila[nfila].anotar = anotar;
    nfila++;
}

static int slot_de(int x, int y, int z) {
    return (x > 0) | ((y > 0) << 1) | ((z > 0) << 2);
}

static Color cor_raylib(int cor) {
    switch (cor) {
    case BRANCO: return (Color){245, 245, 245, 255};
    case AMARELO: return (Color){255, 213, 0, 255};
    case VERDE: return (Color){0, 155, 72, 255};
    case AZUL: return (Color){0, 70, 173, 255};
    case VERMELHO: return (Color){183, 18, 52, 255};
    case LARANJA: return (Color){255, 88, 0, 255};
    default: return MAGENTA;
    }
}

static Vector3 deslocar(Vector3 ponto, Vector3 direcao, float quanto) {
    Vector3 r;
    r.x = ponto.x + direcao.x * quanto;
    r.y = ponto.y + direcao.y * quanto;
    r.z = ponto.z + direcao.z * quanto;
    return r;
}

static void desenhar_adesivo(Vector3 centro, Vector3 normal, float alcance, Color cor) {
    float borda = 0.80f;
    float miolo = 0.66f;
    float t = 0.05f;
    Vector3 base = deslocar(centro, normal, alcance * 0.5f);
    Vector3 pb = deslocar(base, normal, t * 0.55f);
    Vector3 pm = deslocar(base, normal, t * 1.15f);
    if (normal.x != 0) {
        DrawCube(pb, t, borda, borda, BLACK);
        DrawCube(pm, t * 0.45f, miolo, miolo, cor);
    } else if (normal.y != 0) {
        DrawCube(pb, borda, t, borda, BLACK);
        DrawCube(pm, miolo, t * 0.45f, miolo, cor);
    } else {
        DrawCube(pb, borda, borda, t, BLACK);
        DrawCube(pm, miolo, miolo, t * 0.45f, cor);
    }
}

static void desenhar_peca(int x, int y, int z) {
    int s = slot_de(x, y, z);
    float alcance = 0.92f;
    Vector3 centro = {x * 0.54f, y * 0.54f, z * 0.54f};
    DrawCube(centro, alcance, alcance, alcance, PLASTICO);
    desenhar_adesivo(centro, (Vector3){(float)x, 0, 0}, alcance, cor_raylib(atual.cor[s][0]));
    desenhar_adesivo(centro, (Vector3){0, (float)y, 0}, alcance, cor_raylib(atual.cor[s][1]));
    desenhar_adesivo(centro, (Vector3){0, 0, (float)z}, alcance, cor_raylib(atual.cor[s][2]));
}

static void desenhar_cubo(int eixo, int camada, float graus) {
    Vector3 axis = {eixo == 0 ? 1.f : 0.f, eixo == 1 ? 1.f : 0.f, eixo == 2 ? 1.f : 0.f};
    int x, y, z;
    for (x = -1; x <= 1; x += 2)
        for (y = -1; y <= 1; y += 2)
            for (z = -1; z <= 1; z += 2) {
                int valor = eixo == 0 ? x : eixo == 1 ? y : z;
                int gira = eixo >= 0 && valor == camada && graus != 0.f;
                if (gira) {
                    rlPushMatrix();
                    rlRotatef(graus, axis.x, axis.y, axis.z);
                }
                desenhar_peca(x, y, z);
                if (gira) rlPopMatrix();
            }
}

static void giro_visual(int movimento, int *eixo, int *camada, float *graus) {
    *eixo = 1;
    *camada = 1;
    *graus = 0.f;
    switch (movimento) {
    case U: *eixo = 1; *camada = 1; *graus = -90.f; break;
    case U_PRIMA: *eixo = 1; *camada = 1; *graus = 90.f; break;
    case D: *eixo = 1; *camada = -1; *graus = 90.f; break;
    case D_PRIMA: *eixo = 1; *camada = -1; *graus = -90.f; break;
    case R: *eixo = 0; *camada = 1; *graus = 90.f; break;
    case R_PRIMA: *eixo = 0; *camada = 1; *graus = -90.f; break;
    case L: *eixo = 0; *camada = -1; *graus = -90.f; break;
    case L_PRIMA: *eixo = 0; *camada = -1; *graus = 90.f; break;
    case F: *eixo = 2; *camada = 1; *graus = -90.f; break;
    case F_PRIMA: *eixo = 2; *camada = 1; *graus = 90.f; break;
    case B: *eixo = 2; *camada = -1; *graus = 90.f; break;
    case B_PRIMA: *eixo = 2; *camada = -1; *graus = -90.f; break;
    default: break;
    }
}

static Font carregar_fonte(int *propria) {
    const char *extras = "áàãâéêíóôõúüçÁÀÃÂÉÊÍÓÔÕÚÜÇ";
    int nextra = 0;
    int *extra = LoadCodepoints(extras, &nextra);
    int cps[160];
    int n = 0;
    int i;
    const char *caminhos[2] = {
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf"
    };
    *propria = 0;
    for (i = 32; i <= 126 && n < 160; i++) cps[n++] = i;
    for (i = 0; i < nextra && n < 160; i++) cps[n++] = extra[i];
    UnloadCodepoints(extra);
    for (i = 0; i < 2; i++) {
        Font fonte;
        if (!FileExists(caminhos[i])) continue;
        fonte = LoadFontEx(caminhos[i], 48, cps, n);
        if (fonte.texture.id != 0) {
            SetTextureFilter(fonte.texture, TEXTURE_FILTER_BILINEAR);
            *propria = 1;
            return fonte;
        }
    }
    return GetFontDefault();
}

static void escrever(Font fonte, const char *texto, float x, float y, float tamanho, Color cor) {
    DrawTextEx(fonte, texto, (Vector2){x, y}, tamanho, 0.4f, cor);
}

static int mouse_sobre(Rectangle r) {
    return CheckCollisionPointRec(GetMousePosition(), r);
}

static int clique(Rectangle r) {
    return IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && mouse_sobre(r);
}

static void desenhar_botao(Font fonte, Rectangle r, const char *titulo, int destaque, int ativo) {
    Color fundo = destaque ? BOTAO_DESTAQUE : BOTAO;
    Color cor_texto = ativo ? TEXTO : (Color){120, 124, 136, 255};
    Vector2 med;
    if (!ativo) fundo = (Color){36, 38, 46, 255};
    else if (mouse_sobre(r)) fundo = destaque ? (Color){72, 120, 198, 255} : BOTAO_HOVER;
    DrawRectangleRounded(r, 0.18f, 8, fundo);
    med = MeasureTextEx(fonte, titulo, 20.f, 0.4f);
    escrever(fonte, titulo, r.x + (r.width - med.x) * 0.5f, r.y + (r.height - med.y) * 0.5f, 20.f, cor_texto);
}

static void tempo_texto(double ms, char *saida, int tam) {
    if (ms < 0.5) snprintf(saida, (size_t)tam, "< 1 ms");
    else snprintf(saida, (size_t)tam, "%.0f ms", ms);
}

static void linhas_resultado(const char *nome, ResultadoBusca r) {
    char buf[LINHA_TAM];
    char notacao[LINHA_TAM];
    linhas_limpar();
    linhas_por(nome);
    if (r.interrompida) {
        linhas_por("Busca interrompida.");
        linhas_por("Use menos giros no embaralhamento.");
        return;
    }
    if (!r.encontrou) {
        linhas_por("Sem solução");
        return;
    }
    snprintf(buf, sizeof buf, "Movimentos: %d", r.npassos);
    linhas_por(buf);
    snprintf(buf, sizeof buf, "Estados visitados: %lld", r.visitados);
    linhas_por(buf);
    {
        char tempo[40];
        tempo_texto(r.milissegundos, tempo, (int)sizeof tempo);
        snprintf(buf, sizeof buf, "Tempo: %s", tempo);
        linhas_por(buf);
    }
    formatar_notacao(r.passos, r.npassos, notacao, (int)sizeof notacao);
    linhas_por(notacao);
}

static OpcoesBusca opcoes_silenciosas(void) {
    OpcoesBusca op;
    memset(&op, 0, sizeof op);
    op.limite = 14;
    op.heuristica = heuristica;
    op.max_visitados = 2000000;
    return op;
}

static void mostrar_espera(Font fonte, const char *mensagem) {
    BeginDrawing();
    ClearBackground(FUNDO);
    escrever(fonte, mensagem, 48, 48, 32, TEXTO);
    escrever(fonte, "A janela volta sozinha quando a busca terminar.", 48, 96, 22, TEXTO_SUAVE);
    EndDrawing();
}

static void embaralhar_animado(void) {
    int seq[16];
    char nota[LINHA_TAM];
    char titulo[LINHA_TAM];
    int i;
    if (ocupado()) return;
    atual = embaralhar(passos, seed, seq);
    atual = estado_resolvido();
    nhist = 0;
    nfila = 0;
    for (i = 0; i < passos && nfila < FILA_MAX; i++) enfileirar(seq[i], 1);
    linhas_limpar();
    snprintf(titulo, sizeof titulo, "Embaralhamento (seed %u)", seed);
    linhas_por(titulo);
    formatar_notacao(seq, passos, nota, (int)sizeof nota);
    linhas_por(nota);
    duracao = 0.22f;
}

static void desfazer(void) {
    if (ocupado() || nhist <= 0) return;
    enfileirar(movimento_inverso(historico[nhist - 1]), 0);
}

static void reiniciar(void) {
    if (animando) return;
    nfila = 0;
    nhist = 0;
    atual = estado_resolvido();
    linhas_limpar();
    linhas_por("Cubo resolvido.");
}

static void concluir_giro(void) {
    atual = funcao_sucessora(atual, giro_atual.movimento);
    if (giro_atual.anotar) {
        if (nhist < HIST_MAX) historico[nhist++] = giro_atual.movimento;
    } else if (nhist > 0) {
        nhist--;
    }
    animando = 0;
    anim_t = 0.f;
    if (nfila == 0 && funcao_avaliadora(atual)) linhas_por("Cubo resolvido.");
}

static void atualizar_animacao(float dt) {
    if (!animando) {
        if (nfila == 0) return;
        giro_atual = fila[0];
        memmove(fila, fila + 1, (size_t)(nfila - 1) * sizeof(Pedido));
        nfila--;
        animando = 1;
        anim_t = 0.f;
    }
    anim_t += dt;
    if (anim_t >= duracao) concluir_giro();
}

static void animar_passos(const ResultadoBusca *r) {
    int i;
    if (!r->encontrou) return;
    for (i = 0; i < r->npassos; i++) enfileirar(r->passos[i], 1);
    duracao = 0.16f;
}

static void resolver(int qual, Font fonte) {
    OpcoesBusca op;
    if (ocupado()) return;
    op = opcoes_silenciosas();
    if (qual == 0) {
        ResultadoBusca r;
        mostrar_espera(fonte, "Busca em largura...");
        r = busca_largura(atual, op);
        linhas_resultado("Busca em largura", r);
        animar_passos(&r);
    } else if (qual == 1) {
        ResultadoBusca r;
        mostrar_espera(fonte, "Busca em profundidade limitada iterativa...");
        r = busca_profundidade(atual, op);
        linhas_resultado("Profundidade limitada iterativa", r);
        animar_passos(&r);
    } else if (qual == 2) {
        ResultadoBusca r;
        char hlinha[LINHA_TAM];
        int h;
        mostrar_espera(fonte, "Busca A*...");
        h = funcao_heuristica(atual, heuristica);
        r = busca_estrela(atual, op);
        linhas_resultado("Busca A*", r);
        snprintf(hlinha, sizeof hlinha, "h(n) = %d  (%s)", h, nome_heuristica(heuristica));
        if (nlinhas < LINHA_MAX) {
            memmove(linhas[2], linhas[1], (size_t)(nlinhas - 1) * LINHA_TAM);
            snprintf(linhas[1], LINHA_TAM, "%s", hlinha);
            nlinhas++;
        }
        animar_passos(&r);
    } else {
        ResultadoBusca largura, prof, estrela;
        char buf[LINHA_TAM];
        char tempo[40];
        mostrar_espera(fonte, "Comparando as três buscas...");
        largura = busca_largura(atual, op);
        prof = busca_profundidade(atual, op);
        estrela = busca_estrela(atual, opcoes_silenciosas());
        linhas_limpar();
        linhas_por("Comparação na mesma posição");
        if (largura.encontrou) {
            tempo_texto(largura.milissegundos, tempo, (int)sizeof tempo);
            snprintf(buf, sizeof buf, "Largura: %d mov., %lld estados, %s",
                     largura.npassos, largura.visitados, tempo);
        } else snprintf(buf, sizeof buf, "Largura: sem solução");
        linhas_por(buf);
        if (prof.encontrou) {
            tempo_texto(prof.milissegundos, tempo, (int)sizeof tempo);
            snprintf(buf, sizeof buf, "Profundidade: %d mov., %lld estados, %s",
                     prof.npassos, prof.visitados, tempo);
        } else snprintf(buf, sizeof buf, "Profundidade: sem solução");
        linhas_por(buf);
        if (estrela.encontrou) {
            tempo_texto(estrela.milissegundos, tempo, (int)sizeof tempo);
            snprintf(buf, sizeof buf, "A*: %d mov., %lld estados, %s",
                     estrela.npassos, estrela.visitados, tempo);
        } else snprintf(buf, sizeof buf, "A*: sem solução");
        linhas_por(buf);
        if (largura.encontrou && prof.encontrou && estrela.encontrou &&
            largura.npassos == prof.npassos && largura.npassos == estrela.npassos) {
            snprintf(buf, sizeof buf, "As três acharam %d movimentos.", largura.npassos);
            linhas_por(buf);
        }
        if (estrela.encontrou) animar_passos(&estrela);
        else animar_passos(&largura);
    }
}

static void ler_teclas(void) {
    int shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    if (IsKeyPressed(KEY_U)) enfileirar(shift ? U_PRIMA : U, 1);
    if (IsKeyPressed(KEY_R)) enfileirar(shift ? R_PRIMA : R, 1);
    if (IsKeyPressed(KEY_F)) enfileirar(shift ? F_PRIMA : F, 1);
    if (IsKeyPressed(KEY_D)) enfileirar(shift ? D_PRIMA : D, 1);
    if (IsKeyPressed(KEY_L)) enfileirar(shift ? L_PRIMA : L, 1);
    if (IsKeyPressed(KEY_B)) enfileirar(shift ? B_PRIMA : B, 1);
    if (IsKeyPressed(KEY_ESCAPE)) nfila = 0;
}

static void atualizar_camera(int pode) {
    if (pode && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 delta = GetMouseDelta();
        yaw -= delta.x * 0.01f;
        pitch -= delta.y * 0.008f;
        if (pitch > 1.15f) pitch = 1.15f;
        if (pitch < -1.15f) pitch = -1.15f;
    }
    if (pode) {
        distancia -= GetMouseWheelMove() * 0.38f;
        if (distancia < 3.4f) distancia = 3.4f;
        if (distancia > 14.f) distancia = 14.f;
    }
}

static Camera3D camera_de(void) {
    Camera3D camera;
    camera.position.x = distancia * cosf(pitch) * sinf(yaw);
    camera.position.y = distancia * sinf(pitch);
    camera.position.z = distancia * cosf(pitch) * cosf(yaw);
    camera.target = (Vector3){0, 0, 0};
    camera.up = (Vector3){0, 1, 0};
    camera.fovy = 38.f;
    camera.projection = CAMERA_PERSPECTIVE;
    return camera;
}

static Rectangle faixa(float x, float y, float w, float h) {
    Rectangle r;
    r.x = x;
    r.y = y;
    r.width = w;
    r.height = h;
    return r;
}

static Painel montar_painel(int tela_l) {
    Painel p;
    float x, w;
    memset(&p, 0, sizeof p);
    p.largura_px = 330;
    p.x = tela_l - p.largura_px;
    x = (float)(p.x + 18);
    w = (float)(p.largura_px - 36);
    p.seed_menos = faixa(x, 118, 44, 36);
    p.seed_mais = faixa(x + w - 44, 118, 44, 36);
    p.passos_menos = faixa(x, 168, 44, 36);
    p.passos_mais = faixa(x + w - 44, 168, 44, 36);
    p.heuristica = faixa(x, 214, w, 40);
    p.embaralhar = faixa(x, 270, w, 42);
    p.desfazer = faixa(x, 320, w, 42);
    p.reiniciar = faixa(x, 370, w, 42);
    p.largura = faixa(x, 436, w, 44);
    p.profundidade = faixa(x, 488, w, 44);
    p.estrela = faixa(x, 540, w, 44);
    p.comparar = faixa(x, 592, w, 44);
    return p;
}

static void quebrar_texto(const char *texto) {
    char copia[600];
    char *p;
    snprintf(copia, sizeof copia, "%s", texto);
    p = copia;
    while (*p && nlinhas < LINHA_MAX) {
        char *nl = strchr(p, '\n');
        if (nl) *nl = 0;
        linhas_por(p);
        if (!nl) break;
        p = nl + 1;
    }
}

void executar_interface_3d(void) {
    int fonte_propria = 0;
    Font fonte;
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(1240, 780, "Cubo mágico 2x2x2");
    SetWindowMinSize(980, 680);
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    fonte = carregar_fonte(&fonte_propria);
    printf("Janela 3D aberta. Feche a janela para sair.\n");
    printf("Modo texto: .\\cubo.exe --console\n");
    fflush(stdout);

    atual = estado_resolvido();
    linhas_limpar();
    linhas_por("Arraste o cubo para olhar em volta.");
    linhas_por("U R F D L B giram a face.");
    linhas_por("Shift + letra = anti-horário.");
    linhas_por("Espaco embaralha. Backspace desfaz.");

    while (!WindowShouldClose()) {
        Painel painel = montar_painel(GetScreenWidth());
        int mouse_no_painel = GetMouseX() >= painel.x;
        int livre = !ocupado();
        int eixo = -1, camada = 0;
        float graus = 0.f;
        Camera3D camera;
        float x;
        int i;
        float y;

        atualizar_camera(!mouse_no_painel);
        ler_teclas();
        atualizar_animacao(GetFrameTime());

        if (IsKeyPressed(KEY_SPACE) && livre) embaralhar_animado();
        if (IsKeyPressed(KEY_BACKSPACE)) desfazer();
        if (IsKeyPressed(KEY_ONE) && livre) resolver(0, fonte);
        if (IsKeyPressed(KEY_TWO) && livre) resolver(1, fonte);
        if (IsKeyPressed(KEY_THREE) && livre) resolver(2, fonte);
        if (IsKeyPressed(KEY_FOUR) && livre) resolver(3, fonte);

        if (livre && clique(painel.seed_menos) && seed > 0) {
            seed--;
            embaralhar_animado();
        }
        if (livre && clique(painel.seed_mais)) {
            seed++;
            embaralhar_animado();
        }
        if (livre && clique(painel.passos_menos) && passos > 1) {
            passos--;
            embaralhar_animado();
        }
        if (livre && clique(painel.passos_mais) && passos < 7) {
            passos++;
            embaralhar_animado();
        }
        if (livre && clique(painel.heuristica)) {
            if (heuristica == HEURISTICA_CANTOS) heuristica = HEURISTICA_ADESIVOS;
            else if (heuristica == HEURISTICA_ADESIVOS) heuristica = HEURISTICA_ZERO;
            else heuristica = HEURISTICA_CANTOS;
            linhas_limpar();
            {
                char buf[LINHA_TAM];
                snprintf(buf, sizeof buf, "Heurística: %s", nome_heuristica(heuristica));
                linhas_por(buf);
            }
            quebrar_texto(texto_heuristica(heuristica));
        }
        if (livre && clique(painel.embaralhar)) embaralhar_animado();
        if (clique(painel.desfazer)) desfazer();
        if (livre && clique(painel.reiniciar)) reiniciar();
        if (livre && clique(painel.largura)) resolver(0, fonte);
        if (livre && clique(painel.profundidade)) resolver(1, fonte);
        if (livre && clique(painel.estrela)) resolver(2, fonte);
        if (livre && clique(painel.comparar)) resolver(3, fonte);

        if (animando) {
            float t;
            giro_visual(giro_atual.movimento, &eixo, &camada, &graus);
            t = anim_t / duracao;
            if (t > 1.f) t = 1.f;
            graus *= t * t * (3.f - 2.f * t);
        }

        camera = camera_de();
        BeginDrawing();
        ClearBackground(FUNDO);
        BeginMode3D(camera);
        DrawCylinder((Vector3){0, -1.55f, 0}, 2.1f, 2.1f, 0.04f, 48, (Color){8, 8, 12, 255});
        desenhar_cubo(eixo, camada, graus);
        EndMode3D();

        DrawRectangle(painel.x, 0, painel.largura_px, GetScreenHeight(), PAINEL);
        x = (float)(painel.x + 18);
        escrever(fonte, "Cubo 2x2x2", x, 22, 30, TEXTO);
        escrever(fonte, "Branco em cima, verde na frente", x, 58, 16, TEXTO_SUAVE);
        escrever(fonte, "Vermelho na direita", x, 78, 16, TEXTO_SUAVE);

        {
            char buf[40];
            snprintf(buf, sizeof buf, "Seed %u", seed);
            escrever(fonte, buf, x + 58, 124, 22, TEXTO);
            snprintf(buf, sizeof buf, "Passos %d", passos);
            escrever(fonte, buf, x + 52, 174, 22, TEXTO);
        }
        desenhar_botao(fonte, painel.seed_menos, "-", 0, livre);
        desenhar_botao(fonte, painel.seed_mais, "+", 0, livre);
        desenhar_botao(fonte, painel.passos_menos, "-", 0, livre && passos > 1);
        desenhar_botao(fonte, painel.passos_mais, "+", 0, livre && passos < 7);
        desenhar_botao(fonte, painel.heuristica, nome_heuristica(heuristica), 0, livre);
        desenhar_botao(fonte, painel.embaralhar, "Embaralhar", 0, livre);
        desenhar_botao(fonte, painel.desfazer, "Desfazer", 0, livre && nhist > 0);
        desenhar_botao(fonte, painel.reiniciar, "Reiniciar", 0, livre);
        desenhar_botao(fonte, painel.largura, "Busca em largura", 1, livre);
        desenhar_botao(fonte, painel.profundidade, "Profundidade iterativa", 1, livre);
        desenhar_botao(fonte, painel.estrela, "Busca A*", 1, livre);
        desenhar_botao(fonte, painel.comparar, "Comparar as três", 1, livre);

        y = 650.f;
        for (i = 0; i < nlinhas; i++) {
            if (y > GetScreenHeight() - 28) break;
            escrever(fonte, linhas[i], x, y, 16, TEXTO_SUAVE);
            y += 20.f;
        }

        escrever(fonte, "Arraste para girar a câmera   roda do mouse aproxima", 20, 18, 16, TEXTO_SUAVE);
        escrever(fonte, "U R F D L B giram    Shift = anti-horário    Esc cancela a fila", 20, 40, 16, TEXTO_SUAVE);
        if (funcao_avaliadora(atual) && !ocupado())
            escrever(fonte, "Cubo resolvido", 20, (float)(GetScreenHeight() - 42), 28, (Color){120, 220, 140, 255});
        else if (ocupado())
            escrever(fonte, "Girando...", 20, (float)(GetScreenHeight() - 42), 28, (Color){255, 206, 84, 255});

        EndDrawing();
    }

    if (fonte_propria) UnloadFont(fonte);
    CloseWindow();
}
