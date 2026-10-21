# MoDule Seq v7 : spécification

La v6 (`v6-ui-spec.md`) retouchée le 2026-10-20 à la demande de l'utilisateur : « les polices ne sont pas les mêmes
partout », « les sous-textes semblent un peu flotter, on peut leur donner un écran ? », et le device renommé MoDule
Seq. Rien n'est construit : la v5 reste la version approuvée.

## Écrans

Un texte qui dit un état est dans un écran : un `panel` #141a1e bordé de #0b0e10, rayon 2 px, sous un `live.comment`
en Ableton Sans Medium 10 px, #a8cfe3, à 6 px du bord gauche, centré en hauteur. Le nom d'un contrôle reste une
étiquette nue au-dessus de lui.

| Écran | Boîte (x, y, l, h) | Texte |
|---|---|---|
| Bande : état du Pattern | 8, 132, 148 × 30 | deux lignes (« 02 next, after step 48 », « 48 steps, 1/16 ») |
| Bande : nouvelle version | 172, 144, 636 × 18 | #ffa52e ; absent sans nouvelle version |
| Bande : chaîne | 828, 62, 152 × 18 | « Chain 2/3: 01 03 02 » ; vide sans chaîne |
| Bande : prise | 828, 126, 152 × 18 | « Take 12 bars », « No take » |
| Fenêtre : pas montré | 192, 592, 148 × 20 | « Step 47, page 3 », à droite du titre de la Track |
| Fenêtre : vélocité | 144, 654, 64 × 18 | la valeur, #dadada |
| Fenêtre : micro-timing | 172, 734, 168 × 18 | « +6 of 23, late » |
| Fenêtre : courbe des ratchets | 172, 798, 168 × 18 | « -50 %, falling over the 3 » |
| Fenêtre : valeur d'un Lock | x du bouton + 48, y + 18, 60 × 18 | un Lock #7cc6ff gras ; la valeur du Kit #7f97a3 italique (« Kit 40 ») |
| Fenêtre : ligne d'état de Print | 16, 846, 1264 × 18 | le résultat de Print |

## Bande : colonne de droite (x 828 à 980)

| Élément | Boîte (x, y, l, h) |
|---|---|
| Étiquette Play | 828, 6, 152 × 12 |
| FILL, Chain | 828 et 908, 22, 72 × 36 |
| Écran de la chaîne | 828, 62, 152 × 18 |
| Étiquette Output | 828, 86, 152 × 12 |
| Clips only, Capture | 828 et 908, 102, 72 × 20 |
| Écran de la prise | 828, 126, 152 × 18 |
| Edit, Push | 828 et 908, 148, 72 × 16 |

## Fenêtre

- Titre du panneau du pas : « Track 01 TRX-BD », 16, 594, en gras 13 px ; l'écran du pas à sa droite.
- Noms des Tracks des rangées : 11 px pour toutes ; celui de la Track montrée en gras, #dadada, les autres #a2a2a2.
- Nom et valeur d'un Lock : le nom à x + 48, y + 2 ; l'écran de la valeur dessous.
- Pied : filet à y 840, écran de Print, « Check for updates » à 1296, 847, 128 × 16.

Le reste comme la v6.

## Nom

Le device s'appelle **MoDule Seq** : le titre de la bande (le nom du fichier `.amxd`) et celui de la fenêtre (« MoDule
Seq: Edit »).
