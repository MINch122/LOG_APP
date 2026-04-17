"""Load, validate, and persist command definitions from JSON files.

JSON format (one file per app, or a list of apps in one file):

    Single-app file:
    {
      "app": "MY_APP",
      "description": "Optional description shown in the UI",
      "commands": [ <command>, ... ]
    }

    Multi-app file:
    [ { "app": "APP_A", "commands": [...] }, { "app": "APP_B", ... } ]

Command object:
    {
      "name":        "Noop",
      "mid":         "0x1880",    <- hex string or integer
      "fc":          0,
      "description": "No-op command",
      "params":      [ <param>, ... ]   <- may be omitted / empty
    }

Parameter object:
    {
      "name":        "FileName",
      "type":        "string",       <- see payload.py for all types
      "length":      "OS_MAX_PATH_LEN",  <- int OR mission_defs constant name
      "count":       1,              <- optional, for fixed-size arrays
      "description": "Path to file",
      "default":     "",             <- optional default value (string)
      "values": { "RESTART": 1, "PROC_RESET": 2 }  <- enum only
    }

Mission defs:
    A special file named 'mission_defs.json' in the commands directory maps
    constant names to integer values.  Any 'length' or 'count' field that
    is a string (e.g. "OS_MAX_PATH_LEN") is resolved against that dict.
    Simple arithmetic expressions are also supported:
        "length": "OS_MAX_API_NAME + 2"
    Keys whose names begin with '_' are treated as comments and ignored.
"""

import json
import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional

# Reserved filename — not treated as an app definition.
MISSION_DEFS_FILENAME = 'mission_defs.json'


# ---------------------------------------------------------------------------
# Data classes
# ---------------------------------------------------------------------------

@dataclass
class CommandDef:
    name: str
    app: str
    mid: int
    fc: int
    description: str = ''
    params: List[dict] = field(default_factory=list)

    def __str__(self) -> str:
        return f"{self.app}/{self.name} MID=0x{self.mid:04X} FC={self.fc}"


@dataclass
class AppDef:
    name: str
    description: str = ''
    commands: List[CommandDef] = field(default_factory=list)


# ---------------------------------------------------------------------------
# Mission-defs resolution
# ---------------------------------------------------------------------------

def load_mission_defs(directory: Path) -> Dict[str, int]:
    """Load the mission_defs.json file from *directory*, if present.

    Returns a dict mapping constant name -> integer value.
    Keys beginning with '_' are ignored (they are treated as comments).
    """
    defs_path = directory / MISSION_DEFS_FILENAME
    if not defs_path.exists():
        return {}
    try:
        with defs_path.open(encoding='utf-8') as fh:
            raw = json.load(fh)
        result: Dict[str, int] = {}
        for k, v in raw.items():
            if k.startswith('_'):
                continue
            try:
                result[k] = int(v)
            except (TypeError, ValueError):
                pass  # skip non-integer values (e.g. nested comment arrays)
        return result
    except Exception as exc:
        print(f"[loader] Warning: could not parse {MISSION_DEFS_FILENAME}: {exc}")
        return {}


def resolve_int(value, defs: Dict[str, int]) -> int:
    """Resolve *value* to an integer, substituting mission-defs constants.

    Accepted forms:
      - int literal:          64
      - string integer:       "64"
      - constant name:        "OS_MAX_PATH_LEN"
      - arithmetic expression: "OS_MAX_API_NAME + 2"  (only +, -, *, / allowed)

    Raises ValueError if the expression cannot be resolved.
    """
    if isinstance(value, int):
        return value
    if not isinstance(value, str):
        raise TypeError(f"Expected int or string, got {type(value).__name__}")

    expr = value.strip()

    # Fast path: pure integer string
    try:
        return int(expr, 0)
    except ValueError:
        pass

    # Fast path: single constant name
    if expr in defs:
        return defs[expr]

    # Substitute all known constant names (longest first to avoid partial matches)
    substituted = expr
    for name in sorted(defs, key=len, reverse=True):
        substituted = re.sub(r'\b' + re.escape(name) + r'\b',
                             str(defs[name]),
                             substituted)

    # Only allow digits, whitespace, and simple arithmetic operators
    if not re.fullmatch(r'[\d\s\+\-\*\/\(\)]+', substituted):
        raise ValueError(
            f"Cannot safely evaluate '{value}' after substitution -> '{substituted}'. "
            f"Only +, -, *, / and known mission-defs constants are allowed."
        )

    try:
        result = eval(substituted, {'__builtins__': {}})  # noqa: S307
        return int(result)
    except Exception as exc:
        raise ValueError(
            f"Cannot resolve '{value}': expression '{substituted}' -> {exc}"
        ) from exc


