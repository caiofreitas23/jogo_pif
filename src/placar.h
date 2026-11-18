#ifndef PLACAR_H
#define PLACAR_H

#define PLACAR_MAX_REGISTROS 10
#define PLACAR_TAM_NOME 3
#define PLACAR_ARQUIVO "placar.txt"

// Um recorde (struct simples).
typedef struct {
    char nome[PLACAR_TAM_NOME + 1];
    int pontos;
    int dia, mes, ano;
    int hora, minuto;
} Registro;

// Nó da lista encadeada: cada recorde vive em um nó alocado com malloc.
typedef struct NoPlacar {
    Registro registro;
    struct NoPlacar *proximo;
} NoPlacar;

// O placar tá como  uma lista encadeada ordenada do maior para o menor.
typedef struct {
    NoPlacar *inicio;
    int quantidade;
} Placar;

// Preenche pontos e a data/hora ATUAL (momento em que a partida terminou).
void placar_registro_agora(Registro *r, int pontos);

void placar_iniciar(Placar *p);

// Libera todos os nós da lista (free).
void placar_liberar(Placar *p);

// Lê o arquivo e monta a lista (descarta o que estava na memória).
void placar_carregar(Placar *p);

// Percorre a lista e grava cada nó no arquivo.
int placar_salvar(const Placar *p);

// Indica se a pontuação entraria no placar.
int placar_qualifica(const Placar *p, int pontos);

// Cria um nó e o encaixa na posição correspondente à pontuação (empate: fica
// atrás de quem fez primeiro). Retorna o índice (0 = 1º lugar) ou -1 se não entrou.
int placar_inserir(Placar *p, const Registro *novo);

// Retorna o registro da posição indicada (0 = 1º lugar) ou NULL.
const Registro *placar_obter(const Placar *p, int indice);

// Apaga todos os recordes (libera a lista e esvazia o arquivo).
void placar_apagar(Placar *p);

#endif