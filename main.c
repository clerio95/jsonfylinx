#include "jsonfylinx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define MAX_PATH 4096

/* Builds the output path next to nothing — uses out_name relative to the
 * input directory, appending .json when missing. CLI convenience only; the
 * library (jfx_convert) takes a literal path. */
static void build_output_path(const char *in_path, const char *out_name,
                              char *result, size_t size) {
    const char *slash = strrchr(in_path, '/');
    if (slash) {
        size_t dir_len = (size_t)(slash - in_path) + 1;
        if (dir_len >= size) dir_len = size - 1;
        strncpy(result, in_path, dir_len);
        result[dir_len] = '\0';
        strncat(result, out_name, size - strlen(result) - 1);
    } else {
        strncpy(result, out_name, size - 1);
        result[size - 1] = '\0';
    }

    size_t rlen = strlen(result);
    if (rlen < 5 || strcmp(result + rlen - 5, ".json") != 0)
        strncat(result, ".json", size - strlen(result) - 1);
}

static const char *parser_nome(jfx_parser_t p) {
    switch (p) {
        case JFX_POSICAO_ESTOQUE: return "Posição de Estoque";
        case JFX_VALOR_ESTOQUE:   return "Valor do Estoque";
        case JFX_PRODUTIVIDADE:   return "Produtividade por Funcionários";
        default:                  return "desconhecido";
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 4) {
        fprintf(stderr,
            "Uso: jsonfylinx <caminho_arquivo> <nome_saida> [parser]\n"
            "  parser (opcional): 1=Posição de Estoque, 2=Valor do Estoque,\n"
            "                     3=Produtividade por Funcionários.\n"
            "  Sem [parser], o tipo é detectado automaticamente.\n");
        return 1;
    }

    jfx_parser_t parser = JFX_AUTO;
    if (argc == 4) {
        int opt = atoi(argv[3]);
        if (opt < 1 || opt > 3) {
            fprintf(stderr, "Erro: parser '%s' inválido (use 1, 2 ou 3).\n", argv[3]);
            return 1;
        }
        parser = (jfx_parser_t)opt;
    }

    struct stat sb;
    if (stat(argv[1], &sb) != 0) {
        fprintf(stderr, "Erro: arquivo não encontrado: '%s'\n", argv[1]);
        return 1;
    }
    if (!S_ISREG(sb.st_mode)) {
        fprintf(stderr, "Erro: %s.\n", jfx_status_str(JFX_ERR_NOT_REGULAR_FILE));
        return 1;
    }

    char out_path[MAX_PATH];
    build_output_path(argv[1], argv[2], out_path, sizeof(out_path));

    if (parser == JFX_AUTO) {
        jfx_parser_t detected = jfx_detect(argv[1]);
        if (detected == JFX_AUTO) {
            fprintf(stderr, "Erro: %s. Informe o parser manualmente (1, 2 ou 3).\n",
                    jfx_status_str(JFX_ERR_DETECT_FAILED));
            return 1;
        }
        parser = detected;
        printf("\nTipo detectado: %s\n", parser_nome(parser));
    } else {
        printf("\nParser selecionado: %s\n", parser_nome(parser));
    }

    printf("Analisando arquivo...\n");
    jfx_status_t st = jfx_convert(argv[1], out_path, parser);
    if (st != JFX_OK) {
        fprintf(stderr, "Erro: %s.\n", jfx_status_str(st));
        return 1;
    }

    printf("Arquivo JSON gerado com sucesso: %s\n\n", out_path);
    return 0;
}
