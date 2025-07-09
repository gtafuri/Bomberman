#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "structs.h"
#include "jogo.h"
#include "mapa.h"
#include "ui.h"
#include <time.h>

#define MAPA_ARQUIVO "maps/mapa1.txt"
#define BLOCO 20
#define HUD_ALTURA 100
#define TEMPO_BOMBA 180 // 3 segundos a 60 FPS
#define RAIO_EXPLOSAO 5
#define INTERVALO_INIMIGO 10 // frames para mudar direção (mais rápido)

#define SAVE_FILE "savegame.bin"

// Função auxiliar para encontrar posição inicial do jogador
void encontrarPosicaoJogador(const Mapa *mapa, Jogador *jogador) {
    for (int i = 0; i < mapa->linhas; i++) {
        for (int j = 0; j < mapa->colunas; j++) {
            if (mapa->matriz[i][j] == 'J') {
                jogador->pos.x = j;
                jogador->pos.y = i;
                return;
            }
        }
    }
}

// Verifica se a posição é válida para o jogador andar
int podeMover(const Mapa *mapa, int x, int y) {
    if (x < 0 || y < 0 || x >= mapa->colunas || y >= mapa->linhas)
        return 0;
    char c = mapa->matriz[y][x];
    // Não pode atravessar paredes nem caixas
    if (c == 'W' || c == 'D' || c == 'K' || c == 'B')
        return 0;
    return 1;
}

// Adiciona bomba na lista
void plantarBomba(Bomba **lista, int x, int y) {
    Bomba *nova = malloc(sizeof(Bomba));
    nova->pos.x = x;
    nova->pos.y = y;
    nova->tempo_restante = TEMPO_BOMBA;
    nova->ativa = true;
    nova->prox = *lista;
    *lista = nova;
}

// Função para processar explosão da bomba
void explodirBomba(Bomba *b, Jogador *jogador, Mapa *mapa) {
    int x0 = b->pos.x;
    int y0 = b->pos.y;
    // Explode na posição central
    // (Efeito: desenhar explosão por 1 frame)
    DrawRectangle(x0 * BLOCO, y0 * BLOCO, BLOCO, BLOCO, ORANGE);
    // Explode nas 4 direções
    for (int d = 0; d < 4; d++) {
        int dx = 0, dy = 0;
        if (d == 0) dx = 1;   // Direita
        if (d == 1) dx = -1;  // Esquerda
        if (d == 2) dy = 1;   // Baixo
        if (d == 3) dy = -1;  // Cima
        for (int r = 1; r <= RAIO_EXPLOSAO; r++) {
            int x = x0 + dx * r;
            int y = y0 + dy * r;
            if (x < 0 || y < 0 || x >= mapa->colunas || y >= mapa->linhas) break;
            char *c = &mapa->matriz[y][x];
            if (*c == 'W') break; // Parede indestrutível bloqueia
            if (*c == 'D') {
                *c = ' ';
                jogador->pontuacao += 10;
                break; // Parede destrutível bloqueia
            }
            if (*c == 'K') {
                *c = 'C'; // Chave revelada
                jogador->pontuacao += 10;
                break; // Caixa com chave bloqueia
            }
            if (*c == 'B') {
                *c = ' ';
                jogador->pontuacao += 10;
                break; // Caixa sem chave bloqueia
            }
            if (*c == 'E') {
                *c = ' ';
                jogador->pontuacao += 20;
                // Não bloqueia, explosão continua
            }
            // Efeito visual
            DrawRectangle(x * BLOCO, y * BLOCO, BLOCO, BLOCO, ORANGE);
        }
    }
}

// Verifica se o jogador está na área de explosão da bomba
int jogadorAtingidoPorExplosao(const Bomba *b, const Jogador *jogador, const Mapa *mapa) {
    int x0 = b->pos.x;
    int y0 = b->pos.y;
    // Centro
    if (jogador->pos.x == x0 && jogador->pos.y == y0) return 1;
    // 4 direções
    for (int d = 0; d < 4; d++) {
        int dx = 0, dy = 0;
        if (d == 0) dx = 1;
        if (d == 1) dx = -1;
        if (d == 2) dy = 1;
        if (d == 3) dy = -1;
        for (int r = 1; r <= RAIO_EXPLOSAO; r++) {
            int x = x0 + dx * r;
            int y = y0 + dy * r;
            if (x < 0 || y < 0 || x >= mapa->colunas || y >= mapa->linhas) break;
            char c = mapa->matriz[y][x];
            if (c == 'W' || c == 'D' || c == 'K' || c == 'B') break;
            if (jogador->pos.x == x && jogador->pos.y == y) return 1;
        }
    }
    return 0;
}

