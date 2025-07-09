#ifndef MAPA_H
#define MAPA_H

#include "structs.h"

Mapa* carregarMapa(const char *arquivo);
void liberarMapa(Mapa *mapa);

#endif // MAPA_H 
