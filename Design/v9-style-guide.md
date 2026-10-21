# MoDule Seq v9 : guide de style

La v8 (`v8-style-guide.md` : structure Overbridge aux couleurs de MoDule, objets Max sans JavaScript) après son audit
du 2026-10-20, et toutes les tailles en multiples de 2 à la demande de l'utilisateur.

## Une couleur, un rôle

| Token | Valeur | Rôle |
|---|---|---|
| face | #2a2b2f | le device |
| touche | #3d3e43, bord #55585e | une touche au repos (1,33:1 sur la face) |
| puits | #1c1d20 ; tous les 4 pas #222327 | cases du lcd, menus, glissières |
| écran | #151618, bord #55585e | un état |
| encre | #f2f2f2 | le contenu (Hits, valeurs) ; ce qui est choisi, en touche inversée (fond d'encre, texte #1c1d20) : Pattern, Track, page, Hit |
| encre atténuée | #c3c6cb | noms (Machines, paramètres), texte des écrans |
| encre pâle | #9a9da3 | titres de groupe, numéros, valeur du Kit (5,2:1 sur la face) |
| vivant | #6fd1c4, l'accent de MoDule | pas joué, sélection, Lock (marque, valeur, anneau), LED d'un mode, Pattern en file, nouvelle version |
| coupé | #ff7a5c | une Track coupée, rien d'autre |
| Accent | #e8b35a | la rangée Accent : bande de la bande, marque de la rangée des trigs |
| Slide | #9fb7ff | la rangée Slide |

Un mode allumé (Follow, Chain, Clips only, Push, Fill edit) garde sa touche et allume une LED vivante de 4 px le long de
son haut. Un réglage du device (Check for updates) allume la même LED dans l'encre pâle. La vélocité et les glissières
sont du contenu : leur anneau et leur barre sont à l'encre.

## Typographie

Ableton Sans, tailles paires :

| Rôle | Taille, graisse | Casse | Couleur |
|---|---|---|---|
| titre de groupe | 10 px, Bold | capitales | encre pâle |
| touche, menu, onglet | 10 px, Medium (Bold allumée) | capitales | encre |
| nom court d'un paramètre | 10 px, Medium (Bold locké) | capitales | encre atténuée (encre locké) |
| texte d'un écran | 10 px, Medium | casse normale | encre atténuée |
| nom d'une Track, valeur | 12 px, Medium (Bold : Track montrée, valeur) | casse normale | encre atténuée, encre |
| titre du panneau, FILL | 14 px, Bold | casse normale (« Track 01 TRX-BD »), FILL en capitales | encre |

La marque s'écrit toujours « MoDule Seq ». Pas d'espacement des lettres : Max n'en règle pas.

## Écrans

Seulement pour un état : statut du Pattern, chaîne, prise, nouvelle version, pas montré, micro-timing, courbe des
ratchets, ligne d'état de Print. Les valeurs (vélocité, Locks) sont du texte nu sous leur nom. Texte centré sur la
hauteur des capitales, comme en v7.

## Cases du lcd, en multiples de 2

| Élément | Géométrie |
|---|---|
| Hit | à 2 px des bords de la case |
| crans des ratchets | 2 × 4 px, sur des pixels pairs, dans le bord haut du Hit |
| marque d'un Lock | carré de 4 px vivant dans un cadre de 8 px couleur du puits, à 2 px du coin haut droit |
| bande de Condition | 2 px dans la marge basse |
| bande d'Accent (bande) | 2 px dans la marge haute |
| flèche du micro-timing | 2 px de long dans la marge, 8 px de haut |
| rangée Swing | le bloc décalé d'un nombre pair de pixels |
