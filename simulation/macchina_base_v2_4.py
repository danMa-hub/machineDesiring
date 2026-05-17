"""
================================================================
MACCHINA DEL DESIDERIO — v2.4
================================================================
MODIFICHE rispetto a v2.3:

  RENAME-2  ASSORT_COEFFS: r_assort→r_self, r_aspir→r_ideal
              in tuple e in generate_preferences()
  RENAME-3  Direzione attrazione: selfToOther_→myOut_,
              otherToSelf_→myIn_
  RENAME-4  Contatori attrazione: my_visAttractionOutCount→
              myOut_visAttractionCount, my_visAttractionInCount→
              myIn_visAttractionCount
  RENAME-5  Match cruise: my_visMatchCount→my_visMatchCount_cruise,
              my_fullMatchCount→my_visSexMatchCount_cruise
  RENAME-6  Match app: my_visMatchCount/my_fullMatchCount→
              my_matchCount_app
  RENAME-7  Combined app: myOut_combinedAttraction_app→
              myOut_attraction_app
  NEW-1     my_visAttractionBalance = myIn / myOut per NPC
              (>1 = desiderato più di quanto desidera)
  NEW-2     my_visToSexFrustrationRate cruise =
              (my_visMatchCount_cruise - my_visSexMatchCount_cruise)
              / my_visMatchCount_cruise
  REMOVE    generate_report() e dipendenza matplotlib eliminati.
              Output solo testo via _print_results().

PRECEDENTI (v2.3):
  FIX-1..7  vedi storico v2.3
  TODO-10..13  MateValue tripartito, Hair coefficienti,
               pesi gay-specifici, pesi sessuali modulati

ESECUZIONE:
  python macchina_base_v2_4.py
  python macchina_base_v2_4.py --n 500
  python macchina_base_v2_4.py --mode cruising
================================================================
"""

# ── Import ─────────────────────────────────────────────────────
import sys
import argparse
import numpy as np
from collections import defaultdict

# ================================================================
# COSTANTI GLOBALI
# ================================================================

RANDOM_SEED = 42
N_SUBJECTS  = 200

VISUAL_VARS = ['Muscle','Slim','Hair','Beauty','Masculinity','Height','Age']
SEXUAL_VARS = ['BottomTop','SubDom','KinkLevel','PenisSize']

# ── Pesi base dalla letteratura ────────────────────────────────
# Fonte: Li et al. 2002, conjoint analysis 2025, Conroy-Beam 2019
# Fonte: Bailey 1994, Siever 1994 — attrattività fisica domina (gay)
BASE_WEIGHTS_VISUAL = {
    # TODO-12: ricalibrazione su letteratura gay-specifica
    # Fonte: Swami & Tovée 2008 (muscle↑, slim↓ per gay men vs etero)
    # Fonte: Levesque & Vichesky 2006 (beauty meno dominante)
    # Fonte: Martins 2008 (hair salienza subculturale, masc meno centrale)
    'Beauty':      0.23,   # 0.26→0.23
    'Muscle':      0.28,   # 0.24→0.28
    'Slim':        0.15,   # 0.18→0.15
    'Masculinity': 0.15,   # 0.18→0.15
    'Age':         0.07,   # invariato
    'Height':      0.04,   # invariato
    'Hair':        0.05,   # 0.03→0.05 (Martins 2008; 0.08 segregava troppo)
}

# Fonte: Moskowitz 2008 (BT complementarità = necessità)
# Fonte: GMFA 2017, Gay Men's Health Project (PenisSize)
BASE_WEIGHTS_SEXUAL = {
    'BottomTop': 0.40,
    'SubDom':    0.28,
    'KinkLevel': 0.17,
    'PenisSize': 0.15,
}

# ── Pesi single-pass (modalità app: tutte le info visibili) ───
# Normalizzazione unica: pesi raw dalla letteratura, normalizzati a somma 1.
# Nessuno split artificiale vis/sex — il rapporto emerge dai dati.
# Pesi raw: stessa fonte di BASE_WEIGHTS_VISUAL e BASE_WEIGHTS_SEXUAL.
# Somma raw = 1.97 → BottomTop domina (0.40/1.97 = 0.203) perché
# in app tutti i profili sono visibili simultaneamente e BT è la
# variabile funzionalmente più vincolante (Moskowitz 2008).
# NOTA: nessun dato empirico quantificato per rapporto vis/sex su app gay —
#   il 62/38 precedente era una stima editoriale senza fonte diretta.
_RAW_SINGLE = {
    # Visual — stessa fonte BASE_WEIGHTS_VISUAL
    'Muscle':      0.28,
    'Beauty':      0.23,
    'Slim':        0.15,
    'Masculinity': 0.15,
    'Age':         0.07,
    'Hair':        0.05,
    'Height':      0.04,
    # Sexual — stessa fonte BASE_WEIGHTS_SEXUAL
    'BottomTop':   0.40,
    'SubDom':      0.28,
    'KinkLevel':   0.17,
    'PenisSize':   0.15,
}
_sw_tot = sum(_RAW_SINGLE.values())
BASE_WEIGHTS_SINGLE = {k: v / _sw_tot for k, v in _RAW_SINGLE.items()}
# Pesi normalizzati risultanti (somma = 1.00):
#   BottomTop 0.203  Muscle 0.142  SubDom 0.142  Beauty 0.117
#   KinkLevel 0.086  Slim 0.076    Masc 0.076     PenisSize 0.076
#   Age 0.036        Hair 0.025    Height 0.020

# Modalità di simulazione
MODE_CRUISING = 'cruising'   # two-step: visual gate → sexual
MODE_APP      = 'app'        # single-pass: tutte le variabili insieme

# ── DesiredTarget ──────────────────────────────────────────────
# Fonte: Luo 2017 (r_assort), Bruch & Newman 2018 (r_aspir)
CULTURAL_IDEAL = {
    'Muscle':      0.75,
    'Slim':        0.70,
    'Beauty':      0.80,
    'Masculinity': 0.75,
    'Height':      0.65,
    'Age':         0.122,   # (28-18)/82 — Silverthorne & Quinsey 2000
}

POP_MEAN = {
    'Muscle':      0.30,
    'Slim':        0.50,
    'Beauty':      0.45,
    'Hair':        0.46,
    'Masculinity': 0.58,
    'Height':      0.50,
    'Age':         0.249,   # (38.4-18)/82
}

ASSORT_COEFFS = {
    # var: (r_self, r_ideal, r_pop, noise_sd)
    # r_self  = ancoraggio al proprio tratto (Luo 2017)
    # r_ideal = pull verso ideale culturale (Bruch & Newman 2018)
    # r_pop   = pull verso media di popolazione
    'Muscle':      (0.25, 0.35, 0.40, 0.12),
    'Slim':        (0.25, 0.35, 0.40, 0.12),
    'Beauty':      (0.20, 0.40, 0.40, 0.10),
    'Masculinity': (0.45, 0.20, 0.35, 0.12),  # masc4masc (Bailey 1997)
    'Height':      (0.30, 0.20, 0.50, 0.10),
    # TODO-11: Hair — self-similarity debole (Muscarella 2002: r basso),
    # alta varianza individuale (Barthová 2017)
    'Hair':        (0.15, 0.00, 0.85, 0.28),   # r_self 0.35→0.15, noise 0.18→0.28
    'Age':         (0.70, 0.20, 0.10, 0.06),   # forte assortativo + youth bias
    'BottomTop':   (-0.90, 0.00, 0.10, 0.12),  # complementare (Moskowitz 2008)
    # TODO-1: Versatile penalizzato — valutare tolleranza allargata
    'SubDom':      (-0.85, 0.00, 0.15, 0.12),  # complementare
    'KinkLevel':   (0.75, 0.00, 0.25, 0.10),   # speculare
}

