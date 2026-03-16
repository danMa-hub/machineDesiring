"""
================================================================
MACCHINA DEL DESIDERIO — v2.1
================================================================
MODIFICHE rispetto a v2.0:
  TODO-5  MyPenisSize → Passata 1 (indipendente)
          BT in Passata 2 condizionato a PenisSize×0.10
          Fonte: Moskowitz & Hart 2011 (causalità pene→ruolo)
  TODO-7  MyMuscle: picco a 35 anni, calo piecewise
            lento 35-60 (~4.5%/decade), accelera dopo 60 (~8%/decade)
          MySlim: calo −0.15×an (calibrato NHANES), bonus muscle
            nonlineare max(0, muscle−0.30)×0.35
          Eccezione MuscleBear rimossa (emerge naturalmente)
          Fonte: Doherty 2003 (sarcopenia), NHANES 1999-2004
  TODO-8  MySubDom: media 0.44 (Holvoet 2017: 50% sub, 27% dom)
            correlazione BT ridotta a ×0.10 (dimensioni indipendenti)
          MySexActDepth: base 0.38 (Richters 2008: 37.2% MSM anal)
          MySelfEsteem: aggiunto PenisSize×0.12
            Fonte: Grov et al. 2010, Lever et al. 2006
  TODO-9  classify_tribe: prototype scoring (distanza euclidea pesata
            con pesi normalizzati a 1.0), sostituisce hard cutoff
          Leather rimosso dalle tribe (non è body type)
          Average aggiunto via soglia min_score < 0.32
          KinkTypes: lista di flag multipli (non categoria esclusiva)
            Fonte: Parsons 2010 MSM N=1214, meta-analisi chemsex 380k
  REPORT  generate_report() integrato — genera PNG con 15 grafici
          Richiede: matplotlib (pip install matplotlib)
ESECUZIONE:
  python macchina_v2.1.py
  python macchina_v2.1.py --no-report   (solo testo)
  python macchina_v2.1.py --n 500       (N custom)
================================================================
"""
# ── Import ─────────────────────────────────────────────────────
import sys
import argparse
import numpy as np
from collections import defaultdict
# matplotlib: opzionale — serve solo per generate_report()
try:
    import matplotlib
    matplotlib.use('Agg')   # backend headless: funziona senza schermo
    import matplotlib.pyplot as plt
    import matplotlib.gridspec as gridspec
    MATPLOTLIB_OK = True
