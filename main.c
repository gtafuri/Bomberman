#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "structs.h"
#include "jogo.h"
#include "mapa.h"
#include "ui.h"
#include <time.h>
#include <dirent.h> // para ver o diretorio de mapas e verificar quantas fases terá

#define MAPA_ARQUIVO "maps/mapa1.txt"
#define BLOCO 20
#define HUD_ALTURA 100
#define TEMPO_BOMBA 180
#define RAIO_EXPLOSAO 2
#define INTERVALO_INIMIGO 40 // frames para mudar direção dos inimigos (da pra alterar se acharem que tá lento, mas testando me parece a velocidade ok)

#define SAVE_FILE "savegame.bin" // arquivo de salvamento do jogo

#define MAX_MAPAS 100
#define MAX_MAPA_PATH 260

#define RANKING_FILE "ranking.bin" // arquivo de ranking

#define MAX_RANK 10

void atualizarRanking(int novaPontuacao, int *posicao) {
    int ranking[MAX_RANK] = {0};
    FILE *f = fopen(RANKING_FILE, "rb");
    if (f) {
        fread(ranking, sizeof(int), MAX_RANK, f);
        fclose(f);
    }
    *posicao = -1;
    for (int i = 0; i < MAX_RANK; i++) {
        if (novaPontuacao > ranking[i]) {
            for (int j = MAX_RANK-1; j > i; j--) ranking[j] = ranking[j-1];
            ranking[i] = novaPontuacao;
            *posicao = i;
            break;
        }
    }
    f = fopen(RANKING_FILE, "wb");
    fwrite(ranking, sizeof(int), MAX_RANK, f);
    fclose(f);
}

// encontrar posição inicial do jogador
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

// Verifica se a posição é válida para o jogador andar - ALTERADO PARA NAO PASSAR POR CIMA DAS BOMBAS
int podeMover(const Mapa *mapa, const Bomba *bombas, int x, int y) {
    if (x < 0 || y < 0 || x >= mapa->colunas || y >= mapa->linhas)
        return 0;
    char c = mapa->matriz[y][x];
    // Não pode atravessar paredes nem caixas, ou bombas
    if (c == 'W' || c == 'D' || c == 'K' || c == 'B')
        return 0;

    // Adicionar verificação para bombas ativas
    const Bomba *current_bomba = bombas;
    while (current_bomba != NULL) {
        if (current_bomba->pos.x == x && current_bomba->pos.y == y && current_bomba->ativa) {
            return 0; // Colidiu com uma bomba
        }
        current_bomba = current_bomba->prox;
    }

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


#define MAX_EXPLOSIONS 32

typedef struct {
    int x, y;
    int frame;
    int ativa;
} ExplosionAnim;

ExplosionAnim explosoes[MAX_EXPLOSIONS] = {0};

void limparExplosoes() {
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        explosoes[i].ativa = 0;
        explosoes[i].frame = 0;
    }
}

void adicionarExplosao(int x, int y) {
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (!explosoes[i].ativa) {
            explosoes[i].x = x;
            explosoes[i].y = y;
            explosoes[i].frame = 0;
            explosoes[i].ativa = 1;
            break;
        }
    }
}

void desenharExplosoes() {
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (explosoes[i].ativa) {
            // Desenhar o raio de explosão em vermelho
            int x = explosoes[i].x * BLOCO;
            int y = explosoes[i].y * BLOCO;
            int frame = explosoes[i].frame;
            
            // Calcular transparência baseada no frame
            float alpha = 1.0f - (frame / 20.0f);
            
            // Desenhar retângulo vermelho 
            DrawRectangle(x, y, BLOCO, BLOCO, RED);
            
            // Adicionar borda branca
            DrawRectangleLines(x, y, BLOCO, BLOCO, WHITE);
            
            // círculo vermelho no centro
            DrawCircle(x + BLOCO/2, y + BLOCO/2, BLOCO/3, RED);
            
            // círculo branco menor no centro
            DrawCircle(x + BLOCO/2, y + BLOCO/2, BLOCO/6, WHITE);
            
            explosoes[i].frame++;
            if (explosoes[i].frame > 20) explosoes[i].ativa = 0;
        }
    }
}

