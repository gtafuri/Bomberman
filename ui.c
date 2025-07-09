#include <raylib.h>
#include "ui.h"

#define BLOCO 20
#define HUD_ALTURA 100

void desenharMapa(const Mapa *mapa) {
    for (int i = 0; i < mapa->linhas; i++) {
        for (int j = 0; j < mapa->colunas; j++) {
            Color cor = LIGHTGRAY;
            switch (mapa->matriz[i][j]) {
                case 'W': cor = DARKGRAY; break; // Parede indestrutível
                case 'D': cor = BROWN; break;    // Parede destrutível
                case 'K': cor = BEIGE; break;    // Caixa com chave
                case 'B': cor = YELLOW; break;   // Caixa sem chave
                case 'E': cor = RED; break;      // Inimigo
                case 'J': cor = BLUE; break;     // Jogador (posição inicial)
                case ' ': cor = RAYWHITE; break; // Espaço vazio
                default: cor = LIGHTGRAY; break;
            }
            DrawRectangle(j * BLOCO, i * BLOCO, BLOCO, BLOCO, cor);
            DrawRectangleLines(j * BLOCO, i * BLOCO, BLOCO, BLOCO, GRAY);
        }
    }
}

void desenharHUD(const Jogador *jogador) {
    int y = BLOCO * 25 + 10;
    DrawRectangle(0, BLOCO * 25, 1200, HUD_ALTURA, LIGHTGRAY);
    DrawText(TextFormat("Bombas: %d", jogador->bombas), 20, y, 30, DARKBLUE);
    DrawText(TextFormat("Vidas: %d", jogador->vidas), 220, y, 30, MAROON);
    DrawText(TextFormat("Pontuacao: %d", jogador->pontuacao), 420, y, 30, DARKGREEN);
    DrawText(TextFormat("Chaves: %d", jogador->chaves), 720, y, 30, GOLD);
} 
