#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#define MAX_LINE           1024
#define MAX_PRODUTO        512
#define MAX_PATH           4096
#define MAX_PRODS_PER_FUNC 64

typedef struct {
    int    id;
    char   categoria[4];
    char   produto[MAX_PRODUTO];
    double estoque;
} ItemEstoque;

typedef struct {
    int    id;
    char   produto[MAX_PRODUTO];
    double custo;
    double venda;
} ItemReajuste;

typedef struct {
    long   codigo;
    char   nome[MAX_PRODUTO];
    char   fornecedor[MAX_PRODUTO];
    double quantidade;
    char   unidade[16];
    double valor;
    double participacao_pct;
} ProdutoVenda;

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

/* Strips Brazilian thousands dots and converts decimal comma to dot. */
static double parse_br_number(const char *s) {
    char buf[64];
    int j = 0;
    for (int i = 0; s[i] && j < (int)sizeof(buf) - 1; i++) {
        if (s[i] == '.')       continue;
        else if (s[i] == ',')  buf[j++] = '.';
        else                   buf[j++] = s[i];
    }
    buf[j] = '\0';
    return strtod(buf, NULL);
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

static int parse_row_reajuste(const char *line, ItemReajuste *item) {
    if (line[0] != '|') return 0;

    char f[7][MAX_PRODUTO];
    if (split_pipes(line, f, 7) < 6) return 0;
    if (!is_digits_only(f[0])) return 0;

    item->id = atoi(f[0]);
    strncpy(item->produto, f[1], MAX_PRODUTO - 1);
    item->produto[MAX_PRODUTO - 1] = '\0';
    item->custo = parse_br_number(f[3]);
    item->venda = parse_br_number(f[5]);

    return 1;
}

static int parse_produto_venda_row(const char *line, ProdutoVenda *p) {
    if (line[0] != '|') return 0;
    char f[7][MAX_PRODUTO];
    if (split_pipes(line, f, 7) < 7) return 0;
    if (!is_digits_only(f[0])) return 0;

    p->codigo = atol(f[0]);
    strncpy(p->nome,       f[1], MAX_PRODUTO - 1); p->nome[MAX_PRODUTO - 1]       = '\0';
    strncpy(p->fornecedor, f[2], MAX_PRODUTO - 1); p->fornecedor[MAX_PRODUTO - 1] = '\0';
    p->quantidade       = parse_br_number(f[3]);
    strncpy(p->unidade, f[4], 15); p->unidade[15] = '\0';
    p->valor            = parse_br_number(f[5]);
    p->participacao_pct = parse_br_number(f[6]);
    return 1;
}

static void write_relatorio_header(FILE *out,
    const char *generated_at,
    const char *periodo_inicio, const char *periodo_fim,
    long empresa_codigo, const char *empresa_nome)
{
    fputs("{\n", out);
    fprintf(out, "  \"generated_at\": \"%s\",\n", generated_at);
    fputs("  \"periodo\": {\n", out);
    fprintf(out, "    \"inicio\": \"%s\",\n", periodo_inicio);
    fprintf(out, "    \"fim\": \"%s\"\n", periodo_fim);
    fputs("  },\n", out);
    fputs("  \"empresa\": {\n", out);
    fprintf(out, "    \"codigo\": %ld,\n", empresa_codigo);
    fputs("    \"nome\": \"", out);
    json_escape(out, empresa_nome);
    fputs("\"\n", out);
    fputs("  },\n", out);
    fputs("  \"funcionarios\": [\n", out);
}

static void emit_funcionario(FILE *out,
    long codigo, const char *nome, long total_vendas,
    ProdutoVenda *produtos, int num_produtos,
    double t_qtd, double t_valor, double t_pct,
    int first)
{
    if (!first) fputs(",\n", out);
    fputs("    {\n", out);
    fprintf(out, "      \"codigo\": %ld,\n", codigo);
    fputs("      \"nome\": \"", out);
    json_escape(out, nome);
    fputs("\",\n", out);
    fprintf(out, "      \"total_vendas\": %ld,\n", total_vendas);
    fputs("      \"produtos\": [\n", out);
    for (int i = 0; i < num_produtos; i++) {
        ProdutoVenda *p = &produtos[i];
        if (i > 0) fputs(",\n", out);
        fputs("        {\n", out);
        fprintf(out, "          \"codigo\": %ld,\n", p->codigo);
        fputs("          \"nome\": \"", out);
        json_escape(out, p->nome);
        fputs("\",\n", out);
        fputs("          \"fornecedor\": ", out);
        if (p->fornecedor[0]) {
            fputc('"', out);
            json_escape(out, p->fornecedor);
            fputs("\",\n", out);
        } else {
            fputs("null,\n", out);
        }
        fprintf(out, "          \"quantidade\": %.3f,\n", p->quantidade);
        fputs("          \"unidade\": ", out);
        if (p->unidade[0]) {
            fprintf(out, "\"%s\",\n", p->unidade);
        } else {
            fputs("null,\n", out);
        }
        fprintf(out, "          \"valor\": %.3f,\n", p->valor);
        fprintf(out, "          \"participacao_pct\": %.3f\n", p->participacao_pct);
        fputs("        }", out);
    }
    if (num_produtos > 0) fputc('\n', out);
    fputs("      ],\n", out);
    fputs("      \"totais\": {\n", out);
    fprintf(out, "        \"quantidade\": %.3f,\n", t_qtd);
    fprintf(out, "        \"valor\": %.3f,\n", t_valor);
    fprintf(out, "        \"participacao_pct\": %.3f\n", t_pct);
    fputs("      }\n", out);
    fputs("    }", out);
}

static void write_totais_gerais(FILE *out,
    double com_qtd, double com_valor,
    double sem_qtd, double sem_valor,
    double per_qtd, double per_valor)
{
    fputs("  \"totais_gerais\": {\n", out);
    fputs("    \"com_vendedor\": {\n", out);
    fprintf(out, "      \"quantidade\": %.3f,\n", com_qtd);
    fprintf(out, "      \"valor\": %.3f\n", com_valor);
    fputs("    },\n", out);
    fputs("    \"sem_vendedor\": {\n", out);
    fprintf(out, "      \"quantidade\": %.3f,\n", sem_qtd);
    fprintf(out, "      \"valor\": %.3f\n", sem_valor);
    fputs("    },\n", out);
    fputs("    \"total_periodo\": {\n", out);
    fprintf(out, "      \"quantidade\": %.3f,\n", per_qtd);
    fprintf(out, "      \"valor\": %.3f\n", per_valor);
    fputs("    }\n", out);
    fputs("  }\n}\n", out);
}

static int parse_valor_estoque_reajustes(const char *in_path, const char *out_path) {
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
    ItemReajuste item;
    int first = 1;

    fputs("[\n", out);

    while (fgets(line, sizeof(line), in)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';

        if (!parse_row_reajuste(line, &item)) continue;

        if (!first) fputs(",\n", out);
        first = 0;

        fputs("  {\n", out);
        fprintf(out, "    \"id\": %d,\n", item.id);

        fputs("    \"produto\": \"", out);
        json_escape(out, item.produto);
        fputs("\",\n", out);

        fprintf(out, "    \"custo\": %.3f,\n", item.custo);
        fprintf(out, "    \"venda\": %.3f\n", item.venda);

        fputs("  }", out);
    }

    fputs("\n]\n", out);

    fclose(in);
    fclose(out);
    return 1;
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

static int parse_produtividade_funcionarios(const char *in_path, const char *out_path) {
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

    char generated_at[32]      = {0};
    char periodo_inicio[16]    = {0};
    char periodo_fim[16]       = {0};
    long empresa_codigo        = 0;
    char empresa_nome[MAX_PRODUTO] = {0};

    double totais_com_qtd = 0, totais_com_valor = 0;
    double totais_sem_qtd = 0, totais_sem_valor = 0;
    double totais_per_qtd = 0, totais_per_valor = 0;

    /* current funcionário being built */
    long         func_codigo  = 0;
    char         func_nome[MAX_PRODUTO] = {0};
    long         func_vendas  = 0;
    ProdutoVenda produtos[MAX_PRODS_PER_FUNC];
    int          num_produtos = 0;
    int          in_func      = 0;

    int first_line     = 1;
    int first_func     = 1;
    int header_written = 0;

    char line[MAX_LINE];

    while (fgets(line, sizeof(line), in)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';

        /* Line 1: timestamp is the last 16 chars "DD/MM/YYYY HH:MM" */
        if (first_line) {
            first_line = 0;
            if (len >= 16) {
                strncpy(generated_at, line + len - 16, 16);
                generated_at[16] = '\0';
                trim(generated_at);
            }
            continue;
        }

        /* Período: search for "odo: " (avoids matching on UTF-8 accented chars) */
        if (!periodo_inicio[0]) {
            char *pp = strstr(line, "odo: ");
            if (pp) {
                pp += 5;
                strncpy(periodo_inicio, pp, 10);
                periodo_inicio[10] = '\0';
                /* "01/05/2026 a 31/05/2026": fim starts at offset 13 */
                if (strlen(pp) >= 23) {
                    strncpy(periodo_fim, pp + 13, 10);
                    periodo_fim[10] = '\0';
                }
                continue;
            }
        }

        /* Empresa */
        if (!empresa_nome[0]) {
            char *ep = strstr(line, "Empresa: ");
            if (ep) {
                ep += 9;
                empresa_codigo = atol(ep);
                char *dp = strstr(ep, " - ");
                if (dp) {
                    dp += 3;
                    char *end = strrchr(line, '|');
                    if (end && end > dp) {
                        int nlen = (int)(end - dp);
                        while (nlen > 0 && isspace((unsigned char)dp[nlen-1])) nlen--;
                        if (nlen > 0 && nlen < MAX_PRODUTO) {
                            strncpy(empresa_nome, dp, nlen);
                            empresa_nome[nlen] = '\0';
                        }
                    }
                }
                continue;
            }
        }

        /* Totais gerais — must check before "Vendas:" to avoid mismatches */
        if (strstr(line, "Total geral de vendas com")) {
            char f[5][MAX_PRODUTO];
            if (split_pipes(line, f, 5) >= 4) {
                totais_com_qtd   = parse_br_number(f[1]);
                totais_com_valor = parse_br_number(f[3]);
            }
            continue;
        }
        if (strstr(line, "Total geral de vendas sem")) {
            char f[5][MAX_PRODUTO];
            if (split_pipes(line, f, 5) >= 4) {
                totais_sem_qtd   = parse_br_number(f[1]);
                totais_sem_valor = parse_br_number(f[3]);
            }
            continue;
        }
        if (strstr(line, "Total geral de vendas no ")) {
            char f[5][MAX_PRODUTO];
            if (split_pipes(line, f, 5) >= 4) {
                totais_per_qtd   = parse_br_number(f[1]);
                totais_per_valor = parse_br_number(f[3]);
            }
            continue;
        }

        /* Funcionário header — detected by "Vendas:" (capital V, colon) */
        char *vp = strstr(line, "Vendas:");
        if (vp) {
            if (!header_written) {
                write_relatorio_header(out, generated_at, periodo_inicio, periodo_fim,
                                       empresa_codigo, empresa_nome);
                header_written = 1;
            }

            func_codigo = 0; func_nome[0] = '\0'; func_vendas = 0; num_produtos = 0;
            in_func = 1;

            /* total_vendas: number after "Vendas:" */
            char *vnum = vp + 7;
            while (*vnum == ' ') vnum++;
            func_vendas = atol(vnum);

            /* codigo and nome: "| Funcionário: NNN - NAME ... Vendas:" */
            char *cp = strchr(line, ':');
            if (cp) {
                cp += 2;
                while (*cp == ' ') cp++;
                func_codigo = atol(cp);
                char *dp = strstr(cp, " - ");
                if (dp) {
                    dp += 3;
                    int nlen = (int)(vp - dp);
                    while (nlen > 0 && isspace((unsigned char)dp[nlen-1])) nlen--;
                    if (nlen > 0 && nlen < MAX_PRODUTO) {
                        strncpy(func_nome, dp, nlen);
                        func_nome[nlen] = '\0';
                    }
                }
            }
            continue;
        }

        /* Total do vendedor — emit the current funcionário */
        if (in_func && strstr(line, "Total do vendedor")) {
            char f[6][MAX_PRODUTO];
            double t_qtd = 0, t_valor = 0, t_pct = 0;
            if (split_pipes(line, f, 6) >= 6) {
                t_qtd   = parse_br_number(f[2]);
                t_valor = parse_br_number(f[4]);
                t_pct   = parse_br_number(f[5]);
            }
            emit_funcionario(out, func_codigo, func_nome, func_vendas,
                             produtos, num_produtos, t_qtd, t_valor, t_pct, first_func);
            first_func = 0;
            in_func    = 0;
            continue;
        }

        /* Product row */
        if (in_func && num_produtos < MAX_PRODS_PER_FUNC) {
            ProdutoVenda p;
            if (parse_produto_venda_row(line, &p))
                produtos[num_produtos++] = p;
        }
    }

    /* Close funcionarios array */
    if (header_written)
        fputs("\n  ],\n", out);

    write_totais_gerais(out,
        totais_com_qtd, totais_com_valor,
        totais_sem_qtd, totais_sem_valor,
        totais_per_qtd, totais_per_valor);

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
    printf("  2. Valor do Estoque\n");
    printf("  3. Produtividade por Funcionários\n");
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
        case 2:
            printf("\nAnalisando arquivo com parser 'Valor do Estoque'...\n");
            if (!parse_valor_estoque_reajustes(argv[1], out_path))
                return 1;
            break;
        case 3:
            printf("\nAnalisando arquivo com parser 'Produtividade por Funcionários'...\n");
            if (!parse_produtividade_funcionarios(argv[1], out_path))
                return 1;
            break;
        default:
            fprintf(stderr, "Erro: opção '%d' não reconhecida.\n", opcao);
            return 1;
    }

    printf("Arquivo JSON gerado com sucesso: %s\n\n", out_path);
    return 0;
}