ETHNICITY_HAIR_MOD = {
    'White': +0.05, 'Latino': +0.05, 'Black': -0.05,
    'Asian': -0.20, 'MiddleEastern': +0.15, 'Mixed': 0.00,
}

# ================================================================
# TRIBE PROTOTYPES — v2.1
# ================================================================
# Pesi normalizzati a somma 1.0 per prototipo (leggibilità + coerenza)
# Fonte: Moskowitz 2013 (Bears), Wikipedia bear culture, Tran 2020
# Leather rimosso: non è body type, catturato da myKinkTypes
# Average: profilo non sufficientemente vicino a nessun prototipo

def _normalize_proto(proto_raw):
    tot = sum(w for _, w in proto_raw.values())
    return {var: (target, w / tot) for var, (target, w) in proto_raw.items()}

_TRIBE_PROTOTYPES_RAW = {
    # var: (target_value, weight_raw)
    # age target in anni — normalizzato internamente in classify_tribe
    'Bear':       {'sl':(0.35,3.5),'h':(0.72,3.5),'ma':(0.70,1.0),'age':(40,0.8)},
    'Cub':        {'sl':(0.30,3.0),'h':(0.68,3.0),'age':(24,2.5)},
    'Otter':      {'sl':(0.65,2.5),'h':(0.65,3.0),'age':(27,1.2),'m':(0.28,0.8)},
    'Wolf':       {'sl':(0.60,1.5),'h':(0.68,2.5),'m':(0.55,1.5),'ma':(0.75,2.5),'age':(38,0.8)},
    'Twink':      {'sl':(0.78,2.5),'h':(0.12,2.5),'m':(0.18,1.5),'age':(21,2.5),'ma':(0.38,1.0)},
    'Twunk':      {'sl':(0.72,2.0),'h':(0.18,1.5),'m':(0.55,2.5),'age':(23,2.0)},
    'Jock':       {'sl':(0.70,2.0),'m':(0.68,3.0),'h':(0.20,2.0),'age':(27,1.5),'ma':(0.65,1.0)},
    'Muscle':     {'sl':(0.75,2.5),'m':(0.82,3.5),'age':(30,0.8)},
    'MuscleBear': {'m':(0.68,2.5),'h':(0.65,2.5),'sl':(0.35,2.5)},
    'Chub':       {'sl':(0.12,4.5),'age':(40,0.8)},
    'Daddy':      {'age':(52,1.2),'ma':(0.73,3.0),'sl':(0.45,0.8)},
    'CleanCut':   {'po':(0.78,3.5),'h':(0.18,1.8),'sl':(0.60,1.0)},
    'Rugged':     {'po':(0.13,4.0),'h':(0.60,2.0)},
    'Femboy':     {'ma':(0.15,4.5),'h':(0.15,1.5)},
}
TRIBE_PROTOTYPES = {t: _normalize_proto(p) for t, p in _TRIBE_PROTOTYPES_RAW.items()}
TRIBE_MIN_SCORE  = 0.32   # sotto → Average

# ── myKinkTypes — flag multipli ──────────────────────────────────
# Fonte: Parsons et al. 2010 (N=1214 MSM): group sex 60.6%,
#   exhib/voy 39.8%, watersports 32.8%, bondage 29.8%,
#   fisting 20.9%, S/M 20.7%, breath play 8.1%
# Fonte: meta-analisi chemsex 238 studi, N=380.505 MSM → 22%
# Fonte: Dean 2009 "Unlimited Intimacy" — breeding subculture
# Struttura: (base_prob, kl_min_threshold, kl_weight, se_weight)
KINK_TYPES_DEF = {
    'GroupSex':     (0.40, 0.00, 0.20, 0.18),
    # Fonte: Parsons 2010: 39.8% MSM. Darkroom, cruising, sex party.
    # Distinto da MyBodyDisplay (profilo = quanto ti mostri fisicamente).
    # ExhibVoyeur = eroticizzazione dell'essere visto/guardare DURANTE il sesso.
    # Dipende da KL + SE (componente erotica), non da body display.
    'ExhibVoyeur':  (0.25, 0.00, 0.18, 0.22),
    'LightBDSM':    (0.15, 0.05, 0.32, 0.15),
    'Watersports':  (0.10, 0.08, 0.30, 0.22),
    'SM_Pain':      (0.02, 0.12, 0.28, 0.32),
    'Fisting':      (0.02, 0.12, 0.15, 0.40),
    'PupPlay':      (0.02, 0.18, 0.22, 0.12),
    'Chemsex':      (0.06, 0.00, 0.08, 0.18),
    'GearFetish':   (0.03, 0.12, 0.28, 0.12),
    'Roleplay':     (0.08, 0.00, 0.18, 0.10),
    'EdgePlay':     (0.00, 0.42, 0.12, 0.42),
    # Breeding: gestito separatamente in assign_kink_types()
    # perché condizionato a BottomTop (bottom riceve, top dà).
    # Non è nel dict standard — ha la sua logica.
}

# ================================================================
# UTILITY
# ================================================================
def clamp(v, lo=0.0, hi=1.0):
    return max(lo, min(hi, float(v)))

def gauss(m, s, lo=0.0, hi=1.0):
    return clamp(np.random.normal(m, s), lo, hi)

def bimodal(m1, s1, w1, m2, s2, w2):
    return clamp(
        np.random.normal(m1, s1) if np.random.random() < w1
        else np.random.normal(m2, s2)
    )

def exp_inverse(m, s):
    return clamp(np.random.exponential(m) + np.random.normal(0, s * 0.3))

def weighted_random(d):
    k = list(d.keys())
    w = np.array(list(d.values()), dtype=float)
    w /= w.sum()
    return np.random.choice(k, p=w)

# ================================================================
# TRIBE CLASSIFICATION — prototype scoring (v2.1)
# ================================================================
def classify_tribe(p):
    """
    Distanza euclidea pesata tra il profilo e ogni prototipo tribe.
    Pesi già normalizzati → score = exp(-Σ w_i*(val_i - target_i)²)
    Fonte: Moskowitz 2013, Grindr tribes 2013, Tran 2020
    """
    an  = p['myAgeNorm']
    vals = {
        'sl': p['MySlim'],
        'h':  p['MyHair'],
        'm':  p['MyMuscle'],
        'ma': p['MyMasculinity'],
        'po': p['MyPolish'],
        'age': an,   # già normalizzato 0-1
    }
    scores = {}
    for tribe, proto in TRIBE_PROTOTYPES.items():
        wss = sum(
            w * (vals[v] - ((t - 18.0) / 82.0 if v == 'age' else t)) ** 2
            for v, (t, w) in proto.items()
        )
        scores[tribe] = np.exp(-wss)
    best = max(scores, key=scores.get)
    return best if scores[best] >= TRIBE_MIN_SCORE else 'Average'


# ================================================================
# KINKTYPES ASSIGNMENT — flag multipli (v2.1)
# ================================================================
def assign_kink_types(p):
    """
    Lista di flag kink attivi per il personaggio.
    Condizionata a KinkLevel e SexExtremity.
    Ogni flag è indipendente — co-occorrenza documentata in letteratura.
    Fonte: Parsons 2010 (N=1214 MSM), chemsex meta-analisi 2024,
           Dean 2009 (breeding subculture)
    """
    kl = p['MyKinkLevel']
    se = p['MySexExtremity']
    # Floor: solo i "vanilla totale" (KL<0.03, ~18%) sono forzati
    if kl < 0.03:
        return ['Vanilla']

    # Flag standard dal dict
    active = []
    for name, (base, kl_min, kw, sw) in KINK_TYPES_DEF.items():
        if kl < kl_min:
            continue
        prob = clamp(base + kw * kl + sw * se)
        # ExhibVoyeur: dipende solo da KL/SE (componente erotica),
        # non da MyBodyDisplay (body display ≠ esibizionismo erotico)
        if name == 'ExhibVoyeur':
            prob = clamp(prob + se * 0.10)  # boost extra da SexExtremity
        if np.random.random() < prob:
            active.append(name)

    # Breeding — condizionato a BottomTop e SafetyPractice
    # Fonte: Dean 2009 "Unlimited Intimacy": breeding = eroticizzazione
    #   dell'eiaculazione interna, specifica cultura gay barebacke.
    # Più probabile per bottom (ricevono) e per chi fa bareback.
    # Stimato ~15-20% MSM sessualmente attivi non-vanilla.
    bt = p['MyBottomTop']
    # Bottom puro (bt~0.10) → bt_boost ~+0.18; Top puro → ~+0.08; Vers → ~0
    bt_boost = 0.08 + (1.0 - bt) * 0.12   # bottom: 0.20, top: 0.08, vers: 0.14
    bareback_boost = 0.10 if 'Bareback' in p.get('MySafetyPractice', '') else 0.0
    breeding_prob = clamp(0.02 + kl * 0.15 + se * 0.12 + bt_boost + bareback_boost)
    if np.random.random() < breeding_prob:
        active.append('Breeding')

    return active if active else ['Vanilla']


