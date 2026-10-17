# md-drums-seq v1 : guide de style

Deux peaux pour une même géométrie (grille de 4 px, `v1-ui-spec.md`). Les rôles sont communs ; les valeurs diffèrent.

## Rôles et couleurs

| Rôle | A natif Live (approché) | B famille v22 de MD Drums |
|---|---|---|
| autour du device | #2b2b2b | #151618 |
| face du device, fenêtre | #383838 | #1b1c1e |
| puits des contrôles, cases | #1e1e1e | #26272a |
| case d'un temps (tous les 4 pas) | #262626 | #2e2f33 |
| filets | #4a4a4a | #3a3c40 |
| bord des contrôles | #121212 | #6e7177 |
| texte | #dadada | #f2f2f2 |
| texte secondaire | #a2a2a2 | #a7aaaf |
| texte discret, valeurs du Kit | #767676 | #8e9197 |
| Hit, option choisie | #ffa52e (orange « on » de Live) | #f2f2f2 (l'encre) |
| accent : sélection, pas joué, Locks | #7cc6ff | #6fd1c4 |
| Track qui joue | #f2d33a (jaune d'activation de Live) | rien |
| Track muette | #4a4a4a | #ff7a5c |
| Condition (barre) | #c9a0ff | #6fd1c4 |

A est une approximation du thème Dark de Live 12 : dans Max, les objets `live.*` prennent le vrai thème (clair ou
sombre) d'eux-mêmes. B reprend `Design/HANDOFF.md` de gearmulator-md-drums.

## Typographie

- A : police de Live (« Ableton Sans » dans Max, Arial ici), 10 px étiquettes et aides, 11 px valeurs et boutons, 13 px
  titres, 16 px titre du pas sélectionné.
- B : Inter, 12 px étiquettes, 12 px valeurs, 14 px titres, 22 px semi-gras titre du pas sélectionné. Inter doit être
  installée sur la machine : un `.amxd` n'embarque pas de police.
- Noms de la Machinedrum (SYN, EFX, ROUTING, PTCH, FLTF…) en capitales ; le reste en phrase.

## Espacement

Tout sur 4 px. Bande : marges de 8 px, blocs séparés par un filet et 12 px de chaque côté. Fenêtre : marges de 16 px,
rangées de Track au pas de 28, cases au pas de 19 (64 pas) ou 38 (32 pas), 4 px de plus entre pages de 16.

## États

| État | Rendu |
|---|---|
| repos | puits, bord, texte |
| survol | bord en texte secondaire (A) ou puits en face (B) |
| choisi (Pattern joué, page, Track) | plein en couleur « option choisie » (A) ; souligné et cadre accent pour la Track (B) |
| en file (Pattern suivant) | cadre accent intérieur qui clignote à 0,6 s |
| momentané tenu (Fill) | plein tant que tenu |
| pas joué | cadre accent ; colonne teintée à 12 % dans la fenêtre |
| pas sélectionné | contour accent de 2 px |
| Locké | point accent sur la case ; arc et valeur accent sur le bouton |
| muet | A : bouton d'activation éteint, comme une piste de Live ; B : « M » sur le rouge de mute |
| vide (Pattern sans pas) | texte discret |
| inactif (SYN sans paramètre) | nom « unused », bouton gris |