except ImportError:
    MATPLOTLIB_OK = False
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
    'Beauty':      0.26,
    'Muscle':      0.24,
    'Slim':        0.18,
    'Masculinity': 0.18,
    'Age':         0.07,
    'Height':      0.04,
    'Hair':        0.03,
}
# Fonte: Moskowitz 2008 (BT complementarità = necessità)
# Fonte: GMFA 2017, Gay Men's Health Project (PenisSize)
BASE_WEIGHTS_SEXUAL = {
    'BottomTop': 0.40,
    'SubDom':    0.28,
    'KinkLevel': 0.17,
    'PenisSize': 0.15,
}
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
    # var: (r_assort, r_aspir, r_pop, noise_sd)
    'Muscle':      (0.25, 0.35, 0.40, 0.12),
    'Slim':        (0.25, 0.35, 0.40, 0.12),
    'Beauty':      (0.20, 0.40, 0.40, 0.10),
    'Masculinity': (0.45, 0.20, 0.35, 0.12),  # masc4masc (Bailey 1997)
    'Height':      (0.30, 0.20, 0.50, 0.10),
    'Hair':        (0.35, 0.00, 0.65, 0.18),   # bipolare, no ideale unico
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
# Leather rimosso: non è body type, catturato da KinkTypes
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
# ── KinkTypes — flag multipli ──────────────────────────────────
# Fonte: Parsons et al. 2010 (N=1214 MSM): group sex 60.6%,
#   exhib/voy 39.8%, watersports 32.8%, bondage 29.8%,
#   fisting 20.9%, S/M 20.7%, breath play 8.1%
# Fonte: meta-analisi chemsex 238 studi, N=380.505 MSM → 22%
# Struttura: (base_prob, kl_min_threshold, kl_weight, se_weight)
KINK_TYPES_DEF = {
    'GroupSex':    (0.40, 0.00, 0.20, 0.18),
    'LightBDSM':   (0.15, 0.05, 0.32, 0.15),
    'Watersports': (0.10, 0.08, 0.30, 0.22),
    'SM_Pain':     (0.02, 0.12, 0.28, 0.32),
    'Fisting':     (0.02, 0.12, 0.15, 0.40),
    'PupPlay':     (0.02, 0.18, 0.22, 0.12),
    'Chemsex':     (0.06, 0.00, 0.08, 0.18),
    'GearFetish':  (0.03, 0.12, 0.28, 0.12),
    'Roleplay':    (0.08, 0.00, 0.18, 0.10),
    'EdgePlay':    (0.00, 0.42, 0.12, 0.42),
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
    an  = p['AgeNorm']
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
    Fonte: Parsons 2010 (N=1214 MSM), chemsex meta-analisi 2024
    """
    kl = p['MyKinkLevel']
    se = p['MySexExtremity']
    # Vanilla hard floor
    if kl < 0.08:
        return ['Vanilla']
    active = [
        name for name, (base, kl_min, kw, sw) in KINK_TYPES_DEF.items()
        if kl >= kl_min and np.random.random() < clamp(base + kw * kl + sw * se)
    ]
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
    p['AgeNorm']    = an
    p['HeightNorm'] = hn
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
    p['MyBeauty']      = clamp(_beauty_base - an * 0.08)
    p['MyMasculinity'] = clamp(np.random.normal(0.62, 0.18)
                               + p['MyHair'] * 0.15
                               - p['MyQueerness'] * 0.20
                               + (hn - 0.50) * 0.08)
    p['MyExhibitionism'] = clamp(_exhib_base + p['MyMuscle'] * 0.10)
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
    age = p['MyAge']
    af = (0.40 if age < 25
          else (1.00 if age < 35 else (1.30 if age < 50 else 0.70)))
    pw = np.array([0.82, 0.08, 0.10]) * af
    pw /= pw.sum()
    p['MyPozStatus'] = np.random.choice(['None', 'Poz', 'PozUndet'], p=pw)
    sp = {'CondomOnly': 0.40, 'PrEP': 0.30, 'Bareback': 0.30}
    if p['MyPozStatus'] == 'Poz':
        sp['PrEP'] -= 0.25; sp['Bareback'] += 0.15; sp['CondomOnly'] += 0.10
    elif p['MyPozStatus'] == 'PozUndet':
        sp['PrEP'] -= 0.25; sp['Bareback'] += 0.20; sp['CondomOnly'] += 0.05
    sp = {k: max(0, v) for k, v in sp.items()}
    st = sum(sp.values())
    sp = {k: v / st for k, v in sp.items()}
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
    p['MySelfEsteem'] = clamp(0.27 + _se_obj + np.random.normal(0, 0.15))
    # TODO-9: classify_tribe con prototype scoring
    p['TribeTag']   = classify_tribe(p)
    # TODO-9: KinkTypes come lista di flag multipli
    p['KinkTypes']  = assign_kink_types(p)
    return p
# ================================================================
# MATING PREFERENCES — invariata da v2.0
# ================================================================
def generate_preferences(p):
    pref = {}
    profile_map = {
        'Muscle': 'MyMuscle', 'Slim': 'MySlim', 'Beauty': 'MyBeauty',
        'Hair': 'MyHair', 'Masculinity': 'MyMasculinity',
        'Height': 'HeightNorm', 'Age': 'AgeNorm',
        'BottomTop': 'MyBottomTop', 'SubDom': 'MySubDom',
        'KinkLevel': 'MyKinkLevel',
    }
    for var, (r_a, r_asp, r_pop, noise_sd) in ASSORT_COEFFS.items():
        my_val = p[profile_map[var]]
        ideal  = CULTURAL_IDEAL.get(var, POP_MEAN.get(var, 0.50))
        pop_m  = POP_MEAN.get(var, 0.50)
        if r_a < 0:
            desired = abs(r_a) * (1.0 - my_val) + (1 - abs(r_a)) * pop_m
        elif r_asp > 0:
            desired = r_a * my_val + r_asp * ideal + r_pop * pop_m
        else:
            desired = r_a * my_val + (1 - r_a) * pop_m
        desired += np.random.normal(0, noise_sd)
        pref[f'Desired_{var}'] = clamp(desired)
    # Desired_PenisSize — condizionale al ruolo sessuale
    # Fonte: GMFA 2017: bottom vuole top dotato
    bt = p['MyBottomTop']
    pref['Desired_PenisSize'] = clamp(
        0.50 + (1.0 - bt) * 0.25 + np.random.normal(0, 0.12))
    # Pesi individuali con noise ±30%
    pref['WeightsVisual'] = {}
    for v, w in BASE_WEIGHTS_VISUAL.items():
        pref['WeightsVisual'][v] = max(0.01, w + np.random.normal(0, w * 0.30))
    tw = sum(pref['WeightsVisual'].values())
    pref['WeightsVisual'] = {v: w / tw for v, w in pref['WeightsVisual'].items()}
    pref['WeightsSexual'] = {}
    for v, w in BASE_WEIGHTS_SEXUAL.items():
        pref['WeightsSexual'][v] = max(0.01, w + np.random.normal(0, w * 0.30))
    tw = sum(pref['WeightsSexual'].values())
    pref['WeightsSexual'] = {v: w / tw for v, w in pref['WeightsSexual'].items()}
    # Bias osservatore — fisso per persona, non per coppia
    # Fonte: Callander 2015 (etnia), Smit 2012 (PozStatus)
    pref['ObserverEthBias'] = max(0.0, np.random.normal(1.0, 0.40))
    pref['ObserverPozBias'] = max(0.0, np.random.normal(1.0, 0.50))
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
    W = Ap['WeightsVisual']
    profile_map = {
        'Muscle': 'MyMuscle', 'Slim': 'MySlim', 'Beauty': 'MyBeauty',
        'Hair': 'MyHair', 'Masculinity': 'MyMasculinity',
        'Height': 'HeightNorm', 'Age': 'AgeNorm',
    }
    sum_wd2 = 0.0
    for v in VISUAL_VARS:
        if v not in W or v not in profile_map:
            continue
        desired = Ap.get(f'Desired_{v}')
        if desired is None:
            continue
        sum_wd2 += W[v] * (desired - B[profile_map[v]]) ** 2
    d = np.sqrt(sum_wd2)
    sigma = 0.13   # parametro artistico — produce 14% no-match visivo
    attraction = np.exp(-(d ** 2) / (2 * sigma ** 2))
    # Moltiplicatori post-distanza: Etnia (OkCupid 2009 gay-specific)
    eth_dev = ETH_DESIRABILITY.get(B['MyEthnicity'], 1.0) - 1.0
    attraction *= max(0.50, 1.0 + eth_dev * Ap['ObserverEthBias'])
    # Moltiplicatori post-distanza: PozStatus (Survey 2019, GLAAD 2022)
    poz_dev = POZ_DESIRABILITY.get(B['MyPozStatus'], 1.0) - 1.0
    attraction *= max(0.20, 1.0 + poz_dev * Ap['ObserverPozBias'])
    # Varianza relazionale — Eastwick et al. 2024 (chimica imprevedibile)
    attraction += np.random.normal(0, 0.03)
    return clamp(attraction)
def calc_sexual_attraction(A, Ap, B):
    """
    Fase 2 — durante l'interazione.
    Distanza Euclidea pesata su variabili sessuali.
    """
    W = Ap['WeightsSexual']
    profile_map = {
        'BottomTop': 'MyBottomTop', 'SubDom': 'MySubDom',
        'KinkLevel': 'MyKinkLevel', 'PenisSize': 'MyPenisSize',
    }
    sum_wd2 = 0.0
    for v in SEXUAL_VARS:
        if v not in W or v not in profile_map:
            continue
        desired = Ap.get(f'Desired_{v}')
        if desired is None:
            continue
        sum_wd2 += W[v] * (desired - B[profile_map[v]]) ** 2
    sigma = 0.13
    attraction = np.exp(-(np.sqrt(sum_wd2) ** 2) / (2 * sigma ** 2))
    # Dealbreaker BottomTop (Li et al. 2002: necessità)
    bt_distance = abs(Ap['Desired_BottomTop'] - B['MyBottomTop'])
    if bt_distance > 0.60:
        attraction *= 0.10
    attraction += np.random.normal(0, 0.03)
    return clamp(attraction)
# ================================================================
# SIMULAZIONE
# ================================================================
def run_simulation(n=N_SUBJECTS, seed=RANDOM_SEED, threshold=0.35, verbose=True):
    np.random.seed(seed)
    if verbose:
        print(f"Generazione {n} profili...")
    profiles = [generate_profile(i) for i in range(n)]
    if verbose:
        print("Generazione preferenze...")
    prefs = [generate_preferences(p) for p in profiles]
    if verbose:
        print("Calcolo match (Euclidean distance)...")
    vis = np.zeros((n, n))
    sex = np.zeros((n, n))
    for i in range(n):
        for j in range(n):
            if i == j:
                continue
            vis[i][j] = calc_visual_attraction(profiles[i], prefs[i], profiles[j])
            sex[i][j] = calc_sexual_attraction(profiles[i], prefs[i], profiles[j])
    vis_pass = vis >= threshold
    out_vis  = vis_pass.sum(axis=1)
    out_mutual = np.zeros(n, int)
    out_full   = np.zeros(n, int)
    for i in range(n):
        for j in range(i + 1, n):
            if vis_pass[i][j] and vis_pass[j][i]:
                out_mutual[i] += 1
                out_mutual[j] += 1
            if (vis_pass[i][j] and vis_pass[j][i]
                    and sex[i][j] >= threshold and sex[j][i] >= threshold):
                out_full[i] += 1
                out_full[j] += 1
    mate_value = vis.mean(axis=0)
    results = {
        'profiles': profiles, 'prefs': prefs,
        'vis': vis, 'sex': sex,
        'out_vis': out_vis, 'out_mutual': out_mutual,
        'out_full': out_full, 'mate_value': mate_value,
        'threshold': threshold,
    }
    if verbose:
        _print_results(results, n, threshold)
    return results
def _gini(arr):
    arr = np.sort(arr)
    n = len(arr)
    cumul = np.cumsum(arr)
    return 1 - 2 * cumul.sum() / (n * cumul[-1]) if cumul[-1] > 0 else 0
def _print_results(res, n, thr):
    ov = res['out_vis']; om = res['out_mutual']; of = res['out_full']
    mv = res['mate_value']; vis = res['vis']; sex = res['sex']
    P  = res['profiles']; PR = res['prefs']
    vis_pass = vis >= thr
    n_mutual = om.sum() // 2
    n_full   = of.sum() // 2
    tp       = n * (n - 1) // 2
    print(f"\n{'='*60}")
    print(f"  v2.1 EUCLIDEAN — {n} soggetti — soglia {thr}")
    print(f"{'='*60}")
    print(f"  Visual attraction mean    : {vis[vis > 0].mean():.3f}")
    print(f"  Out vis mean/med/max      : {ov.mean():.1f} / {np.median(ov):.0f} / {ov.max()}")
    print(f"  0 visual outgoing         : {(ov==0).sum()} ({(ov==0).mean()*100:.0f}%)")
    print(f"  0 mutual visual           : {(om==0).sum()} ({(om==0).mean()*100:.0f}%)")
    print(f"  0 full match              : {(of==0).sum()} ({(of==0).mean()*100:.0f}%)")
    print(f"  Mutual visual pairs       : {n_mutual} ({n_mutual/tp*100:.2f}%)")
    print(f"  Full match pairs          : {n_full} ({n_full/tp*100:.2f}%)")
    if n_mutual > 0:
        print(f"  Mutual→Full conversion    : {n_full/n_mutual*100:.1f}%")
    # Disuguaglianza
    gini = _gini(mv)
    top20_thr   = np.sort(mv)[::-1][int(n * 0.20)]
    top20_share = mv[mv >= top20_thr].sum() / mv.sum() * 100
    bot30_thr   = np.sort(mv)[::-1][int(n * 0.70)]
    bot30_share = mv[mv <= bot30_thr].sum() / mv.sum() * 100
    one_way     = vis_pass.sum()
    recip       = sum(1 for i in range(n) for j in range(n)
                      if i != j and vis_pass[i][j] and vis_pass[j][i])
    print(f"\n  DISUGUAGLIANZA:")
    print(f"    Gini MateValue          : {gini:.3f}")
    print(f"    Top 20% raccoglie       : {top20_share:.0f}% dell'attenzione")
    print(f"    Bottom 30% raccoglie    : {bot30_share:.0f}% dell'attenzione")
    print(f"    Reciprocità visiva      : {recip/one_way*100:.1f}%" if one_way > 0 else "")
    # MateValue
    print(f"\n  MATEVALUE:")
    print(f"    mean={mv.mean():.3f} sd={mv.std():.3f} "
          f"min={mv.min():.3f} max={mv.max():.3f}")
    print(f"    r(MV, MutualVis)        : {np.corrcoef(mv, om)[0,1]:+.3f}")
    print(f"    r(MV, FullMatch)        : {np.corrcoef(mv, of)[0,1]:+.3f}" if of.std() > 0 else "")
    # Assortative mating
    full_pairs = [(i, j) for i in range(n) for j in range(i+1, n)
                  if vis_pass[i][j] and vis_pass[j][i]
                  and sex[i][j] >= thr and sex[j][i] >= thr]
    if len(full_pairs) >= 5:
        mv_a = np.array([mv[i] for i, j in full_pairs])
        mv_b = np.array([mv[j] for i, j in full_pairs])
        print(f"    r(MV_A, MV_B) in match  : {np.corrcoef(mv_a, mv_b)[0,1]:+.3f}")
    # Frustrazione sessuale
    mutual_vis_pairs = [(i, j) for i in range(n) for j in range(i+1, n)
                        if vis_pass[i][j] and vis_pass[j][i]]
    n_mv = len(mutual_vis_pairs)
    print(f"\n  FRUSTRAZIONE SESSUALE:")
    print(f"    Coppie mutual visual    : {n_mv}")
    print(f"    Coppie full match       : {n_full}")
    print(f"    Frustrate               : {n_mv-n_full} ({(n_mv-n_full)/max(1,n_mv)*100:.0f}%)")
    if n_mv > n_full:
        bt_kill = kink_kill = general_kill = 0
        for i, j in mutual_vis_pairs:
            if sex[i][j] >= thr and sex[j][i] >= thr:
                continue
            bt_ij  = abs(PR[i]['Desired_BottomTop'] - P[j]['MyBottomTop'])
            bt_ji  = abs(PR[j]['Desired_BottomTop'] - P[i]['MyBottomTop'])
            kl_d   = (abs(PR[i].get('Desired_KinkLevel', 0.5) - P[j]['MyKinkLevel'])
                    + abs(PR[j].get('Desired_KinkLevel', 0.5) - P[i]['MyKinkLevel']))
            if bt_ij > 0.60 or bt_ji > 0.60:
                bt_kill += 1
            elif kl_d > 0.80:
                kink_kill += 1
            else:
                general_kill += 1
        fr = n_mv - n_full
        print(f"    BT incompatibile        : {bt_kill} ({bt_kill/max(1,fr)*100:.0f}%)")
        print(f"    Kink mismatch           : {kink_kill} ({kink_kill/max(1,fr)*100:.0f}%)")
        print(f"    Distanza generale       : {general_kill} ({general_kill/max(1,fr)*100:.0f}%)")
    # Predittori esclusione
    excluded = (om == 0).astype(float)
    traits = {
        'Beauty':  [P[i]['MyBeauty'] for i in range(n)],
        'Muscle':  [P[i]['MyMuscle'] for i in range(n)],
        'Slim':    [P[i]['MySlim'] for i in range(n)],
        'Masc':    [P[i]['MyMasculinity'] for i in range(n)],
        'Age':     [P[i]['MyAge'] for i in range(n)],
        'Height':  [P[i]['MyHeight'] for i in range(n)],
        'Hair':    [P[i]['MyHair'] for i in range(n)],
    }
    print(f"\n  PREDITTORI ESCLUSIONE (r con 0-mutual):")
    corrs = {nm: np.corrcoef(vals, excluded)[0,1] for nm, vals in traits.items()}
    for nm, r in sorted(corrs.items(), key=lambda x: -abs(x[1])):
        print(f"    {nm:<10} r={r:+.3f}  {'↑esclusione' if r>0 else '↓esclusione'}")
    print(f"\n  PREDITTORI MATEVALUE:")
    for nm, vals in traits.items():
        r = np.corrcoef(vals, mv)[0,1]
        print(f"    {nm:<10} r={r:+.3f}")
    # Gap desiderio-realtà
    if len(full_pairs) >= 5:
        print(f"\n  COMPROMESSO NEI MATCH:")
        vis_vars_map = {
            'Muscle':'MyMuscle','Slim':'MySlim','Beauty':'MyBeauty',
            'Masculinity':'MyMasculinity','Age':'AgeNorm','Height':'HeightNorm',
            'Hair':'MyHair',
        }
        for var, pkey in vis_vars_map.items():
            gaps_m, gaps_p = [], []
            for i, j in full_pairs:
                di = PR[i].get(f'Desired_{var}')
                dj = PR[j].get(f'Desired_{var}')
                if di is not None:
                    gaps_m += [abs(di-P[j][pkey]), abs(dj-P[i][pkey])]
                    gaps_p += [abs(di-P[k][pkey]) for k in range(min(n, 50))]
            gm = np.mean(gaps_m) if gaps_m else 0
            gp = np.mean(gaps_p) if gaps_p else 0
            print(f"    {var:<12} match={gm:.3f} pop={gp:.3f} "
                  f"compromesso={gm/gp:.0%}" if gp > 0 else f"    {var}")
    # Bias sistemici
    print(f"\n  BIAS SISTEMICI — Etnia:")
    for eth in ['White','Latino','Black','Asian','MiddleEastern','Mixed']:
        idx = [i for i in range(n) if P[i]['MyEthnicity'] == eth]
        if idx:
            mv_e = np.mean([mv[i] for i in idx])
            ex0  = sum(1 for i in idx if om[i] == 0)
            print(f"    {eth:<15} n={len(idx):>3} MV={mv_e:.3f} "
                  f"0mutual={ex0}({ex0/len(idx)*100:.0f}%)")
    print(f"\n  BIAS SISTEMICI — PozStatus:")
    for poz in ['None','PozUndet','Poz']:
        idx = [i for i in range(n) if P[i]['MyPozStatus'] == poz]
        if idx:
            mv_p = np.mean([mv[i] for i in idx])
            ex0  = sum(1 for i in idx if om[i] == 0)
            print(f"    {poz:<12} n={len(idx):>3} MV={mv_p:.3f} "
                  f"0mutual={ex0}({ex0/len(idx)*100:.0f}%)")
    # Tribe
    print(f"\n  TRIBE TAG:")
    td = defaultdict(lambda: {'ov': [], 'om': [], 'of': []})
    for i in range(n):
        t = P[i]['TribeTag']
        td[t]['ov'].append(ov[i])
        td[t]['om'].append(om[i])
        td[t]['of'].append(of[i])
    print(f"    {'Tribe':<14} {'n':>3} {'out_vis':>7} {'0mut%':>5} {'0full%':>6}")
    for tribe, d in sorted(td.items(), key=lambda x: -len(x[1]['ov'])):
        nn  = len(d['ov'])
        ovm = np.mean(d['ov'])
        om0 = sum(1 for x in d['om'] if x == 0) / nn * 100
        of0 = sum(1 for x in d['of'] if x == 0) / nn * 100
        print(f"    {tribe:<14} {nn:>3} {ovm:>7.1f} {om0:>4.0f}% {of0:>5.0f}%")
    # KinkTypes
    print(f"\n  KINKTYPES (flag multipli):")
    kt_cnt = defaultdict(int)
    n_vanilla = 0
    for p in P:
        if p['KinkTypes'] == ['Vanilla']:
            n_vanilla += 1
        else:
            for k in p['KinkTypes']:
                kt_cnt[k] += 1
    print(f"    Vanilla                 : {n_vanilla} ({n_vanilla/n*100:.0f}%)")
    for kt, cnt in sorted(kt_cnt.items(), key=lambda x: -x[1]):
        print(f"    {kt:<20} : {cnt:>3} ({cnt/n*100:.0f}%)")
    # Distribuzione profili
    print(f"\n  DISTRIBUZIONE PROFILI (validazione):")
    check = [
        ('MyAge', 38.0), ('MyMuscle', 0.30), ('MySlim', 0.48),
        ('MyBeauty', 0.45), ('MyMasculinity', 0.62), ('MyHair', 0.46),
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
    print(f"\n  σ=0.13 e soglia={thr} sono parametri artistici.")
# ================================================================
# REPORT VISIVO — integrato in v2.1
# ================================================================
PALETTE = {
    'gold':   '#C8A96E',
    'brown':  '#8B6B4E',
    'cream':  '#E8D5B7',
    'dark':   '#1A1A1A',
    'mid':    '#3D3D3D',
    'light':  '#F5F0E8',
    'red':    '#C45C3A',
    'blue':   '#4A7B9D',
    'green':  '#5A8C6A',
    'bg':     '#0D0D0D',
}
TRIBE_ORDER = [
    'Bear','Cub','Wolf','Otter','Chub','Twink','Twunk',
    'Jock','Muscle','MuscleBear','Daddy','CleanCut',
    'Rugged','Femboy','Average',
]
def generate_report(results, output_path='macchina_report.png'):
    """
    Genera report visivo PNG con 15 grafici.
    Richiede matplotlib (pip install matplotlib).
    Args:
        results : dict restituito da run_simulation()
        output_path : percorso file output (default: macchina_report.png)
    """
    if not MATPLOTLIB_OK:
        print("  [REPORT] matplotlib non disponibile — installa con:")
        print("  pip install matplotlib")
        return None
    P   = results['profiles']
    PR  = results['prefs']
    mv  = results['mate_value']
    ov  = results['out_vis']
    om  = results['out_mutual']
    of  = results['out_full']
    vis = results['vis']
    sex = results['sex']
    thr = results.get('threshold', 0.35)
    n   = len(P)
    plt.style.use('dark_background')
    plt.rcParams.update({
        'font.family':    'monospace',
        'axes.facecolor': PALETTE['dark'],
        'figure.facecolor': PALETTE['bg'],
        'axes.edgecolor': PALETTE['mid'],
        'axes.labelcolor': PALETTE['gold'],
        'xtick.color':    PALETTE['brown'],
        'ytick.color':    PALETTE['brown'],
        'text.color':     PALETTE['cream'],
        'grid.color':     '#2A2A2A',
        'grid.alpha':     0.5,
    })
    fig = plt.figure(figsize=(22, 28), facecolor=PALETTE['bg'])
    fig.suptitle(
        'MACCHINA DEL DESIDERIO v2.1 — Report Simulazione',
        fontsize=18, color=PALETTE['gold'], fontweight='bold', y=0.98,
    )
    gs = gridspec.GridSpec(5, 3, figure=fig,
                           hspace=0.45, wspace=0.35,
                           top=0.95, bottom=0.03, left=0.07, right=0.97)
    gold  = PALETTE['gold'];  red   = PALETTE['red']
    cream = PALETTE['cream']; blue  = PALETTE['blue']
    green = PALETTE['green']
    # ── 1. Distribuzione età ────────────────────────────────────
    ax = fig.add_subplot(gs[0, 0])
    ages = [p['MyAge'] for p in P]
    ax.hist(ages, bins=20, color=gold, alpha=0.85, edgecolor=PALETTE['bg'])
    ax.axvline(np.mean(ages), color=red, lw=1.5, ls='--',
               label=f'μ={np.mean(ages):.1f}')
    ax.set_title('Distribuzione Età', color=gold, fontsize=11)
    ax.set_xlabel('Età'); ax.legend(fontsize=8); ax.grid(True, axis='y')
    # ── 2. Distribuzione MateValue ──────────────────────────────
    ax = fig.add_subplot(gs[0, 1])
    ax.hist(mv, bins=25, color=cream, alpha=0.85, edgecolor=PALETTE['bg'])
    ax.axvline(mv.mean(), color=red, lw=1.5, ls='--',
               label=f'μ={mv.mean():.3f}')
    ax.set_title('MateValue (emergente)', color=gold, fontsize=11)
    ax.set_xlabel('MateValue'); ax.legend(fontsize=8); ax.grid(True, axis='y')
    # ── 3. Scatter MV × FullMatch ───────────────────────────────
    ax = fig.add_subplot(gs[0, 2])
    sc = ax.scatter(mv, of, c=ov, cmap='YlOrBr', s=18, alpha=0.7)
    plt.colorbar(sc, ax=ax, label='OutVis')
    r = np.corrcoef(mv, of)[0, 1] if of.std() > 0 else 0
    ax.set_title(f'MV × FullMatch  r={r:+.3f}', color=gold, fontsize=11)
    ax.set_xlabel('MateValue'); ax.set_ylabel('Full Match #')
    ax.grid(True)
    # ── 4. Ruolo sessuale ───────────────────────────────────────
    ax = fig.add_subplot(gs[1, 0])
    bts = [p['MyBottomTop'] for p in P]
    labels = ['Bottom\n(0-0.33)', 'Versatile\n(0.33-0.66)', 'Top\n(0.66-1)']
    counts = [
        sum(1 for b in bts if b < 0.33),
        sum(1 for b in bts if 0.33 <= b < 0.66),
        sum(1 for b in bts if b >= 0.66),
    ]
    bars = ax.bar(labels, counts, color=[blue, green, red],
                  alpha=0.85, edgecolor=PALETTE['bg'])
    for bar, c in zip(bars, counts):
        ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.5,
                f'{c/n*100:.0f}%', ha='center', va='bottom',
                fontsize=9, color=cream)
    ax.set_title('Ruolo Sessuale', color=gold, fontsize=11)
    ax.grid(True, axis='y')
    # ── 5. Bias etnia ───────────────────────────────────────────
    ax = fig.add_subplot(gs[1, 1])
    ethnicities = ['White','Latino','Black','Asian','MiddleEastern','Mixed']
    eth_mv  = [np.mean([mv[i] for i,p in enumerate(P) if p['MyEthnicity']==e])
               if any(p['MyEthnicity']==e for p in P) else 0 for e in ethnicities]
    eth_exc = [np.mean([(om[i]==0)*100 for i,p in enumerate(P) if p['MyEthnicity']==e])
               if any(p['MyEthnicity']==e for p in P) else 0 for e in ethnicities]
    x = np.arange(len(ethnicities)); w = 0.4
    ax.bar(x - w/2, eth_mv,  w, color=gold, alpha=0.8, label='MV medio')
    ax2 = ax.twinx()
    ax2.bar(x + w/2, eth_exc, w, color=red,  alpha=0.6, label='%0mutual')
    ax.set_xticks(x)
    ax.set_xticklabels([e[:5] for e in ethnicities], rotation=30, fontsize=8)
    ax.set_title('Bias per Etnia', color=gold, fontsize=11)
    ax.set_ylabel('MateValue', color=gold, fontsize=9)
    ax2.set_ylabel('%esclusi', color=red, fontsize=9)
    ax.legend(loc='upper left', fontsize=7)
    ax2.legend(loc='upper right', fontsize=7)
    # ── 6. Distribuzione tribe ──────────────────────────────────
    ax = fig.add_subplot(gs[1, 2])
    td = defaultdict(int)
    for p in P:
        td[p['TribeTag']] += 1
    tribes_p = [t for t in TRIBE_ORDER if td[t] > 0]
    counts_t = [td[t] for t in tribes_p]
    colors_t = plt.cm.get_cmap('YlOrBr')(np.linspace(0.2, 0.9, len(tribes_p)))
    bars = ax.barh(tribes_p, counts_t, color=colors_t,
                   alpha=0.85, edgecolor=PALETTE['bg'])
    for bar, c in zip(bars, counts_t):
        ax.text(bar.get_width() + 0.2, bar.get_y() + bar.get_height()/2,
                f'{c/n*100:.0f}%', va='center', fontsize=7, color=cream)
    ax.set_title('Tribe Distribution', color=gold, fontsize=11)
    ax.grid(True, axis='x')
    # ── 7. Scatter Muscle × Slim per tribe ─────────────────────
    ax = fig.add_subplot(gs[2, 0])
    tribe_colors = {t: plt.cm.tab20(i/20) for i, t in enumerate(TRIBE_ORDER)}
    for tribe in tribes_p:
        idx = [i for i,p in enumerate(P) if p['TribeTag'] == tribe]
        ax.scatter([P[i]['MyMuscle'] for i in idx],
                   [P[i]['MySlim']   for i in idx],
                   s=20, alpha=0.7, color=tribe_colors[tribe], label=tribe)
    ax.set_title('Muscle × Slim (tribe)', color=gold, fontsize=11)
    ax.set_xlabel('Muscle'); ax.set_ylabel('Slim')
    ax.legend(fontsize=5, ncol=2, loc='upper right')
    ax.grid(True)
    # ── 8. KinkTypes distribution ───────────────────────────────
    ax = fig.add_subplot(gs[2, 1])
    kt_cnt = defaultdict(int)
    n_vanilla = 0
    for p in P:
        if p.get('KinkTypes') == ['Vanilla']:
            n_vanilla += 1
        else:
            for k in p.get('KinkTypes', []):
                kt_cnt[k] += 1
    kt_names  = ['Vanilla'] + [k for k,_ in sorted(kt_cnt.items(), key=lambda x: -x[1])]
    kt_values = [n_vanilla] + [kt_cnt[k] for k in kt_names[1:]]
    c_kink = [cream] + [red] * len(kt_names[1:])
    bars = ax.barh(kt_names, kt_values, color=c_kink,
                   alpha=0.8, edgecolor=PALETTE['bg'])
    for bar, c in zip(bars, kt_values):
        ax.text(bar.get_width() + 0.2, bar.get_y() + bar.get_height()/2,
                f'{c/n*100:.0f}%', va='center', fontsize=8, color=cream)
    ax.set_title('KinkTypes (flag multipli)', color=gold, fontsize=11)
    ax.grid(True, axis='x'); ax.invert_yaxis()
    # ── 9. KinkLevel × SexExtremity ────────────────────────────
    ax = fig.add_subplot(gs[2, 2])
    kls  = np.array([p['MyKinkLevel']    for p in P])
    ses  = np.array([p['MySexExtremity'] for p in P])
    nkt  = np.array([len(p.get('KinkTypes', [])) if p.get('KinkTypes') != ['Vanilla']
                     else 0 for p in P])
    sc = ax.scatter(kls, ses, c=nkt, cmap='YlOrBr', s=25, alpha=0.8)
    plt.colorbar(sc, ax=ax, label='# KinkTypes attivi')
    ax.set_title('KinkLevel × SexExtremity', color=gold, fontsize=11)
    ax.set_xlabel('KinkLevel'); ax.set_ylabel('SexExtremity')
    ax.grid(True)
    # ── 10. Frustrazione sessuale (pie) ─────────────────────────
    ax = fig.add_subplot(gs[3, 0])
    n_mutual_pairs = om.sum() // 2
    n_full_pairs   = of.sum() // 2
    sizes  = [n_full_pairs, n_mutual_pairs - n_full_pairs,
              n*(n-1)//2 - n_mutual_pairs]
    labels_pie = [f'Full match\n{n_full_pairs}',
                  f'Frustrate\n{n_mutual_pairs-n_full_pairs}',
                  f'No visivo\n{sizes[2]}']
    ax.pie(sizes, labels=labels_pie, colors=[green, red, '#2A2A2A'],
           autopct='%1.1f%%', startangle=90,
           textprops={'fontsize': 8, 'color': cream})
    ax.set_title(f'Coppie possibili ({n*(n-1)//2})', color=gold, fontsize=11)
    # ── 11. 0/1/≥2 full match per individuo ────────────────────
    ax = fig.add_subplot(gs[3, 1])
    vals_fm = [(of==0).sum(), (of==1).sum(), (of>=2).sum()]
    bars = ax.bar(['0 match','1 match','≥2 match'], vals_fm,
                  color=[red, gold, green], alpha=0.85, edgecolor=PALETTE['bg'])
    for bar, c in zip(bars, vals_fm):
        ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.5,
                f'{c/n*100:.0f}%', ha='center', va='bottom',
                fontsize=10, color=cream)
    ax.set_title('Full Match per individuo', color=gold, fontsize=11)
    ax.grid(True, axis='y')
    # ── 12. Correlazioni tratti × MateValue ────────────────────
    ax = fig.add_subplot(gs[3, 2])
    trait_names = ['Beauty','Muscle','Slim','Masculinity','Age','Hair','PenisSize']
    trait_keys  = ['MyBeauty','MyMuscle','MySlim','MyMasculinity',
                   'MyAge','MyHair','MyPenisSize']
    corrs = [np.corrcoef([p[k] for p in P], mv)[0,1] for k in trait_keys]
    c_corr = [green if c > 0 else red for c in corrs]
    bars = ax.barh(trait_names, corrs, color=c_corr,
                   alpha=0.85, edgecolor=PALETTE['bg'])
    ax.axvline(0, color=cream, lw=0.8)
    ax.set_title('r(tratto, MateValue)', color=gold, fontsize=11)
    ax.set_xlabel('Pearson r'); ax.grid(True, axis='x')
    for bar, c in zip(bars, corrs):
        ax.text(c + (0.01 if c >= 0 else -0.04),
                bar.get_y() + bar.get_height()/2,
                f'{c:+.2f}', va='center', fontsize=8, color=cream)
    # ── 13. Età × Beauty (MV colore) ───────────────────────────
    ax = fig.add_subplot(gs[4, 0])
    sc = ax.scatter([p['MyAge'] for p in P],
                    [p['MyBeauty'] for p in P],
                    c=mv, cmap='YlOrBr', s=20, alpha=0.75)
    plt.colorbar(sc, ax=ax, label='MateValue')
    ax.set_title('Età × Beauty (MV colore)', color=gold, fontsize=11)
    ax.set_xlabel('Età'); ax.set_ylabel('Beauty'); ax.grid(True)
    # ── 14. Bias PozStatus ──────────────────────────────────────
    ax = fig.add_subplot(gs[4, 1])
    poz_states = ['None','PozUndet','Poz']
    poz_mv  = [np.mean([mv[i] for i,p in enumerate(P) if p['MyPozStatus']==ps]) for ps in poz_states]
    poz_exc = [np.mean([(om[i]==0)*100 for i,p in enumerate(P) if p['MyPozStatus']==ps]) for ps in poz_states]
    poz_n   = [sum(1 for p in P if p['MyPozStatus']==ps) for ps in poz_states]
    x = np.arange(3); w = 0.4
    ax.bar(x - w/2, poz_mv,  w, color=blue, alpha=0.8, label='MV medio')
    ax2b = ax.twinx()
    ax2b.bar(x + w/2, poz_exc, w, color=red, alpha=0.6, label='%esclusi')
    ax.set_xticks(x)
    ax.set_xticklabels([f'{s}\nn={poz_n[i]}' for i,s in enumerate(poz_states)],
                       fontsize=8)
    ax.set_title('Bias PozStatus', color=gold, fontsize=11)
    ax.set_ylabel('MateValue', color=blue, fontsize=9)
    ax2b.set_ylabel('%esclusi', color=red, fontsize=9)
    ax.legend(loc='upper left', fontsize=7)
    ax2b.legend(loc='upper right', fontsize=7)
    # ── 15. Metriche testo ──────────────────────────────────────
    ax = fig.add_subplot(gs[4, 2])
    ax.axis('off')
    conv = n_full_pairs/n_mutual_pairs*100 if n_mutual_pairs > 0 else 0
    stats_text = (
        f"  N = {n}   soglia = {thr}\n\n"
        f"  VisAttr mean    {vis[vis>0].mean():.3f}\n"
        f"  MateValue μ     {mv.mean():.3f}  σ={mv.std():.3f}\n"
        f"  MV max          {mv.max():.3f}\n"
        f"  MV min          {mv.min():.3f}\n"
        f"  Gini MV         {_gini(mv):.3f}\n\n"
        f"  Mutual vis      {n_mutual_pairs} ({n_mutual_pairs/(n*(n-1)//2)*100:.1f}%)\n"
        f"  Full match      {n_full_pairs} ({n_full_pairs/(n*(n-1)//2)*100:.2f}%)\n"
        f"  Vis→Full conv   {conv:.1f}%\n\n"
        f"  0 mutual vis    {(om==0).sum()} ({(om==0).mean()*100:.0f}%)\n"
        f"  0 full match    {(of==0).sum()} ({(of==0).mean()*100:.0f}%)\n\n"
        f"  r(MV, MutVis)   {np.corrcoef(mv,om)[0,1]:+.3f}\n"
        f"  r(MV, Full)     {np.corrcoef(mv,of)[0,1]:+.3f}"
    )
    ax.text(0.05, 0.95, stats_text, transform=ax.transAxes,
            fontsize=9, verticalalignment='top',
            fontfamily='monospace', color=cream,
            bbox=dict(boxstyle='round', facecolor=PALETTE['dark'], alpha=0.8))
    ax.set_title('Metriche chiave', color=gold, fontsize=11)
    plt.savefig(output_path, dpi=150, bbox_inches='tight',
                facecolor=PALETTE['bg'])
    print(f"  Report salvato: {output_path}")
    try:
        from IPython.display import display
        display(fig)
    except ImportError:
        pass
    plt.close(fig)
    return output_path
# ================================================================
# ENTRYPOINT
# ================================================================
def _parse_args():
    parser = argparse.ArgumentParser(
        description='Macchina del Desiderio v2.1')
    parser.add_argument('--n',         type=int,   default=N_SUBJECTS,
                        help=f'Numero soggetti (default: {N_SUBJECTS})')
    parser.add_argument('--seed',      type=int,   default=RANDOM_SEED,
                        help=f'Random seed (default: {RANDOM_SEED})')
    parser.add_argument('--threshold', type=float, default=0.35,
                        help='Soglia match (default: 0.35)')
    parser.add_argument('--no-report', action='store_true',
                        help='Salta generazione report visivo PNG')
    parser.add_argument('--output',    type=str,   default='macchina_report.png',
                        help='Path report PNG (default: macchina_report.png)')
    return parser.parse_args()
if __name__ == '__main__':
    args = _parse_args()
    print("MACCHINA DEL DESIDERIO — v2.1")
    print("Euclidean mate choice | prototype tribes | kink flags\n")
    results = run_simulation(
        n=args.n,
        seed=args.seed,
        threshold=args.threshold,
        verbose=True,
    )
    if not args.no_report:
        print("\nGenerazione report visivo...")
        generate_report(results, output_path=args.output)
    else:
        print("\n[Report visivo saltato — usa --no-report=False per generarlo]")