# ================================================================
# PROFILE GENERATION — v2.1
# ================================================================
def generate_profile(pid):
    p = {'id': pid}

    # ── Passata 1 — indipendenti ──────────────────────────────
    p['MyAge']     = clamp(np.random.normal(38.0, 15.7), 18, 100)
    p['MyHeight']  = np.random.normal(177.0, 7.0)
    p['MyEthnicity'] = weighted_random({
        'White': 0.58, 'Latino': 0.16, 'Black': 0.12,
        'Asian': 0.08, 'MiddleEastern': 0.02, 'Mixed': 0.04,
    })
    _hair_base    = np.random.normal(0.46, 0.18)
    _beauty_base  = np.random.normal(0.45, 0.15)
    _bodymod_base = np.random.exponential(0.18) + np.random.normal(0, 0.06)
    _exhib_base   = np.random.exponential(0.22) + np.random.normal(0, 0.054)

    p['MyPolish']    = gauss(0.44, 0.18)
    p['MyQueerness'] = bimodal(0.20, 0.10, 0.55, 0.72, 0.12, 0.45)
    p['MyKinkLevel'] = clamp(exp_inverse(0.18, 0.20))
    p['MyOpenClosed'] = gauss(0.55, 0.22)
    p['MyRelDepth']   = gauss(0.50, 0.22)

    # TODO-5: MyPenisSize → Passata 1 (indipendente)
    # Fonte: Veale et al. 2015: media eretto 13.12cm SD 1.66cm
    # Dimensione è biologica, non causata dal ruolo
    p['MyPenisSize'] = clamp(np.random.normal(0.50, 0.15))

    # ── Passata 2 — condizionate ──────────────────────────────
    an = (p['MyAge'] - 18.0) / 82.0
    hn = clamp((p['MyHeight'] - 160.0) / 35.0)
    p['myAgeNorm']    = an
    p['myHeightNorm'] = hn

    p['MyHair']    = clamp(_hair_base + an * 0.10
                           + ETHNICITY_HAIR_MOD.get(p['MyEthnicity'], 0))
    p['MyBodyMod'] = clamp(_bodymod_base - max(0, (an - 0.30)) * 0.12
                           + p['MyQueerness'] * 0.10)

    # TODO-5: BottomTop condizionato anche a PenisSize×0.10
    # Fonte: Moskowitz & Hart 2011 — top riportano peni più grandi
    # Causalità: pene→ruolo, non il contrario
    roll = np.random.random()
    bt = (gauss(0.10, 0.08) if roll < 0.28
          else (gauss(0.50, 0.10) if roll < 0.78 else gauss(0.90, 0.08)))
    bt += (an - 0.33) * 0.10 + (hn - 0.50) * 0.10
    bt += (p['MyPenisSize'] - 0.50) * 0.10   # TODO-5
    p['MyBottomTop'] = clamp(bt)

    # TODO-7: MyMuscle — picco a 35 anni, calo piecewise
    # Fonte: Doherty 2003: 3-8%/decade dopo 30, accelera dopo 60
    # Fonte: sarcopenia review: -30% massa muscolare da 20 a 80 anni
    muscle_base = np.random.normal(0.30, 0.15)
    if an < 0.21:       # < 35 anni: leggero bonus giovinezza
        age_penalty = -(0.21 - an) * 0.08
    elif an < 0.51:     # 35-60: calo moderato ~4.5%/decade
        age_penalty = (an - 0.21) * 0.15
    else:               # > 60: calo accelerato ~8%/decade
        age_penalty = (0.51 - 0.21) * 0.15 + (an - 0.51) * 0.28
    p['MyMuscle'] = clamp(muscle_base - age_penalty)

    # TODO-7: MySlim — calo calibrato NHANES, bonus muscle nonlineare
    # Fonte: NHANES 1999-2004: +8pp grasso 16-79 anni negli uomini
    # Fonte: r(ASMI, %fat) = −0.761 (correlazione nonlineare sopra soglia)
    slim_base = np.random.normal(0.48, 0.22)
    muscle_slim_bonus = max(0.0, p['MyMuscle'] - 0.30) * 0.35
    slim = slim_base - an * 0.15 + muscle_slim_bonus
    p['MySlim'] = clamp(slim)

    # FIX-5 v2.3: beauty decay 0.08→0.20
    # Prima: 70yr perdeva 0.05 punti (0.3 SD — invisibile)
    # Ora: 70yr perde ~0.13 punti (~0.9 SD — ageismo strutturale visibile)
    # Coerente con letteratura: Schope 2005, Rhodes 2006 (forte age effect)
    p['MyBeauty']      = clamp(_beauty_base - an * 0.20)
    p['MyMasculinity'] = clamp(np.random.normal(0.62, 0.18)
                               + p['MyHair'] * 0.15
                               - p['MyQueerness'] * 0.20
                               + (hn - 0.50) * 0.08)
    # MyBodyDisplay — quanto il personaggio mostra il corpo
    # Rinominato da MyExhibitionism (v2.3): non è esibizionismo erotico,
    # è body display (torso nudo, poca stoffa). Per LOD vestiti UE5.
    # Dipende da Muscle: chi ha un bel corpo lo mostra.
    p['MyBodyDisplay'] = clamp(_exhib_base + p['MyMuscle'] * 0.10)

    p['MySexExtremity'] = clamp(np.random.normal(0.32, 0.18)
                                + p['MyKinkLevel'] * 0.25)

    # TODO-8: MySexActDepth — base corretta su Richters 2008
    # Solo 37.2% MSM ha sesso anale nell'incontro più recente
    p['MySexActDepth'] = clamp(np.random.normal(0.38, 0.20)
                               + p['MySexExtremity'] * 0.15)

    # TODO-8: MySubDom — media 0.44, correlazione BT ridotta a ×0.10
    # Fonte: Holvoet 2017 (N=256 BDSM): 50% Sub, 27% Dom, 23% Switch
    # Top/Bottom e Dom/Sub sono dimensioni indipendenti
    p['MySubDom'] = clamp(np.random.normal(0.44, 0.20)
                          + (p['MyBottomTop'] - 0.50) * 0.10)

    # PozStatus — condizionato all'età
    # Fonte: CDC 2022 ~16-17% MSM HIV+; ~85-90% in trattamento soppressivo
    # BUG FIX v2.3: af moltiplicava uniformemente tutti e 3 gli elementi,
    #   la normalizzazione annullava l'effetto → tasso HIV identico a tutte le età.
    # Fix: af modula solo il tasso HIV+, None è il complemento
    age = p['MyAge']
    af = (0.40 if age < 25
          else (1.00 if age < 35 else (1.30 if age < 50 else 0.70)))
    base_hiv = 0.18   # tasso base HIV+ MSM urbani occidentali
    hiv_rate = min(0.40, base_hiv * af)
    undet_frac = 0.67  # ~2/3 HIV+ sono undetectable (CDC 2022)
    pw = np.array([1.0 - hiv_rate,
                   hiv_rate * (1 - undet_frac),   # Poz
                   hiv_rate * undet_frac])          # PozUndet
    pw /= pw.sum()
    p['MyPozStatus'] = np.random.choice(['None', 'Poz', 'PozUndet'], p=pw)

    # FIX-6 v2.3: SafetyPractice ristruturata
    # PrEP è profilassi per HIV-negativi. HIV+ prendono ART, non PrEP.
    # Per None: CondomOnly/PrEP/Bareback (come prima)
    # Per Poz/PozUndet: CondomOnly/ART/Bareback (ART non è una scelta
    #   ma quasi tutti lo prendono; Bareback = chi non si protegge ulteriormente)
    if p['MyPozStatus'] == 'None':
        sp = {'CondomOnly': 0.40, 'PrEP': 0.30, 'Bareback': 0.30}
    elif p['MyPozStatus'] == 'PozUndet':
        # Undetectable = in ART per definizione; scelta residua su barriera
        sp = {'ART_CondomOnly': 0.35, 'ART_Bareback': 0.65}
    else:  # Poz (non soppresso)
        sp = {'CondomOnly': 0.45, 'Bareback': 0.40, 'ART_NonAdherent': 0.15}
    p['MySafetyPractice'] = weighted_random(sp)

    # ── Passata 3 — derivate ──────────────────────────────────

    # TODO-8: MySelfEsteem — aggiunto PenisSize×0.12
    # Fonte: Grov et al. 2010 (MSM): benessere psicosociale + dimensione
    # Fonte: Lever et al. 2006 (N=52.031): uomini con pene > media
    #   valutano il proprio aspetto più favorevolmente
    # Fonte: Morrison et al. 2004, Peplau 2009: gay men media ~0.48
    _se_obj = (p['MyBeauty']      * 0.20
             + p['MyMuscle']      * 0.15
             + p['MySlim']        * 0.10
             + p['MyMasculinity'] * 0.08
             + p['MyPenisSize']   * 0.12   # TODO-8
             - an                 * 0.10)
    # Base 0.21: calibrato su target media gay ~0.48 (Morrison 2004, Peplau 2009)
    # dopo aggiunta PenisSize×0.12 in TODO-8 (era 0.27 senza PenisSize)
    p['MySelfEsteem'] = clamp(0.21 + _se_obj + np.random.normal(0, 0.15))

    # TODO-9: classify_tribe con prototype scoring
    p['myTribeTag']   = classify_tribe(p)

    # TODO-9: myKinkTypes come lista di flag multipli
    p['myKinkTypes']  = assign_kink_types(p)

    return p