// Atualiza bombas: decrementa timer, explode e remove se necessário
void atualizarBombas(Bomba **lista, Jogador *jogador, Mapa *mapa, int *invencivel) {
    Bomba **ptr = lista;
    while (*ptr) {
        Bomba *b = *ptr;
        b->tempo_restante--;
        if (b->tempo_restante <= 0) {
            // Explodir bomba (real)
            explodirBomba(b, jogador, mapa);
            // Verifica se jogador está na explosão
            if (*invencivel == 0 && jogadorAtingidoPorExplosao(b, jogador, mapa)) {
                jogador->vidas--;
                *invencivel = 60;
            }
            jogador->bombas++;
            *ptr = b->prox;
            free(b);
        } else {
            ptr = &((*ptr)->prox);
        }
    }
}

// Desenha bombas no mapa
void desenharBombas(const Bomba *lista) {
    while (lista) {
        DrawCircle(lista->pos.x * BLOCO + BLOCO/2, lista->pos.y * BLOCO + BLOCO/2, BLOCO/2 - 2, BLACK);
        lista = lista->prox;
    }
}

// No loop principal, desenhar chaves reveladas
void desenharChaves(const Mapa *mapa) {
    for (int i = 0; i < mapa->linhas; i++) {
        for (int j = 0; j < mapa->colunas; j++) {
            if (mapa->matriz[i][j] == 'C') {
                DrawCircle(j * BLOCO + BLOCO/2, i * BLOCO + BLOCO/2, BLOCO/3, GOLD);
            }
        }
    }
}

// No loop principal, permitir coleta de chave
void coletarChave(Jogador *jogador, Mapa *mapa) {
    if (mapa->matriz[jogador->pos.y][jogador->pos.x] == 'C') {
        jogador->chaves++;
        mapa->matriz[jogador->pos.y][jogador->pos.x] = ' ';
    }
}

// Inicializa lista de inimigos a partir do mapa
Inimigo* inicializarInimigos(const Mapa *mapa) {
    Inimigo *lista = NULL;
    for (int i = 0; i < mapa->linhas; i++) {
        for (int j = 0; j < mapa->colunas; j++) {
            if (mapa->matriz[i][j] == 'E') {
                Inimigo *novo = malloc(sizeof(Inimigo));
                novo->pos.x = j;
                novo->pos.y = i;
                novo->direcao = GetRandomValue(0, 3);
                novo->ativo = true;
                novo->prox = lista;
                lista = novo;
            }
        }
    }
    return lista;
}

// Verifica se a posição é válida para o inimigo andar
int podeMoverInimigo(const Mapa *mapa, int x, int y) {
    if (x < 0 || y < 0 || x >= mapa->colunas || y >= mapa->linhas)
        return 0;
    char c = mapa->matriz[y][x];
    if (c == 'W' || c == 'D' || c == 'K' || c == 'B')
        return 0;
    return 1;
}

// Limpa todos os 'E' do mapa
void limparInimigosMapa(Mapa *mapa) {
    for (int i = 0; i < mapa->linhas; i++)
        for (int j = 0; j < mapa->colunas; j++)
            if (mapa->matriz[i][j] == 'E')
                mapa->matriz[i][j] = ' ';
}

// Movimenta inimigos aleatoriamente e atualiza o mapa
void atualizarInimigos(Inimigo *lista, Mapa *mapa, int frame) {
    for (Inimigo *ini = lista; ini; ini = ini->prox) {
        if (!ini->ativo) continue;
        if (frame % INTERVALO_INIMIGO == 0) {
            ini->direcao = GetRandomValue(0, 3);
        }
        int dx = 0, dy = 0;
        if (ini->direcao == 0) dy = -1;
        if (ini->direcao == 1) dx = 1;
        if (ini->direcao == 2) dy = 1;
        if (ini->direcao == 3) dx = -1;
        int nx = ini->pos.x + dx;
        int ny = ini->pos.y + dy;
        if (podeMoverInimigo(mapa, nx, ny)) {
            mapa->matriz[ini->pos.y][ini->pos.x] = ' ';
            ini->pos.x = nx;
            ini->pos.y = ny;
            mapa->matriz[ny][nx] = 'E';
        } else {
            ini->direcao = GetRandomValue(0, 3);
        }
    }
}

