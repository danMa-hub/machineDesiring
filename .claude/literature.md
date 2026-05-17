# literature.md — Fonti
# Macchina del Desiderio

---

## Modello attrazione

| Fonte | Risultato usato |
|-------|-----------------|
| Conroy-Beam (2016-2022), N=14487, 45 paesi | Distanza euclidea pesata come modello universale di mate evaluation |
| Fisman et al. (2006) — speed dating | Floor soglia visiva 0.20 — previene gradient collapse |

---

## Pesi visual (gay-specifici)

| Fonte | Risultato usato |
|-------|-----------------|
| Swami & Tovée (2008) | Muscle 0.28 (↑ da 0.24), Slim 0.15 (↓ da 0.18) — campione gay UK |
| Levesque & Vichesky (2006) | Beauty 0.23 (↓ da 0.26) — valutazione estetica gay |
| Martins et al. (2008) | Hair peso 0.05, noise 40% (polarizzazione) |
| Bailey (1997) | Masculinity 0.15, noise 40% — assortativo forte |
| Antfolk (2017) | Age con youth bias — assortativo + aspirazionale |
| Valentova (2014) | Height 0.04 — aspirazionale, effetto debole |

---

## Pesi sexual (gay-specifici)

| Fonte | Risultato usato |
|-------|-----------------|
| Moskowitz et al. (2008) | BottomTop 0.40, complementarità, extremity modulazione ×0.60 |
| Moskowitz & Hart (2011) | PenisSize → BottomTop condizionato ×0.10 |
| Parsons et al. (2010) | KinkLevel 0.17, modulazione ×0.80; KinkTypes come flag multipli |
| GMFA (2017) | PenisSize 0.15, bottomness modulazione ×0.80 |
| Bailey (1997) | SubDom 0.28, complementare |

---

## Distribuzioni demografiche

| Fonte | Risultato usato |
|-------|-----------------|
| Doherty (2003) | Muscle picco età 35, piecewise decline |
| NHANES (1999-2004) | Slim calibrazione −0.15/anno |
| Schope (2005), Rhodes (2006) | Beauty age decay 0.20 (↑ da 0.08) |

---

## Preferenze e assortative mating

| Fonte | Risultato usato |
|-------|-----------------|
| Muscarella et al. (2002) | Hair r_assort 0.15 (↓ da 0.35) — preferenza capelli gay |
| Barthová et al. (2017) | Hair noise_sd 0.28 (↑ da 0.18) |
| Buston & Emlen (2003) | Overestimate mate value +0.23 bias — modello aspirazionale |
| Todd et al. (2007) | Aspirazione scende con fallimenti — decay aspirazione |
| Regan (2008) | 87% pre-relazionali desidera "meglio di sé" |

---

## Bias sistemici

| Fonte | Risultato usato |
|-------|-----------------|
| OkCupid (2009) | myEthBias — bias etnico nelle preferenze documentato |
| CDC (2022) | myPozBias — stigma sierologico nelle preferenze |

---

## Soglie adattive e sociometro

| Fonte | Risultato usato |
|-------|-----------------|
| Bale & Archer (2013) | Sociometro con ancora identitaria — Mode D MySelfEsteem |

---

## Note metodologiche

- **Campioni eterosessuali usati per math, non per pesi:** Li (2002), Conroy-Beam (2016-2022), Bruch & Newman (2018)
- **Fonti gay-specifiche obbligatorie per calibrazione pesi**
- Ogni parametro deve avere fonte nei commenti del codice Python
