

# Spectre Divide's Custom Metagame Backend Server
# made by @OhmV-IR and @AstroVal0

> **Status:** on pause for now. PRs welcome.

## What this repo contains

- HTTP handlers (some static, some dynamic)
- WebSocket RPC handlers (static + a few dynamic processors)
- SQLiteCpp
- Protobuf models
- vcpkg manifest dependencies (repo includes `vcpkg` as a submodule)

## Requirements

This is currently set up primarily for **Windows** (can build for linux, check prod branch):

- Windows 10+ (targeted)
- Visual Studio / MSVC toolchain (or clang-cl) https://visualstudio.microsoft.com/downloads/?q=build+tools#build-tools-for-visual-studio-2026

> Dependency management is via **vcpkg** (manifest mode). You need to build with the vcpkg toolchain file enabled.

You can follow this tutorial if you want (make sure to read about the configuration first)

https://youtu.be/j1DOWg2PbLA

## Clone

git clone https://github.com/astroval0/pragmabackend
cd pragmabackend
git submodule update --init --recursive

## Configuration (`auth.json`)

Create an `auth.json` in the **repo root**:

```json
{
  "steamApiKey": "<YOUR STEAM WEB API KEY HERE https://steamcommunity.com/dev/apikey >"
}
```

### Vivox-Voice branch

If you're on the `vivox reimplementation` branch and you actually want to use voice / text, extend `auth.json` with (edit nevermind i forgot to push those changes, will do at some point lol):

```json
{
  "steamApiKey": "<...>",
  "vivox": {
    "server": "",
    "domain": "",
    "issuer": "",
    "key": ""
  }
}
```

> **Do not commit `auth.json` lol.**

## Build



### Recommended: JetBrains CLion (Windows)

In **Settings -> Build, Execution, Deployment -> CMake**, set:

* **Generator:** Ninja (default is fine)
* **CMake options:** set the vcpkg toolchain file, e.g.
* * if your absolute path is `E:\dev\spectre\srv\pragmabackend` then your toolchain file path is
  * `E:\dev\spectre\srv\pragmabackend\vcpkg\scripts\buildsystems\vcpkg.cmake`

```
-DCMAKE_TOOLCHAIN_FILE="<path>"
```


## Run

Logs are written to `logs/app.log`.

## Default ports

By default the server starts three listeners:

* **Game HTTP:** `http://127.0.0.1:8081`
* **Social HTTP:** `http://127.0.0.1:8082`
* **WebSocket:** `ws://127.0.0.1:80`

## Client / launcher

Any client needs to point at the backend address:

* `http://127.0.0.1:8081` (game)
* `http://127.0.0.1:8082` (social)

This project is meant for local, but you can forward it publicly.

## Contributing

PRs welcome. If you contribute:

* keep changes focused (one feature/fix per PR)
* don't commit secrets (`auth.json`, tokens, private keys)
* include a quick note on how to test the change locally

## Troubleshooting

**CMake can't find packages**

* You probably forgot the vcpkg toolchain file. Set `-DCMAKE_TOOLCHAIN_FILE="<your absolute path plus \vcpkg\scripts\buildsystems\vcpkg.cmake > "`.
* * if your absolute path is `E:\dev\spectre\srv\pragmabackend` then your toolchain file path is `E:\dev\spectre\srv\pragmabackend\vcpkg\scripts\buildsystems\vcpkg.cmake`
 
**failed to open InventoryStore file**
* you didnt follow the instructions properly so it couldnt build fully.

**Port already in use**

* Change the port defines and rebuild. Make sure whatever client you use is updated too.

