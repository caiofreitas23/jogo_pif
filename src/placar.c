#include "placar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void placar_registro_agora(Registro *r, int pontos) {
    time_t agora = time(NULL);
    struct tm *t = localtime(&agora);

    r->nome[0] = '\0';
    r->pontos = pontos;

    if (t != NULL) {
        r->dia = t->tm_mday;
        r->mes = t->tm_mon + 1;
        r->ano = t->tm_year + 1900;
        r->hora = t->tm_hour;
        r->minuto = t->tm_min;
    } else {
        r->dia = 1;
        r->mes = 1;
        r->ano = 2026;
        r->hora = 0;
        r->minuto = 0;
    }
}

void placar_iniciar(Placar *p) {
    p->inicio = NULL;
    p->quantidade = 0;
}

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

void placar_carregar(Placar *p) {
    placar_liberar(p);

    FILE *f = fopen(PLACAR_ARQUIVO, "r");
    if (f == NULL) {
        return;
    }

    char linha[128];

    while (fgets(linha, sizeof(linha), f) != NULL) {
        char nome[32];
        Registro r;

        if (sscanf(linha, "%31s %d %d %d %d %d %d",
                   nome, &r.pontos, &r.dia, &r.mes, &r.ano,
                   &r.hora, &r.minuto) == 7) {
            strncpy(r.nome, nome, PLACAR_TAM_NOME);
            r.nome[PLACAR_TAM_NOME] = '\0';

            // A inserção ordenada mantém a lista certa mesmo se o arquivo
            // tiver sido editado à mão.
            placar_inserir(p, &r);
        }
    }

    fclose(f);
}

int placar_salvar(const Placar *p) {
    FILE *f = fopen(PLACAR_ARQUIVO, "w");
    if (f == NULL) {
        return 0;
    }

    for (const NoPlacar *no = p->inicio; no != NULL; no = no->proximo) {
        const Registro *r = &no->registro;
        fprintf(f, "%s %d %d %d %d %d %d\n",
                r->nome, r->pontos, r->dia, r->mes, r->ano,
                r->hora, r->minuto);
    }

    fclose(f);
    return 1;
}

int placar_qualifica(const Placar *p, int pontos) {
    if (p->quantidade < PLACAR_MAX_REGISTROS) {
        return 1;
    }

    // Lista cheia: compara com o último nó.
    const NoPlacar *ultimo = p->inicio;
    while (ultimo != NULL && ultimo->proximo != NULL) {
        ultimo = ultimo->proximo;
    }

    return ultimo == NULL || pontos > ultimo->registro.pontos;
}

int placar_inserir(Placar *p, const Registro *novo) {
    // Procura o ponto de encaixe: "anterior" é o nó depois do qual o novo entra.
    NoPlacar *anterior = NULL;
    NoPlacar *atual = p->inicio;
    int pos = 0;

    while (atual != NULL && atual->registro.pontos >= novo->pontos) {
        anterior = atual;
        atual = atual->proximo;
        pos++;
    }

    if (pos >= PLACAR_MAX_REGISTROS) {
        return -1;
    }

    NoPlacar *no = (NoPlacar *)malloc(sizeof(NoPlacar));
    if (no == NULL) {
        return -1;
    }

    no->registro = *novo;
    no->proximo = atual;

    if (anterior == NULL) {
        p->inicio = no;
    } else {
        anterior->proximo = no;
    }

    p->quantidade++;

    // Passou do limite: descarta o último da lista.
    if (p->quantidade > PLACAR_MAX_REGISTROS) {
        NoPlacar *penultimo = p->inicio;

        while (penultimo->proximo != NULL && penultimo->proximo->proximo != NULL) {
            penultimo = penultimo->proximo;
        }

        free(penultimo->proximo);
        penultimo->proximo = NULL;
        p->quantidade--;
    }

    return pos;
}

const Registro *placar_obter(const Placar *p, int indice) {
    const NoPlacar *no = p->inicio;

    while (no != NULL && indice > 0) {
        no = no->proximo;
        indice--;
    }

    return no != NULL ? &no->registro : NULL;
}

void placar_apagar(Placar *p) {
    placar_liberar(p);
    placar_salvar(p);
}