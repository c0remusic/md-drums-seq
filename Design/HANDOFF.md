# MoDule Seq : passation de l'UI

**Version approuvée : v12** (`v12-test.html`, `v12-ui.js`, `v12-ui.css`), le 2026-10-20, après v5 (approuvée le
2026-10-16, la v4 telle que Max la construit) et ses retouches : v6 (critique de l'UI, rôles des couleurs, pages hors
longueur), v7 (Ableton Sans partout, les écrans), v8 (la structure d'Overbridge aux couleurs de MoDule), v9 (son audit),
v10 (la couleur rendue : ce qui est choisi en turquoise plein, les écrans colorés), v11 (touches et cases pleines, sans
barre ; les pas du séquenceur visibles), v12 (les menus en touches pleines). Approuvée sans look-dev : le device est
plat, fait d'objets natifs de Max (ADR 0001 : pas de JS, donc ni `jsui` ni `v8ui`). Construite par
`tools/make_device.py` (le patch) et `source/mdseq/mdseqView.*` (les cases des deux `lcd`). Contrôles et
identifiants : `parameter-spec.md`. Sémantique des pas : ticket « Sémantique du Pattern ».

## Deux surfaces

- **La bande**, dans la chaîne de devices de Live : 992 × 169 px de présentation, pour jouer. Un `panel` de la couleur
  de la face, sur le calque de fond, sous tout le reste : le device ne suit plus le thème de Live.
- **La fenêtre d'édition**, ouverte par Edit (`pcontrol` sur un sous-patch en présentation) : 1440 × 868 px, pour
  éditer ; sa face est la couleur de fond du sous-patch.

## Bande : placement

| Élément | Boîte (x, y, l, h) | Objet Max |
|---|---|---|
| Titre PATTERN | 8, 6, 140, 12 | `live.comment` |
| Patterns 01-16 | 8 + 36 × colonne, 22 + 28 × rangée, 32 × 24 (4 × 4) | 16 `live.text` bascules, qui choisissent par le `live.tab` « Pattern », paramètre Live hors de la bande (un `live.tab` sur plusieurs rangées passe à la ligne selon sa largeur, mesuré dans Live) ; chacun selon l'état que le cœur envoie (`patternstate p état`) : numéro pâle sur une touche enfoncée (#323338) si le Pattern est vide, à l'encre s'il tient un Hit, plein de turquoise s'il joue, bordé de turquoise s'il est en file, qui clignote toutes les 500 ms |
| État du Pattern | écran 8, 132, 148 × 30 | deux lignes : `status` (« 02 next, after step 48 ») et `lengthtext` (« 48 steps, 1/16 ») |
| Filet | 160, 8, 1, 153 | `live.line` |
| Titre TRACK | 172, 6, 300, 12 | `live.comment` |
| Tracks 01-16 | 172 + 40 × t, 22, 36 × 24 | `live.tab` d'une rangée (`multiline` 0) : numéro et nom court de la Machine (« 01 BD », « 07 R14 »), le numéro seul tant que MD Drums ne dit rien |
| Mutes 1-16 | 172 + 40 × t, 50, 36 × 16 | `live.text` bascules : une touche à son numéro tant que la Track joue, corail (#ff7a5c, texte #111111) une fois coupée ; 16 paramètres Live « Track n On » |
| Pas de la page | 172, 74, 636 × 40 (16 cases de 36 × 40 au pas de 40) | `lcd` (voir « Une case ») |
| Pages | 172 + 160 × p, 122, 156 × 16 | 4 `live.text` bascules (1-16 … 49-64) : la page montrée pleine, celles au-delà de la longueur inactives (`pages p n` du cœur) ; un clic demande sa page, le cœur répond chaque fois |
| Nouvelle version | écran 172, 144, 636 × 18 | texte ambre (#e8b35a) ; l'écran n'existe que s'il dit quelque chose (`updateshown`, `script sendbox … hidden`) |
| Filet | 816, 8, 1, 153 | `live.line` |
| Titre PLAY | 828, 6, 152, 12 | `live.comment` |
| FILL | 828, 22, 72 × 36 | `live.text` momentané, paramètre Live, 14 px gras |
| Chain | 908, 22, 72 × 36 | `live.text` bascule : allumé, les boutons Pattern cliqués font une nouvelle chaîne ; éteint, elle joue, chaque Pattern une Pass (ticket 31) |
| Chaîne | écran 828, 62, 152 × 18 | `chaintext` (« Chain 2/3: 01 03 02 ») |
| Titre OUTPUT | 828, 86, 152, 12 | `live.comment` |
| Clips only | 828, 102, 72 × 20 | `live.text` bascule, paramètre Live automatisable « Output » (éteint 0, allumé 1, comme Device et Clips avant la v12) : allumé, le device n'envoie plus de note-on, les clips imprimés ou capturés jouent MoDule seuls (ticket 23) |
| Capture | 908, 102, 72 × 20 | `live.text` bouton, actif quand la prise a une note : la dernière prise en clips de Session, « Take n » (ticket 23) |
| Prise | écran 828, 126, 152 × 18 | `take` (« Take 12 bars », « No take ») |
| Edit | 828, 148, 72 × 16 | `live.text` bouton |
| Push | 908, 148, 72 × 16 | `live.text` bascule, paramètre caché, éteint au chargement : Push 2 pris tant que le device est allumé et sa piste sélectionnée (ticket 33) |

## Fenêtre : placement

Barre du haut, en groupes séparés par des filets verticaux (616, 952, 1152 et 1232 ; 1 × 24 à y 16).

| Élément | Boîte (x, y, l, h) | Objet Max |
|---|---|---|
| Titre PATTERN | 16, 22, 72, 12 | `live.comment` |
| Patterns 01-16 | 96 + 32 × i, 16, 28 × 24 | 16 `live.text` comme ceux de la bande, qui choisissent par le même paramètre « Pattern » |
| Copy to | 628, 16, 64 × 24 | `live.menu` « COPY TO », 01-16 : le Pattern affiché copié sur celui choisi, puis le menu revient sur son étiquette (ticket 29) |
| Length | titre 704, 22 ; menu 748, 16, 52 × 24 | `live.menu` |
| ×2 | 804, 16, 28 × 24 | `live.text` bouton : Length doublée (64 au plus), les pas recopiés après eux, comme « Dupl. Loop » d'un clip Live |
| Scale | titre 844, 22 ; menu 880, 16, 60 × 24 | `live.menu` |
| Moitiés 1-32, 33-64 | 964, 16, 116 × 24 | `live.tab` 2 cases, une rangée |
| Follow | 1084, 16, 56 × 24 | `live.text` bascule |
| FILL | 1164, 16, 56 × 24 | `live.text` momentané, même paramètre que la bande |
| Source de Print | 1244, 16, 120 × 24 | `live.menu` « PATTERN ×1 » … « PATTERN ×16 », « LAST TAKE » (`printsource i`) ; « PATTERN ×4 » au chargement |
| Print | 1368, 16, 56 × 24 | `live.text` bouton : le Pattern affiché sur les Passes choisies, ou la dernière prise comme Capture, en clips de Session |
| Filet | 0, 52, 1440 × 1 | `live.line` |
| Fill edit | 16, 60, 72 × 20 | `live.text` bascule : les Hits posés à la souris (grille et bande) prennent la Condition FILL ; les Hits ni FILL ni !FILL pâlissent à 35 % |
| Clear | 96, 60, 64 × 20 | `live.text` bouton : le Pattern affiché rendu vide, longueur, échelle, swing, Accent et Slide compris ; Ctrl+Z dans Live le rend |
| Numéros de pas | 176 + colonne, 66, tous les 4 pas | `live.comment` pâles |
| Tracks | 16 (numéro), 40 (nom entier de la Machine, 12 px, à l'encre pour la Track montrée par la bande), 136 (Mute 24 × 24), rangées au pas de 28 depuis y 88 | `live.comment`, `live.text` |
| Grille | 176, 88, 1220 × 472 : 32 cases de 36 × 24 au pas de 38, 4 px de plus entre les pages de 16, 16 rangées au pas de 28, puis la rangée des trigs | `lcd` |
| Rangée des trigs | menu 16, 536, 60 × 24 (SWING, ACCENT, SLIDE) ; à côté, 80, 536, 72 × 24, le réglage de la rangée choisie : menu de 50 % à 80 % pour Swing, nombre de 0 à 127 pour Accent (sur une touche comme un menu), rien pour Slide, l'un ou l'autre montré par `thispatcher` (`script sendbox … hidden`) (ticket 26) | `live.menu`, `live.numbox`, `thispatcher` |
| Filet | 0, 576, 1440 × 1 | `live.line` |
| Titre du panneau | 16, 594, 168 × 18 | `live.comment` 14 px gras à l'encre (« Track 01 TRX-BD ») |
| Pas montré | écran 192, 592, 148 × 20 | `steptitle` (« Step 47, page 3 ») |
| Hit | 16, 644, 64 × 24 | `live.text` bascule |
| Vélocité | 96, 636, 40 × 40 ; nom 144, 638 ; valeur 144, 656 | `live.dial` sans nom ni nombre, son arc à l'encre ; « VELOCITY » et la valeur (12 px gras) en `live.comment` |
| Condition | titre 16, 692 ; menu 16, 708, 140 × 24 | `live.menu` (liste de « Sémantique du Pattern », en capitales) |
| Micro-timing | titre 172, 692 ; barre 172, 708, 168 × 24 ; écran 172, 734, 168 × 18 (« +6 of 23, late ») | `multislider` d'une barre horizontale signée (depuis le milieu), encre sur #1c1d20, bordé comme une touche, un filet au milieu, −23 à +23 |
| Ratchets | titre 16, 756 ; menu 16, 772, 64 × 24 | `live.menu` ×1 à ×8 |
| Ratchet velocity | titre 172, 756 ; barre 172, 772, 168 × 24 ; écran 172, 798, 168 × 18 (« -50 %, falling over the 3 ») | le même `multislider`, −100 à +100 % |
| Filet | 352, 592, 1 × 248 | `live.line` |
| Locks | trois rangées (SYN, EFX, ROUTING) à y 596, 684, 772, leur titre à 368, y + 14 ; 8 boutons de 40 × 40 au pas de 120 depuis x 448, nom à x + 48, y + 2 et valeur à x + 48, y + 20 | `live.dial` sans nombre ; un Lock : arc turquoise, aiguille et nom à l'encre, valeur turquoise ; sans Lock : arc couleur de l'anneau, aiguille et valeur pâles (« Kit 40 », « Kit », « unused » sans valeur) |
| Filet | 0, 840, 1440 × 1 | `live.line` |
| Retour d'impression | écran 16, 846, 1264 × 18 | `printstatus`, vide jusqu'à la première impression |
| Check for updates | 1296, 847, 128 × 16 | `live.text` bascule, réglage du device |

## Écrans, menus, textes

- **Écran** : un `panel` sur le calque de fond, #10201e bordé de #2c5650, arrondi 2, sous un `live.comment` par ligne
  en turquoise, 10 px, casse de phrase, à 6 px du bord gauche ; les lignes à 12 px l'une de l'autre, le milieu de
  leurs capitales centré sur la hauteur de l'écran comme `screen()` de `v12-ui.js`. Un écran dit un état, jamais le nom
  d'un contrôle.
- **Menu** : une touche (`panel` #3d3e43 bordé de #55585e, arrondi 2) avec le `live.menu` de 16 px centré dedans, de la
  même couleur (`activebgcolor`, `bordercolor`), texte à l'encre, flèche (`tricolor`) #c3c6cb. Ouvert, sa liste garde le
  fond de Live ; seule la ligne sous la souris prend le turquoise (`hltcolor`) et #10201e (`hlttextcolor`). Un
  `live.menu` dessine son texte en haut d'une boîte d'environ 15 px, quelle que soit la sienne (mesuré dans Live le
  2026-10-17).
- **Texte d'un `live.comment`** : posé par le milieu de ses capitales, que Live place à 8, 9 et 11 px sous le haut de
  la boîte pour 10, 12 et 14 px, et 4 px à droite du bord gauche de la boîte (mesuré dans Live le 2026-10-20, dbg36 à
  dbg38 : encre centrée à un demi-pixel près dans les écrans et les boîtes de la maquette, titres alignés à x 16).
- **Filet** : un `live.line` de 1 px, sa ligne sur le bord haut ou gauche d'une boîte de 4 px ; un `panel` n'est jamais
  plus fin que 4 px (mesuré dans Live).
- **Touche qui agit** (×2, Clear, Print, Capture, Edit) : un `live.text` bascule remis à 0 aussitôt ; en mode bouton,
  `live.text` dessine une pilule et non une touche (vu dans Live).
- **Ordre de dessin** : dans un même calque, Max dessine devant les boîtes créées d'abord (vu dans Live : un `panel`
  créé avant un `live.menu` ou un `live.comment` le cachait, la face de la bande créée en premier cachait les écrans).
  Les fonds d'écran et de menu sont sur le calque de fond, et la face de la bande y est créée en dernier.

Pas de texte d'aide (l'utilisateur, le 2026-10-16 : « pas besoin du texte ») : restent les textes qui disent une
valeur ou un état.

## Une case (lcd)

Case de 36 × 24 dans la grille, 36 × 40 dans la bande ; tout en couleurs opaques que le cœur calcule (`lcd` n'a pas de
transparence ; son fond est transparent, la face se voit entre les cases). Ordre de dessin (`drawCell`) :

1. Fond : touche #3d3f44, ou #4a4c52 tous les 4 pas ; mélangée à 18 % de turquoise (#6fd1c4) sur le pas joué.
2. Pas de la rangée Accent, dans la bande : l'ambre (#e8b35a) prend toute la case, celle du Hit, ou sans Hit un creux
   d'un quart d'ambre sur le fond, à 2 px des bords.
3. Hit : rectangle plein à 2 px des bords, du fond (#3d3f44) vers l'encre #f2f2f2 (l'ambre sur un pas accentué), à
   50 % + 50 % × vélocité / 127 : jamais plus sombre qu'à mi-chemin.
4. Ratchets (N > 1) : N crans de 2 × 4 px couleur du fond, dans le bord haut du Hit, sur un pixel pair au milieu de
   chaque part du pas.
5. Locks : carré #3d3f44 de 8 px dans le coin haut droit (à 2 px des bords), carré turquoise de 4 px dedans.
6. Condition : son texte seul, Ableton Sans Bold 10 px #1c1d20, centré par les chasses de la police, ligne de base à
   6 px du bas.
7. Pas joué : cadre turquoise de 1 px ; pas sélectionné : cadre turquoise de 2 px.
8. Micro-timing non nul : triangle couleur du Hit, 2 px de long, 8 px de haut, à mi-hauteur, dans la marge gauche (en
   avance) ou droite (en retard), par-dessus les cadres.

Au-delà de la longueur du Pattern : la case #202125, plus sombre que la face, éteinte ; un Hit gardé là n'est qu'un
rectangle à 18 % de l'encre, sans rien d'autre ; les rangées des trigs s'éteignent aussi. Un filet #a2a2a2 de 2 px,
toute la hauteur du `lcd`, centré dans l'intervalle après le dernier pas, marque la fin (`strip::endLine`,
`grid::endLine`) ; une nouvelle longueur redessine bande et grille.

Une case de la rangée Swing : le même fond ; sur un pas de la rangée, un rectangle #9a9da3 mélangé au fond à 55 % (30 %
quand le swing est à 50 %, où il ne joue rien), à 2 px des bords, son bord gauche décalé vers la droite de la moitié du
retard, sur un pixel pair ; le cadre du pas joué.

Une case de la rangée Accent ou Slide : le même fond ; sur un pas de la rangée, un rectangle à 2 px des bords, ambre
#e8b35a pour l'Accent, pervenche #9fb7ff pour le Slide ; le cadre du pas joué.

Primitives `lcd` : `frgb`, `paintrect`, `framerect`, `paintoval`, `paintpoly`, `font`, `textface`, `moveto`, `write`.
Police : « Ableton Sans Bold Regular » 10 px, `textface normal` (la graisse est celle de la police). Un clic dans le
`lcd` donne ses coordonnées : Track = y / 28, pas = colonne.

Redessin : la colonne du pas joué à chaque pas (16 cases, ou 1 dans la bande), une case à chaque édition, tout au
changement de Pattern, de longueur, de Track montrée, de page ou de moitié.

## Couleurs et typographie

| Rôle | Couleur |
|---|---|
| Face | #2a2b2f |
| Touche, menu fermé | #3d3e43 bordé de #55585e ; Pattern vide #323338 |
| Choisi, mode allumé, réglage allumé | turquoise #6fd1c4 plein, texte #10201e |
| Inactif (page hors longueur, Capture sans prise) | fond de la face, texte #5f6267 |
| Encre (contenu, valeurs) | #f2f2f2 ; atténuée #c3c6cb (noms) ; pâle #9a9da3 (titres de groupe, indices, valeurs du Kit) |
| Écrans | fond #10201e, bord #2c5650, texte turquoise ; nouvelle version en ambre #e8b35a |
| Track coupée | corail #ff7a5c, texte #111111 |
| Rangées | Accent ambre #e8b35a, Slide pervenche #9fb7ff |
| Filets | #55585e ; anneau d'un bouton, milieu d'une barre #4a4c51 ; puits d'une barre #1c1d20 |

Typographie : Ableton Sans, que Max porte dans Live, sous les noms que lui donnent les devices de Live : « Ableton Sans
Medium » et « Ableton Sans Bold Regular ». Titres de groupe 10 px gras en capitales pâles ; touches et menus 10 px en
capitales ; noms 10 px en capitales atténuées ; noms de Machine et valeurs 12 px ; titre du panneau et FILL 14 px gras ;
écrans 10 px en casse de phrase ; la marque écrite « MoDule Seq ». Tailles paires.

## Comportements

- Pattern : un clic met en file pour la fin du Pass (témoin clignotant), option de bascule immédiate.
- Pas : un clic dans la bande ou la grille pose ou retire le Hit ; dans la grille il sélectionne aussi le pas pour le
  panneau.
- Pages : la page montrée reste dans la longueur ; une longueur raccourcie ramène la bande à sa dernière page.
- Print : le menu choisit la source, le bouton imprime ; « LAST TAKE » écrit la dernière prise comme Capture.
- Lock : sans Lock, le bouton est posé, éteint, sur la valeur du Kit que MD Drums dit (« Kit 40 » dessous), et le
  tourner part de là ; le Kit est la position du milieu, à la place de 63, et le double-clic de Live le rend au Kit ;
  le Kit rendu depuis le bouton le laisse au milieu jusqu'à la lecture suivante du Kit (500 ms au plus), qui le repose
  sur la valeur du Kit, pour ne pas faire sauter un glisser qui passe par le milieu.
- Mutes : le paramètre « Track n On » vaut 1 quand la Track joue, l'inverse du Mute du noyau.
- Noms SYN1-8 et Machines : MD Drums les écrit dans un fichier que le device nomme par un hello dans son MIDI (ADR
  0003) ; la Machine s'affiche sur les onglets de Track de la bande (« 01 TRX-BD »), dans la colonne Machine des
  rangées et dans le titre du panneau.

## Actifs

Aucun : pas de filmstrip ni d'image. Tout est objet natif ou dessin `lcd`.
