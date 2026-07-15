# jsonfylinx

Converts ERP-exported TXT reports into JSON.

## Usage

```
jsonfylinx <file_path> <output_name> [parser]
```

The output JSON is saved in the same directory as the input file. When
`[parser]` (1, 2, 3 or 4) is omitted, the report type is **auto-detected** from
the title line in the file header.

## Parsers

| # | Name                          | Description                                      |
|---|-------------------------------|--------------------------------------------------|
| 1 | Posição de Estoque            | Stock position report (table format)             |
| 2 | Valor do Estoque              | Stock value report (id, produto, custo, venda)   |
| 3 | Produtividade por Funcionários | Sales productivity per employee (nested JSON)   |
| 4 | Movimentação de Produtos      | Monthly stock movement — total Entrada per product (nested JSON) |

Auto-detection reads the centered title in the header block (e.g.
`POSIÇÃO DE ESTOQUE`) and maps it to the matching parser.

## Use as a library (C / FFI)

The parsing logic is exposed as a flat C ABI in `jsonfylinx.h`, so another
program can drive it without spawning a subprocess:

```c
#include "jsonfylinx.h"

jfx_status_t st = jfx_convert("in.txt", "out.json", JFX_AUTO); /* detects + converts */
if (st != JFX_OK) fprintf(stderr, "%s\n", jfx_status_str(st));
```

- `jfx_detect(path)` — returns `JFX_POSICAO_ESTOQUE` / `JFX_VALOR_ESTOQUE` /
  `JFX_PRODUTIVIDADE` / `JFX_MOVIMENTACAO_PRODUTOS`, or `JFX_AUTO` (0) if it
  can't classify.
- `jfx_convert(in, out, parser)` — converts; pass `JFX_AUTO` to detect first.
  The output path is used literally. No function prints to stdout/stderr.
- `jfx_periodo_ym(path, buf, buf_sz)` — writes the report period's year-month as
  `"YYYY-MM"` into `buf` (needs `buf_sz >= 8`); returns 1 on success, 0 otherwise.
  Lets the caller name the output file (e.g. `2018-03.json`).

Build the libraries with `make lib`:

- `bin/libjsonfylinx.a` — static (link directly; recommended for Rust `cc`/`build.rs`)
- `bin/libjsonfylinx.so` — shared

From Rust, the simplest path is compiling `jsonfylinx.c` via the `cc` crate in
`build.rs` and declaring the three functions in an `extern "C"` block.

## Download

Pre-built binaries are in the `bin/` folder:

- **Linux:** `bin/jsonfylinx`
- **Windows:** `bin/jsonfylinx.exe`

## Build from source

**Linux:**
```
make linux
sudo make install
```

**Windows (cross-compile from Linux, requires mingw-w64):**
```
sudo apt install mingw-w64
make windows
```

**Both at once:**
```
make release
```
