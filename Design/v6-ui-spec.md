# md-drums-seq v6 : spécification

Proposition du 2026-10-20, après la critique de l'interface telle que Live la montrait ce jour-là
(`v6-before-strip.png`, `v6-before-edit.png`). Rien n'est construit : la v5 reste la version approuvée
(`HANDOFF.md`). Seuls changent les placements ci-dessous ; le reste de la v5 tient.

## Bande : colonne de droite (x 828 à 980)

| Élément | Boîte (x, y, l, h) | Objet Max |
|---|---|---|
| Étiquette Play | 828, 6, 152, 12 | `live.comment` |
| FILL | 828, 22, 72 × 40 | `live.text` momentané, paramètre Live |
| Chain | 908, 22, 72 × 40 | `live.text` bascule, couleur de mode |
| Ligne de la chaîne | 828, 66, 152 × 12 | `live.comment` (« Chain 2/3: 01 03 02 ») |
| Étiquette Output, longueur de la prise | 828, 88, 72 × 12 ; 908, 88, 72 × 12, alignée à droite | `live.comment` (« Take 12 bars », « No take ») |
| Clips only | 828, 104, 72 × 20 | `live.text` bascule, paramètre Live automatisable « Output », couleur de mode |
| Capture | 908, 104, 72 × 20 | `live.text` bouton, actif quand la prise a une note |
| Filet | 828, 132, 152 × 1 | `panel` |
| Edit | 828, 141, 72 × 20 | `live.text` bouton |
| Push | 908, 141, 72 × 20 | `live.text` bascule, paramètre caché, couleur de mode |

Pages : une page au-delà de la longueur du Pattern est éteinte (fond #232323, texte #555) et ne répond pas.

## Fenêtre : barre du haut (y 16, h 24)

| Groupe | Éléments |
|---|---|
| Patterns | 01-16 comme en v5 (96 + 32 × i) |
| Filet | 616 |
| Pattern | Copy to 628 (64) ; Length, étiquette 704, menu 748 (52) ; ×2 804 (28) ; Scale, étiquette 844, menu 880 (60) |
| Filet | 952 |
| Vue | 1-32 964 (56) ; 33-64 1024 (56) ; Follow 1084 (56), couleur de mode |
| Filet | 1152 |
| Jeu | FILL 1164 (56) |
| Filet | 1232 |
| Print | menu de source 1244 (120) : « Pattern ×1 » à « Pattern ×16 », puis « Last take (12 bars) » ; bouton Print 1368 (56) |

La case « Capture : Session » de la v5 disparaît. Choisir dans le menu ne lance rien ; le bouton imprime.

## Fenêtre : grille

- Pas au-delà de la longueur : puits #161616, un Hit gardé à peine indiqué (#161616 mélangé à 18 % de l'orange), sans
  détail ; numéros de pas #4f4f4f.
- Fin du Pattern : un trait #a2a2a2 de 2 px dans l'écart après le dernier pas, sur toute la hauteur de la grille.
- Rangée des trigs : le menu de la rangée (Swing, Accent, Slide) à 16 (60 × 24), son réglage à 80 (72 × 24), comme
  construit.
- Fill edit (16, 60, 72 × 20) et Clear (96, 60, 64 × 20), comme construits ; Fill edit prend la couleur de mode.

## Fenêtre : pied

| Élément | Boîte (x, y, l, h) | Objet Max |
|---|---|---|
| Filet | 0, 844, 1440 × 1 | `panel` |
| Ligne d'état de Print | 16, 852, 1100 × 12 | `live.comment` |
| Check for updates | 1296, 848, 128 × 16 | `live.text` bascule, couleur de réglage |

Le bouton « Check for updates » quitte le panneau du pas.
