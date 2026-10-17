# Ratchets

Type: task
Status: resolved
Assignee: c0remusic (session du 2026-10-13)
Blocked by: 13, 15

## Question

Les ratchets dans le device, comme « Sémantique du Pattern » les a fixés : 1 à 8 Hits répartis également dans le pas,
une courbe de vélocité de −100 % à +100 %, la Condition du pas qui décide pour tout le groupe (un tirage), chaque Hit
avec ses Locks, une tranche de pas de long ; les crans dans la case (v3/v4) et le panneau du pas.

## Comments

- 2026-10-13 : construit.
  - Noyau : un pas tient `ratchets` (1-8) et `curve` (−100 à +100), qui demandent un Hit ; retirer le Hit les remet
    à 1 et 0. Le ratchet r de N commence à r/N du pas et finit où commence le suivant, note-off avant note-on ; chacun
    est le Hit suivi de ses Locks (ADR 0003 amendée de MD Drums). Un départ ou un saut au milieu du pas joue les
    ratchets à partir de là.
  - Courbe, tranchée ici (la sémantique disait « descente ou montée sur les N ») : le ratchet le plus fort garde la
    vélocité du pas, le premier en descente (courbe < 0), le dernier en montée (courbe > 0) ; celui de l'autre bout
    perd |courbe| % de cette vélocité, ceux du milieu en proportion, arrondi, jamais sous 1. Ainsi la case, teintée par
    la vélocité du pas, montre le plus fort, et +100 % ne sature pas à 127 comme le ferait un décalage ajouté.
    Exemple : vélocité 100, courbe −50 %, 3 ratchets : 100, 75, 50.
  - Temps : le noyau place ses événements en sous-ticks, 840 par pas (ppcm de 1 à 8, et 24 × 35 pour le micro-timing
    à venir) : 7 ratchets tombent sur des entiers, les comparaisons sont exactes, et les ticks de Live n'en sont que la
    conversion.
  - Charge, mesurée dans `mdseqSequencerTest` (Release, ce PC) sur le Pattern le plus lourd, 16 Tracks × 16 pas de
    8 ratchets à 24 Locks (6400 événements par pas) : insérer chaque événement trié dans la file prenait 9,6 ms au
    wake le plus long et 4,8 ms pour replanifier un pas, plus que le vecteur de 1,45 ms du fil audio de Live ; un
    tri par pas, 1,2 ms au départ du transport (deux pas) et 0,45 ms par pas, dont 0,38 de tri. Le pas est maintenant
    produit dans l'ordre, sous-tick par sous-tick (note-offs puis notes, Track par Track), puis fusionné en une passe :
    0,2 ms au départ, 0,1 ms au wake le plus long, 0,07 ms pour replanifier. Le test exige moins de 1 ms, sans
    allocation.
  - Case : un cran de 2 × 4 px couleur du fond par ratchet (à partir de 2), dans le bord haut du Hit, au milieu de sa
    part, aux positions de `v4-ui.js` (arrondi de `Math.round`). Panneau : menu « Ratchets » x1-x8 et glissière
    « Ratchet velocity » −100 à +100, sans valeur initiale (contrôles d'édition, `check_device`), et sous elle le texte
    « −50 %, falling over the 3 » de la maquette.
  - Trouvé en passant : l'adaptateur ne transmettait au Sequencer que 4 champs d'une édition, sans `param` ; un Lock
    posé au panneau jouait donc sur SYN1 (paramètre 0) pendant que l'interface le montrait sur le bon. Une édition passe
    maintenant par `pack`/`unpack` du noyau, et `mdseqEditorTest` nourrit son Sequencer par les mêmes mots (le test
    casse si `pack` oublie `param`).
- À vérifier dans Live : les crans dans la bande et la grille, le menu et la glissière, et à l'oreille un pas de 4
  ratchets en descente sur MD Drums. Fait le 2026-10-13 : niveaux mesurés (plus bas), crans vus dans la fenêtre
  d'édition (ticket 15).
- 2026-10-13 : mesuré dans Live 12.2.1 à 126 BPM, sur une piste « MDSEQ TEST » du set de l'utilisateur (MD Drums
  construit le 2026-10-13 à 17:05, après cf3385cd et 812551fa ; build de dev `mdseq_dbg9` puis `mdseq_dbg10` sur le
  Pattern de `measure/steps.py`), sortie de la piste enregistrée par `live-record-track.mjs`, analysée par `steps.py`. 4
  ratchets en descente de 75 % (vélocités 120, 90, 60, 30) : −12,2, −14,6, −18,1 à
  −18,5, −23,6 à −24,6 dB, soit −2,4, −5,9 à −6,3 et −11,4 à −12,4 dB contre −2,5, −6,0 et −12,0 attendus si la
  vélocité règle le niveau linéairement ; 3 en montée de 100 % (1, 60, 120) : −52,5, −17,9 à −19,3, −12,7 dB. Chaque
  ratchet dans sa tranche de pas.
