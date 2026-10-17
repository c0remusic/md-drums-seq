# md-drums-seq : passation de l'UI

**Version approuvée : v5** (`v5-test.html`, `v5-ui-spec.md`, `v5-style-guide.md`), le 2026-10-16, après v1
(familles), v2 (lcd), v3 (ratchets et Conditions lisibles), v4 (micro-timing en flèche, approuvée le 2026-10-10) et v5
(la v4 telle que Max la construit, mesurée dans Live, avec les valeurs du Kit que MD Drums dit au device ; sans textes
d'aide). Approuvée sans look-dev : le device est plat, fait d'objets natifs de Max (ADR 0001 : pas de JS, donc ni
`jsui` ni `v8ui`). Contrôles et identifiants : `parameter-spec.md`. Sémantique des pas : ticket « Sémantique du
Pattern ». Construction de la v5 : ticket 28.

## Deux surfaces

- **La bande**, dans la chaîne de devices de Live : 992 × 169 px de présentation, pour jouer.
- **La fenêtre d'édition**, ouverte par Edit (`pcontrol` sur un sous-patch en présentation) : 1440 × 868 px, pour
  éditer.

## Bande : placement

| Élément | Boîte (x, y, l, h) | Objet Max |
|---|---|---|
| Étiquette Pattern | 8, 6, 140, 12 | `live.comment` |
| Patterns 01-16 | 8 + 36 × colonne, 22 + 28 × rangée, 32 × 24 (4 × 4) | 16 `live.text` bascules, qui choisissent par le `live.tab` « Pattern », paramètre Live hors de la bande (un `live.tab` sur plusieurs rangées passe à la ligne selon sa largeur, mesuré dans Live) ; chacun selon l'état que le cœur envoie (`patternstate p état`) : numéro pâle si le Pattern est vide, clair s'il tient un Hit, allumé s'il joue, contour et numéro #7cc6ff s'il est en file, qui clignote toutes les 500 ms |
| État du Pattern | 8, 136 et 150, 148 × 12 | `live.comment` (« 02 next, after step 64 », « 64 steps, 1/16 ») |
| Filet | 160, 8, 1, 153 | `panel` |
| Étiquette Track | 172, 6, 300, 12 | `live.comment` |
| Tracks 01-16 | 172 + 40 × t, 22, 36 × 24 | `live.tab` d'une rangée (`multiline` 0) : numéro et nom court de la Machine (« 01 BD », « 07 R14 »), le numéro seul tant que MD Drums ne dit rien |
| Mutes 1-16 | 172 + 40 × t, 50, 36 × 16 | `live.text` bascules, l'activateur de piste de Live : allumés tant que la Track joue ; 16 paramètres Live « Track n On » ; couleurs fixées : #f2d33a allumé, encre #1e1e1e ; #4a4a4a coupé |
| Pas de la page | 172, 74, 636 × 40 (16 cases de 36 × 40 au pas de 40) | `lcd` (voir « Une case ») |
| Pages | 172 + 160 × p, 122, 156 × 16 | `live.tab` 4 cases (1-16 … 49-64) |
| Filet | 816, 8, 1, 153 | `panel` |
| FILL | 828, 22, 72 × 64 | `live.text` momentané, paramètre Live |
| Chain | 828, 96, 152 × 20 ; ligne de la chaîne 828, 122, 152 × 36 | `live.text` bascule et `live.comment` : allumé, les boutons Pattern cliqués font une nouvelle chaîne ; éteint, elle joue, chaque Pattern une Pass (ticket 31) |
| Edit | 908, 22, 72 × 24 | `live.text` bouton |
| Capture | 908, 52 (étiquette) et 70 (état), 72 × 16 | `live.comment`, `live.text` en lecture seule |

## Fenêtre : placement

| Élément | Boîte (x, y, l, h) | Objet Max |
|---|---|---|
| Patterns 01-16 | 96 + 32 × i, 16, 28 × 24 | 16 `live.text` comme ceux de la bande, qui choisissent par le même paramètre « Pattern » |
| Copy to | 612, 16, 64 × 24 | `live.menu` « Copy to », 01-16 : le Pattern affiché copié sur celui choisi, puis le menu revient sur son étiquette (ticket 29) |
| Length | étiquette 688, 20 ; menu 736, 16, 52 × 24 | `live.menu` dans une case |
| ×2 | 792, 16, 28 × 24 | `live.text` bouton : Length doublée (64 au plus), les pas recopiés après eux, comme « Dupl. Loop » d'un clip Live |
| Scale | étiquette 832, 20 ; menu 872, 16, 60 × 24 | `live.menu` dans une case |
| Moitiés 1-32, 33-64 | 944, 16, 116 × 24 | `live.tab` 2 cases, une rangée |
| Follow | 1064, 16, 56 × 24 | `live.text` bascule |
| FILL | 1132, 16, 56 × 24 | `live.text` momentané, même paramètre que la bande |
| Print | 1200, 16, 116 × 24 | `live.menu` « Print 1 pass » … « Print 16 passes » dans une case : choisir règle les Passes puis imprime ; il montre « Print 4 passes » au chargement |
| Capture | 1328, 16, 96 × 24 | `live.text` en lecture seule |
| Filet | 0, 52, 1440 × 1 | `panel` |
| Numéros de pas | 176 + colonne, 66, tous les 4 pas | `live.comment` |
| Fill edit | 16, 60, 72 × 20 | `live.text` bascule : les Hits posés à la souris (grille et bande) prennent la Condition FILL ; les Hits ni FILL ni !FILL pâlissent à 35 % |
| Clear | 96, 60, 64 × 20 | `live.text` bouton : le Pattern affiché rendu vide, longueur, échelle et swing compris ; Ctrl+Z dans Live le rend |
| Tracks | 16 (numéro), 40 (nom entier de la Machine), 136 (activateur 24 × 24), rangées au pas de 28 depuis y 88 | `live.comment`, `live.text` |
| Grille | 176, 88, 1220 × 472 : 32 cases de 36 × 24 au pas de 38, 4 px de plus entre les pages de 16, 16 rangées au pas de 28, puis la rangée Swing | `lcd` |
| Swing | « Swing » à 16, 542 ; menu 80, 536, 72 × 24, de 50 % à 80 % ; la rangée Swing de la grille à y 536 | `live.comment`, `live.menu` |
| Filet | 0, 576, 1440 × 1 | `panel` |
| Titre du pas | 16, 592 (« Track 01 TRX-BD ») et 620 (« Step 47, page 3 ») | `live.comment` |
| Hit | 16, 644, 64 × 24 | `live.text` bascule |
| Vélocité | 96, 636, 40 × 40 (+ nom et valeur à 144) | `live.dial` sans nom ni nombre ; « Velocity » et sa valeur en `live.comment` à 144, 642 et 658 |
| Condition | 16, 708, 140 × 24 | `live.menu` (liste de « Sémantique du Pattern ») |
| Micro-timing | 172, 708, 168 × 24 ; texte à 736 (« +6 of 23, late ») | `multislider` d'une barre horizontale signée (depuis le milieu), #7cc6ff sur #1e1e1e, un filet au milieu, −23 à +23 |
| Ratchets | 16, 772, 64 × 24 | `live.menu` x1 à x8 |
| Ratchet velocity | 172, 772, 168 × 24 ; texte à 800 (« -50 %, falling over the 3 ») | le même `multislider`, −100 à +100 % |
| Filet | 352, 592, 1 × 248 | `panel` |
| Locks | trois rangées (SYN, EFX, ROUTING) à y 596, 684, 772 ; 8 boutons de 40 × 40 au pas de 120 depuis x 448, nom à x + 48, y + 6 et valeur à x + 48, y + 22 | `live.dial` sans nombre, `active` à 0 sans Lock ; `live.comment` (« 52 », « Kit 40 », « Kit », « unused » sans valeur) |
| Retour d'impression | 448, 840, 976 × 12 | `live.comment`, vide jusqu'à la première impression |

Menus : un `live.menu` dessine son texte en haut d'une boîte d'environ 15 px, quelle que soit la sienne (mesuré dans Live le 2026-10-17 : 4 px au-dessus du texte d'un bouton voisin). Chaque menu est donc un `panel` #1e1e1e bordé de #121212, la case de la maquette, avec le menu de 16 px centré dedans. Texte : celui d'un `live.comment` commence 4 px sous le haut de sa boîte (centre à 7 px pour 9 px), celui d'un bouton est centré 11 px sous son haut ; une étiquette posée 4 px sous le haut de son contrôle partage sa ligne.

Pas de texte d'aide (l'utilisateur, le 2026-10-16 : « pas besoin du texte ») : restent les textes qui disent une
valeur.

## Une case (lcd)

Case de 36 × 24 dans la grille, 36 × 40 dans la bande ; tout en couleurs opaques que le patch calcule (`lcd` n'a pas de
transparence). Ordre de dessin :

1. Fond : puits #1e1e1e, ou #262626 tous les 4 pas ; mélangé à 18 % de #7cc6ff sur le pas joué.
2. Hit : rectangle plein à 3 px des bords, #1e1e1e → #ffa52e à 35 % + 65 % × vélocité / 127.
3. Ratchets (N > 1) : N crans de 2 × 4 px couleur du fond, dans le bord haut du Hit, au milieu de chaque part du pas.
4. Locks : rond #1e1e1e de 7 px, puis rond #7cc6ff de 5 px, centrés à 5 px du coin haut droit.
5. Condition : bande #c9a0ff de 3 px dans la marge basse ; texte Arial gras 10 px #1e1e1e centré, ligne de base à 6 px
   du bas.
6. Pas joué : cadre #7cc6ff de 1 px ; pas sélectionné : cadre #7cc6ff de 2 px.
7. Micro-timing non nul : triangle couleur du Hit, 3 px de long, 8 px de haut, à mi-hauteur, dans la marge gauche (en
   avance) ou droite (en retard), par-dessus les cadres.

Une case de la rangée Swing : le même fond ; sur un pas de la rangée, un rectangle #7cc6ff à 3 px des bords, son bord
gauche décalé vers la droite de la moitié du retard (jusqu'à 9 px à 80 %), à 40 % sur le fond quand le swing est à
50 % (il ne joue rien) ; le cadre du pas joué.

Primitives `lcd` attendues : `frgb`, `paintrect`, `framerect`, `paintoval`, `paintpoly`, `font`, `textface`, `moveto`,
`write` (noms à confirmer contre la référence de `lcd` de Max 9 à la construction). Un clic dans le `lcd` donne ses
coordonnées : Track = y / 28, pas = colonne.

Redessin : la colonne du pas joué à chaque pas (16 cases, ou 1 dans la bande), une case à chaque édition, tout au
changement de Pattern, de Track montrée, de page ou de moitié.

## Couleurs et typographie

- `live.*` : le thème de Live, sans réglage (famille A, `v1-style-guide.md`).
- `lcd` : les couleurs ci-dessus, thème Dark de Live 12 approché ; à revoir si l'utilisateur passe Live en thème clair.
- Texte : la police de Live dans les `live.*` (10 / 11 / 13 / 16 px) ; Arial gras 10 px dans le `lcd`.

## Comportements

- Pattern : un clic met en file pour la fin du Pass (témoin clignotant), option de bascule immédiate.
- Pas : un clic dans la bande ou la grille pose ou retire le Hit ; dans la grille il sélectionne aussi le pas pour le
  panneau.
- Lock : un bouton sans Lock est inactif et lit « Kit » (le device ignore la valeur du Kit) ; le tourner crée le Lock ;
  les valeurs sautent 63. Construit (ticket « Locks ») : le Kit est la position du milieu, à la place de 63, si bien
  que tourner part de 62 ou 64, et le double-clic de Live le rend au Kit ; la passation demandait un départ du
  dernier Lock du paramètre et Alt-clic, qu'un `live.dial` ne peut pas transmettre. v5 : sans Lock, le bouton est
  posé, éteint, sur la valeur du Kit que MD Drums dit (« Kit 40 » dessous), et le tourner part de là ; le Kit rendu
  depuis le bouton le laisse au milieu jusqu'à la lecture suivante du Kit (500 ms au plus), qui le repose sur la
  valeur du Kit, pour ne pas faire sauter un glisser qui passe par le milieu.
- Mutes : l'activateur de piste de Live, allumé tant que la Track joue ; le paramètre « Track n On » vaut 1 quand la
  Track joue, l'inverse du Mute du noyau.
- Noms SYN1-8 : selon la Machine de la Track, lue par LOM dans les paramètres « Track n Machine » de MD Drums (à
  configurer une fois dans Live), avec une table embarquée des noms par Machine ; sinon SYN1-8. Remplacé le
  2026-10-16 (ADR 0003) : MD Drums écrit les Machines et leurs noms de SYN1-8 dans un fichier que le device nomme par
  un hello dans son MIDI ; rien à configurer, aucune table embarquée. La Machine s'affiche aussi sur les onglets de
  Track de la bande (« 01 TRX-BD »), dans la colonne Machine des rangées et dans le titre du panneau.

## Actifs

Aucun : pas de filmstrip ni d'image. Tout est objet natif ou dessin `lcd`.
