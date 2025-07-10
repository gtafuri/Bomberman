#include <raylib.h>
#include "ui.h"

#define BLOCO 20
#define HUD_Y 500
#define HUD_ALTURA 100

void desenharMapa(const Mapa *mapa) {
    for (int i = 0; i < mapa->linhas; i++) {
        for (int j = 0; j < mapa->colunas; j++) {
            int y = i * BLOCO;
            if (y >= HUD_Y) continue; // Não desenhar abaixo do HUD
            Color cor = LIGHTGRAY;
            switch (mapa->matriz[i][j]) {
                case 'W': cor = DARKGRAY; break; // Parede indestrutível
                case 'D': cor = BROWN; break;    // Parede destrutível
                case 'K': cor = YELLOW; break;    // Caixa com chave
                case 'B': cor = YELLOW; break;   // Caixa sem chave
                case 'E': cor = RED; break;      // Inimigo
                case 'J': cor = RAYWHITE; break;     // Jogador (posição inicial)
                case ' ': cor = RAYWHITE; break; // Espaço vazio
                default: cor = LIGHTGRAY; break;
            }
            DrawRectangle(j * BLOCO, y, BLOCO, BLOCO, cor);
            DrawRectangleLines(j * BLOCO, y, BLOCO, BLOCO, GRAY);
        }
    }
}

void desenharHUD(const Jogador *jogador) {
    int y = HUD_Y + 10;
    DrawRectangle(0, HUD_Y, 1200, HUD_ALTURA, LIGHTGRAY);
    DrawText(TextFormat("Bombas: %d", jogador->bombas), 20, y, 30, DARKBLUE);
    DrawText(TextFormat("Vidas: %d", jogador->vidas), 220, y, 30, MAROON);
    DrawText(TextFormat("Pontuacao: %d", jogador->pontuacao), 420, y, 30, DARKGREEN);
    DrawText(TextFormat("Chaves: %d", jogador->chaves), 720, y, 30, GOLD);
} 