void explodirBomba(Bomba *b, Jogador *jogador, Mapa *mapa) {
    int x0 = b->pos.x;
    int y0 = b->pos.y;
    adicionarExplosao(x0, y0); // Centro da explosão
    
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
            
            adicionarExplosao(x, y);
            
            char *c = &mapa->matriz[y][x];
            if (*c == 'W') break; // Parede para a explosão
            if (*c == 'D') { *c = ' '; jogador->pontuacao += 10; break; } //a pontuaçao aumetna quando explode objetos
            if (*c == 'K') { *c = 'C'; jogador->pontuacao += 10; break; }
            if (*c == 'B') { *c = ' '; jogador->pontuacao += 10; break; }
            if (*c == 'E') { *c = ' '; jogador->pontuacao += 20; }
          
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

// Atualiza bombas: decrementa timer, explode e remove
void atualizarBombas(Bomba **lista, Jogador *jogador, Mapa *mapa, int *invencivel) {
    Bomba **ptr = lista;
    while (*ptr) {
        Bomba *b = *ptr;
        b->tempo_restante--;
        if (b->tempo_restante <= 0) {
            explodirBomba(b, jogador, mapa);
            // Verifica se jogador está na explosão
            if (*invencivel == 0 && jogadorAtingidoPorExplosao(b, jogador, mapa)) {
                jogador->vidas--;
                jogador->pontuacao -= 100;
                if (jogador->pontuacao < 0) jogador->pontuacao = 0;
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

// Desenha bombas no mapa com animação de piscar
void desenharBombas(const Bomba *lista) {
    while (lista) {

        Color cor_bomba = BLACK;
        if (lista->tempo_restante <= 30) { // Piscar nos últimos 30 frames 
            if ((lista->tempo_restante / 3) % 2 == 0) {
                cor_bomba = RED; // Piscar em vermelho
            } else {
                cor_bomba = BLACK;
            }
        }
        DrawCircle(lista->pos.x * BLOCO + BLOCO/2, lista->pos.y * BLOCO + BLOCO/2, BLOCO/2 - 2, cor_bomba); 
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

// permitir coleta de chave
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

// Verifica se a posição é válida para o inimigo andar - ALTERADO PARA NAO PASSAR POR CIMA DAS BOMBAS
int podeMoverInimigo(const Mapa *mapa, const Bomba *bombas, int x, int y) {
    if (x < 0 || y < 0 || x >= mapa->colunas || y >= mapa->linhas)
        return 0;
    char c = mapa->matriz[y][x];
    if (c == 'W' || c == 'D' || c == 'K' || c == 'B' || c == 'E')
        return 0;

    // Adicionar verificação para bombas ativas
    const Bomba *current_bomba = bombas;
    while (current_bomba != NULL) {
        if (current_bomba->pos.x == x && current_bomba->pos.y == y && current_bomba->ativa) {
            return 0; // Colidiu com uma bomba
        }
        current_bomba = current_bomba->prox;
    }

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
void atualizarInimigos(Inimigo *lista, Mapa *mapa, const Bomba *bombas, int frame) {
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
        if (podeMoverInimigo(mapa, bombas, nx, ny)) {
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
        if (ini->ativo) {
            DrawCircleLines(ini->pos.x * BLOCO + BLOCO/2, ini->pos.y * BLOCO + BLOCO/2, BLOCO/2-2, BLACK);
            DrawCircle(ini->pos.x * BLOCO + BLOCO/2, ini->pos.y * BLOCO + BLOCO/2, BLOCO/2-4, RED);
        }
    }
}

// Verifica colisão jogador-inimigo
void checarColisaoJogadorInimigo(Jogador *jogador, Inimigo *lista, int *invencivel) {
    for (Inimigo *ini = lista; ini; ini = ini->prox) {
        if (ini->ativo && ini->pos.x == jogador->pos.x && ini->pos.y == jogador->pos.y && *invencivel == 0) {
            jogador->vidas--;
            jogador->pontuacao -= 100;
            if (jogador->pontuacao < 0) jogador->pontuacao = 0;
            *invencivel = 60; // 1 segundo de invencel
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
//usada para mostrar a mensagem de salvamento ou carregamento 
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

// Função para listar mapas disponíveis
void listarMapas(char mapas[][MAX_MAPA_PATH], int *total_mapas) {
    DIR *d = opendir("maps");
    struct dirent *dir;
    *total_mapas = 0;
    while ((dir = readdir(d)) != NULL) {
        if (strncmp(dir->d_name, "mapa", 4) == 0 && strstr(dir->d_name, ".txt")) {
            snprintf(mapas[*total_mapas], MAX_MAPA_PATH, "maps/%s", dir->d_name);
            (*total_mapas)++;
            if (*total_mapas >= MAX_MAPAS) break;
        }
    }
    closedir(d);
    // Ordenar mapas por nome
    for (int i = 0; i < *total_mapas-1; i++) {
        for (int j = i+1; j < *total_mapas; j++) {
            if (strcmp(mapas[i], mapas[j]) > 0) {
                char tmp[MAX_MAPA_PATH]; strcpy(tmp, mapas[i]); strcpy(mapas[i], mapas[j]); strcpy(mapas[j], tmp);
            }
        }
    }
}

// Função para validar mapa
int validarMapa(const Mapa *mapa) {
    int chaves = 0, inimigos = 0;
    for (int i = 0; i < mapa->linhas; i++)
        for (int j = 0; j < mapa->colunas; j++) {
            if (mapa->matriz[i][j] == 'K') chaves++;
            if (mapa->matriz[i][j] == 'E') inimigos++;
        }
    return (chaves == 5 && inimigos == 5);
}

int main(void) {
    const int screenWidth = 1200;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Bomberman - Trabalho Prático");
    SetTargetFPS(60);
    srand(time(NULL));

    char mapas[MAX_MAPAS][MAX_MAPA_PATH];
    int total_mapas = 0;
    int mapa_atual = 0;
    listarMapas(mapas, &total_mapas);

    if (total_mapas == 0) {
        printf("Nenhum mapa encontrado!\n");
        CloseWindow();
        return 1;
    }
    mapa_atual = 0;
    Mapa *mapa = carregarMapa(mapas[mapa_atual]);
    if (!mapa) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Erro ao carregar o mapa!", 100, 300, 32, RED);
        EndDrawing();
        WaitTime(3.0);
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
    int menu_aberto = 1; // Começa com o menu aberto
    int menu_selecionado = 0;
    int tick_inimigo = 0;
    int tick_jogador = 0;
    int lastDirX = 1, lastDirY = 0; // Começa para a direita

    while (!WindowShouldClose()) {
        if (jogador.vidas <= 0) gameover = 1;
        if (gameover) {
            int posicao_rank = -1;
            atualizarRanking(jogador.pontuacao, &posicao_rank);
            BeginDrawing();
            ClearBackground(BLACK);
            DrawText("GAME OVER", 400, 300, 80, RED);
            if (posicao_rank != -1) {
                DrawText(TextFormat("Você obteve a %dª maior pontuação! Parabéns!", posicao_rank+1), 300, 370, 30, GOLD);
            }
            const char* msg = "Pressione TAB para reiniciar ou ESC para sair do jogo";
            int fontSize = 30;
            int textWidth = MeasureText(msg, fontSize);
            DrawText(msg, (screenWidth - textWidth)/2, 400, fontSize, WHITE);
            EndDrawing();
            if (IsKeyPressed(KEY_TAB)) {
                // Reinicialize o estado do jogo
                while (bombas) { Bomba *tmp = bombas; bombas = bombas->prox; free(tmp); }
                while (inimigos) { Inimigo *tmp = inimigos; inimigos = inimigos->prox; free(tmp); }
                liberarMapa(mapa);
                mapa_atual = 0;
                mapa = carregarMapa(mapas[mapa_atual]);
                encontrarPosicaoJogador(mapa, &jogador);
                jogador.vidas = 3;
                jogador.bombas = 3;
                jogador.pontuacao = 0;
                jogador.chaves = 0;
                bombas = NULL;
                inimigos = inicializarInimigos(mapa);
                frame = 0;
                invencivel = 0;
                menu_aberto = 1;
                gameover = 0;
            }
            continue;
        }
        frame++;
        if (invencivel > 0) invencivel--;
        tick_inimigo++;
        if (tick_inimigo >= 20) { // Inimigos andam a cada 20 frames
            limparInimigosMapa(mapa);
            for (Inimigo *ini = inimigos; ini; ini = ini->prox) {
                mapa->matriz[ini->pos.y][ini->pos.x] = 'E';
            }
            atualizarInimigos(inimigos, mapa, bombas, frame);
            tick_inimigo = 0;
        }
        removerInimigosExplodidos(&inimigos, mapa);
        checarColisaoJogadorInimigo(&jogador, inimigos, &invencivel);

        // Permitir abrir/fechar menu com TAB durante o jogo
        if (IsKeyPressed(KEY_TAB)) menu_aberto = !menu_aberto;
        if (menu_aberto) {
            // Navegação por setas/W/S
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) menu_selecionado = (menu_selecionado + 1) % MENU_OPCOES;
            if (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W)) menu_selecionado = (menu_selecionado - 1 + MENU_OPCOES) % MENU_OPCOES;
            // Seleção por ENTER/ESPAÇO
            int acao = -1;
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) acao = menu_selecionado;
            // Seleção por teclas específicas
            if (IsKeyPressed(KEY_N)) acao = 0;
            if (IsKeyPressed(KEY_S)) acao = 1;
            if (IsKeyPressed(KEY_C)) acao = 2;
            if (IsKeyPressed(KEY_Q)) acao = 3;
            if (IsKeyPressed(KEY_V)) acao = 4;
            if (acao != -1) {
                if (acao == 0) { // Novo Jogo
                    while (bombas) { Bomba *tmp = bombas; bombas = bombas->prox; free(tmp); }
                    while (inimigos) { Inimigo *tmp = inimigos; inimigos = inimigos->prox; free(tmp); }
                    liberarMapa(mapa);
                    mapa_atual = 0;
                    mapa = carregarMapa(mapas[mapa_atual]);
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
                } else if (acao == 1) { // Salvar Jogo
                    salvarJogo(&jogador, mapa, bombas, inimigos);
                    strcpy(mensagem, "Jogo salvo!");
                    tempo_mensagem = 120;
                    menu_aberto = 0;
                } else if (acao == 2) { // Carregar Jogo
                    carregarJogo(&jogador, &mapa, &bombas, &inimigos);
                    strcpy(mensagem, "Jogo carregado!");
                    tempo_mensagem = 120;
                    menu_aberto = 0;
                } else if (acao == 3) { // Sair do Jogo
                    break;
                } else if (acao == 4) { // Voltar
                    menu_aberto = 0;
                }
            }
            BeginDrawing();
            ClearBackground(RAYWHITE);
            desenharMenu(menu_selecionado);
            EndDrawing();
            continue;
        }
        tick_jogador++;
        int pode_andar = 0;
        if (tick_jogador >= 12) {
            pode_andar = 1;
            tick_jogador = 0;
        }
        if (pode_andar) {
            int dx = 0, dy = 0;

            // ALTERADO PARA NAO CORRER NA DIAGONAL (elseif)
            if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) {
                dy = -1;
            } else if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) {
                dy = 1;
            } else if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
                dx = -1;
            } else if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
                dx = 1;
            }

            int novoX = jogador.pos.x + dx;
            int novoY = jogador.pos.y + dy;
            if ((dx != 0 || dy != 0) && podeMover(mapa, bombas, novoX, novoY)) { // Passa a lista de bombas
                jogador.pos.x = novoX;
                jogador.pos.y = novoY;
                lastDirX = dx;
                lastDirY = dy;
            }
        }

        // Plantar bomba
        if ((IsKeyPressed(KEY_B) || IsKeyPressed(KEY_SPACE)) && jogador.bombas > 0) {
            int bx = jogador.pos.x + lastDirX;
            int by = jogador.pos.y + lastDirY;
            // Só planta se houver direção pressionada e espaço válido, e se não houver bomba já na posição
            // A função podeMover já verifica a colisão com bombas, então podemos usá-la aqui.
            if (podeMover(mapa, bombas, bx, by)) { // Reusa podeMover para verificar se o local da bomba é válido
                plantarBomba(&bombas, bx, by);
                jogador.bombas--;
            }
        }

        // Atualizar bombas (explosão)
        atualizarBombas(&bombas, &jogador, mapa, &invencivel);

        // Coletar chave
        coletarChave(&jogador, mapa);
        if (jogador.chaves >= 5) {
            mapa_atual++;
            if (mapa_atual >= total_mapas) {
                int posicao_rank = -1;
                atualizarRanking(jogador.pontuacao, &posicao_rank);
                BeginDrawing();
                ClearBackground(RAYWHITE);
                DrawText("Parabéns! Você venceu todos os mapas!", 300, 300, 40, DARKGREEN);
                if (posicao_rank != -1) {
                    DrawText(TextFormat("Você obteve a %dª maior pontuação! Parabéns!", posicao_rank+1), 300, 370, 30, GOLD);
                }
                EndDrawing();
                WaitTime(3.0);
                break;
            }
            liberarMapa(mapa);
            mapa = carregarMapa(mapas[mapa_atual]);
            if (!mapa) {
                printf("Erro ao carregar o mapa!\n");
                return 1;
            }
            encontrarPosicaoJogador(mapa, &jogador);
            jogador.chaves = 0;
            jogador.bombas = 3;
            bombas = NULL;
            inimigos = inicializarInimigos(mapa);
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);
        desenharMapa(mapa); 
        desenharBombas(bombas);
        desenharChaves(mapa);
        desenharInimigos(inimigos);
        DrawCircleLines(jogador.pos.x * BLOCO + BLOCO/2, jogador.pos.y * BLOCO + BLOCO/2, BLOCO/2-2, WHITE);
        DrawCircle(jogador.pos.x * BLOCO + BLOCO/2, jogador.pos.y * BLOCO + BLOCO/2, BLOCO/2-4, invencivel && (frame%10<5) ? Fade(BLUE,0.3f) : BLUE);
        desenharExplosoes(); 
        desenharHUD(&jogador); /
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
