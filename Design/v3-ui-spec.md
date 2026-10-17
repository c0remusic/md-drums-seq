# md-drums-seq v3 : spec d'UI

v2 (`v2-ui-spec.md`), avec une case plus lisible : sur la v2, l'utilisateur prenait les traits des ratchets pour des
Conditions, parce qu'ils traversaient le texte de la Condition sur les pas qui portaient les deux.

## Une case (lcd)

- **Ratchets** : un cran par Hit, 2 × 4 px couleur du fond, découpé dans le bord haut du bloc au milieu de sa part du
  pas (3 ratchets = 3 crans). Rien en travers du bloc.
- **Condition** : une bande de 3 px violette dans la marge basse de la case, sous le bloc, et son texte en gras 10 px
  au-dessus (ligne de base à 6 px du bas). Une Condition se repère avant d'être lue.
- Le reste comme v2 : Hit teinté par la vélocité, décalé par le micro-timing ; point bleu cerclé pour les Locks ; cadre
  du pas joué et du pas sélectionné.

## Légende

`v3-test.html` ouvre sur « Lire une case » : une case d'exemple par élément (vélocité 127 et 40, Locks, Condition 1:2
et 67 %, 3 ratchets, micro-timing +12, tout ensemble, pas joué, pas sélectionné), dessinées comme la grille. C'est une
aide de la maquette, pas un élément du device.

Le reste (bande, fenêtre, panneau du pas, Locks sans valeur du Kit) ne change pas depuis v2.
