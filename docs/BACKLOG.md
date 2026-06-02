# Space Rock Breaker — Backlog

Living document for planned work. Check off items as they ship.

**Legend:** P0 = critical · P1 = high · P2 = polish · P3 = later · S/M/L = effort

---

## A — Vertrouwen & snelle wins

| ID | Item | Size | Prio | Status |
|----|------|------|------|--------|
| B1 | Tab-wissel koopt per ongeluk upgrade | S | P0 | done |
| UX21 | Geen auto-switch naar Skill Tree; rode tab/sectie badges | M | P1 | done |
| G7 | Space = start run op basis (Mining) | S | P1 | done |
| U1 | Grotere ore + credits in side panel | S | P2 | done |
| U2 | Grotere Plinko balls-weergave | S | P2 | done |
| G8 | Run-end outro (~1s): veld blijft, sfx + overlay, dan basis | S | P1 | done |

## B — Onboarding

| ID | Item | Size | Prio | Status |
|----|------|------|------|--------|
| T1 | Tutorial: rode indicators (tab → sectie → node) | L | P1 | done (basis) |
| T2 | Pad: Skill Tree → Asteroids → Ore Value → Ship → Warp → Mining | — | P1 | done |
| T3 | Na basis: hint Start run + Space (G7) | S | P2 | done |

## C — UX polish

| ID | Item | Size | Prio | Status |
|----|------|------|------|--------|
| UX1 | Laatste run-reden op basis ("Fuel op" / "Botsing") | S | P2 | done |
| UX2 | Warp-doel zichtbaar (ore X/Y, boss gate) | S | P2 | planned |
| UX3 | Shortcuts op knoppen ([Space], tabs 1–5) | S | P2 | planned |
| UX4 | Na run-end: Mining-tab + pulse Start (geen auto Skill Tree) | S | P2 | done |
| UX5 | Notificatie-queue / minder spam | M | P3 | planned |
| UX6 | Tab-click bug (duplicate of B1) | — | — | merged → B1 |
| UX7 | Skill tree: afford/grey node styling | M | P2 | planned |
| UX8 | Locked tooltip met andere sectie-tab | S | P2 | partial |
| UX9 | Highlight volgende logische upgrade na koop | M | P3 | planned |
| UX10 | Plinko: grote Balls x/y + ore cost | S | P2 | overlaps U2 |
| UX11 | Side panel: volgende boss + crystal preview | S | P2 | planned |
| UX12 | Waarschuwing ore bijna vol (auto-Plinko) | S | P3 | planned |
| UX13 | Shield buffer uitleg bij schip | S | P3 | planned |
| UX14 | Target mode altijd zichtbaar in side panel | S | P2 | planned |
| UX15 | Pauze-menu: duidelijke Terug/Hervat | S | P3 | planned |
| UX16 | Zone-kiezer: aanbevolen zone | M | P3 | planned |

## D — Balans (na 1e boss te makkelijk)

| ID | Item | Size | Prio | Status |
|----|------|------|------|--------|
| BAL1 | Zachtere crystal-curve vroege bosses | S | P1 | done |
| BAL1b | Ore tier zone-gates + plinko/credit soft caps | S | P1 | done |
| GEM | Gem drops, cap-break, crafting, save v24 | L | P1 | done |
| BAL2 | Auto-Plinko: halve na 1e boss, vol na 2e | S | P1 | done |
| BAL3 | Upgrade-prijzen / credits na boss temperen | M | P2 | planned |
| BAL4 | Max upgrades per terugkeer basis (soft cap) | M | P3 | planned |

## E — Gameplay / platform (deels live)

| ID | Item | Size | Prio | Status |
|----|------|------|------|--------|
| G1 | Shield buffers, geen run-HP | — | — | done (1.1.56+) |
| G2 | Fuel on Pickup genert | — | — | done |
| G3 | Target Priority side panel | — | — | done (1.1.53+) |
| G4 | Skill tree secties + scroll | — | — | done (1.1.55+) |
| G5 | Sectie-indeling Ship/Asteroids/Plinko/Keukenlaatje | M | P2 | partial (tabs) |
| G6 | Roadmap upgrades (missiles, …) | L | P3 | see UPGRADE_ROADMAP.txt |
| R1 | Windows CI + launcher zip | — | — | done (1.1.57) |
| R2 | macOS .app lokaal | — | — | done |
| R3 | macOS auto-update launcher | L | P3 | planned |

---

## Implementatie-volgorde

1. A: B1, UX21, G7, U1, U2  
2. B: T1–T3 (deelt badge/highlight systeem met UX21)  
3. C: UX1–UX4  
4. D: BAL1–BAL3  
