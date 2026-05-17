"""
================================================================
MACCHINA DEL DESIDERIO — Vergine Totale v1.1
================================================================
MODIFICHE DA v1.0:
  1. aspiration_coeff iniziale: N(0.65, 0.08) → [0.50, 0.80]
     NON correlato a MySelfEsteem — varianza innata, emerge da esperienza
  2. selfMirror_coeff iniziale: N(0.35, 0.08) → [0.20, 0.50]
     INDIPENDENTE da aspiration (non più reciproci)
  3. culturalPull_coeff e marketAwareness_coeff espliciti
  4. Formula myDesired_X completa:
     core = (asp×better + sm×my) / (asp+sm)  [normalizzati]
     desired = (core + cP×ideal + mA×pop) / (1 + cP + mA)
  5. observedCulturalIdeal = media top 30% match (non tutti)
  6. Matrice 7 eventi (V1-V3, S1-S2a-S2b-S3) con update coefficienti
  7. Nomenclatura: my_learnedDesired → myDesired_Muscle, myDesired_Beauty, etc.
  8. Pesi (myWeightsVisual/Sexual) FISSI per individuo

ARCHITETTURA COEFFICIENTI:
  aspiration     = quanto voglio "meglio di me" [0.50, 0.80]
  selfMirror     = quanto accetto "simile a me" [0.20, 0.50]
  (indipendenti ma aggiornati con dinamica anticorrelata)
  culturalPull   = quanto peso ideale osservato [0, 0.30]
  marketAwareness = quanto mi adatto a pop reale [0, 0.30]

ESECUZIONE:
  python macchina_vergine_totale_1_1.py --n 700 --rounds 50
================================================================
"""

import sys
import argparse
import numpy as np
from collections import defaultdict

sys.path.insert(0, '.')
from macchina_v2_3_fixed_names import (
    generate_profile, generate_preferences,
    calc_visual_attraction, calc_sexual_attraction,
    RANDOM_SEED, _gini,
    VISUAL_VARS, SEXUAL_VARS,
    CULTURAL_IDEAL, POP_MEAN, ASSORT_COEFFS,
    BASE_WEIGHTS_VISUAL, BASE_WEIGHTS_SEXUAL,
)

# ================================================================
# COSTANTI
# ================================================================

N_SUBJECTS     = 700
N_ROUNDS       = 50
SUBSAMPLE_SIZE = 30

# Soglie
INITIAL_VIS_THRESHOLD = 0.42
INITIAL_SEX_THRESHOLD = 0.42
FLOOR_COEFFICIENT     = 0.75
THRESHOLD_ABSOLUTE_FLOOR = 0.02
THRESHOLD_CEILING     = 0.95

# ── Coefficienti Apprendimento ────────────────────────────────

# Aspiration iniziale: media 0.65, SD 0.08, clipped [0.50, 0.80]
ASPIRATION_MEAN = 0.65
ASPIRATION_SD   = 0.08
ASPIRATION_MIN  = 0.50
ASPIRATION_MAX  = 0.80
ASPIRATION_CEILING = 0.75  # ceiling dopo successi

# SelfMirror iniziale: media 0.35, SD 0.08, clipped [0.20, 0.50]
# Indipendente da aspiration (non più reciproco)
SELFMIRROR_MEAN = 0.35
SELFMIRROR_SD   = 0.08
SELFMIRROR_MIN  = 0.20
SELFMIRROR_MAX  = 0.50

# culturalPull iniziale (round 1, dopo osservazione primo batch)
CULTURAL_PULL_INIT_BASE = 0.15
CULTURAL_PULL_INIT_MATCH_BONUS = 0.10  # +0.10 se match_rate alto

# marketAwareness cresce progressivamente
MARKET_AWARENESS_INCREMENT = 0.003  # per interazione

# Learning rate ideale osservato
LR_OBSERVED_IDEAL = 0.10

# ── Matrice Eventi Coefficienti ───────────────────────────────

# Moltiplicatori aspiration per scenario
ASP_MULT_V1_MUTUAL_EXCLUSION = 0.98
ASP_MULT_V2_RECEIVED_UNMATCHED = 0.99
ASP_MULT_V3_EXPRESSED_REJECTED = 0.97
ASP_MULT_S1_VIS_NO_SEX = 0.96
ASP_MULT_S2A_I_YES_J_NO = 0.97
ASP_MULT_S2B_J_YES_I_NO = 0.98
ASP_MULT_S3_FULL_MATCH = 1.01