# ================================================================
# MATING PREFERENCES — invariata da v2.0
# ================================================================
def generate_preferences(p):
    pref = {}

    profile_map = {
        'Muscle': 'MyMuscle', 'Slim': 'MySlim', 'Beauty': 'MyBeauty',
        'Hair': 'MyHair', 'Masculinity': 'MyMasculinity',
        'Height': 'myHeightNorm', 'Age': 'myAgeNorm',
        'BottomTop': 'MyBottomTop', 'SubDom': 'MySubDom',
        'KinkLevel': 'MyKinkLevel',
    }

    for var, (r_self, r_ideal, r_pop, noise_sd) in ASSORT_COEFFS.items():
        my_val = p[profile_map[var]]
        ideal  = CULTURAL_IDEAL.get(var, POP_MEAN.get(var, 0.50))
        pop_m  = POP_MEAN.get(var, 0.50)
        if r_self < 0:
            # Complementare: desidero l'opposto di me
            desired = abs(r_self) * (1.0 - my_val) + (1 - abs(r_self)) * pop_m
        elif r_ideal > 0:
            # Aspirazionale: ancoraggio al sé + pull verso ideale + pull verso media
            desired = r_self * my_val + r_ideal * ideal + r_pop * pop_m
        else:
            # Assortativo puro: solo sé + media
            desired = r_self * my_val + (1 - r_self) * pop_m
        desired += np.random.normal(0, noise_sd)
        pref[f'myDesired_{var}'] = clamp(desired)

    # myDesired_PenisSize — condizionale al ruolo sessuale
    # Fonte: GMFA 2017: bottom vuole top dotato
    bt = p['MyBottomTop']
    pref['myDesired_PenisSize'] = clamp(
        0.50 + (1.0 - bt) * 0.25 + np.random.normal(0, 0.12))

    # Pesi individuali con noise variabile
    # FIX-7 v2.3: Masculinity e Hair hanno noise 40% (non 30%)
    # per catturare la polarizzazione (masc4masc vs femmephilia; Bear vs glabro)
    _WEIGHT_NOISE_SD = {
        'Beauty': 0.30, 'Muscle': 0.30, 'Slim': 0.30,
        'Masculinity': 0.40, 'Age': 0.30, 'Height': 0.30,
        'Hair': 0.40,
    }
    pref['myWeightsVisual'] = {}
    for v, w in BASE_WEIGHTS_VISUAL.items():
        noise_frac = _WEIGHT_NOISE_SD.get(v, 0.30)
        pref['myWeightsVisual'][v] = max(0.01, w + np.random.normal(0, w * noise_frac))
    tw = sum(pref['myWeightsVisual'].values())
    pref['myWeightsVisual'] = {v: w / tw for v, w in pref['myWeightsVisual'].items()}

    pref['myWeightsSexual'] = {}
    for v, w in BASE_WEIGHTS_SEXUAL.items():
        pref['myWeightsSexual'][v] = max(0.01, w + np.random.normal(0, w * 0.30))

    # TODO-13: modulazione pesi sessuali in base al profilo
    # Chi è un estremo BT (puro top o puro bottom) dà più peso al BT
    # Fonte: Li 2002 — BT complementarità come "necessità" più forte agli estremi
    bt_extremity = abs(p['MyBottomTop'] - 0.5) * 2   # 0=versatile, 1=puro estremo
    pref['myWeightsSexual']['BottomTop'] *= (1 + bt_extremity * 0.60)
    # Chi è molto kinky dà più peso al KinkLevel del partner
    # Fonte: Parsons 2010 — high-kink MSM cercano specificatamente partner compatibili
    pref['myWeightsSexual']['KinkLevel'] *= (1 + p['MyKinkLevel'] * 0.80)
    # Bottom dà più peso a PenisSize del partner
    # Fonte: GMFA 2017 — 22% ha rifiutato partner per dimensione (dominato da bottom)
    # Per un bottom, la dimensione del partner determina direttamente l'esperienza
    # fisica della penetrazione. Per un top, molto meno rilevante.
    # bottomness: 0.90 per bottom puro, 0.50 per vers, 0.10 per top puro
    bottomness = 1.0 - p['MyBottomTop']
    pref['myWeightsSexual']['PenisSize'] *= (1 + bottomness * 0.80)

    tw = sum(pref['myWeightsSexual'].values())
    pref['myWeightsSexual'] = {v: w / tw for v, w in pref['myWeightsSexual'].items()}

    # Bias osservatore — fisso per persona, non per coppia
    # Fonte: Callander 2015 (etnia), Smit 2012 (PozStatus)
    pref['myEthBias'] = max(0.0, np.random.normal(1.0, 0.40))
    pref['myPozBias'] = max(0.0, np.random.normal(1.0, 0.50))

    # ── Pesi single-pass (modalità app) ───────────────────────
    # Noise individuale + modulazione TODO-13 (BT extremity, KinkLevel)
    # applicata anche qui per coerenza
    _WEIGHT_NOISE_SD_SINGLE = {
        'Beauty': 0.30, 'Muscle': 0.30, 'Slim': 0.30,
        'Masculinity': 0.40, 'Age': 0.30, 'Height': 0.30,
        'Hair': 0.40,
        'BottomTop': 0.30, 'SubDom': 0.30, 'KinkLevel': 0.30,
        'PenisSize': 0.30,
    }
    pref['myWeightsSingle'] = {}
    for v, w in BASE_WEIGHTS_SINGLE.items():
        nf = _WEIGHT_NOISE_SD_SINGLE.get(v, 0.30)
        pref['myWeightsSingle'][v] = max(0.01, w + np.random.normal(0, w * nf))
    # TODO-13 modulazione anche nel single-pass
    pref['myWeightsSingle']['BottomTop'] *= (1 + bt_extremity * 0.60)
    pref['myWeightsSingle']['KinkLevel'] *= (1 + p['MyKinkLevel'] * 0.80)
    pref['myWeightsSingle']['PenisSize'] *= (1 + bottomness * 0.80)
    tw = sum(pref['myWeightsSingle'].values())
    pref['myWeightsSingle'] = {v: w / tw for v, w in pref['myWeightsSingle'].items()}

    return pref


