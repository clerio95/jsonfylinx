#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#define MAX_LINE    1024
#define MAX_PRODUTO 512
#define MAX_PATH    4096

typedef struct {
    int    id;
    char   categoria[4];
    char   produto[MAX_PRODUTO];
    double estoque;
} ItemEstoque;

static void trim(char *s) {
    char *p = s + strlen(s) - 1;
    while (p >= s && isspace((unsigned char)*p)) *p-- = '\0';
    p = s;
    while (*p && isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
}

static int is_digits_only(const char *s) {
    if (!*s) return 0;
    while (*s)
        if (!isdigit((unsigned char)*s++)) return 0;
    return 1;
}

/* Splits a pipe-delimited table row into inner fields, trimming whitespace. */
static int split_pipes(const char *line, char out[][MAX_PRODUTO], int max) {
    int n = 0;
    const char *p = line;
    while (*p && n < max) {
        if (*p++ != '|') continue;
        const char *start = p;
        while (*p && *p != '|') p++;
        size_t len = (size_t)(p - start);
        if (len >= MAX_PRODUTO) len = MAX_PRODUTO - 1;
        strncpy(out[n], start, len);
        out[n][len] = '\0';
        trim(out[n]);
        n++;
    }
    return n;
}

static int parse_row(const char *line, ItemEstoque *item) {
    if (line[0] != '|') return 0;

    char f[4][MAX_PRODUTO];
    if (split_pipes(line, f, 4) < 3) return 0;
    if (!is_digits_only(f[0])) return 0;

    item->id = atoi(f[0]);

    /* Extract optional (LETTER) prefix from the product name */
    if (f[1][0] == '(' && isalpha((unsigned char)f[1][1]) && f[1][2] == ')') {
        item->categoria[0] = (char)toupper((unsigned char)f[1][1]);
        item->categoria[1] = '\0';
        char *rest = f[1] + 3;
        while (*rest && isspace((unsigned char)*rest)) rest++;
        strncpy(item->produto, rest, MAX_PRODUTO - 1);
    } else {
        item->categoria[0] = '\0';
        strncpy(item->produto, f[1], MAX_PRODUTO - 1);
    }
    item->produto[MAX_PRODUTO - 1] = '\0';

    /* Quantity: replace Brazilian comma separator with dot for strtod */
    char qty[64];
    strncpy(qty, f[2], sizeof(qty) - 1);
    qty[sizeof(qty) - 1] = '\0';
    for (int i = 0; qty[i]; i++)
        if (qty[i] == ',') qty[i] = '.';
    item->estoque = strtod(qty, NULL);

    return 1;
}

static void json_escape(FILE *out, const char *s) {
    while (*s) {
        unsigned char c = (unsigned char)*s++;
        switch (c) {
            case '"':  fputs("\\\"", out); break;
            case '\\': fputs("\\\\", out); break;
            case '\n': fputs("\\n",  out); break;
            case '\r': fputs("\\r",  out); break;
            case '\t': fputs("\\t",  out); break;
            default:   fputc(c, out);      break;
        }
    }
}

static int parse_posicao_estoque(const char *in_path, const char *out_path) {
    FILE *in = fopen(in_path, "r");
    if (!in) {
        fprintf(stderr, "Erro: não foi possível abrir '%s'\n", in_path);
        return 0;
    }
    FILE *out = fopen(out_path, "w");
    if (!out) {
        fclose(in);
        fprintf(stderr, "Erro: não foi possível criar '%s'\n", out_path);
        return 0;
    }

    char line[MAX_LINE];
    ItemEstoque item;
    int first = 1;

    fputs("[\n", out);

    while (fgets(line, sizeof(line), in)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';

        if (!parse_row(line, &item)) continue;

        if (!first) fputs(",\n", out);
        first = 0;

        fputs("  {\n", out);
        fprintf(out, "    \"id\": %d,\n", item.id);

        if (item.categoria[0])
            fprintf(out, "    \"categoria\": \"%s\",\n", item.categoria);
        else
            fputs("    \"categoria\": null,\n", out);

        fputs("    \"produto\": \"", out);
        json_escape(out, item.produto);
        fputs("\",\n", out);

        fprintf(out, "    \"estoque\": %.2f\n", item.estoque);

        fputs("  }", out);
    }

    fputs("\n]\n", out);

    fclose(in);
    fclose(out);
    return 1;
}

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

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: jsonfylinx <caminho_arquivo> <nome_saida>\n");
        return 1;
    }

    struct stat st;
    if (stat(argv[1], &st) != 0) {
        fprintf(stderr, "Erro: arquivo não encontrado: '%s'\n", argv[1]);
        return 1;
    }
    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "Erro: '%s' não é um arquivo regular.\n", argv[1]);
        return 1;
    }

    char out_path[MAX_PATH];
    build_output_path(argv[1], argv[2], out_path, sizeof(out_path));

    printf("\nSelecione o tipo de parser:\n");
    printf("  1. Posição de Estoque\n");
    printf("\nOpção: ");
    fflush(stdout);

    int opcao;
    if (scanf("%d", &opcao) != 1) {
        fprintf(stderr, "Erro: entrada inválida.\n");
        return 1;
    }

    switch (opcao) {
        case 1:
            printf("\nAnalisando arquivo com parser 'Posição de Estoque'...\n");
            if (!parse_posicao_estoque(argv[1], out_path))
                return 1;
            break;
        default:
            fprintf(stderr, "Erro: opção '%d' não reconhecida.\n", opcao);
            return 1;
    }

    printf("Arquivo JSON gerado com sucesso: %s\n\n", out_path);
    return 0;
}