# Moltiplicatori selfMirror per scenario (indipendenti)
SM_MULT_V1_MUTUAL_EXCLUSION = 1.02
SM_MULT_V2_RECEIVED_UNMATCHED = 1.01
SM_MULT_V3_EXPRESSED_REJECTED = 1.03
SM_MULT_S1_VIS_NO_SEX = 1.04
SM_MULT_S2A_I_YES_J_NO = 1.03
SM_MULT_S2B_J_YES_I_NO = 1.02
SM_MULT_S3_FULL_MATCH = 0.99  # successo → lieve riduzione accettazione simile

# Incrementi marketAwareness per scenario
MA_INC_V1 = 0.003
MA_INC_V2 = 0.002
MA_INC_V3 = 0.004
MA_INC_S1 = 0.005
MA_INC_S2A = 0.004
MA_INC_S2B = 0.003
MA_INC_S3 = 0.010

# Learning rate myDesired per scenario
LR_DESIRED_V2_RECEIVED = +0.03   # rinforzo debole verso chi mi desidera
LR_DESIRED_V3_REJECTED = -0.02   # allontano da chi mi rifiuta
LR_DESIRED_S2A_REJECTED_SEX = -0.03  # su variabili sessuali
LR_DESIRED_S2B_DESIRED_SEX = +0.02   # su variabili sessuali
LR_DESIRED_S3_FULL_MATCH = +0.12  # rinforzo forte su tutte le variabili

# Soglie aggiustamenti
THR_VIS_UP_S_MUTUAL = 0.008
THR_VIS_DOWN_V1 = 0.020
THR_SEX_DOWN_S1 = 0.015
THR_SEX_MULT_S2A = 0.988
THR_UP_S3 = 0.025

# Frustrazione
FRUST_VIS_V1 = 0.12
FRUST_VIS_V3 = 0.10
FRUST_SEX_S1 = 0.18
FRUST_SEX_S2A = 0.15
FRUST_SEX_S2B = 0.08
FRUST_ISO_MULT_V2 = 0.95
FRUST_DECAY_S3 = 0.80
FRUST_ISO_DECAY_S3 = 0.70

# SelfEsteem sociometro (da Vergine v1.0/D)
SE_SIGNAL_FULL_MATCH = 0.68
SE_SIGNAL_VIS_MATCH  = 0.51
SE_SIGNAL_NO_MATCH   = 0.35
SE_OUTCOME_LR = 0.08
SE_ANCHOR_LR  = 0.015
SE_FRUSTRATION_EROSION = 0.30

# ── Variabili e Mapping ───────────────────────────────────────
VISUAL_VARS_LIST = ['Muscle', 'Beauty', 'Slim', 'Hair', 'Masculinity', 'Height', 'Age']
SEXUAL_VARS_LIST = ['BottomTop', 'SubDom', 'KinkLevel', 'PenisSize']
ALL_MATCH_VARS = VISUAL_VARS_LIST + SEXUAL_VARS_LIST

TRAIT_MAP = {
    'Muscle': 'MyMuscle', 'Slim': 'MySlim', 'Beauty': 'MyBeauty',
    'Hair': 'MyHair', 'Masculinity': 'MyMasculinity',
    'Height': 'myHeightNorm', 'Age': 'myAgeNorm',
    'BottomTop': 'MyBottomTop', 'SubDom': 'MySubDom',
    'KinkLevel': 'MyKinkLevel', 'PenisSize': 'MyPenisSize',
}


# ================================================================
# INIZIALIZZAZIONE STATO
# ================================================================

