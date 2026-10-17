# Micro-timing

Type: task
Status: resolved
Assignee: c0remusic (session du 2026-10-13)
Blocked by: 13, 15, 18

## Question

Le micro-timing dans le device, comme « Sémantique du Pattern » l'a fixé : −23 à +23 en 1/24 de pas (384 par ronde,
comme Elektron), tout le groupe de ratchets décalé ; le tirage un pas d'avance que −23/24 impose (ticket 08, round 1) ;
la flèche de la v4 dans la case, la glissière du panneau du pas.

## Comments

- 2026-10-13 : construit.
  - Noyau : un pas tient `micro` (−23 à +23), qui demande un Hit ; retirer le Hit le remet à 0. Un 24e de pas vaut 35
    sous-ticks (840 par pas, ticket 18), soit 5 ticks de Live : +6 décale le Hit et son note-off de 30 ticks. Les
    ratchets se décalent ensemble. Le pas n+1 est planifié au début du pas n : son Hit le plus en avance (−23) tombe
    1/24 de pas après, donc jamais avant d'être planifié.
  - Un Hit en retard que le pas suivant rattrape en avance (même Track) est coupé par lui, comme dans un clip :
    note-off puis note-on au même tick, et le note-off d'origine n'est plus envoyé.
  - Départ, saut : un pas que la position atteint (sa grille à la position ou après) envoie aussitôt les Hits que son
    micro-timing plaçait avant elle. Le premier pas d'un départ joue donc dès le départ, même en avance, puis en avance
    aux Passes suivants. Un pas commencé avant la position (saut au milieu) perd ses ratchets d'avant elle, comme au
    ticket 18.
  - Édition du pas planifié, tranchée ici : avec le micro-timing, les Tracks d'un même pas commencent à des instants
    différents, et la règle « pas planifié et pas encore commencé » du ticket 08 devient par Track. Une Track qui a
    déjà commencé à jouer le pas planifié garde ce qu'elle joue (ni rejoué, ni en retard) et donne à NEI et PRE le
    résultat qu'elle joue ; les autres sont replanifiées depuis la mémoire d'avant le pas, comme avant. Un Hit qu'une
    édition place avant la position part aussitôt plutôt qu'un Pass plus tard. Deux mutations vérifiées : sans la
    détection par Track, une Track rejoue son Hit ; en réévaluant la Condition d'une Track gardée, NEI diverge.
  - Charge : le Pattern le plus lourd du ticket 18, chaque pas avec un micro-timing, mesuré au mieux de trois passages
    (un passage juste après un build a vu un wake de 3,1 ms, ses voisins 0,3 ms) : départ 0,2 ms, wake le plus long
    0,1-0,15 ms, replanification 0,04 ms ; sans allocation. Le test garde le seuil de 1 ms, en Release.
  - Case : flèche couleur du Hit, 3 px de long et 8 de haut, à mi-hauteur dans la marge gauche (avance) ou droite
    (retard), dessinée après les cadres ; `paintpoly` (paires x/y, couleur du `frgb` courant, d'après la référence de
    `lcd` livrée avec Live). Panneau : glissière « Micro-timing » −23 à +23 sans valeur initiale, et sous elle
    « 8/24 of a step early », « 12/24 of a step late » ou « On the grid ».
- À vérifier dans Live : la flèche (`paintpoly`), la glissière, et à l'oreille un Hit en avance et un en retard contre
  un clip sur la grille. Fait le 2026-10-13 : attaques mesurées (plus bas), flèches vues dans la fenêtre d'édition
  (ticket 15).
- 2026-10-13 : mesuré dans Live 12.2.1 à 126 BPM, sur une piste « MDSEQ TEST » du set de l'utilisateur (MD Drums
  construit le 2026-10-13 à 17:05, après cf3385cd et 812551fa ; build de dev `mdseq_dbg9` puis `mdseq_dbg10` sur le
  Pattern de `measure/steps.py`), sortie de la piste enregistrée par `live-record-track.mjs`, analysée par `steps.py`.
  Attaques d'un Hit à +12/24 et d'un à −12/24 contre le Hit sur la grille du même
  enregistrement : +0,4998 à +0,5002 et −0,5002 à −0,4998 pas, à un échantillon près (un pas = 5250 échantillons),
  comme les Hits sur la grille entre eux.
