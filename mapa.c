#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mapa.h"

#define MAX_LINHA 256

Mapa* carregarMapa(const char *arquivo) {
    FILE *fp = fopen(arquivo, "r");
    if (!fp) return NULL;

    int linhas = 0, colunas = 0;
    char buffer[MAX_LINHA];

    // Descobrir número de linhas e colunas
    while (fgets(buffer, MAX_LINHA, fp)) {
        int len = strlen(buffer);
        if (buffer[len-1] == '\n') buffer[len-1] = '\0';
        if (colunas == 0) colunas = strlen(buffer);
        linhas++;
    }
    rewind(fp);

    Mapa *mapa = malloc(sizeof(Mapa));
    mapa->linhas = linhas;
    mapa->colunas = colunas;
    mapa->matriz = malloc(linhas * sizeof(char*));
    for (int i = 0; i < linhas; i++) {
        mapa->matriz[i] = malloc((colunas + 1) * sizeof(char));
        fgets(buffer, MAX_LINHA, fp);
        int len = strlen(buffer);
        if (buffer[len-1] == '\n') buffer[len-1] = '\0';
        strncpy(mapa->matriz[i], buffer, colunas);
        mapa->matriz[i][colunas] = '\0';
    }
    fclose(fp);
    return mapa;
}

void liberarMapa(Mapa *mapa) {
    if (!mapa) return;
    for (int i = 0; i < mapa->linhas; i++) {
        free(mapa->matriz[i]);
    }
    free(mapa->matriz);
    free(mapa);
} 