def _initialize_state(n, profiles):
    """Inizializza stato vergine totale v1.1."""
    state = []
    for i in range(n):
        # ── Coefficienti ──────────────────────────────────────
        asp_init = np.random.normal(ASPIRATION_MEAN, ASPIRATION_SD)
        asp_init = np.clip(asp_init, ASPIRATION_MIN, ASPIRATION_MAX)
        
        sm_init = np.random.normal(SELFMIRROR_MEAN, SELFMIRROR_SD)
        sm_init = np.clip(sm_init, SELFMIRROR_MIN, SELFMIRROR_MAX)
        
        s = {
            # Coefficienti
            'aspiration_coeff': asp_init,
            'selfMirror_coeff': sm_init,  # indipendente, non più 1-asp
            'culturalPull_coeff': 0.0,  # inizializza round 1
            'marketAwareness_coeff': 0.0,
            
            # Stime apprese
            'my_observedCulturalIdeal': {var: 0.50 for var in ALL_MATCH_VARS},
            'my_estimatedPopMean': {var: 0.50 for var in ALL_MATCH_VARS},
            
            # Target desiderio (chiavi separate, non dizionario)
            **{f'myDesired_{var}': 0.50 for var in ALL_MATCH_VARS},
            
            # Soglie adattive
            'my_visThreshold': INITIAL_VIS_THRESHOLD,
            'my_sexThreshold': INITIAL_SEX_THRESHOLD,
            'my_visFloor': THRESHOLD_ABSOLUTE_FLOOR,
            'my_sexFloor': THRESHOLD_ABSOLUTE_FLOOR,
            
            # Frustrazione
            'my_visFrustration': 0.0,
            'my_sexFrustration': 0.0,
            'my_isolationFrustration': 0.0,
            
            # Contatori
            'my_visMatchCount': 0,
            'my_fullMatchCount': 0,
            'my_encounterCount': 0,
            'my_receivedAttractionSum': 0.0,
            'my_receivedAttractionN': 0,
            'my_receivedSexAttractionSum': 0.0,
            'my_receivedSexAttractionN': 0,
            
            # SelfEsteem
            'MySelfEsteem_dynamic': profiles[i]['MySelfEsteem'],
            'MySelfEsteem_static': profiles[i]['MySelfEsteem'],
            
            # Lista partner matchati (per stats)
            'my_matchedPartners': [],
        }
        state.append(s)
    return state


def _calculate_myDesired(var, state_i, profile_i):
    """
    Calcola myDesired_X usando formula completa con coefficienti.
    
    Formula:
      better_than_self = my + aspiration × (1 - my)
      core_desired = (aspiration × better + selfMirror × my) / (aspiration + selfMirror)
      
      myDesired_X = (
          core_desired +
          culturalPull × observedIdeal[X] +
          marketAwareness × estimatedPopMean[X]
      ) / (1 + culturalPull + marketAwareness)
    
    Caso speciale: BottomTop (complementarità)
    """
    my_trait = profile_i[TRAIT_MAP[var]]
    asp = state_i['aspiration_coeff']
    sm = state_i['selfMirror_coeff']  # indipendente
    cp = state_i['culturalPull_coeff']
    ma = state_i['marketAwareness_coeff']
    
    observed_ideal = state_i['my_observedCulturalIdeal'][var]
    estimated_pop = state_i['my_estimatedPopMean'][var]
    
    # Caso speciale: BottomTop (complementarità innata)
    if var == 'BottomTop':
        # Bottom vuole Top → better = 1-my
        better_than_self = 1.0 - my_trait
        # Peso fisso complementarità 0.70, self 0.30
        core_desired = 0.70 * better_than_self + 0.30 * my_trait
    else:
        # Caso generale: aspirazione verso alto
        better_than_self = my_trait + asp * (1.0 - my_trait)
        # Normalizza: aspiration e selfMirror sono indipendenti
        total_core = asp + sm
        core_desired = (asp * better_than_self + sm * my_trait) / total_core
    
    # Formula completa
    if cp == 0 and ma == 0:
        # Round 0: solo core
        desired = core_desired
    else:
        # Round 1+: core + ideale + mercato
        desired = (
            core_desired +
            cp * observed_ideal +
            ma * estimated_pop
        ) / (1.0 + cp + ma)
    
    return np.clip(desired, 0.01, 0.99)


def _recalculate_all_myDesired(state, profiles):
    """Ricalcola tutti i myDesired_X per tutti gli NPC."""
    for i, s in enumerate(state):
        for var in ALL_MATCH_VARS:
            s[f'myDesired_{var}'] = _calculate_myDesired(var, s, profiles[i])


# ================================================================
# ATTRAZIONE — usa myDesired_X separati
# ================================================================

def _calc_vis_attraction_learned(A_profile, A_state, A_prefs, B_profile):
    """Calcolo attrazione visiva usando myDesired_X."""
    W = A_prefs['myWeightsVisual']
    sum_wd2 = 0.0
    
    for v in VISUAL_VARS_LIST:
        if v not in W:
            continue
        desired = A_state[f'myDesired_{v}']
        actual = B_profile[TRAIT_MAP[v]]
        diff = desired - actual
        sum_wd2 += W[v] * (diff ** 2)
    
    # Euclidea + bias (come v2.3)
    attr_base = np.exp(-sum_wd2)
    
    # Bias (come v2.3 ma NON disabilitati in v1.1)
    eth_mult = 1.0
    if B_profile['MyEthnicity'] in ['Black', 'Asian', 'Latino', 'Mixed', 'MiddleEastern']:
        from macchina_v2_3_fixed_names import ETH_DESIRABILITY
        eth_mult = ETH_DESIRABILITY.get(B_profile['MyEthnicity'], 1.0)
        eth_mult = eth_mult ** A_prefs['myEthBias']
    
    poz_mult = 1.0
    if B_profile['MyPozStatus'] in ['Poz', 'PozUndet']:
        from macchina_v2_3_fixed_names import POZ_DESIRABILITY
        poz_mult = POZ_DESIRABILITY.get(B_profile['MyPozStatus'], 1.0)
        poz_mult = poz_mult ** A_prefs['myPozBias']
    
    return attr_base * eth_mult * poz_mult


