# Makefile for raylib project
CC=gcc
CFLAGS=-I. -Wall
LDFLAGS=-lraylib -lopengl32 -lgdi32 -lwinmm -lm
SRC=main.c jogo.c mapa.c ui.c
HEADERS=structs.h jogo.h mapa.h ui.h
OUT=bomberman.exe

all: $(OUT)

$(OUT): $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC) $(LDFLAGS)

clean:
	rm -f $(OUT) 
