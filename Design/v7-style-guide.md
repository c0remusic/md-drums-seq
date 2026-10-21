# MoDule Seq v7 : guide de style

`v6-style-guide.md`, avec :

## Typographie

Une seule famille : **Ableton Sans**, que le Max de Live fournit (`Resources/Max/resources/fonts`). Chaque objet en
reçoit le nom explicitement (`fontname`), plutôt que de dépendre des valeurs par défaut, et le `lcd` aussi (`font`),
qui dessinait en Arial.

| Rôle | Face | Taille | Couleur |
|---|---|---|---|
| étiquette d'un contrôle ou d'un groupe | Medium | 10 px | #a2a2a2 |
| texte d'un bouton, d'un menu, d'un onglet | Medium | 10 px | thème de Live |
| texte d'un écran | Medium | 10 px | #a8cfe3 (une valeur : #dadada) |
| nom d'une Track dans les rangées | Medium, Bold pour la Track montrée | 11 px | #a2a2a2, #dadada |
| titre (panneau du pas), FILL | Bold | 13 px | #dadada |
| texte du lcd (Conditions) | Bold | 10 px | #1e1e1e |

Pas d'autre taille. La v5 utilisait 9, 10, 11, 12 et 16 px, l'Arial gras dans le lcd et dans les défauts du patch.

## Écrans

| Rôle | Valeur |
|---|---|
| fond | #141a1e |
| bord | #0b0e10, 1 px, rayon 2 px |
| texte | #a8cfe3 ; une valeur #dadada ; un Lock #7cc6ff gras ; la valeur du Kit #7f97a3 italique ; une nouvelle version #ffa52e |
| marge | 6 px à gauche ; une valeur (vélocité, Lock) centrée en largeur, sans marge |
| hauteur | la hauteur des capitales et des chiffres (8 px à 10 px) centrée : première ligne de base à (h − bloc) / 2 + 8 du haut de l'écran, bloc = 12 px par ligne au-delà de la première + 8 ; 13 px dans un écran de 18 px, 14 dans un de 20 |

Mesuré dans la maquette le 2026-10-20, après la remarque de l'utilisateur (« les valeurs ne semblent pas centrées au
milieu de leur case écran ») : avant, 3 px au-dessus des chiffres pour 5 dessous, et les valeurs collées à gauche ;
après, autant au-dessus qu'en dessous dans les 33 écrans, autant à gauche qu'à droite pour les valeurs. Dans Max, le
texte d'un `live.comment` commence 4 px sous le haut de sa boîte (`HANDOFF.md`) : la boîte se place pour que sa ligne de
base tombe à la hauteur ci-dessus, à mesurer dans Live à la construction.

À vérifier dans Live : que le `lcd` accepte « Ableton Sans Bold » par son message `font`, et le nom exact des faces
qu'attend `fontname`.