# ================================================================
# ATTRAZIONE — Distanza Euclidea pesata (Conroy-Beam 2016-2022)
# ================================================================

ETH_DESIRABILITY = {
    'MiddleEastern': 1.07,
    'White':         1.00,
    'Latino':        0.95,
    'Mixed':         0.95,
    'Asian':         0.85,
    'Black':         0.80,
}

POZ_DESIRABILITY = {
    'None':     1.00,
    'PozUndet': 0.70,
    'Poz':      0.36,
}


def calc_visual_attraction(A, Ap, B):
    """
    Fase 1 — primo contatto visivo.
    Distanza Euclidea pesata (Conroy-Beam 2016-2022, N=14487, 45 paesi).
    """
    W = Ap['myWeightsVisual']
    profile_map = {
        'Muscle': 'MyMuscle', 'Slim': 'MySlim', 'Beauty': 'MyBeauty',
        'Hair': 'MyHair', 'Masculinity': 'MyMasculinity',
        'Height': 'myHeightNorm', 'Age': 'myAgeNorm',
    }
    sum_wd2 = 0.0
    for v in VISUAL_VARS:
        if v not in W or v not in profile_map:
            continue
        desired = Ap.get(f'myDesired_{v}')
        if desired is None:
            continue
        sum_wd2 += W[v] * (desired - B[profile_map[v]]) ** 2

    d = np.sqrt(sum_wd2)
    sigma = 0.13   # parametro artistico — produce 14% no-match visivo
    attraction = np.exp(-(d ** 2) / (2 * sigma ** 2))

    # Moltiplicatori post-distanza: Etnia (OkCupid 2009 gay-specific)
    eth_dev = ETH_DESIRABILITY.get(B['MyEthnicity'], 1.0) - 1.0
    attraction *= max(0.50, 1.0 + eth_dev * Ap['myEthBias'])

    # Moltiplicatori post-distanza: PozStatus (Survey 2019, GLAAD 2022)
    poz_dev = POZ_DESIRABILITY.get(B['MyPozStatus'], 1.0) - 1.0
    attraction *= max(0.20, 1.0 + poz_dev * Ap['myPozBias'])

    # Varianza relazionale — Eastwick et al. 2024 (chimica imprevedibile)
    attraction += np.random.normal(0, 0.03)
    return clamp(attraction)


def calc_sexual_attraction(A, Ap, B):
    """
    Fase 2 — durante l'interazione.
    Distanza Euclidea pesata su variabili sessuali.
    """
    W = Ap['myWeightsSexual']
    profile_map = {
        'BottomTop': 'MyBottomTop', 'SubDom': 'MySubDom',
        'KinkLevel': 'MyKinkLevel', 'PenisSize': 'MyPenisSize',
    }
    sum_wd2 = 0.0
    for v in SEXUAL_VARS:
        if v not in W or v not in profile_map:
            continue
        desired = Ap.get(f'myDesired_{v}')
        if desired is None:
            continue
        sum_wd2 += W[v] * (desired - B[profile_map[v]]) ** 2

    sigma = 0.13
    attraction = np.exp(-(np.sqrt(sum_wd2) ** 2) / (2 * sigma ** 2))

    # Dealbreaker BottomTop (Li et al. 2002: necessità)
    bt_distance = abs(Ap['myDesired_BottomTop'] - B['MyBottomTop'])
    if bt_distance > 0.60:
        attraction *= 0.10

    attraction += np.random.normal(0, 0.03)
    return clamp(attraction)


def calc_combined_attraction(A, Ap, B):
    """
    Modalità APP — single-pass.
    Tutte le 11 variabili in un'unica distanza Euclidea pesata.
    Pesi: raw dalla letteratura, normalizzati a somma 1 senza split vis/sex.
    BottomTop domina (0.203) perché funzionalmente vincolante (Moskowitz 2008).
    Dealbreaker BT: bt_distance > 0.60 → ×0.15.
      Bottom (desired≈0.82) vs Bottom (actual=0.10): distanza 0.72 → penalizzato
      Versatile (desired≈0.46): distanza max 0.44 → mai dealbreaker
    Fonte modello: Conroy-Beam 2016 (distanza Euclidea).
    Fonte pesi: Swami & Tovée 2008, Levesque 2006, Moskowitz 2008, GMFA 2017.
    """
    W = Ap['myWeightsSingle']
    profile_map = {
        'Muscle': 'MyMuscle', 'Slim': 'MySlim', 'Beauty': 'MyBeauty',
        'Hair': 'MyHair', 'Masculinity': 'MyMasculinity',
        'Height': 'myHeightNorm', 'Age': 'myAgeNorm',
        'BottomTop': 'MyBottomTop', 'SubDom': 'MySubDom',
        'KinkLevel': 'MyKinkLevel', 'PenisSize': 'MyPenisSize',
    }
    ALL_VARS = VISUAL_VARS + SEXUAL_VARS

    sum_wd2 = 0.0
    for v in ALL_VARS:
        if v not in W or v not in profile_map:
            continue
        desired = Ap.get(f'myDesired_{v}')
        if desired is None:
            continue
        sum_wd2 += W[v] * (desired - B[profile_map[v]]) ** 2

    d = np.sqrt(sum_wd2)
    # σ più alto per 11 variabili (range distanza più ampio vs 7 o 4)
    # σ=0.15 calibrato per produrre ~5-8% mutual pairs (coerente con app match rates)
    sigma = 0.15
    attraction = np.exp(-(d ** 2) / (2 * sigma ** 2))

    # Moltiplicatori post-distanza (identici al visivo)
    eth_dev = ETH_DESIRABILITY.get(B['MyEthnicity'], 1.0) - 1.0
    attraction *= max(0.50, 1.0 + eth_dev * Ap['myEthBias'])

    poz_dev = POZ_DESIRABILITY.get(B['MyPozStatus'], 1.0) - 1.0
    attraction *= max(0.20, 1.0 + poz_dev * Ap['myPozBias'])

    # Dealbreaker BT (anche nel single-pass: incompatibilità funzionale)
    bt_distance = abs(Ap['myDesired_BottomTop'] - B['MyBottomTop'])
    if bt_distance > 0.60:
        attraction *= 0.15  # meno severo del two-step (0.10) perché
                            # nel contesto app l'info è preventiva, non una sorpresa

    attraction += np.random.normal(0, 0.03)
    return clamp(attraction)


# ================================================================
# SIMULAZIONE
# ================================================================

