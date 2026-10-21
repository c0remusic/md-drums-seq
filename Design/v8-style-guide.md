# MoDule Seq v8 : guide de style

La disposition et les écrans de la v7 (`v7-ui-spec.md`, `v7-style-guide.md`), la structure de la variante Overbridge,
les couleurs de MoDule. Choisi par l'utilisateur le 2026-10-20 (« Overbridge aux couleurs MoDule »), contraste relevé
après la variante sombre (« très sombre, pas très lisible »).

## Couleurs

Lues dans le skin de MoDule (MModule, `mdDrums.rcss`, couleurs de la passation v22), relevées pour la lecture :

| Rôle | Valeur | Origine dans MoDule |
|---|---|---|
| face du device | #2c2d31 | surface #26272a, relevée |
| barre de titre | #1b1c1e | chrome |
| puits du lcd, menus, glissières | #1f2023 ; tous les 4 pas #25262a | — |
| écran | #151618, bord #55585e | display, rule |
| touche | #34353a, bord #55585e | — |
| encre | #f2f2f2 | ink |
| encre atténuée | #c3c6cb | ink-dim #a7aaaf, relevée |
| encre pâle | #8e9197 | ink-faint |
| accent | #6fd1c4, texte dessus #10201e | accent, on-accent |
| Track coupée, rangée Accent | #ff7a5c, texte dessus #111111 | mute |
| hors longueur | puits #141517 ; page #26272a, texte #5f6267 | — |

Rôles : le contenu est à l'encre (un Hit blanc, de #1f2023 à #f2f2f2 selon la vélocité) ; tout ce qui est choisi
(Pattern, Track, page) se remplit de l'accent ; un mode allumé (Follow, Chain, Clips only, Push, Fill edit) garde sa
touche et allume une LED de 3 px d'accent le long de son haut ; un Lock et la sélection sont dans l'accent ; une Track
coupée est corail.

## Typographie

Ableton Sans, que le Max de Live fournit, partout, le lcd compris : Inter, la police de MoDule, n'est pas installée, et
Max ne voit que les polices installées. Étiquettes, touches, menus et écrans en capitales, sans espacement des
lettres (aucun attribut ne le règle dans Max). Tailles de la v7 : 10 px, 11 px pour les noms des Tracks, 13 px gras
pour les titres et FILL ; Bold pour une étiquette, une valeur et un Lock, Medium pour le reste.

## Objets Max

Vérifié dans les références livrées avec Live (`Resources/Max/resources/docs/refpages/m4l-ref`) :

| Élément | Objet et attributs |
|---|---|
| faces, puits, écrans, filets | `panel` (`bgcolor`, `bordercolor`, `rounded`) |
| touches | `live.text` (`bgcolor`, `bgoncolor`, `bordercolor`, `textcolor`, `activetextoncolor`, `rounded`) |
| LED d'un mode | `live.text` avec `pictures` / `usepicture`, une image SVG éteinte et allumée |
| encodeurs | `live.dial` (`appearance`, `dialcolor`, `activedialcolor`, `needlecolor`) : l'épaisseur de l'anneau reste la sienne |
| menus | `live.menu` (`bgcolor`, `textcolor`, `hltcolor`) : la liste déroulante reste celle de Live |
| texte des écrans et étiquettes | `live.comment` (`textcolor`, `fontname`, `fontsize`), le texte écrit en capitales |
| cases | `lcd` |

Les couleurs sont fixées : le device ne suit plus le thème de Live, et reste sombre sous un thème clair.
