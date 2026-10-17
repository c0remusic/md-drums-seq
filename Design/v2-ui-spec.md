# md-drums-seq v2 : spec d'UI

v1 (`v1-ui-spec.md`) avec les décisions prises depuis : famille A, look natif de Live (« Direction visuelle du
device ») ; 32 pas visibles ; pas de JS (ADR 0001) ; pas de valeur du Kit lisible (« Paramètres de MD Drums visibles
par LOM ») ; ratchets en nombre et courbe de vélocité (« Sémantique du Pattern »). Contrôles et identifiants :
`parameter-spec.md`. Ce qui change par rapport à v1 :

## Ce que dessine un lcd

Les pas de la bande (172, 74, 636, 40) et la grille de la fenêtre (176, 88, 1220, 444) sont des objets `lcd` : le
patch y envoie des primitives (`paintrect`, `framerect`, `paintoval`, `linesegment`, `write`) avec des couleurs opaques
(`frgb`) qu'il mélange lui-même, puisque `lcd` n'a pas de transparence. Un clic y donne les coordonnées ; le patch en
déduit Track et pas.

Une case (36 × 24 dans la grille, 36 × 40 dans la bande) :

- fond : puits, ou puits des temps tous les 4 pas ; mélangé à 18 % d'accent sur le pas joué ;
- Hit : rectangle plein à 3 px des bords, orange mélangé au puits selon la vélocité (35 % + 65 % × vélocité / 127) ;
  décalé jusqu'à 30 % de la case par le micro-timing ;
- ratchets : un trait de 1 px couleur du fond par subdivision ;
- Locks : un rond accent de 5 px, cerclé de 1 px de puits, en haut à droite ;
- Condition : son texte en gras 10 px, couleur du puits, centré en bas ;
- pas joué : cadre accent de 1 px ; pas sélectionné : cadre accent de 2 px.

Redessin : la colonne du pas joué à chaque pas (16 cases), une case à chaque édition, tout au changement de Pattern ou
de moitié.

## Fenêtre

- Barre : `1-32` et `33-64` (892 et 952, 16, 56 × 24) choisissent la moitié montrée ; `Follow` (1012) la fait suivre
  la lecture ; FILL, Print et l'état de capture se décalent à 1084, 1164 et 1308.
- Grille : 32 pas, cases 36 × 24 au pas de 38, 4 px de plus entre les deux pages de 16, toujours 16 rangées au pas de 28.
- Pas sélectionné : le menu de vitesse des ratchets disparaît ; « Ratchet velocity » (172, 744, 168, 24) est une
  glissière centrée de −100 % à +100 %.
- Locks : un bouton sans Lock est inactif et lit « Kit » (le device ignore la valeur du Kit) ; le tourner crée le Lock
  (départ : le dernier Lock de ce paramètre sur la Track, sinon 64), Alt-clic le retire ; un Lock se lit en accent.
  Les valeurs sautent 63. Une ligne d'aide sous les rangées dit d'où viennent les noms SYN1-8.

## Bande

Inchangée, sauf ses 16 pas, dessinés par un `lcd` comme la grille.

## Construction dans Max

`live.text`, `live.tab`, `live.menu`, `live.dial` (avec `active` à 0 pour un paramètre sans Lock), `live.slider`,
`live.numbox`, `panel`, `comment`, deux `lcd` (bande et grille), la fenêtre par `pcontrol`. Pas de `jsui` ni de
`v8ui`. Les couleurs du `lcd` sont fixées par le patch : thème Dark approché, à revoir si Live passe en thème clair.