def _calc_sex_attraction_learned(A_profile, A_state, A_prefs, B_profile):
    """Calcolo attrazione sessuale usando myDesired_X."""
    W = A_prefs['myWeightsSexual']
    sum_wd2 = 0.0
    
    for v in SEXUAL_VARS_LIST:
        if v not in W:
            continue
        desired = A_state[f'myDesired_{v}']
        actual = B_profile[TRAIT_MAP[v]]
        diff = desired - actual
        sum_wd2 += W[v] * (diff ** 2)
    
    return np.exp(-sum_wd2)


# ================================================================
# BATCH PROCESSING
# ================================================================

def _process_subbatch(batch_indices, state, summary, profiles, prefs):
    """
    Processa un sub-batch di 30 persone.
    Tutti vedono tutti contemporaneamente (batch simultaneo).
    
    1. Calcola tutte le attrazioni (visual + sexual)
    2. Classifica eventi (V1-V3, S1-S2a-S2b-S3)
    3. Aggiorna coefficienti, myDesired, soglie
    4. Aggiorna stime (estimatedPopMean, observedCulturalIdeal)
    """
    n_batch = len(batch_indices)
    
    # Cache attrazioni
    vis_cache = {}
    sex_cache = {}
    
    # ── FASE 1: Calcola attrazioni visuali ───────────────────────
    for i in batch_indices:
        for j in batch_indices:
            if i == j:
                continue
            vis_ij = _calc_vis_attraction_learned(profiles[i], state[i], prefs[i], profiles[j])
            vis_cache[(i, j)] = vis_ij
            
            # Aggiorna received sum (per floor)
            state[j]['my_receivedAttractionSum'] += vis_ij
            state[j]['my_receivedAttractionN'] += 1
    
    # ── FASE 2: Identifica mutual visual, calcola sexual ─────────
    for i in batch_indices:
        for j in batch_indices:
            if i >= j:
                continue
            
            vis_ij = vis_cache.get((i, j))
            vis_ji = vis_cache.get((j, i))
            
            thr_i = state[i]['my_visThreshold']
            thr_j = state[j]['my_visThreshold']
            
            i_vis_j = (vis_ij >= thr_i)
            j_vis_i = (vis_ji >= thr_j)
            
            if i_vis_j and j_vis_i:
                # Mutual visual → calcola sexual
                sex_ij = _calc_sex_attraction_learned(profiles[i], state[i], prefs[i], profiles[j])
                sex_ji = _calc_sex_attraction_learned(profiles[j], state[j], prefs[j], profiles[i])
                sex_cache[(i, j)] = sex_ij
                sex_cache[(j, i)] = sex_ji
                
                # Aggiorna received sex sum
                state[j]['my_receivedSexAttractionSum'] += sex_ij
                state[j]['my_receivedSexAttractionN'] += 1
                state[i]['my_receivedSexAttractionSum'] += sex_ji
                state[i]['my_receivedSexAttractionN'] += 1
                
                # Conta vis match
                summary[i]['roundVisMatchCount'] += 1
                summary[j]['roundVisMatchCount'] += 1
    
    # ── FASE 3: Classifica eventi e applica effetti ──────────────
    for i in batch_indices:
        for j in batch_indices:
            if i == j:
                continue
            
            scenario = _classify_scenario(i, j, vis_cache, sex_cache, state)
            if scenario:
                _apply_scenario_effects(i, j, scenario, state, summary, profiles)
    
    # ── FASE 4: Aggiorna stime popolazione ───────────────────────
    _update_pop_estimates_from_batch(batch_indices, state, profiles)
    
    # ── FASE 5: Aggiorna ideale osservato (top 30%) ──────────────
    _update_observed_ideal_top30(batch_indices, state, summary, profiles)
    
    # ── FASE 6: Ricalcola tutti i myDesired_X ────────────────────
    _recalculate_all_myDesired(state, profiles)


