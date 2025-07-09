#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "raylib.h"

// codigo unico, falta modularizar
// tipos e structs
typedef enum {
    VAZIO, JOGADOR, PAREDE_I, PAREDE_D, CAIXA_CHAVE, CAIXA_VAZIA, CHAVE, INIMIGO, BOMBA
} TipoCelula;

typedef struct {
    TipoCelula tipo;
} Celula;

typedef struct {
    Celula **grid;
    int linhas;
    int colunas;
} Mapa;

typedef struct {
    int linha;
    int coluna;
    int vidas;
    int bombas;
    int pontuacao;
} Jogador;

typedef struct {
    int linha;
    int coluna;
    float tempo; // segundos restantes
    int ativa;   // 1 se bomba está ativa
} Bomba;

// --- Funções de Mapa ---
Mapa *carregar_mapa_txt(const char *arquivo) {
    FILE *fp = fopen(arquivo, "r");
    if (!fp) return NULL;

    Mapa *m = malloc(sizeof(Mapa));
    m->linhas = 25;
    m->colunas = 60;

    m->grid = malloc(m->linhas * sizeof(Celula*));
    for (int i = 0; i < m->linhas; i++) {
        m->grid[i] = malloc(m->colunas * sizeof(Celula));
        for (int j = 0; j < m->colunas; j++) {
            char c = fgetc(fp);
            switch (c) {
                case 'W': m->grid[i][j].tipo = PAREDE_I; break;
                case 'D': m->grid[i][j].tipo = PAREDE_D; break;
                case 'B': m->grid[i][j].tipo = CAIXA_VAZIA; break;
                case 'K': m->grid[i][j].tipo = CAIXA_CHAVE; break;
                case 'J': m->grid[i][j].tipo = JOGADOR; break;
                case 'E': m->grid[i][j].tipo = INIMIGO; break;
                case '\n': j--; break;
                default: m->grid[i][j].tipo = VAZIO; break;
            }
        }
    }
    fclose(fp);
    return m;
}

void liberar_mapa(Mapa *m) {
    for (int i = 0; i < m->linhas; i++) {
        free(m->grid[i]);
    }
    free(m->grid);
    free(m);
}

void desenhar_mapa(Mapa *m) {
    for (int i = 0; i < m->linhas; i++) {
        for (int j = 0; j < m->colunas; j++) {
            Color cor;
            switch (m->grid[i][j].tipo) {
                case PAREDE_I: cor = DARKGRAY; break;
                case PAREDE_D: cor = GRAY; break;
                case CAIXA_VAZIA: cor = BROWN; break;
                case CAIXA_CHAVE: cor = GOLD; break;
                case INIMIGO: cor = RED; break;
                default: cor = BLACK; break;
            }
            DrawRectangle(j * 20, i * 20, 20, 20, cor);
        }
    }
}

Jogador inicializar_jogador(Mapa *m) {
    Jogador j = {0};
    for (int i = 0; i < m->linhas; i++) {
        for (int k = 0; k < m->colunas; k++) {
            if (m->grid[i][k].tipo == JOGADOR) {
                j.linha = i;
                j.coluna = k;
                j.vidas = 3;
                j.bombas = 3;
                j.pontuacao = 0;
                return j;
            }
        }
    }
    return j;
}

void desenhar_jogador(Jogador j) {
    DrawRectangle(j.coluna * 20, j.linha * 20, 20, 20, BLUE);
}

void mover_jogador(Jogador *j, Mapa *m, int dx, int dy) {
    int nova_linha = j->linha + dy;
    int nova_coluna = j->coluna + dx;
    if (nova_linha < 0 || nova_linha >= m->linhas || nova_coluna < 0 || nova_coluna >= m->colunas)
        return;

    TipoCelula destino = m->grid[nova_linha][nova_coluna].tipo;
    if (destino == VAZIO || destino == CHAVE) {
        j->linha = nova_linha;
        j->coluna = nova_coluna;
    }
}

void plantar_bomba(Bomba *b, Jogador j) {
    if (!b->ativa) {
        b->linha = j.linha;
        b->coluna = j.coluna;
        b->tempo = 3.0f;
        b->ativa = 1;
    }
}

void atualizar_bomba(Bomba *b, Mapa *m, float delta) {
    if (b->ativa) {
        b->tempo -= delta;
        if (b->tempo <= 0) {
            // Explosão simples: limpa a célula
            b->ativa = 0;
            // Remove qualquer caixa ou inimigo nas 4 direções
            int dir[5][2] = {{0,0},{0,1},{0,-1},{1,0},{-1,0}};
            for (int i = 0; i < 5; i++) {
                int y = b->linha + dir[i][0];
                int x = b->coluna + dir[i][1];
                if (y >= 0 && y < m->linhas && x >= 0 && x < m->colunas) {
                    if (m->grid[y][x].tipo == CAIXA_VAZIA || m->grid[y][x].tipo == CAIXA_CHAVE || m->grid[y][x].tipo == INIMIGO) {
                        m->grid[y][x].tipo = VAZIO;
                    }
                }
            }
        }
    }
}

void desenhar_bomba(Bomba b) {
    if (b.ativa)
        DrawCircle(b.coluna * 20 + 10, b.linha * 20 + 10, 8, ORANGE);
}


int main(void) {
    InitWindow(1200, 600, "Bomberman UFRJ");
    SetTargetFPS(60);

    Mapa *mapa = carregar_mapa_txt("maps/mapa1.txt");
    if (!mapa) {
        CloseWindow();
        printf("Erro ao carregar mapa.\n");
        return 1;
    }

    Jogador jogador = inicializar_jogador(mapa);
    Bomba bomba = {0};

    while (!WindowShouldClose()) {
        float delta = GetFrameTime();

        if (IsKeyPressed(KEY_W)) mover_jogador(&jogador, mapa, 0, -1);
        if (IsKeyPressed(KEY_S)) mover_jogador(&jogador, mapa, 0, 1);
        if (IsKeyPressed(KEY_A)) mover_jogador(&jogador, mapa, -1, 0);
        if (IsKeyPressed(KEY_D)) mover_jogador(&jogador, mapa, 1, 0);
        if (IsKeyPressed(KEY_B)) plantar_bomba(&bomba, jogador);

        atualizar_bomba(&bomba, mapa, delta);

        BeginDrawing();
        ClearBackground(BLACK);

        desenhar_mapa(mapa);
        desenhar_bomba(bomba);
        desenhar_jogador(jogador);

        EndDrawing();
    }

    liberar_mapa(mapa);
    CloseWindow();
    return 0;
}
