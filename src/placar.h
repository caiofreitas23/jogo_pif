#ifndef PLACAR_H
#define PLACAR_H

#define PLACAR_MAX_REGISTROS 10
#define PLACAR_TAM_NOME 3
#define PLACAR_ARQUIVO "placar.txt"

// um recorde do placar
typedef struct {
    char nome[PLACAR_TAM_NOME + 1];
    int pontos;
    int dia, mes, ano;
    int hora, minuto;
} Registro;

// cada recorde fica guardado em um no da lista
typedef struct NoPlacar {
    Registro registro;
    struct NoPlacar *proximo;
} NoPlacar;

// o placar e a lista, do maior para o menor
typedef struct {
    NoPlacar *inicio;
    int quantidade;
} Placar;

void placar_registro_agora(Registro *r, int pontos);
void placar_iniciar(Placar *p);
void placar_liberar(Placar *p);
void placar_carregar(Placar *p);
void placar_salvar(const Placar *p);
int placar_qualifica(const Placar *p, int pontos);
int placar_inserir(Placar *p, const Registro *novo);
void placar_apagar(Placar *p);

#endif
