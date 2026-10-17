# md-drums-seq v1 : spec d'UI

Proposition v1 pour le ticket « Direction visuelle du device ». Une seule géométrie, deux peaux en variantes
(`v1-var-family.html` : A natif Live, B famille v22 de MD Drums, C bande native et fenêtre v22) ; `v1-test.html`
montre A. Contrôles et identifiants : `parameter-spec.md`.

## Bande (dans la chaîne de devices, pour jouer)

Zone de présentation 992 × 169 px sous la barre de titre de Live ; tout sur une grille de 4 px.

| Bloc | Boîte (x, y, l, h) | Contenu |
|---|---|---|
| Pattern | 8, 6, 140, 156 | étiquette ; 16 boutons 32 × 24 en 4 × 4, pas de 36 × 28 (`pattern-1` …) ; le joué plein, le suivant en file clignote ; une ligne d'état « 02 next, after step 64 » et « 64 steps, 1/16 » |
| Filet | 160, 8, 1, 153 | |
| Tracks | 172, 22, 636, 24 | 16 boutons 36 × 24 au pas de 40 (`track-*`) : numéro, nom court de la Machine |
| Mutes | 172, 50, 636, 16 | 16 bascules 36 × 16 (`mute-*`), sous leur Track |
| Pas | 172, 74, 636, 40 | 16 cases 36 × 40 (`step-*`) : la page montrée de la Track choisie ; cadre accent sur le pas joué ; badge de Condition, point de Lock |
| Pages | 172, 122, 636, 16 | 4 boutons 156 × 16 au pas de 160 (`page-*`) |
| Aide | 172, 146, 636, 12 | Track et page montrées |
| Filet | 816, 8, 1, 153 | |
| Fill | 828, 22, 72, 64 | momentané (`fill`) |
| Edit | 908, 22, 72, 24 | ouvre la fenêtre (`edit`) |
| Capture | 908, 52, 72, 34 | étiquette et état (`capture`) : Session, ou Arrangement quand Live enregistre |

## Fenêtre d'édition

1440 × 840 px, ouverte par Edit, fermée par la croix de la fenêtre.

| Bloc | Boîte (x, y, l, h) | Contenu |
|---|---|---|
| Barre | 0, 0, 1440, 52 | Patterns 28 × 24 au pas de 32 depuis x 96 (`win-pattern-*`) ; Length (`length`), Scale (`scale`), FILL (`win-fill`), « Print 4 passes » (`print`), état de capture (`win-capture`) |
| Numéros de pas | 176, 66, …, 12 | un tous les 4 pas |
| Tracks | 16, 88, 144, 444 | rangées au pas de 28 : numéro, Machine, mute 24 × 24 (`win-mute-*`) |
| Grille | 176, 88, 1228, 444 | 64 pas : cases 17 × 24 au pas de 19, 4 px de plus toutes les 16 (`cell-<t>-<s>`) ; ou 32 pas (33-64 ici) : cases 36 × 24 au pas de 38, avec badges de Condition ; colonne du pas joué teintée |
| Filet | 0, 548, 1440, 1 | |
| Pas sélectionné | 16, 564, 324, 256 | titre Track et Machine, « Step 47, page 3 » ; Hit (`step-hit`), vélocité (`step-velocity`), Condition (`step-condition`), micro-timing (`step-microtiming`), ratchets et leur vitesse (`step-ratchets`, `step-ratchet-rate`) |
| Locks | 368, 568, 1056, 248 | trois rangées de 8 au pas de 88 : SYN (noms de la Machine), EFX, ROUTING ; boutons 40 × 40 au pas de 120 (`lock-1` … `lock-24`) |

## Une case de pas

- Hit : bloc plein, opacité 35 % + 65 % × vélocité / 127 ;
- micro-timing : le bloc glisse jusqu'à 30 % de la case (−23 à +23) ;
- ratchets : un trait vertical par subdivision ;
- Locks : un point accent en haut à droite ;
- Condition : barre de 2 px à 64 pas, badge texte à 32 pas ;
- pas joué : cadre accent ; pas sélectionné : contour accent de 2 px.

## Un bouton de Lock

Arc de fond, arc fin à la valeur du Kit (épais quand pas de Lock), arc accent à la valeur du Lock, aiguille à la valeur
qui joue ; dessous le nom (noms de la Machine pour SYN1-8, « unused » si la Machine n'en a pas) et « 52 (kit 40) » en
accent, ou la valeur du Kit en discret.

## Construction dans Max

Tout est faisable en natif : `live.text` (boutons, bascules, momentanés), `live.tab` (Patterns, pages, Tracks),
`live.menu`, `live.dial`, `live.slider`, `panel`, `comment` ; la grille en `matrixctrl` ou `live.grid` (pas
d'opacité par case : la vélocité devra se montrer autrement, à trancher dans « Maquettes de l'UI ») ; la fenêtre par
`pcontrol` sur un sous-patch en présentation. Pas de `jsui` ni de `v8ui` : aucun JS dans le device (ADR 0001).
