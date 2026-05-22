# jsonfylinx

Converts ERP-exported TXT reports into JSON.

## Usage

```
jsonfylinx <file_path> <output_name>
```

The output JSON is saved in the same directory as the input file.

## Parsers

| # | Name                  | Description                        |
|---|-----------------------|------------------------------------|
| 1 | Posição de Estoque    | Stock position report (table format) |

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