// Remove inimigos atingidos por explosão
void removerInimigosExplodidos(Inimigo **lista, const Mapa *mapa) {
    Inimigo **ptr = lista;
    while (*ptr) {
        Inimigo *ini = *ptr;
        if (mapa->matriz[ini->pos.y][ini->pos.x] != 'E') {
            *ptr = ini->prox;
            free(ini);
        } else {
            ptr = &((*ptr)->prox);
        }
    }
}

// Desenha inimigos
void desenharInimigos(const Inimigo *lista) {
    for (const Inimigo *ini = lista; ini; ini = ini->prox) {
        if (ini->ativo)
            DrawRectangle(ini->pos.x * BLOCO, ini->pos.y * BLOCO, BLOCO, BLOCO, RED);
    }
}

// Verifica colisão jogador-inimigo 
//Nao tá funcionando, mas nao consigo pensar em outra logica socorro
void checarColisaoJogadorInimigo(Jogador *jogador, Inimigo *lista, int *invencivel) {
    for (Inimigo *ini = lista; ini; ini = ini->prox) {
        if (ini->ativo && ini->pos.x == jogador->pos.x && ini->pos.y == jogador->pos.y && *invencivel == 0) {
            jogador->vidas--;
            *invencivel = 60; // 1 segundo de invencibilidade
        }
    }
}

// Função para salvar o jogo
void salvarJogo(const Jogador *jogador, const Mapa *mapa, const Bomba *bombas, const Inimigo *inimigos) {
    FILE *f = fopen(SAVE_FILE, "wb");
    if (!f) return;
    fwrite(jogador, sizeof(Jogador), 1, f);
    fwrite(&mapa->linhas, sizeof(int), 1, f);
    fwrite(&mapa->colunas, sizeof(int), 1, f);
    for (int i = 0; i < mapa->linhas; i++)
        fwrite(mapa->matriz[i], sizeof(char), mapa->colunas, f);
    // Bombas
    int nb = 0;
    for (const Bomba *b = bombas; b; b = b->prox) nb++;
    fwrite(&nb, sizeof(int), 1, f);
    for (const Bomba *b = bombas; b; b = b->prox)
        fwrite(b, sizeof(Bomba) - sizeof(Bomba*), 1, f);
    // Inimigos
    int ni = 0;
    for (const Inimigo *i = inimigos; i; i = i->prox) ni++;
    fwrite(&ni, sizeof(int), 1, f);
    for (const Inimigo *i = inimigos; i; i = i->prox)
        fwrite(i, sizeof(Inimigo) - sizeof(Inimigo*), 1, f);
    fclose(f);
}

// Função para carregar o jogo
void carregarJogo(Jogador *jogador, Mapa **mapa, Bomba **bombas, Inimigo **inimigos) {
    FILE *f = fopen(SAVE_FILE, "rb");
    if (!f) return;
    // Limpar estado atual
    while (*bombas) { Bomba *tmp = *bombas; *bombas = (*bombas)->prox; free(tmp); }
    while (*inimigos) { Inimigo *tmp = *inimigos; *inimigos = (*inimigos)->prox; free(tmp); }
    if (*mapa) liberarMapa(*mapa);
    // Jogador
    fread(jogador, sizeof(Jogador), 1, f);
    // Mapa
    int linhas, colunas;
    fread(&linhas, sizeof(int), 1, f);
    fread(&colunas, sizeof(int), 1, f);
    Mapa *novoMapa = malloc(sizeof(Mapa));
    novoMapa->linhas = linhas;
    novoMapa->colunas = colunas;
    novoMapa->matriz = malloc(linhas * sizeof(char*));
    for (int i = 0; i < linhas; i++) {
        novoMapa->matriz[i] = malloc((colunas + 1) * sizeof(char));
        fread(novoMapa->matriz[i], sizeof(char), colunas, f);
        novoMapa->matriz[i][colunas] = '\0';
    }
    *mapa = novoMapa;
    // Bombas
    int nb = 0;
    fread(&nb, sizeof(int), 1, f);
    *bombas = NULL;
    for (int i = 0; i < nb; i++) {
        Bomba btmp;
        fread(&btmp, sizeof(Bomba) - sizeof(Bomba*), 1, f);
        Bomba *b = malloc(sizeof(Bomba));
        *b = btmp;
        b->prox = *bombas;
        *bombas = b;
    }
    // Inimigos
    int ni = 0;
    fread(&ni, sizeof(int), 1, f);
    *inimigos = NULL;
    for (int i = 0; i < ni; i++) {
        Inimigo itmp;
        fread(&itmp, sizeof(Inimigo) - sizeof(Inimigo*), 1, f);
        Inimigo *ini = malloc(sizeof(Inimigo));
        *ini = itmp;
        ini->prox = *inimigos;
        *inimigos = ini;
    }
    fclose(f);
}

