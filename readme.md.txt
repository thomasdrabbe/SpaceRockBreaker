# Space Rock Breaker

Een incrementele space shooter gebouwd met C++17, SFML 3 en ImGui-SFML.

Ondersteunde platformen: **Windows** (primair, incl. installer/launcher) en **macOS** (Apple Silicon en Intel).

## Vereisten

### Windows
- Visual Studio 2022 of nieuwer (werkbelasting “Desktop development with C++”)
- CMake 3.16+
- [vcpkg](https://vcpkg.io/) (bijv. `C:\vcpkg` of `%VCPKG_ROOT%`)

### macOS
- Xcode Command Line Tools: `xcode-select --install`
- CMake 3.16+ en Ninja: `brew install cmake ninja`
- vcpkg in `~/vcpkg` (of zet `VCPKG_ROOT`):

  ```bash
  git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
  ~/vcpkg/bootstrap-vcpkg.sh
  export VCPKG_ROOT=~/vcpkg
  ```

## Bouwen

### Windows (Visual Studio)

```bat
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

Executable: `build\Release\SpaceRockBreaker.exe`

Of met presets: `cmake --preset default` en `cmake --build --preset release`

### macOS

Zorg dat `VCPKG_ROOT` wijst naar je vcpkg-installatie (`~/vcpkg`). Snel bouwen:

```bash
./scripts/build-macos.sh
```

Handmatig (Apple Silicon):

```bash
export VCPKG_ROOT=~/vcpkg
cmake --preset macos-arm64
cmake --build --preset macos-release
```

Zonder Homebrew: Xcode CLI tools + vcpkg volstaan; het script kan optioneel CMake/Ninja in `.tools/` gebruiken (eerste build via Cursor/agent heeft die geïnstalleerd).

Op Intel Mac gebruik preset `macos-x64` in plaats van `macos-arm64`.

Executable: `build-macos/SpaceRockBreaker`

Starten:

```bash
./build-macos/SpaceRockBreaker
```

Bij de eerste configure downloadt vcpkg automatisch SFML 3 en ImGui-SFML (manifest in `vcpkg.json`).

## Assets en lettertypen

De map `assets/` wordt bij build naast het programma gekopieerd. Zet minstens één UI-font in `assets/` (bijv. `font.ttf` of `NotoSans-Regular.ttf`). Als die ontbreekt, valt het spel terug op een systeemfont (Arial op Windows/macOS).

## Saves

Save-bestanden (`srb_save_0.bin`, …) worden in de werkmap van het programma gezet. Bij start wordt de map van de executable gebruikt als werkmap, zodat saves naast de binary blijven.

Er is **geen automatisch laden** bij opstarten; alleen via hoofdmenu → **Doorgaan**.

## Windows-only: updates en installer

- **`make_update_zip.ps1`** — release-zip + launcher (PowerShell)
- **`installer.iss`** — Inno Setup-installer
- **`SpaceRockLauncher`** — auto-update (alleen Windows)

Op macOS bouw en run je alleen `SpaceRockBreaker`.

## Sneltoetsen in de game

| Toets | Actie        |
|-------|--------------|
| 1–4   | Tabs         |
| Space | Plinko drop / warp (mining) |
| S     | Handmatig opslaan |
| M     | Geluid aan/uit |
