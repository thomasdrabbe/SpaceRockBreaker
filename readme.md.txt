# Space Rock Breaker

Een incrementele space shooter gebouwd met C++ en SFML.

## Vereisten
- Visual Studio 2022
- CMake 3.16+
- SFML 2.6.1

## Bouwen
```bat
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

## Update-zip en release (PowerShell)

Vanaf de repo-root (`SpaceRockBreaker`):

- **`make_update_zip.ps1`** — bouwt (tenzij `-SkipBuild`) de **Release**-preset, stopt game + launcher + DLLs + `assets` in `.update_package`, maakt `SpaceRockBreaker.zip` en kopieert naar `installer_output/`. Optioneel: `-Version 1.2.3` zet eerst `version.txt`. Let op: een release-build bump’t automatisch het patch-nummer in `version.txt` / `installer.iss` als dat in CMake staat.

  ```powershell
  .\make_update_zip.ps1
  .\make_update_zip.ps1 -SkipBuild
  .\make_update_zip.ps1 -Version 1.0.80 -SkipBuild
  ```

- **`sync_release_artifacts.ps1`** — synchroniseert `installer.iss` met `version.txt`, pakt opnieuw in met `make_update_zip.ps1 -SkipBuild`, controleert dat de zip dezelfde versie bevat. Met **`-AutoGit`** doet het daarna `git add -A`, commit en `git push` (committekst o.a. `-CommitMessage "..."`).

  ```powershell
  .\sync_release_artifacts.ps1
  .\sync_release_artifacts.ps1 -AutoGit -CommitMessage "Release 1.0.80"
  ```

- **`installer.iss`** — Inno Setup-script voor de Windows-installer (AppVersion wordt door de sync/build scripts afgestemd op `version.txt`).

- **`assets/sounds/split_into_10_shots.ps1`** — hulp-script voor geluidsassets (ontwikkelaars).