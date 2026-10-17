# Direction visuelle du device

Type: grilling
Status: resolved
Assignee: c0remusic (session du 2026-10-09)
Blocked by: —

## Question

Un device M4L s'affiche dans la chaîne de Live sur 169 px de haut (à confirmer par la recherche « Ce que LOM permet dans
Live 12 »), largeur libre : une grille 16 Tracks × 64 pas n'y tient pas. Où vit l'UI : bande compacte (une Track à la
fois, sélecteur de Track, page de 16 pas), fenêtre flottante plus grande, ou les deux ? Quelle famille visuelle : celle
de l'éditeur v22 de MD Drums (Inter, grille de 4 px, `Design/HANDOFF.md` de gearmulator-md-drums) ou le look natif de
Live (objets `live.*` qui suivent le thème) ? Que montre-t-on d'un pas d'un coup d'œil : Hit, vélocité, Locks,
Condition, micro-timing, ratchets ?

Tout cela se fait en Max natif, sans JS : une fenêtre flottante s'ouvre depuis le device par `pcontrol` ou
`thispatcher` (sous-patch en mode présentation), et Max a ses propres grilles (`live.step`, `live.grid`, `matrixctrl`,
`multislider`) et ses contrôles `live.*` au thème de Live ; `jsui`/`v8ui` ne servent qu'à un dessin sur mesure. Le JS
du cadrage concerne la logique (tests Node), pas l'UI.

## Comments

- 2026-10-09, l'utilisateur : la bande dans la chaîne sert à jouer, une grande fenêtre ouverte par le device sert à
  éditer. Un device M4L ouvre nativement sa propre fenêtre, jusqu'au plein écran (notes de version : Max 8.6.1 dans
  Live 12.0.5, « device window always opens on a display » ; Live 10.0.2, crash corrigé d'un device « which had a
  fullscreen window ») ; seule la bande a une hauteur fixe.
- 2026-10-09, maquettes v1 (`Design/v1-var-family.html`, `v1-test.html`, `v1-ui-spec.md`, `v1-style-guide.md`) : même
  géométrie en trois peaux (A natif Live, B famille v22 de MD Drums, C bande native et fenêtre v22), grille à 64 ou 32
  pas.

## Answer

- **Où** : la bande dans la chaîne (169 px de haut, confirmé par la recherche « Ce que LOM permet dans Live 12 ») sert à
  jouer : Patterns 1-16 avec bascule en file, Tracks 1-16, une rangée de mutes à part, les 16 pas de la page montrée,
  pages, Fill, Edit, état de capture. Une grande fenêtre ouverte par Edit sert à éditer : grille des 16 Tracks, panneau
  du pas sélectionné avec ses 24 Locks.
- **Famille** : A, le look natif de Live (objets `live.*`, thème de Live suivi tout seul), choisi par l'utilisateur sur
  les maquettes v1.
- **Grille** : 32 pas visibles seulement, cases larges avec badges de Condition lisibles ; 64 pas en deux pages.
- **Une case de pas** : Hit en bloc dont l'opacité suit la vélocité, décalé par le micro-timing, un trait par ratchet,
  un point pour les Locks, un badge pour la Condition (détail dans `Design/v1-ui-spec.md`).
- Les maquettes suivantes (« Maquettes de l'UI ») partent de la v1 en famille A à 32 pas.
