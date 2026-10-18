# Aseprite MCP Setup

This guide connects [diivi/aseprite-mcp](https://github.com/diivi/aseprite-mcp)
to OpenCode on Arch Linux or Windows, using Aseprite installed through Steam.
The MCP server runs locally and invokes Aseprite in batch mode; it does not
control the Aseprite window. You do not need to build Aseprite from source or
put the MCP server inside this game repository.

The server currently requires Python 3.13 or newer and `uv`. Check the upstream
README for current requirements and tools before installing, since the project
may change: <https://github.com/diivi/aseprite-mcp>.

## 1. Install prerequisites

### Arch Linux

Install Git, Python, and uv:

```sh
sudo pacman -S git python uv
```

Confirm Python is version 3.13 or newer:

```sh
python --version
```

If your installed Python is older than 3.13, install a supported Python version
using your preferred Arch package or version-management method, then ensure the
MCP server's `uv` environment uses it.

Find the Steam Aseprite executable. Common locations include
`~/.local/share/Steam/steamapps/common/Aseprite/aseprite` and
`~/.steam/steam/steamapps/common/Aseprite/aseprite`; Steam libraries can be in
other locations:

```sh
find "$HOME/.local/share/Steam" "$HOME/.steam" \
  -type f -path '*/Aseprite/aseprite' -print 2>/dev/null
```

Check that the result exists and is executable:

```sh
test -x "/full/path/to/Aseprite/aseprite" && echo "Aseprite executable found"
```

### Windows

Install Git, Python 3.13 or newer, and `uv`. For example, if `winget` is
available:

```powershell
winget install --id Git.Git
winget install --id Python.Python.3.13
winget install --id astral-sh.uv
```

Open a new PowerShell window, then check the installations:

```powershell
python --version
Get-Command uv.exe | Select-Object -ExpandProperty Source
```

Find the Steam install directory in Steam under **Aseprite → Properties →
Installed Files → Browse**. The default executable path is usually:

```text
C:\Program Files (x86)\Steam\steamapps\common\Aseprite\Aseprite.exe
```

If Aseprite is in another Steam library, use that library's path instead.

## 2. Get the MCP server

Clone the server somewhere outside the game repository. The examples use a
`src` directory in your home folder.

Arch Linux:

```sh
mkdir -p ~/src
git clone https://github.com/diivi/aseprite-mcp.git ~/src/aseprite-mcp
cd ~/src/aseprite-mcp
uv sync
```

Windows PowerShell:

```powershell
New-Item -ItemType Directory -Force "$HOME\src" | Out-Null
git clone https://github.com/diivi/aseprite-mcp.git "$HOME\src\aseprite-mcp"
Set-Location "$HOME\src\aseprite-mcp"
uv sync
```

## 3. Configure OpenCode

Add an `aseprite` entry under `mcp.servers` in your OpenCode configuration.
Use the **absolute paths for your own machine** in the examples below. Keep
existing configuration; merge this server entry into the existing
`mcp.servers` object rather than replacing the whole file.

### Arch Linux configuration

Find the absolute paths with `command -v uv` and your Aseprite search result.
For example:

```jsonc
{
  "$schema": "https://opencode.ai/config.json",
  "mcp": {
    "servers": {
      "aseprite": {
        "type": "local",
        "command": ["/usr/bin/uv", "--directory", "/home/YOUR_USER/src/aseprite-mcp", "run", "-m", "aseprite_mcp"],
        "environment": {
          "ASEPRITE_PATH": "/home/YOUR_USER/.local/share/Steam/steamapps/common/Aseprite/aseprite"
        }
      }
    }
  }
}
```

Replace `/usr/bin/uv`, `/home/YOUR_USER/src/aseprite-mcp`, and
`ASEPRITE_PATH` with the actual paths on your system.

### Windows configuration

Get the absolute `uv.exe` path by running
`(Get-Command uv.exe).Source` in PowerShell. Use JSON-escaped backslashes in the
configuration:

```jsonc
{
  "$schema": "https://opencode.ai/config.json",
  "mcp": {
    "servers": {
      "aseprite": {
        "type": "local",
        "command": [
          "C:\\Users\\YOUR_USER\\.local\\bin\\uv.exe",
          "--directory",
          "C:\\Users\\YOUR_USER\\src\\aseprite-mcp",
          "run",
          "-m",
          "aseprite_mcp"
        ],
        "environment": {
          "ASEPRITE_PATH": "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Aseprite\\Aseprite.exe"
        }
      }
    }
  }
}
```

Replace the `uv.exe`, server, and Aseprite paths with the actual paths on your
machine. If your Aseprite path contains a different Steam library, use that
path.

## 4. Verify the connection

Restart or reload OpenCode, then check the MCP server list with `opencode mcp
list` or the `/mcps` command. The `aseprite` server should show as connected.
Try an inspection-only request first, such as asking the server to report
metadata for an existing `.aseprite` file.

Before editing an asset, make a copy and point the first test at the copy. The
server can change and save sprite files, and its `run_lua_script` tool executes
Lua code on the host with unrestricted access. Only run scripts and requests
you trust. Do not give the MCP server Steam credentials; a local install does
not need them.

## Troubleshooting

- **Server fails to start:** check that Python is 3.13 or newer, `uv sync`
  completed in the cloned server directory, and the configured `uv` path is
  correct.
- **Aseprite cannot be launched:** check `ASEPRITE_PATH` points to the actual
  Steam executable and that your user can execute it.
- **OpenCode cannot find the server:** validate the JSON/JSONC syntax and make
  sure the entry is nested under `mcp.servers`.
- **Steam library is elsewhere:** locate Aseprite using Steam's **Browse**
  action (Windows) or search the Steam library directories (Linux), then update
  `ASEPRITE_PATH`.

For upstream-specific troubleshooting and updates, see the
[aseprite-mcp README](https://github.com/diivi/aseprite-mcp).
