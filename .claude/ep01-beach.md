# ep01-beach.md — Episodio 01: Spiaggia Croatia
# Macchina del Desiderio — aggiornare a ogni sessione

---

## Concept

Spiaggia di scogli ispirata alle coste della Croazia. Prevalentemente rocce,
poca sabbia. Zona di cruising nella pineta sovrastante. Ambientazione estiva.

La logica di cruising generale (NPC simulation) si applica all'intero livello.
Le micro-aree sono zone esplorative autonome: personaggi con micro-azioni,
ambientazioni visive specifiche. Non intaccano la logica cruising globale.

**Livello UE5:** `ep01_beach` (da creare)
**Dimensione micro-aree:** ~10×10 m ciascuna

---

## Micro-aree — stato

| ID | Nome | Concept | Asset | Blueprint | Stato |
|----|------|---------|-------|-----------|-------|
| 01 | pineta | Zona cruising nella pineta sopra la spiaggia | ○ | ○ | concept |
| 02 | scogliere | Rocce a picco sul mare, NPC in pausa/esposizione | ○ | ○ | concept |
| 03 | zona-cruising | Area centrale cruising tra rocce e pineta | ○ | ○ | concept |
| 04 | area-04 | TBD | ○ | ○ | ○ |
| 05 | area-05 | TBD | ○ | ○ | ○ |
| 06 | area-06 | TBD | ○ | ○ | ○ |
| 07 | area-07 | TBD | ○ | ○ | ○ |

---

## Struttura cartelle asset per area

```
assets/environments/ep01-beach/
  pineta/
    pineta.blend        ← modellazione alberi, sottobosco (Blender)
    pineta.hip          ← scatter procedurale pini (Houdini)
    pineta_export.fbx   ← export → UE5
    README.md           ← concept, stato, note
  scogliere/
    ...
```

---

## Asset globali livello (condivisi tra aree)

| Asset | Tool | Stato | Note |
|-------|------|-------|------|
| Terrain rocce base | Blender/UE5 Landscape | ○ | riferimento: costa Dalmazia |
| Acqua mare | UE5 Water plugin | ○ | |
| Cielo/lighting estate | UE5 Sky Atmosphere | ○ | |
| Vegetazione generica | Megascans + Houdini scatter | ○ | |
| NPC swimwear/nudità | Mutable (futuro) | ○ | |

---

## Workflow Blender → Houdini → UE5

```
Blender:  modellazione base, UV, shading, texturing manuale
          ↓ FBX export
Houdini:  scatter procedurale, simulazioni (muscoli, cloth, vfx)
          ↓ FBX / Alembic export
UE5:      import in Content/Environments/ep01-beach/
          setup materiali, lighting, NavMesh
```

**Export path UE5:** `Content/Environments/ep01-beach/[area]/`
**assetsxBlender/:** generato automaticamente da UE5 — non modificare

---

## Prossimo step

1. Definire concept completo 7-8 micro-aree (nomi, temi, micro-azioni NPC)
2. Creare livello base `ep01_beach` in UE5 con terrain placeholder
3. Prima area operativa: **pineta** — modellazione base Blender

---

## Legenda
```
✓  completato
⏳ in corso
○  da fare
```
