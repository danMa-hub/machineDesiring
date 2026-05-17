# MACCHINA DEL DESIDERIO — Bibliography

**versione 2.3 — aggiungere fonti con link e risultato chiave usato nel sistema**

---

## Modello di attrazione — Distanza Euclidea

**Conroy-Beam, D., Buss, D.M., Pham, M.N., & Shackelford, T.K. (2015)** *How Sexually Dimorphic Are Human Mate Preferences?* Personality and Social Psychology Bulletin, 41(8). Risultato nel sistema: framework per importanza relativa dei tratti. Link: [https://journals.sagepub.com/doi/10.1177/0146167215590987](https://journals.sagepub.com/doi/10.1177/0146167215590987)

**Conroy-Beam, D. (2016, 2017, 2019, 2022)** Serie di studi sulla distanza Euclidea pesata come modello di mate choice. N=14487, 45 paesi, 8 modelli testati. Risultato nel sistema: la distanza Euclidea pesata è il miglior predittore della scelta reale del partner — adottata come modello core. **NOTA: campione solo eterosessuale.** Il modello matematico è valido universalmente; i pesi specifici nel nostro sistema vengono dalla letteratura gay-specifica (vedi sotto). Link: [https://labs.psych.ucsb.edu/conroy-beam/daniel/](https://labs.psych.ucsb.edu/conroy-beam/daniel/)

**Conroy-Beam, D. (2018)** *Euclidean mate value and power of choice on the mating market.* Risultato nel sistema: MateValue emergente — MV(B) = media delle attrazioni di tutti gli A verso B. Non è un input ma un output della simulazione. Esteso in v2.3 a MV tripartito (visual, sexual, total).

---

## Assortative mating e preferenze

**Luo, S. (2017)** *Assortative mating and couple similarity: Patterns, mechanisms, and consequences.* Social and Personality Psychology Compass, 11(8). Risultato nel sistema: r=0.30-0.40 per tratti fisici tra partner. Base per r_assort in ASSORT_COEFFS.

**Bruch, E.E. & Newman, M.E.J. (2018)** *Aspirational pursuit of mates in online dating markets.* Science Advances. Risultato nel sistema: si punta ~25% sopra di sé. Base per r_aspir in ASSORT_COEFFS. **NOTA: campione solo eterosessuale** — dati gay esclusi dallo studio originale. Il meccanismo aspirazionale è trasferibile; l'entità non è verificata su gay men.

---

## Ruoli sessuali e compatibilità

**Moskowitz, D.A., Rieger, G., & Roloff, M.E. (2008)** *Tops, bottoms, and versatiles.* Archives of Sexual Behavior, 37(3), 397-406. Risultato nel sistema: complementarità forte documentata. Base per BottomTop complementare (r_assort=-0.90). Link: [https://link.springer.com/article/10.1007/s10508-007-9260-x](https://link.springer.com/article/10.1007/s10508-007-9260-x)

**Moskowitz, D.A. & Hart, T.A. (2011)** *The influence of physical body traits and masculinity on anal sex roles in gay and bisexual men.* Archives of Sexual Behavior. Risultato nel sistema: top riportano peni leggermente più grandi. Causalità invertita in v2.1: PenisSize→BT (non BT→PenisSize). NOTA: NON conferma BT×corpo interaction (rimossa).

**Yale Law Journal (2014)** Review su distribuzione ruoli sessuali gay. Risultato nel sistema: B28%/V50%/T22% come distribuzione trimodale.

---

## Corpo e attrattività — gay-specifici

**Swami, V. & Tovée, M.J. (2008)** *The muscular male: A comparison of the physical attractiveness preferences of gay and heterosexual men.* International Journal of Men's Health, 7(1), 59-71. Risultato nel sistema: muscle/body più importante della faccia per gay men vs etero. Base per Muscle=0.28 (il più alto) e Beauty=0.23 in BASE_WEIGHTS_VISUAL. **Gay-specifico: confronto diretto gay vs etero.**

**Levesque, M.J. & Vichesky, D.R. (2006)** *Raising the bar on the body beautiful: An analysis of the body image concerns of homosexual men.* Body Image, 3(1). Risultato nel sistema: beauty meno dominante per gay men rispetto a popolazioni etero. Base per Beauty 0.26→0.23.

**Morrison, M.A., Morrison, T.G., & Sager, C.L. (2004)** *Does body satisfaction differ between gay men and lesbian women and heterosexual men and women?* Body Image, 1(3), 291-299. Risultato nel sistema: gay men hanno maggiore insoddisfazione corporea. Media autostima gay ~0.48. Link: [https://www.sciencedirect.com/science/article/pii/S1740144504000397](https://www.sciencedirect.com/science/article/pii/S1740144504000397)

**Yelland, C., & Tiggemann, M. (2003)** *Muscularity and the gay ideal.* Eating Behaviors, 4(2), 107-116. Risultato nel sistema: muscolosità come standard estetico dominante nella cultura gay. Link: [https://www.sciencedirect.com/science/article/pii/S1471015303000102](https://www.sciencedirect.com/science/article/pii/S1471015303000102)

**Veale, D. et al. (2015)** *Am I normal? A systematic review and construction of nomograms for flaccid and erect penis length and circumference.* BJU International. Risultato nel sistema: media eretto 13.12cm SD 1.66cm. Base per MyPenisSize scala 0-1.

---

## Preferenze cross-orientamento

**Lippa, R.A. (2007)** *The preferred traits of mates in a cross-national study of heterosexual and homosexual men and women.* Archives of Sexual Behavior, 36(2), 193-208. **N=5,938 gay men** su 200K+ totali (BBC survey, 53 nazioni). Risultato nel sistema: i profili di preferenze dei gay men clusterizzano per sesso biologico, non per orientamento sessuale. Gay men valutano l'attrattività fisica come gli etero men. Base per il rapporto visual/sexual 62/38 nel single-pass. Link: [https://pubmed.ncbi.nlm.nih.gov/17380374/](https://pubmed.ncbi.nlm.nih.gov/17380374/)

**Bailey, J.M., Gaulin, S., Agyei, Y., & Gladue, B.A. (1994)** *Effects of gender and sexual orientation on evolutionarily relevant aspects of human mating psychology.* Journal of Personality and Social Psychology. Risultato nel sistema: gay men danno PIÙ importanza all'attrattività fisica del partner rispetto a lesbiche e donne etero. **Gay-specifico.**

**Siever, M.D. (1994)** *Sexual orientation and gender as factors in socioculturally acquired vulnerability to body dissatisfaction and eating disorders.* Journal of Consulting and Clinical Psychology. Risultato nel sistema: conferma che attrattività fisica domina nella cultura gay maschile.

---

## Importanza relativa dei tratti

**Li, N.P., Bailey, J.M., Kenrick, D.T., & Linsenmeier, J.A.W. (2002)** *The necessities and luxuries of mate preferences.* Journal of Personality and Social Psychology, 82(6). Risultato nel sistema: attrattività fisica = "necessità" per uomini. BottomTop complementarità = necessità per gay men. Base per gerarchia pesi e per modulazione TODO-13 (necessità più forte agli estremi). **NOTA: campione solo eterosessuale.** Il framework necessità/lusso è trasferibile; i pesi specifici no.

---

## Preferenze di età

**Antfolk, J. (2017)** *Age Limits: Men's and Women's Youngest and Oldest Considered and Actual Sex Partners.* Evolutionary Psychology, 15(1). N=2655. Risultato nel sistema: forte assortativo + youth bias. Base per Age in ASSORT_COEFFS (r_assort=0.70, r_aspir=0.20). Link: [https://journals.sagepub.com/doi/10.1177/1474704917690401](https://journals.sagepub.com/doi/10.1177/1474704917690401)

**Silverthorne, Z.A. & Quinsey, V.L. (2000)** *Sexual partner age preferences of homosexual and heterosexual men and women.* Archives of Sexual Behavior, 29, 67-76. N=192. Risultato nel sistema: picco attrattività ~28 anni, non varia con età del soggetto. Base per CULTURAL_IDEAL Age=0.122. **Gay-specifico.** Link: [https://link.springer.com/article/10.1023/A:1001886521449](https://link.springer.com/article/10.1023/A:1001886521449)

**Antfolk, J., Salo, B., Alanko, K., et al. (2015)** *Women's and men's sexual preferences and activities with respect to the partner's age.* Evolution and Human Behavior, 36, 73-79. Risultato nel sistema: gap verso partner più giovane cresce progressivamente con l'età.

**Schope, R.D. (2005)** *Who's Afraid of Growing Old? Gay and Lesbian Perceptions of Aging.* Journal of Gerontological Social Work. Risultato nel sistema: ageismo documentato nella cultura gay. Base per peso Age=0.07 e beauty decay ×0.20. **Gay-specifico.**

---

## Mascolinità e preferenze

**Bailey, J.M., Kim, P.Y., Hills, A., & Linsenmeier, J.A.W. (1997)** *Butch, femme, or straight acting? Partner preferences of gay men and lesbians.* Journal of Personality and Social Psychology, 73(5), 960-973. Risultato nel sistema: gay men preferiscono partner simili in mascolinità (masc4masc). r_assort=0.45. Noise peso Masc 40% per catturare polarizzazione. **Gay-specifico.** Link: [https://psycnet.apa.org/record/1997-04633-005](https://psycnet.apa.org/record/1997-04633-005)

---

## Pelo corporeo

**Martins, Y., Tiggemann, M., & Churchett, L. (2008)** *Hair today: The role of body hair in the formation and maintenance of gay men's identities.* Body Image, 5(4), 323-331. Risultato nel sistema: pelo corporeo come marcatore identitario subculturale (Bear), non dimensione universale di valutazione. Peso Hair=0.05 (non 0.08 — segregava troppo). Noise 40% per catturare bipolarità. **Gay-specifico.** Link: [https://www.sciencedirect.com/science/article/pii/S1740144508000776](https://www.sciencedirect.com/science/article/pii/S1740144508000776)

**Muscarella, F. & Cunningham, M.R. (1996)** *The evolutionary significance and social perception of male pattern baldness and facial hair.* Ethology and Sociobiology, 17(2). Risultato nel sistema: self-similarity per pelo debole. Base per r_assort Hair 0.35→0.15.

---

## Altezza

**Valentova, J.V. et al. (2014)** *Preferred and actual relative height among homosexual male partners vary with preferred dominance and sex role.* PLoS ONE. Risultato nel sistema: altezza come dimensione di mate evaluation nei gay. Peso Height=0.04. **Gay-specifico.**

---

## Bias etnici nelle preferenze

**OkCupid (2009)** *How Your Race Affects The Messages You Get — Same-Sex Data.* Blog post con dati interni. Risultato nel sistema: response rate gay-specifici per etnia: White=45, Black=36, MiddleEastern=48 per 100 messaggi. Base per ETH_DESIRABILITY moltiplicatori. **Gay-specifico.** Link: [https://www.gwern.net/docs/psychology/okcupid/howyourraceaffectsthemessagesyouget.html](https://www.gwern.net/docs/psychology/okcupid/howyourraceaffectsthemessagesyouget.html)

**Callander, D., Newman, C.E., & Holt, M. (2015)** *Is Sexual Racism Really Racism?* Archives of Sexual Behavior, 44(7), 1991-2000. Risultato nel sistema: varianza individuale significativa nel bias etnico. Base per ObserverEthBias SD=0.40. Link: [https://link.springer.com/article/10.1007/s10508-015-0487-9](https://link.springer.com/article/10.1007/s10508-015-0487-9)

---

## Sierostatus e stigma

**Survey 2019 / GLAAD (2022)** *State of HIV Stigma Report.* Risultato nel sistema: 64% uncomfortable con HIV+ anche in trattamento. Base per POZ_DESIRABILITY: Poz=0.36, PozUndet=0.70.

**CDC (2022)** HIV prevalence data MSM USA. Risultato nel sistema: ~16-17% MSM HIV+, ~85-90% in trattamento soppressivo. Base per PozStatus formula age-dependent (v2.3 fix).

**Smit, P.J., Brady, M., Carter, M., et al. (2012)** *HIV-related stigma within communities of gay men.* Social Science & Medicine, 74(2), 181-188. Risultato nel sistema: sierofobia documentata. Base per ObserverPozBias SD=0.50. Link: [https://www.sciencedirect.com/science/article/pii/S0277953611006629](https://www.sciencedirect.com/science/article/pii/S0277953611006629)

---

## Dimensioni del pene

**GMFA (2017)** *The Gay Men's Sex Survey.* N=566. Risultato nel sistema: 22% ha rifiutato un partner per dimensione del pene (dominato da bottom). Base per PenisSize nel match sessuale e per modulazione peso PenisSize×bottomness (v2.3). **Gay-specifico.**

**Gay Men's Health Project** *Survey on body image and sexual health.* Risultato nel sistema: 38% gay men riporta ansia su dimensioni. Base per peso PenisSize=0.15.

---

## Self-esteem

**Bale, C. & Archer, J. (2013)** *Self-perceived attractiveness, romantic desirability and self-esteem: A mating sociometer perspective.* Evolutionary Psychology. Risultato nel sistema: predittori dell'autostima come sociometro del valore di accoppiamento. Base per MySelfEsteem derivata.

**Morrison, M.A. et al. (2004)** e **Peplau, L.A. et al. (2009)** Risultato nel sistema: gay men media autostima ~0.48 (RSES normalizzato) vs popolazione generale ~0.55. Base per MySelfEsteem target 0.48 e base costante 0.21.

**Grov, C. et al. (2010)** Risultato nel sistema: benessere psicosociale correlato a dimensione del pene in MSM. Base per PenisSize×0.12 in MySelfEsteem (v2.1).

**Lever, J. et al. (2006)** N=52,031. Risultato nel sistema: uomini con pene sopra la media valutano il proprio aspetto più favorevolmente. Supporta PenisSize in MySelfEsteem.

---

## Subculture e identità

**Moskowitz, D.A. et al. (2013)** *Bears, body image, and health.* Risultato nel sistema: Bears shorter, heavier, hairier. Base per tribe prototypes. **Gay-specifico.**

---

## Kink e pratiche sessuali

**Parsons, J.T. et al. (2010)** *Sexual practices of HIV-seropositive men who have sex with men.* N=1214 MSM. Risultato nel sistema: group sex 60.6%, exhib/voy 39.8%, watersports 32.8%, bondage 29.8%, fisting 20.9%, S/M 20.7%, breath play 8.1%. Base per KINK_TYPES_DEF probabilità e per floor Vanilla=0.03. **Gay-specifico.**

**Dean, T. (2009)** *Unlimited Intimacy: Reflections on the Subculture of Barebacking.* University of Chicago Press. Risultato nel sistema: breeding come eroticizzazione dell'eiaculazione interna, specifica cultura gay barebacke. Base per Breeding KinkType condizionato a BT e bareback. **Gay-specifico.**

**Holvoet, L. et al. (2017)** N=256 BDSM practitioners. Risultato nel sistema: 50% Sub, 27% Dom, 23% Switch. Top/Bottom e Dom/Sub sono dimensioni indipendenti. Base per correlazione BT→SubDom ridotta a ×0.10.

---

## Non-monogamia

**Hosking, W. (2013)** *Australian gay men's satisfaction with sexual agreements.* Archives of Sexual Behavior, 42(6), 1077-1086. Risultato nel sistema: >50% gay non-monogami. Supporta MyOpenClosed media 0.55. **Gay-specifico.** Link: [https://link.springer.com/article/10.1007/s10508-012-9975-6](https://link.springer.com/article/10.1007/s10508-012-9975-6)

---

## Invecchiamento e decadimento fisico

**Doherty, T.J. (2003)** *Invited review: aging and sarcopenia.* Journal of Applied Physiology. Risultato nel sistema: 3-8%/decade perdita massa muscolare dopo 30, accelera dopo 60. Base per MyMuscle piecewise age penalty (v2.1).

**Rhodes, G. (2006)** Meta-analisi attrattività facciale. Risultato nel sistema: forte age effect sulla bellezza percepita. Base per beauty decay ×0.20 (v2.3).

---

## Note metodologiche

Quando si aggiunge una fonte al sistema:

1. Aggiungere qui con autori, anno, titolo, link, risultato chiave
2. Nel codice Python citare solo: `# Fonte: Autore et al. (anno)`
3. In open_questions.md citare come: `Autore et al. (anno) — descrizione`
4. **Marcare esplicitamente se gay-specifico o eterosessuale**
