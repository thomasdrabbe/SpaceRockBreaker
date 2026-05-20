# Update uitrollen (launcher / GitHub)

De launcher vergelijkt de lokale build met `version.txt` op **GitHub `main`** en downloadt bij een hogere versie de **zip**. Gebruik onderstaande checklist zodat de **semver vaststaat** (geen extra patch-bump tijdens de ship-build) en alles op elkaar blijft staan.

## Release vanaf macOS (aanbevolen)

GitHub Actions bouwt Windows automatisch na elke push naar **`main`** (workflow: **Windows release**).

1. Test lokaal op Mac: `./scripts/build-macos.sh`
2. Zet **`version.txt`** op het nieuwe nummer (bijv. `1.1.39`) — **zelf** verhogen; CI bumpet niet.
3. Commit je code + `version.txt`, push naar `main`.
4. Open **GitHub → Actions → Windows release** en wacht tot groen (~10–20 min eerste keer, daarna sneller door cache).
5. De job commit/pusht **`SpaceRockBreaker.zip`**, **`installer_output/…`** en de versioned zip. Windows-spelers met de launcher krijgen daarna de update.

Je hoeft **geen** Windows-pc meer voor de update-zip. Optioneel: tag `vX.Y.Z` voor een GitHub Release-pagina.

**Let op:** alleen wijzigingen onder `src/`, `version.txt`, enz. triggeren de build (zie `paths` in `.github/workflows/windows-release.yml`). Een commit met alleen documentatie start geen Windows-build.

---

## Kort: waarom auto-bump uit bij release?

Met `SRB_AUTO_BUMP_VERSION_ON_BUILD=ON` (standaard voor dev) verhoogt elke Release-build de patch in `version.txt` en `installer.iss`. Dat is handig lokaal, maar bij een **geplande release** wil je zelf bepalen welk nummer live gaat. Zet auto-bump daarom **uit**, stel de versie handmatig in, bouw één keer, commit/push/tag, en zet daarna auto-bump weer **aan**.

---

## Checklist (Windows / PowerShell)

Vervang `X.Y.Z` door je release, bijvoorbeeld `1.0.88`.

### 1. Auto-bump uitzetten en opnieuw configureren

Eenmalig CMake configure met optie uit (vanaf repo-root):

```powershell
cmake -S . -B build -DSRB_AUTO_BUMP_VERSION_ON_BUILD=OFF
```

*(Als je toolchain/preset anders gebruikt, blijf dezelfde generator/vcpkg-args gebruiken als normaal; alleen `-DSRB_AUTO_BUMP_VERSION_ON_BUILD=OFF` is essentieel.)*

### 2. Versie vastzetten

- Zet de eerste regel van **`version.txt`** op `X.Y.Z` (alleen die regel, geen extra newline nodig).
- **`sync_release_artifacts.ps1`** sync’t `installer.iss` (`AppVersion`) met `version.txt` — zie stap 4, of pas `installer.iss` handmatig gelijk aan als je geen script draait.

### 3. Release bouwen + zip’s maken

```powershell
.\make_update_zip.ps1
```

Dit bouwt Release en vult o.a.:

- `SpaceRockBreaker.zip` (repo-root)
- `SpaceRockBreaker_X.Y.Z.zip` (repo-root)
- `installer_output\SpaceRockBreaker.zip`
- `installer_output\SpaceRockBreaker_X.Y.Z.zip`

Controleer in de zip dat **`version.txt`** = `X.Y.Z`.

### 4. Git: commit, push, tag

```powershell
.\sync_release_artifacts.ps1 -AutoGit -CommitMessage "versie X.Y.Z: korte omschrijving"
```

Of handmatig: alles committen wat bij de release hoort (sources + `version.txt` + `installer.iss` + zip’s), push naar `main`, daarna:

```powershell
git tag vX.Y.Z -m "X.Y.Z"
git push origin main
git push origin vX.Y.Z
```

### 5. Auto-bump weer aanzetten (volgende dev-builds)

```powershell
cmake -S . -B build -DSRB_AUTO_BUMP_VERSION_ON_BUILD=ON
```

*(Of verwijder de cache-optie en configureer opnieuw met je gebruikelijke preset — standaard in `CMakeLists.txt` staat de optie op `ON`.)*

### 6. GitHub (optioneel)

- Maak een **Release** vanaf tag `vX.Y.Z`.
- Upload **`SpaceRockBreaker_X.Y.Z.zip`** als download als je dat zo wilt documenteren; de launcher volgt doorgaans **`main`** + zip zoals jij die publiceert (zie launcher-config in `launcher/`).

---

## Snelle referentie

| Situatie | `SRB_AUTO_BUMP_VERSION_ON_BUILD` |
|----------|----------------------------------|
| Dagelijks bouwen / feature-work | **ON** (standaard) |
| Geplande update uitrollen | **OFF** tot commit + tag klaar is, daarna weer **ON** |

| Script | Doel |
|--------|------|
| `make_update_zip.ps1` | Release-build + zip’s (+ `-SkipBuild` alleen herpakken) |
| `sync_release_artifacts.ps1` | `installer.iss` sync, zip verifiëren, optioneel `-AutoGit` commit + push |

---

## Bekende valkuilen

- **Dubbele build met auto-bump aan:** patch springt onbedoeld op (bijv. 85 → 86) vóór je commit.
- **Zip en `version.txt` op GitHub niet gelijk:** launcher gedrag wordt onvoorspelbaar; altijd zip-inhoud checken.
- Na wijziging van CMake-opties: bij twijfel **`cmake -S . -B build ...`** opnieuw draaien vóór `make_update_zip.ps1`.