def _resolve_params(params: list, defs: Dict[str, int]) -> list:
    """Return a copy of *params* with 'length' and 'count' resolved to ints."""
    if not defs:
        return params
    resolved = []
    for p in params:
        p = dict(p)  # shallow copy — don't mutate the original
        for field_name in ('length', 'count'):
            if field_name in p and isinstance(p[field_name], str):
                try:
                    p[field_name] = resolve_int(p[field_name], defs)
                except (ValueError, TypeError) as exc:
                    print(f"[loader] Warning: cannot resolve {field_name}="
                          f"{p[field_name]!r} for param '{p.get('name', '?')}': {exc}")
        resolved.append(p)
    return resolved


# ---------------------------------------------------------------------------
# Loading
# ---------------------------------------------------------------------------

def load_commands_dir(directory: str) -> List[AppDef]:
    """Scan *directory* for *.json files and return a list of AppDef objects."""
    path = Path(directory)
    if not path.is_dir():
        return []

    # Read mission constants first so they can be referenced by command files.
    defs = load_mission_defs(path)
    if defs:
        print(f"[loader] Loaded {len(defs)} mission constants from {MISSION_DEFS_FILENAME}")

    apps: Dict[str, AppDef] = {}
    for json_file in sorted(path.glob('*.json')):
        # Skip the reserved mission-defs file — it is not an app definition.
        if json_file.name == MISSION_DEFS_FILENAME:
            continue
        try:
            _load_file(json_file, apps, defs)
        except Exception as exc:
            print(f"[loader] Warning: could not parse {json_file.name}: {exc}")

    return list(apps.values())


def _load_file(path: Path, apps: Dict[str, AppDef], defs: Dict[str, int]) -> None:
    with path.open(encoding='utf-8') as fh:
        data = json.load(fh)

    if isinstance(data, list):
        for item in data:
            _load_app_dict(item, apps, defs)
    elif isinstance(data, dict) and 'commands' in data:
        _load_app_dict(data, apps, defs)
    else:
        raise ValueError("Expected a dict with 'commands' key or a list of such dicts")


def _load_app_dict(data: dict, apps: Dict[str, AppDef], defs: Dict[str, int]) -> None:
    app_name: str = data.get('app', 'Unknown').strip()
    if app_name not in apps:
        apps[app_name] = AppDef(name=app_name, description=data.get('description', ''))

    app = apps[app_name]
    for cmd_raw in data.get('commands', []):
        try:
            app.commands.append(_parse_command(cmd_raw, app_name, defs))
        except Exception as exc:
            print(f"[loader] Warning: skipping command {cmd_raw!r}: {exc}")


def _parse_command(raw: dict, app_name: str, defs: Dict[str, int]) -> CommandDef:
    mid_raw = raw['mid']
    if isinstance(mid_raw, str):
        mid = int(mid_raw, 16) if mid_raw.startswith(('0x', '0X')) else int(mid_raw)
    else:
        mid = int(mid_raw)

    params = _resolve_params(list(raw.get('params', [])), defs)

    return CommandDef(
        name=str(raw['name']),
        app=app_name,
        mid=mid,
        fc=int(raw['fc']),
        description=str(raw.get('description', '')),
        params=params,
    )


# ---------------------------------------------------------------------------
# Saving (runtime add-command)
# ---------------------------------------------------------------------------

def save_command(cmd_dict: dict, app_name: str, commands_dir: str) -> Path:
    """Append *cmd_dict* to <commands_dir>/<app_name_sanitised>.json.

    Creates the file if it doesn't exist.  Returns the path written.
    """
    directory = Path(commands_dir)
    directory.mkdir(parents=True, exist_ok=True)

    safe_name = app_name.lower().replace(' ', '_').replace('/', '_')
    file_path = directory / f"{safe_name}.json"

    if file_path.exists():
        with file_path.open(encoding='utf-8') as fh:
            existing = json.load(fh)
        # Normalise to single-app dict
        if isinstance(existing, list):
            for entry in existing:
                if entry.get('app') == app_name:
                    entry['commands'].append(cmd_dict)
                    break
            else:
                existing.append({'app': app_name, 'description': '', 'commands': [cmd_dict]})
            out = existing
        else:
            existing.setdefault('commands', []).append(cmd_dict)
            out = existing
    else:
        out = {'app': app_name, 'description': '', 'commands': [cmd_dict]}

    with file_path.open('w', encoding='utf-8') as fh:
        json.dump(out, fh, indent=2)

    return file_path
