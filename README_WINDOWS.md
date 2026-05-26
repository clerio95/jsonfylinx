# jsonfylinx — Windows Guide

Converts ERP-exported TXT reports into JSON.

## Requirements

No installation required. The pre-built executable is in the `bin/` folder:

```
bin\jsonfylinx.exe
```

Copy it anywhere convenient — your Desktop, `C:\Tools\`, or directly into the folder where your TXT reports live.

## Running the program

Open **Command Prompt** (`Win + R` → type `cmd` → Enter) or **PowerShell**, then run:

```
jsonfylinx.exe <path_to_file> <output_name>
```

| Argument        | Description                                      |
|-----------------|--------------------------------------------------|
| `path_to_file`  | Path to the ERP-exported `.txt` file             |
| `output_name`   | Name for the output file (`.json` added automatically) |

The program will then show a menu asking which parser to use. Type the number and press Enter.

## Parsers

| # | Name                          | Description                                  |
|---|-------------------------------|----------------------------------------------|
| 1 | Posição de Estoque            | Stock position report — outputs `id`, `categoria`, `produto`, `estoque` |
| 2 | Valor do Estoque Reajustes    | Price adjustment report — outputs `id`, `produto`, `custo`, `venda` |

## Examples

**Easiest: run from the same folder as the file**

Open a Command Prompt in the folder that contains the TXT file (you can Shift+Right-click the folder → "Open command window here"), then:

```
jsonfylinx.exe relatorio.txt saida
```

This reads `relatorio.txt` and creates `saida.json` in the same folder.

---

**Using a full path with forward slashes**

Windows accepts forward slashes in paths. Use them so the output file is placed next to the input file:

```
jsonfylinx.exe C:/Users/Fulano/Documentos/relatorio.txt saida
```

Output: `C:/Users/Fulano/Documentos/saida.json`

> **Note:** If you use backslashes (`C:\Users\...`), the output file will be created in whichever directory you are currently in, not next to the input file.

---

**Placing the .exe in the same folder as the reports**

If you copy `jsonfylinx.exe` into the folder that contains your TXT files, you can always use just the filename:

```
cd C:\Users\Fulano\Documentos\Relatorios
jsonfylinx.exe relatorio.txt saida
```

## Sample session

```
C:\Users\Fulano> jsonfylinx.exe C:/relatorios/estoque.txt resultado

Selecione o tipo de parser:
  1. Posição de Estoque
  2. Valor do Estoque Reajustes

Opção: 1

Analisando arquivo com parser 'Posição de Estoque'...
Arquivo JSON gerado com sucesso: C:/relatorios/resultado.json
```

## Troubleshooting

**Windows blocked or warned about the file**
Windows Defender SmartScreen may show a warning the first time you run an unrecognized executable. Click "More info" → "Run anyway" to proceed.

**"'jsonfylinx.exe' is not recognized..."**
The executable is not in your PATH. Either navigate to its folder first (`cd C:\Tools`) or provide the full path to it:
```
C:\Tools\jsonfylinx.exe relatorio.txt saida
```

**Output file appears in the wrong place**
You used backslashes in the file path. Switch to forward slashes, or run the command from inside the same folder as the TXT file.
