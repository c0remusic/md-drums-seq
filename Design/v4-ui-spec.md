# md-drums-seq v4 : spec d'UI

v3 (`v3-ui-spec.md`), avec le Hit toujours centré : sur la v3, l'utilisateur a pris le décalage du micro-timing (le
bloc glissé dans sa case) pour une case mal centrée.

## Micro-timing dans une case (lcd)

- Le Hit reste à sa place, à 3 px des bords de la case, quel que soit son micro-timing.
- Un micro-timing non nul dessine une flèche de la couleur du Hit (même teinte de vélocité) qui sort de son bord dans la
  marge de la case : à gauche en avance (micro < 0), à droite en retard (micro > 0) ; triangle de 3 px de long et 8 px
  de haut, à mi-hauteur (`paintpoly`). Sa taille ne dit pas la valeur, que le panneau du pas donne (−23 à +23).
- Elle est dessinée après les cadres du pas joué et du pas sélectionné, pour rester visible sur eux.
- La légende « Lire une case » montre −8 (en avance) et +12 (en retard).

Le reste ne change pas depuis v3 : crans des ratchets sur le bord haut du Hit, point bleu cerclé des Locks en haut à
droite, bande violette et texte de la Condition en bas.