def _classify_scenario(i, j, vis_cache, sex_cache, state):
    """Classifica quale dei 7 scenari è avvenuto."""
    vis_ij = vis_cache.get((i, j))
    vis_ji = vis_cache.get((j, i))
    
    thr_vis_i = state[i]['my_visThreshold']
    thr_vis_j = state[j]['my_visThreshold']
    
    i_vis_j = (vis_ij >= thr_vis_i)
    j_vis_i = (vis_ji >= thr_vis_j)
    
    # Fase visual
    if not i_vis_j and not j_vis_i:
        return 'V1_mutual_exclusion_visual'
    
    if not i_vis_j and j_vis_i:
        return 'V2_received_unmatched_visual'
    
    if i_vis_j and not j_vis_i:
        return 'V3_expressed_rejected_visual'
    
    # Mutual visual → fase sexual
    sex_ij = sex_cache.get((i, j))
    sex_ji = sex_cache.get((j, i))
    
    if sex_ij is None:
        return None  # non dovrebbe succedere
    
    thr_sex_i = state[i]['my_sexThreshold']
    thr_sex_j = state[j]['my_sexThreshold']
    
    i_sex_j = (sex_ij >= thr_sex_i)
    j_sex_i = (sex_ji >= thr_sex_j)
    
    if not i_sex_j and not j_sex_i:
        return 'S1_mutual_visual_no_sexual'
    
    if i_sex_j and not j_sex_i:
        return 'S2a_i_yes_j_no_sexual'
    
    if not i_sex_j and j_sex_i:
        return 'S2b_j_yes_i_no_sexual'
    
    if i_sex_j and j_sex_i:
        return 'S3_full_match_success'
    
    return None


