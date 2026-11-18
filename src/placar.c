#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "placar.h"

// guarda os pontos e a data/hora de agora
void placar_registro_agora(Registro *r, int pontos) {
    time_t agora = time(NULL);
    struct tm *t = localtime(&agora);

    strcpy(r->nome, "");
    r->pontos = pontos;
    r->dia = t->tm_mday;
    r->mes = t->tm_mon + 1;
    r->ano = t->tm_year + 1900;
    r->hora = t->tm_hour;
    r->minuto = t->tm_min;
}

void placar_iniciar(Placar *p) {
    p->inicio = NULL;
    p->quantidade = 0;
}

// da free em todos os nos
void placar_liberar(Placar *p) {
    NoPlacar *atual = p->inicio;

    while (atual != NULL) {
        NoPlacar *proximo = atual->proximo;
        free(atual);
        atual = proximo;
    }

    p->inicio = NULL;
    p->quantidade = 0;
}

// tira o ultimo no da lista
static void remover_ultimo(Placar *p) {
    NoPlacar *atual = p->inicio;
    NoPlacar *anterior = NULL;

    while (atual->proximo != NULL) {
        anterior = atual;
        atual = atual->proximo;
    }

    if (anterior == NULL) {
        p->inicio = NULL;
    } else {
        anterior->proximo = NULL;
    }

    free(atual);
    p->quantidade--;
}

// cria um no e coloca na posicao certa (maior pontuacao primeiro)
// retorna a posicao (0 = primeiro lugar) ou -1 se nao ficou no placar
int placar_inserir(Placar *p, const Registro *novo) {
    NoPlacar *novo_no = malloc(sizeof(NoPlacar));
    NoPlacar *atual = p->inicio;
    NoPlacar *anterior = NULL;
    int posicao = 0;

    if (novo_no == NULL) {
        return -1;
    }

    novo_no->registro = *novo;

    // anda ate achar o lugar do novo recorde
    while (atual != NULL && atual->registro.pontos >= novo->pontos) {
        anterior = atual;
        atual = atual->proximo;
        posicao++;
    }

    novo_no->proximo = atual;

    if (anterior == NULL) {
        p->inicio = novo_no;
    } else {
        anterior->proximo = novo_no;
    }

    p->quantidade++;

    // o placar so guarda os 10 melhores
    if (p->quantidade > PLACAR_MAX_REGISTROS) {
        remover_ultimo(p);

        if (posicao >= PLACAR_MAX_REGISTROS) {
            return -1;
        }
    }

    return posicao;
}

// ve se a pontuacao entra no placar
int placar_qualifica(const Placar *p, int pontos) {
    NoPlacar *no = p->inicio;

    if (p->quantidade < PLACAR_MAX_REGISTROS) {
        return 1;
    }

    // lista cheia: compara com o ultimo
    while (no->proximo != NULL) {
        no = no->proximo;
    }

    return pontos > no->registro.pontos;
}

// le o arquivo e monta a lista
void placar_carregar(Placar *p) {
    FILE *arq;
    Registro r;

    placar_liberar(p);

    arq = fopen(PLACAR_ARQUIVO, "r");
    if (arq == NULL) {
        return;
    }

    while (fscanf(arq, "%3s %d %d %d %d %d %d", r.nome, &r.pontos,
                  &r.dia, &r.mes, &r.ano, &r.hora, &r.minuto) == 7) {
        placar_inserir(p, &r);
    }

    fclose(arq);
}

// escreve a lista no arquivo, um recorde por linha
void placar_salvar(const Placar *p) {
    FILE *arq = fopen(PLACAR_ARQUIVO, "w");
    NoPlacar *no;

    if (arq == NULL) {
        return;
    }

    for (no = p->inicio; no != NULL; no = no->proximo) {
        fprintf(arq, "%s %d %d %d %d %d %d\n", no->registro.nome,
                no->registro.pontos, no->registro.dia, no->registro.mes,
                no->registro.ano, no->registro.hora, no->registro.minuto);
    }

    fclose(arq);
}

void placar_apagar(Placar *p) {
    placar_liberar(p);
    placar_salvar(p);
}
