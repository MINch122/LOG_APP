# 🌵 CACTUS  
**A Convenient Alternative to the cFS Telecommand Utility System**

```
          ▄█▄ ✿
         ▐███▌
      ▄█▄▐███▌▄█▄
     ▐██████████▌
      ▀▀▐███▌▀▀
        ▐███▌ ▄█▄
        ▐███▌ ▐███▌▄█▄    ∧ ∧
        ▐███▌ ▐██████▌  (=^·^=)
        ▐███▌  ▀▐███▌    (  ˘)つ
        ▐███▌   ▐███▌     U U
     ▄▄▄█████▄▄▐███▌
    ░░░░░░░░░░░░░░░░░░
```

---

## Overview

CACTUS is a lightweight, user-friendly ground system GUI for NASA's
[core Flight System (cFS)](https://github.com/nasa/cFS).

It was written as a sane replacement for the original cFS ground system, which:

- Stores command definitions in opaque pickle files
- Requires an infinately painful wizard to add a single command
- Has no live packet preview
- Has no mission-portable configuration

CACTUS is here for you to provide:

- A more coherent **JSON-based command definitions and mission constants**
- **Live hex preview** of every packet before you send it
- **Runtime-editable IP, port, MID and Function Code**
- **Adding commands during runtime** and saving back to the JSON
- **Real-time telemetry display** with MID, sequence count, and hex preview
- And a nice cat sitting next to the cactus

---

## Requirements

- Python 3.5 or later (tested on 3.8+)
- PyQt5

```bash
pip install PyQt5
```

---

## Usage

```bash
python cactus.py
```

or

```bash
python3 cactus.py
```


---

## Configuring Commands

### One file per app (recommended)

Create `commands/<appname>.json`:

```json
{
  "app": "MY_APP",
  "description": "My mission application",
  "commands": [
    {
      "name": "Noop",
      "mid": "0x1880",
      "fc": 0,
      "description": "No-op command.",
      "params": []
    },
    {
      "name": "Set Mode",
      "mid": "0x1880",
      "fc": 2,
      "description": "Change the operating mode.",
      "params": [
        {
          "name": "Mode",
          "type": "enum",
          "storage": "uint8",
          "values": { "STANDBY": 0, "NOMINAL": 1, "SAFE": 2 },
          "default": "NOMINAL",
          "description": "Desired mode"
        }
      ]
    }
  ]
}
```

Click **Reload commands** (or restart) to pick up new files.

### Multiple apps in one file

Use a JSON array instead of a dict:

```json
[
  { "app": "APP_A", "commands": [ ... ] },
  { "app": "APP_B", "commands": [ ... ] }
]
```

---

## Parameter Types

| `type` | Storage | Notes |
|---|---|---|
| `uint8` / `int8` | 1 byte | |
| `uint16` / `int16` | 2 bytes | |
| `uint32` / `int32` | 4 bytes | Accepts `0x…` hex input |
| `uint64` / `int64` | 8 bytes | |
| `float` | 4 bytes | IEEE 754 single-precision |
| `double` | 8 bytes | IEEE 754 double-precision |
| `string` | `length` bytes | Null-padded fixed-length ASCII. **Requires `"length"`** |
| `bytes` | `length` bytes | Raw hex input (space-separated). **Requires `"length"`** |
| `enum` | `storage` type (default `uint16`) | Dropdown. **Requires `"values": {"NAME": int, ...}`** |

**Array parameters** — add `"count": N` to any scalar type to get N consecutive
values encoded from a single comma-separated input field:

```json
{ "name": "Samples", "type": "float", "count": 4, "default": "0.0, 0.0, 0.0, 0.0" }
```

**Optional fields** on every parameter:

| Field | Description |
|---|---|
| `default` | Pre-filled value shown in the form |
| `description` | Tooltip shown on hover |
| `length` | Byte length for `string` / `bytes`; accepts a mission-defs constant name |
| `count` | Array element count; accepts a mission-defs constant name |

---

## Mission Constants (`mission_defs.json`)

Many cFS string parameters are sized by compile-time constants like
`OS_MAX_PATH_LEN` or `CFE_TBL_MAX_FULL_NAME_LEN`. Instead of
scattering the same magic number across dozens of command files, define
them once:

**`commands/mission_defs.json`**
```json
{
  "_comment": "Edit these to match your target's CMake configuration.",
  "OS_MAX_API_NAME":  20,
  "OS_MAX_PATH_LEN":  64,
  "CFE_TBL_MAX_FULL_NAME_LEN": 40
}
```

Then reference them by name in any command file:

```json
{ "name": "AppName",  "type": "string", "length": "OS_MAX_API_NAME" }
{ "name": "FileName", "type": "string", "length": "OS_MAX_PATH_LEN" }
```

Simple arithmetic is supported too:

```json
{ "name": "CDSName", "type": "string", "length": "OS_MAX_API_NAME + 2" }
```

Keys beginning with `_` are treated as comments and ignored.
`mission_defs.json` is a reserved filename and is never loaded as an app.

### Default constants (standard cFS Draco values)

| Constant | Default | Typical use |
|---|---|---|
| `OS_MAX_API_NAME` | 20 | App names, entry-point names |
| `OS_MAX_PATH_LEN` | 64 | File paths |
| `OS_MAX_SYM_LEN` | 64 | Symbol / function names |
| `CFE_MISSION_MAX_API_LEN` | 20 | cFE-level API name alias |
| `CFE_MISSION_MAX_PATH_LEN` | 64 | cFE-level path alias |
| `CFE_TBL_MAX_FULL_NAME_LEN` | 40 | `APP_NAME.table_name` |
| `CFE_ES_CDS_MAX_FULL_NAME_LEN` | 38 | `APP_NAME.cds_name` |
| `CFE_MISSION_EVS_MAX_MESSAGE_LENGTH` | 122 | EVS message text |

---

## Adding Commands at Runtime

Click the **`+`** button above the command tree.
Fill in the app name, MID, FC, description, and any parameters.
On **OK**, the command is appended to `commands/<app_name>.json` and the
tree is reloaded automatically.

The saved JSON uses integer `length`/`count` values, not symbolic names.
If you want symbolic names, open the file in a text editor afterward and
replace the numbers manually.

---

## Credits

- Built with **PyQt5**
- Entire code written by *Claude Sonnet 4.6*  
- Directed by *ryu@yonsei.ac.kr*

---