def run_simulation(n=N_SUBJECTS, seed=RANDOM_SEED, threshold=0.35,
                   mode=MODE_CRUISING, verbose=True):
    """
    mode=MODE_CRUISING: two-step (visual gate → sexual). Default.
    mode=MODE_APP: single-pass (tutte le 11 variabili insieme).
    """
    np.random.seed(seed)
    if verbose:
        print(f"Generazione {n} profili...")
    profiles = [generate_profile(i) for i in range(n)]

    if verbose:
        print("Generazione preferenze...")
    prefs = [generate_preferences(p) for p in profiles]

    if verbose:
        print(f"Calcolo match ({mode})...")

    if mode == MODE_CRUISING:
        # ── TWO-STEP: visual gate → sexual ────────────────────
        vis = np.zeros((n, n))
        sex = np.zeros((n, n))
        for i in range(n):
            for j in range(n):
                if i == j:
                    continue
                vis[i][j] = calc_visual_attraction(profiles[i], prefs[i], profiles[j])
                sex[i][j] = calc_sexual_attraction(profiles[i], prefs[i], profiles[j])

        vis_pass = vis >= threshold
        # myOut: attrazioni uscenti (quanti j NPC_i ha trovato attraenti)
        myOut_vis = vis_pass.sum(axis=1)
        # myIn: attrazioni ricevute (quanti i hanno trovato NPC_j attraente)
        myIn_vis  = vis_pass.sum(axis=0)

        my_visMatch  = np.zeros(n, int)   # mutual visual
        my_sexMatch  = np.zeros(n, int)   # mutual visual + sexual
        for i in range(n):
            for j in range(i + 1, n):
                if vis_pass[i][j] and vis_pass[j][i]:
                    my_visMatch[i] += 1
                    my_visMatch[j] += 1
                if (vis_pass[i][j] and vis_pass[j][i]
                        and sex[i][j] >= threshold and sex[j][i] >= threshold):
                    my_sexMatch[i] += 1
                    my_sexMatch[j] += 1

        # my_visAttractionBalance: myIn / myOut — >1 desiderato più di quanto desidera
        # Fonte: asimmetria di mercato per-NPC (Bruch & Newman 2018)
        with np.errstate(divide='ignore', invalid='ignore'):
            my_visAttractionBalance = np.where(
                myOut_vis > 0, myIn_vis / myOut_vis.astype(float), 0.0)

        # my_visToSexFrustrationRate: quota di vis match che non converte a sex match
        with np.errstate(divide='ignore', invalid='ignore'):
            my_visToSexFrustrationRate = np.where(
                my_visMatch > 0,
                (my_visMatch - my_sexMatch) / my_visMatch.astype(float),
                0.0)

        # myPop_visAttraction: appeal visivo medio — media attrazioni ricevute
        myPop_vis = vis.mean(axis=0)
        # myPop_sexAttraction: appeal sessuale gated su mutual visual
        myPop_sex = np.zeros(n)
        for j in range(n):
            mutual_vis_j = np.array([
                i for i in range(n)
                if i != j and vis_pass[i][j] and vis_pass[j][i]
            ])
            if len(mutual_vis_j) > 0:
                myPop_sex[j] = sex[mutual_vis_j, j].mean()
        _alpha = 0.60
        myPop_glob = np.where(myPop_sex > 0,
                              _alpha * myPop_vis + (1 - _alpha) * myPop_sex,
                              myPop_vis)

        results = {
            'profiles': profiles, 'prefs': prefs, 'mode': mode,
            'myOut_visAttraction_cruise':        vis,
            'myOut_sexAttraction_cruise':        sex,
            'myOut_visAttractionCount':          myOut_vis,
            'myIn_visAttractionCount':           myIn_vis,
            'my_visMatchCount_cruise':           my_visMatch,
            'my_visSexMatchCount_cruise':        my_sexMatch,
            'my_visAttractionBalance':           my_visAttractionBalance,
            'my_visToSexFrustrationRate':        my_visToSexFrustrationRate,
            'myPop_visAttraction':               myPop_vis,
            'myPop_sexAttraction':               myPop_sex,
            'myPop_globalAttraction':            myPop_glob,
            'threshold': threshold,
        }

    elif mode == MODE_APP:
        # ── SINGLE-PASS: tutte le 11 variabili insieme ───────
        comb = np.zeros((n, n))
        for i in range(n):
            for j in range(n):
                if i == j:
                    continue
                comb[i][j] = calc_combined_attraction(
                    profiles[i], prefs[i], profiles[j])

        comb_pass = comb >= threshold
        myOut_comb = comb_pass.sum(axis=1)
        myIn_comb  = comb_pass.sum(axis=0)

        my_match = np.zeros(n, int)
        for i in range(n):
            for j in range(i + 1, n):
                if comb_pass[i][j] and comb_pass[j][i]:
                    my_match[i] += 1
                    my_match[j] += 1

        # my_visAttractionBalance: myIn / myOut
        with np.errstate(divide='ignore', invalid='ignore'):
            my_visAttractionBalance = np.where(
                myOut_comb > 0, myIn_comb / myOut_comb.astype(float), 0.0)

        myPop_comb = comb.mean(axis=0)

        results = {
            'profiles': profiles, 'prefs': prefs, 'mode': mode,
            'myOut_attraction_app':         comb,
            'myOut_visAttractionCount':     myOut_comb,
            'myIn_visAttractionCount':      myIn_comb,
            'my_matchCount_app':            my_match,
            'my_visAttractionBalance':      my_visAttractionBalance,
            'myPop_visAttraction':          myPop_comb,
            'myPop_sexAttraction':          myPop_comb,
            'myPop_globalAttraction':       myPop_comb,
            'threshold': threshold,
        }
    else:
        raise ValueError(f"Mode sconosciuto: {mode}")

    if verbose:
        _print_results(results, n, threshold)
    return results


def _gini(arr):
    arr = np.sort(arr)
    n = len(arr)
    cumul = np.cumsum(arr)
    return 1 - 2 * cumul.sum() / (n * cumul[-1]) if cumul[-1] > 0 else 0


