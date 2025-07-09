#ifndef JOGO_H
#define JOGO_H

#include "structs.h"

void inicializarJogo(Jogador *jogador, Mapa *mapa);
void atualizarJogo(Jogador *jogador, Mapa *mapa);
void liberarJogo(Jogador *jogador, Mapa *mapa);

#endif 