def _apply_scenario_effects(i, j, scenario, state, summary, profiles):
    """Applica gli effetti dello scenario sui parametri di i."""
    s = state[i]
    
    if scenario == 'V1_mutual_exclusion_visual':
        s['aspiration_coeff'] *= ASP_MULT_V1_MUTUAL_EXCLUSION
        s['selfMirror_coeff'] *= SM_MULT_V1_MUTUAL_EXCLUSION
        s['marketAwareness_coeff'] += MA_INC_V1
        
        delta = THR_VIS_DOWN_V1 * s['my_visThreshold']
        s['my_visThreshold'] = max(s['my_visFloor'], s['my_visThreshold'] - delta)
        s['my_visFrustration'] += FRUST_VIS_V1
    
    elif scenario == 'V2_received_unmatched_visual':
        s['aspiration_coeff'] *= ASP_MULT_V2_RECEIVED_UNMATCHED
        s['selfMirror_coeff'] *= SM_MULT_V2_RECEIVED_UNMATCHED
        s['marketAwareness_coeff'] += MA_INC_V2
        s['my_isolationFrustration'] *= FRUST_ISO_MULT_V2
        
        # Rinforzo debole verso j
        for var in ALL_MATCH_VARS:
            j_trait = profiles[j][TRAIT_MAP[var]]
            current = s[f'myDesired_{var}']
            s[f'myDesired_{var}'] = current + LR_DESIRED_V2_RECEIVED * (j_trait - current)
    
    elif scenario == 'V3_expressed_rejected_visual':
        s['aspiration_coeff'] *= ASP_MULT_V3_EXPRESSED_REJECTED
        s['selfMirror_coeff'] *= SM_MULT_V3_EXPRESSED_REJECTED
        s['marketAwareness_coeff'] += MA_INC_V3
        s['my_visFrustration'] += FRUST_VIS_V3
        
        # Allontano da j
        for var in ALL_MATCH_VARS:
            j_trait = profiles[j][TRAIT_MAP[var]]
            current = s[f'myDesired_{var}']
            s[f'myDesired_{var}'] = current + LR_DESIRED_V3_REJECTED * (j_trait - current)
    
    elif scenario == 'S1_mutual_visual_no_sexual':
        s['aspiration_coeff'] *= ASP_MULT_S1_VIS_NO_SEX
        s['selfMirror_coeff'] *= SM_MULT_S1_VIS_NO_SEX
        s['marketAwareness_coeff'] += MA_INC_S1
        s['my_visThreshold'] += THR_VIS_UP_S_MUTUAL
        
        delta = THR_SEX_DOWN_S1 * s['my_sexThreshold']
        s['my_sexThreshold'] = max(s['my_sexFloor'], s['my_sexThreshold'] - delta)
        s['my_sexFrustration'] += FRUST_SEX_S1
    
    elif scenario == 'S2a_i_yes_j_no_sexual':
        s['aspiration_coeff'] *= ASP_MULT_S2A_I_YES_J_NO
        s['selfMirror_coeff'] *= SM_MULT_S2A_I_YES_J_NO
        s['marketAwareness_coeff'] += MA_INC_S2A
        s['my_visThreshold'] += THR_VIS_UP_S_MUTUAL
        s['my_sexThreshold'] *= THR_SEX_MULT_S2A
        s['my_sexFrustration'] += FRUST_SEX_S2A
        
        # Allontano da j (solo variabili sessuali)
        for var in SEXUAL_VARS_LIST:
            j_trait = profiles[j][TRAIT_MAP[var]]
            current = s[f'myDesired_{var}']
            s[f'myDesired_{var}'] = current + LR_DESIRED_S2A_REJECTED_SEX * (j_trait - current)
    
    elif scenario == 'S2b_j_yes_i_no_sexual':
        s['aspiration_coeff'] *= ASP_MULT_S2B_J_YES_I_NO
        s['selfMirror_coeff'] *= SM_MULT_S2B_J_YES_I_NO
        s['marketAwareness_coeff'] += MA_INC_S2B
        s['my_visThreshold'] += THR_VIS_UP_S_MUTUAL
        s['my_sexFrustration'] += FRUST_SEX_S2B
        
        # Rinforzo verso j (solo variabili sessuali)
        for var in SEXUAL_VARS_LIST:
            j_trait = profiles[j][TRAIT_MAP[var]]
            current = s[f'myDesired_{var}']
            s[f'myDesired_{var}'] = current + LR_DESIRED_S2B_DESIRED_SEX * (j_trait - current)
    
    elif scenario == 'S3_full_match_success':
        s['aspiration_coeff'] *= ASP_MULT_S3_FULL_MATCH
        s['aspiration_coeff'] = min(ASPIRATION_CEILING, s['aspiration_coeff'])
        s['selfMirror_coeff'] *= SM_MULT_S3_FULL_MATCH
        s['marketAwareness_coeff'] += MA_INC_S3
        
        # culturalPull adjustment basato su allineamento partner-ideale
        alignment = _calculate_partner_alignment_with_ideal(j, s, profiles)
        if alignment > 0.70:
            s['culturalPull_coeff'] += 0.01 * alignment
        else:
            s['culturalPull_coeff'] *= 0.99
        
        # Rinforzo forte verso TUTTE le variabili
        for var in ALL_MATCH_VARS:
            j_trait = profiles[j][TRAIT_MAP[var]]
            current = s[f'myDesired_{var}']
            s[f'myDesired_{var}'] = current + LR_DESIRED_S3_FULL_MATCH * (j_trait - current)
        
        # Soglie salgono
        s['my_visThreshold'] += THR_UP_S3
        s['my_sexThreshold'] += THR_UP_S3
        s['my_visThreshold'] = min(THRESHOLD_CEILING, s['my_visThreshold'])
        s['my_sexThreshold'] = min(THRESHOLD_CEILING, s['my_sexThreshold'])
        
        # Frustrazione decade
        s['my_visFrustration'] *= FRUST_DECAY_S3
        s['my_sexFrustration'] *= FRUST_DECAY_S3
        s['my_isolationFrustration'] *= FRUST_ISO_DECAY_S3
        
        # Registra match
        s['my_fullMatchCount'] += 1
        summary[i]['roundFullMatchCount'] += 1
        s['my_matchedPartners'].append(j)


def _calculate_partner_alignment_with_ideal(j, state_i, profiles):
    """Calcola quanto il partner j è allineato con l'ideale osservato di i."""
    ideal = state_i['my_observedCulturalIdeal']
    diffs = []
    for var in ALL_MATCH_VARS:
        j_trait = profiles[j][TRAIT_MAP[var]]
        ideal_trait = ideal[var]
        diffs.append(abs(j_trait - ideal_trait))
    
    mean_diff = np.mean(diffs)
    # Allineamento = 1 - distanza_media
    return max(0.0, 1.0 - mean_diff)


def _update_pop_estimates_from_batch(batch_indices, state, profiles):
    """Aggiorna estimatedPopMean da batch visto."""
    for i in batch_indices:
        s = state[i]
        n_prev = s['my_encounterCount']
        n_curr = len(batch_indices) - 1  # escludo me stesso
        
        for var in ALL_MATCH_VARS:
            # Media batch (escludo me stesso)
            batch_vals = [profiles[j][TRAIT_MAP[var]] for j in batch_indices if j != i]
            batch_mean = np.mean(batch_vals) if batch_vals else 0.50
            
            if n_prev == 0:
                # Round 0: bootstrap
                s['my_estimatedPopMean'][var] = batch_mean
            else:
                # Round 1+: media ponderata esatta
                old_mean = s['my_estimatedPopMean'][var]
                new_mean = (old_mean * n_prev + batch_mean * n_curr) / (n_prev + n_curr)
                s['my_estimatedPopMean'][var] = new_mean
        
        s['my_encounterCount'] += n_curr


