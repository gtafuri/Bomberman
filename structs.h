#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdbool.h>

// Representa uma posição no mapa
typedef struct {
    int x;
    int y;
} Posicao;

// Jogador
typedef struct {
    Posicao pos;
    int vidas;
    int bombas;
    int pontuacao;
    int chaves;
} Jogador;

// Bomba (lista encadeada)
typedef struct Bomba {
    Posicao pos;
    int tempo_restante; 
    bool ativa;
    struct Bomba *prox;
} Bomba;

// Inimigo (lista encadeada)
typedef struct Inimigo {
    Posicao pos;
    int direcao; // 0=up, 1=right, 2=down, 3=left
    bool ativo;
    struct Inimigo *prox;
} Inimigo;

// Mapa
typedef struct {
    char **matriz; // matriz dinâmica de caracteres
    int linhas;
    int colunas;
} Mapa;

#endif // STRUCTS_H 
