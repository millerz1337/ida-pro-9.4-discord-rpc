# RPC 9.4 — Discord Rich Presence for IDA Pro 9.4

A lightweight IDA Pro plugin that displays your current reversing session in Discord.

![IDA Pro 9.4](https://img.shields.io/badge/IDA%20Pro-9.4-blue)
![Platform](https://img.shields.io/badge/platform-Windows%20x64-lightgrey)
![License](https://img.shields.io/badge/license-MIT-green)

## Features

- Shows the file you're reversing in Discord
- Shows the current function name and address
- Elapsed time tracking
- Configurable options via `Edit → Plugins → RPC 9.4`
- Toggle RPC on/off without restarting IDA

## Discord Preview

```
IDA Pro 9.4
Reversing: target.exe
Function: main (0x140001000)
⏱ 01:23:45 elapsed
```

## Building

### Requirements

- Visual Studio 2022 (v143 toolset)
- IDA Pro SDK (9.x)
- [discord-rpc](https://github.com/discord/discord-rpc) static library (win64)

### Setup

1. Clone this repo
2. Create a `deps/` folder in the project root:

```
deps/
├── idasdk/
│   ├── include/    ← IDA SDK headers
│   └── lib/        ← ida.lib
└── discord-rpc/
    ├── include/    ← discord-rpc.h
    └── lib/        ← discord-rpc.lib
```

3. Open `rpc.sln` in Visual Studio
4. Select **Release | x64**
5. Build → Build Solution (`Ctrl+Shift+B`)
6. Copy `x64/Release/rpc94.dll` to your IDA `plugins/` folder

## Usage

1. Launch IDA Pro
2. The plugin loads automatically and connects to Discord
3. Go to `Edit → Plugins → RPC 9.4` or press `Ctrl+Alt+D` to configure

## Configuration

| Option             | Description                          | Default |
|--------------------|--------------------------------------|---------|
| Show filename      | Display current file in Discord      | ✅      |
| Show function name | Display current function             | ✅      |
| Show address       | Display function start address       | ✅      |
| Show elapsed time  | Display time since IDA was opened    | ✅      |
| RPC enabled        | Toggle Discord RPC on/off            | ✅      |

## Discord Developer Portal Setup

1. Go to [Discord Developer Portal](https://discord.com/developers/applications)
2. Create a new application (or use an existing one)
3. Go to **Rich Presence → Art Assets**
4. Upload your IDA logo as `ida_logo` (512×512 recommended)

## License

MIT