// Variável para mensagem temporária
char mensagem[64] = "";
int tempo_mensagem = 0;

// Opções do menu
const char *menu_opcoes[] = {"Novo Jogo", "Salvar Jogo", "Carregar Jogo", "Sair do Jogo", "Voltar"};
#define MENU_OPCOES 5

// Função para desenhar o menu
void desenharMenu(int selecionado) {
    DrawRectangle(300, 120, 600, 400, LIGHTGRAY);
    DrawRectangleLines(300, 120, 600, 400, DARKGRAY);
    DrawText("MENU", 550, 140, 40, DARKBLUE);
    for (int i = 0; i < MENU_OPCOES; i++) {
        Color cor = (i == selecionado) ? BLUE : BLACK;
        DrawText(menu_opcoes[i], 400, 200 + i * 50, 32, cor);
    }
}

int main(void) {
    const int screenWidth = 1200;
    const int screenHeight = 600 + HUD_ALTURA;
    InitWindow(screenWidth, screenHeight, "Bomberman - Trabalho Prático");
    SetTargetFPS(60);
    srand(time(NULL));

    // Carregar mapa
    Mapa *mapa = carregarMapa(MAPA_ARQUIVO);
    if (!mapa) {
        printf("Erro ao carregar o mapa!\n");
        CloseWindow();
        return 1;
    }

    // Inicializar jogador
    Jogador jogador = {0};
    encontrarPosicaoJogador(mapa, &jogador);
    jogador.vidas = 3;
    jogador.bombas = 3;
    jogador.pontuacao = 0;
    jogador.chaves = 0;

    Bomba *bombas = NULL;
    Inimigo *inimigos = inicializarInimigos(mapa);
    int frame = 0;
    int invencivel = 0;
    int gameover = 0;
    int menu_aberto = 0;
    int menu_selecionado = 0;

    while (!WindowShouldClose()) {
        if (jogador.vidas <= 0) gameover = 1;
        if (gameover) {
            BeginDrawing();
            ClearBackground(BLACK);
            DrawText("GAME OVER", 400, 300, 80, RED);
            DrawText("Pressione ESC para sair", 420, 400, 30, WHITE);
            EndDrawing();
            continue;
        }
        frame++;
        if (invencivel > 0) invencivel--;
        // Abrir/fechar menu
        if (IsKeyPressed(KEY_TAB)) menu_aberto = !menu_aberto;
        if (menu_aberto) {
            // Navegação
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) menu_selecionado = (menu_selecionado + 1) % MENU_OPCOES;
            if (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W)) menu_selecionado = (menu_selecionado - 1 + MENU_OPCOES) % MENU_OPCOES;
            // Seleção
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                if (menu_selecionado == 0) { // Novo Jogo
                    // Libere listas dinâmicas
                    while (bombas) { Bomba *tmp = bombas; bombas = bombas->prox; free(tmp); }
                    while (inimigos) { Inimigo *tmp = inimigos; inimigos = inimigos->prox; free(tmp); }
                    liberarMapa(mapa);
                    // Recarregue o mapa e reinicialize tudo
                    mapa = carregarMapa(MAPA_ARQUIVO);
                    encontrarPosicaoJogador(mapa, &jogador);
                    jogador.vidas = 3;
                    jogador.bombas = 3;
                    jogador.pontuacao = 0;
                    jogador.chaves = 0;
                    bombas = NULL;
                    inimigos = inicializarInimigos(mapa);
                    frame = 0;
                    invencivel = 0;
                    gameover = 0;
                    menu_aberto = 0;
                } else if (menu_selecionado == 1) { // Salvar Jogo
                    salvarJogo(&jogador, mapa, bombas, inimigos);
                    strcpy(mensagem, "Jogo salvo!");
                    tempo_mensagem = 120;
                    menu_aberto = 0;
                } else if (menu_selecionado == 2) { // Carregar Jogo
                    carregarJogo(&jogador, &mapa, &bombas, &inimigos);
                    strcpy(mensagem, "Jogo carregado!");
                    tempo_mensagem = 120;
                    menu_aberto = 0;
                } else if (menu_selecionado == 3) { // Sair do Jogo
                    break;
                } else if (menu_selecionado == 4) { // Voltar
                    menu_aberto = 0;
                }
            }
            BeginDrawing();
            ClearBackground(RAYWHITE);
            desenharMenu(menu_selecionado);
            EndDrawing();
            continue;
        }
        // Movimentação do jogador
        int dx = 0, dy = 0;
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) dx = 1;
        if (IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_A)) dx = -1;
        if (IsKeyPressed(KEY_UP)    || IsKeyPressed(KEY_W)) dy = -1;
        if (IsKeyPressed(KEY_DOWN)  || IsKeyPressed(KEY_S)) dy = 1;
        int novoX = jogador.pos.x + dx;
        int novoY = jogador.pos.y + dy;
        if ((dx != 0 || dy != 0) && podeMover(mapa, novoX, novoY)) {
            jogador.pos.x = novoX;
            jogador.pos.y = novoY;
        }

        // Plantar bomba
        if ((IsKeyPressed(KEY_B) || IsKeyPressed(KEY_SPACE)) && jogador.bombas > 0) {
            // Posição à frente do jogador
            int dirX = 0, dirY = 0;
            if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) dirX = 1;
            else if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) dirX = -1;
            else if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) dirY = -1;
            else if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) dirY = 1;
            int bx = jogador.pos.x + dirX;
            int by = jogador.pos.y + dirY;
            // Se não houver direção pressionada, planta na posição do jogador
            if (dirX == 0 && dirY == 0) {
                bx = jogador.pos.x;
                by = jogador.pos.y;
            }
            // Só planta se for espaço vazio
            if (podeMover(mapa, bx, by)) {
                plantarBomba(&bombas, bx, by);
                jogador.bombas--;
            }
        }

        // Atualizar bombas (explosão)
        atualizarBombas(&bombas, &jogador, mapa, &invencivel);

        // Limpar todos os 'E' do mapa e coloque nas posições atuais dos inimigos
        limparInimigosMapa(mapa);
        for (Inimigo *ini = inimigos; ini; ini = ini->prox) {
            mapa->matriz[ini->pos.y][ini->pos.x] = 'E';
        }
        // Atualizar inimigos
        atualizarInimigos(inimigos, mapa, frame);
        removerInimigosExplodidos(&inimigos, mapa);
        checarColisaoJogadorInimigo(&jogador, inimigos, &invencivel);

        // Coletar chave
        coletarChave(&jogador, mapa);

        BeginDrawing();
        ClearBackground(RAYWHITE);
        desenharMapa(mapa);
        desenharBombas(bombas);
        desenharChaves(mapa);
        desenharInimigos(inimigos);
        DrawRectangle(jogador.pos.x * BLOCO, jogador.pos.y * BLOCO, BLOCO, BLOCO, invencivel ? LIGHTGRAY : BLUE);
        desenharHUD(&jogador);
        if (tempo_mensagem > 0) {
            DrawText(mensagem, 800, BLOCO * 25 + 50, 28, DARKGREEN);
            tempo_mensagem--;
        }
        EndDrawing();
    }

    // Liberar bombas
    while (bombas) {
        Bomba *tmp = bombas;
        bombas = bombas->prox;
        free(tmp);
    }
    while (inimigos) {
        Inimigo *tmp = inimigos;
        inimigos = inimigos->prox;
        free(tmp);
    }
    liberarMapa(mapa);
    CloseWindow();
    return 0;
} 