def _update_observed_ideal_top30(batch_indices, state, summary, profiles):
    """
    Aggiorna observedCulturalIdeal da TOP 30% match osservati.
    Solo chi ha matchato E è nel top 30% per attraction ricevuta.
    """
    for i in batch_indices:
        s = state[i]
        
        # Chi ha matchato in questo batch?
        # (chi ha roundFullMatchCount > 0 o è in my_matchedPartners)
        matched_in_batch = [j for j in batch_indices 
                           if j in s.get('my_matchedPartners', [])]
        
        if len(matched_in_batch) < 3:
            # Troppo pochi match per stimare ideale
            continue
        
        # Ordina per attraction ricevuta (media)
        vis_received = []
        for j in matched_in_batch:
            if state[j]['my_receivedAttractionN'] > 0:
                mean_recv = (state[j]['my_receivedAttractionSum'] / 
                            state[j]['my_receivedAttractionN'])
            else:
                mean_recv = 0.0
            vis_received.append((j, mean_recv))
        
        vis_received.sort(key=lambda x: -x[1])  # ordine decrescente
        
        # Top 30%
        n_top = max(1, int(len(matched_in_batch) * 0.30))
        top_matches = [j for j, _ in vis_received[:n_top]]
        
        # Ideale = media solo dei top
        for var in ALL_MATCH_VARS:
            ideal_vals = [profiles[j][TRAIT_MAP[var]] for j in top_matches]
            new_ideal = np.mean(ideal_vals)
            
            old_ideal = s['my_observedCulturalIdeal'][var]
            s['my_observedCulturalIdeal'][var] = (
                old_ideal + LR_OBSERVED_IDEAL * (new_ideal - old_ideal)
            )


# ================================================================
# MAIN LOOP
# ================================================================

def run_simulation(n, n_rounds):
    """Esegue simulazione vergine totale v1.1."""
    np.random.seed(RANDOM_SEED)
    
    print(f"\n{'='*70}")
    print(f"  VERGINE TOTALE v1.1 — {n} NPC, {n_rounds} rounds")
    print(f"{'='*70}\n")
    
    # Genera popolazione
    profiles = [generate_profile() for _ in range(n)]
    prefs = [generate_preferences(p) for p in profiles]
    state = _initialize_state(n, profiles)
    
    # Inizializza culturalPull dopo round 0
    # (round 0 serve per bootstrap estimatedPopMean)
    
    for round_num in range(n_rounds):
        summary = defaultdict(lambda: {
            'roundVisMatchCount': 0,
            'roundFullMatchCount': 0,
            'receivedVisScores': [],
            'receivedSexScores': [],
        })
        
        # Subsample batch
        batch_indices = np.random.choice(n, size=min(SUBSAMPLE_SIZE, n), replace=False)
        
        # Processa batch
        _process_subbatch(batch_indices, state, summary, profiles, prefs)
        
        # Inizializza culturalPull dopo round 0
        if round_num == 0:
            for i in range(n):
                s = state[i]
                # Base + bonus se match rate alto
                match_rate = s['my_fullMatchCount'] / max(1, len(batch_indices))
                s['culturalPull_coeff'] = (CULTURAL_PULL_INIT_BASE + 
                                          CULTURAL_PULL_INIT_MATCH_BONUS * match_rate)
        
        # Update SelfEsteem (da Vergine v1.0/D)
        _update_selfesteem(state, summary, profiles)
        
        # Print round
        round_matches = sum(sm['roundFullMatchCount'] for sm in summary.values())
        _print_round(round_num, state, n, round_matches)
    
    # Print finale
    _print_final(state, profiles, prefs, n)


def _update_selfesteem(state, summary, profiles):
    """Aggiorna MySelfEsteem sociometro."""
    for i, sm in summary.items():
        s = state[i]
        n_full = sm['roundFullMatchCount']
        n_vis_match = sm['roundVisMatchCount']
        
        # Segnale outcome
        if n_full > 0:
            outcome_signal = SE_SIGNAL_FULL_MATCH
        elif n_vis_match > 0:
            outcome_signal = SE_SIGNAL_VIS_MATCH
        else:
            outcome_signal = SE_SIGNAL_NO_MATCH
        
        # Frustrazione erode anchor
        frust_norm = min(1.0, s['my_visFrustration'] / 3.0)
        anchor_eff = max(0.002, SE_ANCHOR_LR * (1.0 - SE_FRUSTRATION_EROSION * frust_norm))
        
        # Update
        se = s['MySelfEsteem_dynamic']
        se += SE_OUTCOME_LR * (outcome_signal - se)
        se += anchor_eff * (s['MySelfEsteem_static'] - se)
        s['MySelfEsteem_dynamic'] = np.clip(se, 0.05, 0.95)