def _print_results(res, n, thr):
    myOut_visCount  = res['myOut_visAttractionCount']
    myIn_visCount   = res['myIn_visAttractionCount']
    myPop_vis       = res['myPop_visAttraction']
    myPop_sex       = res['myPop_sexAttraction']
    myPop_glob      = res.get('myPop_globalAttraction', myPop_vis)
    my_balance      = res['my_visAttractionBalance']
    mode            = res.get('mode', MODE_CRUISING)

    if mode == MODE_CRUISING:
        attr_matrix  = res['myOut_visAttraction_cruise']
        sex_matrix   = res['myOut_sexAttraction_cruise']
        visMatchCount = res['my_visMatchCount_cruise']
        sexMatchCount = res['my_visSexMatchCount_cruise']
        frustRate     = res['my_visToSexFrustrationRate']
    else:
        attr_matrix  = res['myOut_attraction_app']
        sex_matrix   = res['myOut_attraction_app']
        visMatchCount = res['my_matchCount_app']
        sexMatchCount = visMatchCount
        frustRate     = np.zeros(n)

    vis_pass    = attr_matrix >= thr
    P           = res['profiles']
    PR          = res['prefs']
    n_visPairs  = visMatchCount.sum() // 2
    n_sexPairs  = sexMatchCount.sum() // 2
    tp          = n * (n - 1) // 2

    mode_label = 'CRUISING (two-step)' if mode == MODE_CRUISING else 'APP (single-pass)'
    print(f"\n{'='*60}")
    print(f"  v2.4 — {mode_label} — {n} soggetti — soglia {thr}")
    print(f"{'='*60}")

    # ── Attrazioni ──────────────────────────────────────────────
    print(f"  myOut_visAttraction mean         : {attr_matrix[attr_matrix > 0].mean():.3f}")
    print(f"  myOut_visAttractionCount mean/med/max: {myOut_visCount.mean():.1f} / {np.median(myOut_visCount):.0f} / {myOut_visCount.max()}")
    print(f"  myIn_visAttractionCount  mean/med/max: {myIn_visCount.mean():.1f} / {np.median(myIn_visCount):.0f} / {myIn_visCount.max()}")
    print(f"  my_visAttractionBalance  mean/med    : {my_balance.mean():.3f} / {np.median(my_balance):.3f}")

    # ── Match ────────────────────────────────────────────────────
    if mode == MODE_CRUISING:
        print(f"  pop_neverVisDesired_cruise       : {(myIn_visCount==0).sum()} ({(myIn_visCount==0).mean()*100:.0f}%)")
        print(f"  pop_neverVisMatched_cruise       : {(visMatchCount==0).sum()} ({(visMatchCount==0).mean()*100:.0f}%)")
        print(f"  pop_neverSexMatched_cruise       : {(sexMatchCount==0).sum()} ({(sexMatchCount==0).mean()*100:.0f}%)")
        print(f"  pairs_visCompatibilityRate       : {n_visPairs} ({n_visPairs/tp*100:.2f}%)")
        print(f"  pairs_fullCompatibilityRate      : {n_sexPairs} ({n_sexPairs/tp*100:.2f}%)")
        if n_visPairs > 0:
            print(f"  pairs_vis→sex conversion         : {n_sexPairs/n_visPairs*100:.1f}%")
        print(f"  my_visToSexFrustrationRate mean  : {frustRate[visMatchCount>0].mean():.3f}"
              f"  (su {(visMatchCount>0).sum()} NPC con ≥1 visMatch)")
    else:
        print(f"  pop_neverMatched_app             : {(visMatchCount==0).sum()} ({(visMatchCount==0).mean()*100:.0f}%)")
        print(f"  pairs_fullCompatibilityRate_app  : {n_visPairs} ({n_visPairs/tp*100:.2f}%)")

    # ── Disuguaglianza ───────────────────────────────────────────
    gini        = _gini(myPop_vis)
    top20_thr   = np.sort(myPop_vis)[::-1][int(n * 0.20)]
    top20_share = myPop_vis[myPop_vis >= top20_thr].sum() / myPop_vis.sum() * 100
    bot30_thr   = np.sort(myPop_vis)[::-1][int(n * 0.70)]
    bot30_share = myPop_vis[myPop_vis <= bot30_thr].sum() / myPop_vis.sum() * 100
    one_way     = vis_pass.sum()
    recip       = sum(1 for i in range(n) for j in range(n)
                      if i != j and vis_pass[i][j] and vis_pass[j][i])
    print(f"\n  DISUGUAGLIANZA:")
    print(f"    pop_visAttractionPolarization : {gini:.3f}")
    print(f"    Top 20% raccoglie             : {top20_share:.0f}% dell'attenzione")
    print(f"    Bottom 30% raccoglie          : {bot30_share:.0f}% dell'attenzione")
    if one_way > 0:
        print(f"    Reciprocità visiva            : {recip/one_way*100:.1f}%")

    # ── Appeal per-NPC ───────────────────────────────────────────
    has_mutual = (visMatchCount > 0)
    print(f"\n  APPEAL (emergente, Conroy-Beam 2018):")
    print(f"    myPop_visAttraction   : mean={myPop_vis.mean():.3f} sd={myPop_vis.std():.3f} "
          f"min={myPop_vis.min():.3f} max={myPop_vis.max():.3f}")
    if mode == MODE_CRUISING and myPop_sex[has_mutual].std() > 0:
        print(f"    myPop_sexAttraction   : mean={myPop_sex[has_mutual].mean():.3f} sd={myPop_sex[has_mutual].std():.3f} "
              f"(su {has_mutual.sum()} NPC con ≥1 visMatch)")
    print(f"    myPop_globalAttraction: mean={myPop_glob.mean():.3f} sd={myPop_glob.std():.3f}")
    print(f"    r(visAttr, globalAttr)         : {np.corrcoef(myPop_vis, myPop_glob)[0,1]:+.3f}")
    print(f"    r(visAttr, visMatchCount)      : {np.corrcoef(myPop_vis, visMatchCount)[0,1]:+.3f}")
    if sexMatchCount.std() > 0:
        print(f"    r(visAttr, sexMatchCount)      : {np.corrcoef(myPop_vis, sexMatchCount)[0,1]:+.3f}")
    if mode == MODE_CRUISING and myPop_sex[has_mutual].std() > 0 and sexMatchCount.std() > 0:
        print(f"    r(sexAttr, sexMatchCount)      : {np.corrcoef(myPop_sex, sexMatchCount)[0,1]:+.3f}")
        print(f"    r(globalAttr, sexMatchCount)   : {np.corrcoef(myPop_glob, sexMatchCount)[0,1]:+.3f}")
    ps_vals = np.array([P[i]['MyPenisSize'] for i in range(n)])
    print(f"    r(PenisSize, visAttr)          : {np.corrcoef(ps_vals, myPop_vis)[0,1]:+.3f}  "
          f"r(PenisSize, sexAttr): {np.corrcoef(ps_vals, myPop_sex)[0,1]:+.3f}")

    # ── Assortative mating ───────────────────────────────────────
    if mode == MODE_CRUISING:
        full_pairs = [(i, j) for i in range(n) for j in range(i+1, n)
                      if vis_pass[i][j] and vis_pass[j][i]
                      and sex_matrix[i][j] >= thr and sex_matrix[j][i] >= thr]
    else:
        full_pairs = [(i, j) for i in range(n) for j in range(i+1, n)
                      if vis_pass[i][j] and vis_pass[j][i]]
    if len(full_pairs) >= 5:
        mv_a = np.array([myPop_vis[i] for i, j in full_pairs])
        mv_b = np.array([myPop_vis[j] for i, j in full_pairs])
        print(f"    r(visAttr_A, visAttr_B) match  : {np.corrcoef(mv_a, mv_b)[0,1]:+.3f}")

    # ── Sex frustration breakdown (solo cruising) ────────────────
    if mode == MODE_CRUISING:
        mutual_vis_pairs = [(i, j) for i in range(n) for j in range(i+1, n)
                            if vis_pass[i][j] and vis_pass[j][i]]
        n_mv = len(mutual_vis_pairs)
        fr   = n_mv - n_sexPairs
        print(f"\n  SEX FRUSTRATION (cruising):")
        print(f"    pairs_visCompatibilityRate       : {n_mv}")
        print(f"    pairs_visSexCompatibilityRate    : {n_sexPairs}")
        print(f"    Frustrate                        : {fr} ({fr/max(1,n_mv)*100:.0f}%)")
        if fr > 0:
            bt_kill = kink_kill = general_kill = 0
            for i, j in mutual_vis_pairs:
                if sex_matrix[i][j] >= thr and sex_matrix[j][i] >= thr:
                    continue
                bt_ij = abs(PR[i]['myDesired_BottomTop'] - P[j]['MyBottomTop'])
                bt_ji = abs(PR[j]['myDesired_BottomTop'] - P[i]['MyBottomTop'])
                kl_d  = (abs(PR[i].get('myDesired_KinkLevel', 0.5) - P[j]['MyKinkLevel'])
                       + abs(PR[j].get('myDesired_KinkLevel', 0.5) - P[i]['MyKinkLevel']))
                if bt_ij > 0.60 or bt_ji > 0.60:
                    bt_kill += 1
                elif kl_d > 0.80:
                    kink_kill += 1
                else:
                    general_kill += 1
            print(f"    pairs_bottomTopIncompatibilityShare : {bt_kill} ({bt_kill/fr*100:.0f}%)")
            print(f"    pairs_kinkIncompatibilityShare      : {kink_kill} ({kink_kill/fr*100:.0f}%)")
            print(f"    General distance                    : {general_kill} ({general_kill/fr*100:.0f}%)")

    # ── Predittori esclusione ────────────────────────────────────
    excluded = (visMatchCount == 0).astype(float)
    traits = {
        'MyBeauty':      [P[i]['MyBeauty'] for i in range(n)],
        'MyMuscle':      [P[i]['MyMuscle'] for i in range(n)],
        'MySlim':        [P[i]['MySlim'] for i in range(n)],
        'MyMasculinity': [P[i]['MyMasculinity'] for i in range(n)],
        'MyAge':         [P[i]['MyAge'] for i in range(n)],
        'MyHeight':      [P[i]['MyHeight'] for i in range(n)],
        'MyHair':        [P[i]['MyHair'] for i in range(n)],
    }
    sfx = '_cruise' if mode == MODE_CRUISING else '_app'
    print(f"\n  PREDITTORI pop_neverVisMatched{sfx} (r):")
    corrs = {nm: np.corrcoef(vals, excluded)[0,1] for nm, vals in traits.items()}
    for nm, r in sorted(corrs.items(), key=lambda x: -abs(x[1])):
        print(f"    {nm:<16} r={r:+.3f}  {'↑esclusione' if r>0 else '↓esclusione'}")

    print(f"\n  PREDITTORI APPEAL (r con myPop_visAttraction / myPop_sexAttraction):")
    for nm, vals in traits.items():
        r_vis = np.corrcoef(vals, myPop_vis)[0,1]
        r_sex = np.corrcoef(vals, myPop_sex)[0,1] if myPop_sex.std() > 0 else 0
        print(f"    {nm:<16} vis={r_vis:+.3f}  sex={r_sex:+.3f}")

    # ── Compromesso nei match ────────────────────────────────────
    if len(full_pairs) >= 5:
        print(f"\n  COMPROMESSO NEI MATCH:")
        vis_vars_map = {
            'Muscle':'MyMuscle','Slim':'MySlim','Beauty':'MyBeauty',
            'Masculinity':'MyMasculinity','Age':'myAgeNorm','Height':'myHeightNorm',
            'Hair':'MyHair',
        }
        for var, pkey in vis_vars_map.items():
            gaps_m, gaps_p = [], []
            for i, j in full_pairs:
                di = PR[i].get(f'myDesired_{var}')
                dj = PR[j].get(f'myDesired_{var}')
                if di is not None:
                    gaps_m += [abs(di-P[j][pkey]), abs(dj-P[i][pkey])]
                    gaps_p += [abs(di-P[k][pkey]) for k in range(min(n, 50))]
            gm_val = np.mean(gaps_m) if gaps_m else 0
            gp_val = np.mean(gaps_p) if gaps_p else 0
            if gp_val > 0:
                print(f"    {var:<12} match={gm_val:.3f} pop={gp_val:.3f} "
                      f"compromesso={gm_val/gp_val:.0%}")

    # ── Bias sistemici — Etnia ───────────────────────────────────
    print(f"\n  BIAS SISTEMICI — Etnia:")
    for eth in ['White','Latino','Black','Asian','MiddleEastern','Mixed']:
        idx = [i for i in range(n) if P[i]['MyEthnicity'] == eth]
        if idx:
            mv_e = np.mean([myPop_vis[i] for i in idx])
            ex0  = sum(1 for i in idx if visMatchCount[i] == 0)
            print(f"    {eth:<15} n={len(idx):>3} myPop_visAttr={mv_e:.3f} "
                  f"neverVisMatched={ex0}({ex0/len(idx)*100:.0f}%)")

    # ── Bias sistemici — PozStatus ───────────────────────────────
    print(f"\n  BIAS SISTEMICI — PozStatus:")
    for poz in ['None','PozUndet','Poz']:
        idx = [i for i in range(n) if P[i]['MyPozStatus'] == poz]
        if idx:
            mv_p = np.mean([myPop_vis[i] for i in idx])
            ex0  = sum(1 for i in idx if visMatchCount[i] == 0)
            print(f"    {poz:<12} n={len(idx):>3} myPop_visAttr={mv_p:.3f} "
                  f"neverVisMatched={ex0}({ex0/len(idx)*100:.0f}%)")

    # ── Tribe ────────────────────────────────────────────────────
    print(f"\n  TRIBE TAG:")
    td = defaultdict(lambda: {'myOut_visCount': [], 'visMatchCount': [], 'sexMatchCount': []})
    for i in range(n):
        t = P[i]['myTribeTag']
        td[t]['myOut_visCount'].append(myOut_visCount[i])
        td[t]['visMatchCount'].append(visMatchCount[i])
        td[t]['sexMatchCount'].append(sexMatchCount[i])
    print(f"    {'Tribe':<14} {'n':>3} {'myOut_visCount':>14} {'neverVisMat%':>12} {'neverSexMat%':>12}")
    for tribe, d in sorted(td.items(), key=lambda x: -len(x[1]['myOut_visCount'])):
        nn  = len(d['myOut_visCount'])
        ovm = np.mean(d['myOut_visCount'])
        om0 = sum(1 for x in d['visMatchCount'] if x == 0) / nn * 100
        of0 = sum(1 for x in d['sexMatchCount'] if x == 0) / nn * 100
        print(f"    {tribe:<14} {nn:>3} {ovm:>14.1f} {om0:>11.0f}% {of0:>11.0f}%")

    # ── KinkTypes ────────────────────────────────────────────────
    print(f"\n  KINKTYPES (flag multipli):")
    kt_cnt = defaultdict(int)
    n_vanilla = 0
    for p in P:
        if p['myKinkTypes'] == ['Vanilla']:
            n_vanilla += 1
        else:
            for k in p['myKinkTypes']:
                kt_cnt[k] += 1
    print(f"    Vanilla                 : {n_vanilla} ({n_vanilla/n*100:.0f}%)")
    for kt, cnt in sorted(kt_cnt.items(), key=lambda x: -x[1]):
        print(f"    {kt:<20} : {cnt:>3} ({cnt/n*100:.0f}%)")

    # ── Distribuzione profili ────────────────────────────────────
    print(f"\n  DISTRIBUZIONE PROFILI (validazione):")
    check = [
        ('MyAge', 38.0), ('MyMuscle', 0.30), ('MySlim', 0.48),
        ('MyBeauty', 0.40), ('MyMasculinity', 0.62), ('MyHair', 0.46),
        ('MyKinkLevel', 0.18), ('MyPenisSize', 0.50), ('MySelfEsteem', 0.48),
        ('MySubDom', 0.44),
    ]
    for vname, target in check:
        vals = [P[i][vname] for i in range(n)]
        m = np.mean(vals); s = np.std(vals)
        delta = m - target
        flag = " ⚠" if abs(delta) > s * 0.30 else ""
        print(f"    {vname:<16} mean={m:.3f} sd={s:.3f} "
              f"(target={target:.2f} Δ={delta:+.3f}){flag}")

    print(f"\n  σ=0.13 (cruise) / σ=0.15 (app) e soglia={thr} sono parametri artistici.")


# ================================================================
# ENTRYPOINT
# ================================================================
def _parse_args():
    parser = argparse.ArgumentParser(
        description='Macchina del Desiderio v2.4')
    parser.add_argument('--n',         type=int,   default=N_SUBJECTS,
                        help=f'Numero soggetti (default: {N_SUBJECTS})')
    parser.add_argument('--seed',      type=int,   default=RANDOM_SEED,
                        help=f'Random seed (default: {RANDOM_SEED})')
    parser.add_argument('--threshold', type=float, default=0.35,
                        help='Soglia match (default: 0.35)')
    parser.add_argument('--mode',      type=str,   default='both',
                        choices=['cruising', 'app', 'both'],
                        help='Modalità: cruising (two-step), app (single-pass), both')
    return parser.parse_args()


if __name__ == '__main__':
    args = _parse_args()

    print("MACCHINA DEL DESIDERIO — v2.4")
    print("Dual mode: cruising (two-step) + app (single-pass)\n")

    modes = [MODE_CRUISING, MODE_APP] if args.mode == 'both' else [args.mode]

    for m in modes:
        results = run_simulation(
            n=args.n,
            seed=args.seed,
            threshold=args.threshold,
            mode=m,
            verbose=True,
        )