# ================================================================
# OUTPUT
# ================================================================

def _print_round(round_num, state, n, round_matches):
    vt = np.array([s['my_visThreshold'] for s in state])
    asp = np.array([s['aspiration_coeff'] for s in state])
    cp = np.array([s['culturalPull_coeff'] for s in state])
    ma = np.array([s['marketAwareness_coeff'] for s in state])
    d_mus = np.array([s['myDesired_Muscle'] for s in state])
    
    print(f"  Round {round_num:>2}: visThr={vt.mean():.3f} asp={asp.mean():.3f} "
          f"cP={cp.mean():.3f} mA={ma.mean():.3f} dMus={d_mus.mean():.3f} match={round_matches:>3}")


def _print_final(state, profiles, prefs, n):
    vt = np.array([s['my_visThreshold'] for s in state])
    st = np.array([s['my_sexThreshold'] for s in state])
    asp = np.array([s['aspiration_coeff'] for s in state])
    sm = np.array([s['selfMirror_coeff'] for s in state])
    cp = np.array([s['culturalPull_coeff'] for s in state])
    ma = np.array([s['marketAwareness_coeff'] for s in state])
    fm = np.array([s['my_fullMatchCount'] for s in state])
    vm = np.array([s['my_visMatchCount'] for s in state])
    se = np.array([s['MySelfEsteem_dynamic'] for s in state])
    se0 = np.array([s['MySelfEsteem_static'] for s in state])
    
    print(f"\n{'='*70}")
    print(f"  VERGINE TOTALE v1.1 — RISULTATI FINALI — {n} NPC")
    print(f"{'='*70}")
    
    print(f"\n  COEFFICIENTI:")
    print(f"    aspiration       μ={asp.mean():.3f} σ={asp.std():.3f} range=[{asp.min():.3f}, {asp.max():.3f}]")
    print(f"    selfMirror       μ={sm.mean():.3f} σ={sm.std():.3f}")
    print(f"    culturalPull     μ={cp.mean():.3f} σ={cp.std():.3f}")
    print(f"    marketAwareness  μ={ma.mean():.3f} σ={ma.std():.3f}")
    
    print(f"\n  SOGLIE:")
    print(f"    my_visThreshold  μ={vt.mean():.3f} σ={vt.std():.3f}")
    print(f"    my_sexThreshold  μ={st.mean():.3f} σ={st.std():.3f}")
    
    print(f"\n  MySELFESTEEM:")
    print(f"    statica μ={se0.mean():.3f}  dinamica μ={se.mean():.3f}  "
          f"Δ={( se - se0).mean():+.3f}  r={np.corrcoef(se0, se)[0,1]:+.3f}")
    
    print(f"\n  MATCH CUMULATIVI:")
    print(f"    my_fullMatchCount totali={fm.sum():.0f} (pop_anyMatchRate={100*(fm>0).mean():.0f}%)")
    print(f"    my_visMatchCount  totali={vm.sum():.0f}")
    
    # Convergenza desiderio
    print(f"\n  CONVERGENZA DESIDERIO (myDesired vs v2.3):")
    for var in ['Muscle', 'Beauty', 'BottomTop', 'Age', 'PenisSize']:
        learned = np.array([s[f'myDesired_{var}'] for s in state])
        
        # v2.3 target
        np.random.seed(RANDOM_SEED)
        static_targets = []
        for p in profiles:
            tp = generate_preferences(p)
            static_targets.append(tp.get(f'myDesired_{var}', 0.5))
        static_arr = np.array(static_targets[:n])
        
        r = np.corrcoef(learned, static_arr)[0, 1] if len(static_arr) == n else 0
        print(f"    {var:<14} learned μ={learned.mean():.3f} σ={learned.std():.3f}  "
              f"v2.3 μ={static_arr.mean():.3f}  r={r:+.3f}")
    
    print(f"\n{'='*70}\n")


# ================================================================
# ENTRY POINT
# ================================================================

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--n', type=int, default=N_SUBJECTS)
    parser.add_argument('--rounds', type=int, default=N_ROUNDS)
    args = parser.parse_args()
    
    run_simulation(args.n, args.rounds)
