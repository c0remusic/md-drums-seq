# Mode Push du device

Type: task
Status: resolved
Assignee: —
Blocked by: 26 (l'Accent d'abord, décidé le 2026-10-17)

## Question

La ligne « Push 2 » de la carte (grille de pas, Track choisie, Locks au pas tenu) et la piste que l'utilisateur a
ouverte au ticket 25 le 2026-10-14 : un mode Push « à la Tuple », la grille de pads prise par `grab_control`, sans
paramètre Live, donc sans double automation. Que montre la grille, comment choisir Track et Pattern, que deviennent
les Locks et l'écran ?

## Answer

Décidé avec l'utilisateur le 2026-10-17 (questions 1 à 11, 16 et 17 ; 12 à 15 : l'Accent, ticket 26) :

- **Push 2 seul** ; Push 3 plus tard, sur demande.
- **Activation** : un interrupteur « Push » dans la bande, paramètre caché (aucune automation), éteint au chargement.
  La grille n'est prise que si l'interrupteur est allumé, le device actif et sa piste sélectionnée ; quitter la
  piste rend Push à Live, y revenir le reprend (les règles de Tuple).
- **Une seule disposition, celle du Drum Rack de Push** (pas de bascule de mode) :
  - les 4 rangées du haut : les 32 pas de la Track choisie, page 33-64 par Page gauche/droite ;
  - le bloc 4 × 4 en bas à gauche : les 16 Tracks ; frapper joue la Track à la vélocité du pad, par la sortie MIDI du
    device, et la choisit ; ces frappes ne s'enregistrent pas (chantier « enregistrement live ») ;
  - le bloc 4 × 4 en bas à droite : les 16 Patterns, comme les boutons de la bande (choix immédiat à l'arrêt, en file
    en lecture, ajout à la chaîne en mode Chain).
- **Pas** : frapper un pas vide pose un Hit à 127, frapper un Hit le retire ; tenir un pas le sélectionne, et le
  panneau de la fenêtre le suit.
- **Locks aux encodeurs** : pas tenu + encodeurs 1-8, un cran = ±1 ; trois pages (SYN1-8, EFX, ROUTING), choisies par
  les trois premiers boutons sous l'écran (`Track_Select_Button0..2`), le bouton de la page allumé, la page gardée.
  Encodeurs (`Track_Control_0..7`, relatifs en complément à deux) pris tant que le mode est allumé ; ils ne règlent
  plus MD Drums pendant ce temps. La construction commence par un essai de prise d'encodeur sur le Push de
  l'utilisateur ; s'il échoue, pas d'encodeurs en v1.
- **Boutons pris** : Page gauche/droite (pas 1-32 / 33-64) ; Delete tenu + pad de pas (efface le pas) ou de Pattern
  (Clear) ; Duplicate tenu + Pattern source puis cible (Copy to) ; Double (×2) ; Mute tenu + pad de Track (coupe) ;
  Accent tenu + pad de pas (bascule l'accent, ticket 26). Le reste à Live.
- **Couleurs** comme la bande : Hit en couleur d'accent selon sa vélocité, tête de lecture blanche, pas sélectionné
  cerclé, au-delà de la longueur éteint ; Tracks jaunes si elles jouent, sombres coupées, la Track choisie plus vive,
  un éclair à chaque Hit ; Patterns vide / utilisé / affiché / en file clignotant. Palette réécrite par SysEx.
- **Écran** laissé à Live en v1 ; les valeurs des Locks se lisent dans la fenêtre.
- **À vérifier sur le matériel** : la prise d'encodeur, et que la prise survit aux boutons de mode de Push.

## Comments

- 2026-10-17 : recherche faite dans le device Tuple de l'utilisateur, qui joue déjà Push 2 :
  [research/33-push.md](../research/33-push.md). Ce qui marche (prise de `Button_Matrix` sur la deuxième surface,
  `send_value col row couleur`, palette par SysEx, appuis observés), ce qui manque (écran inaccessible depuis M4L,
  appuis non enregistrables par Live, pads qui lancent encore des clips en vue Session), et six questions pour la
  séance. Rien de construit ; à trancher avec l'utilisateur (grilling).
- 2026-10-17, **prise des encodeurs vérifiée** sur le Push 2 de l'utilisateur, Live 12.2.1 :
  `measure/md-seq-push-probe.amxd` et `measure/push_probe.js` (un `js` et `LiveAPI`, posés à la place de
  `md-seq-channel-probe` dans la User Library le temps de l'essai, puis remis). Deux surfaces `Push2` exposent les
  encodeurs (0 et 2) ; sur la seconde, `get_control Track_Control_0..7` puis `grab_control` : les huit encodeurs
  répondent à l'observateur `value` (de 71 à 165 messages chacun quand l'utilisateur les tourne un par un), en relatif
  au complément à deux avec accélération (de −8 à +13 par message). `release_control` les rend, la suppression du
  device aussi. Le paramètre « Probe Surface » de la sonde, automatisable pour que la LOM le règle, a reçu des valeurs
  0 parasites pendant le chargement d'une autre piste : le mode Push ne doit pas dépendre d'un paramètre Live.
  Les Locks aux encodeurs sont donc possibles ; la prise qui survit aux boutons de mode de Push reste à voir.
- 2026-10-17, **noyau construit** (`mdseqPush.*`, `mdseqPushTest`), sans Live : la disposition, les gestes (pas : Hit
  à 127 au premier appui, retiré par un appui de moins de 400 ms sans Lock tourné, gardé et sélectionné sinon ;
  Delete, Accent ; Tracks : jouées à la vélocité du pad, choisies, Mute ; Patterns : choix ou chaîne, Clear,
  Duplicate source puis cible), les encodeurs sur les Locks du pas tenu (63 sauté, départ du Kit ou de 64), les
  boutons (Page, Double, pages de Locks), les couleurs envoyées seulement quand elles changent, la palette par SysEx
  (cases 101 à 118, noir en 0). La sélection n'a sa couleur que tant que le pas est tenu : un pad ne se cercle pas, et
  une couleur de sélection cacherait le Hit. L'Editor gagne `selectStep`, `copyPattern(cible, source)`,
  `clearPattern(p)`, `patternState`, `lockOrKit` et quelques lectures. Reste l'adaptateur : prise par la LOM (règles
  de Tuple : interrupteur, device actif, piste sélectionnée), `live.observer` des pads, boutons et encodeurs, envoi
  des couleurs (`send_value`) et de la palette (`send_midi`), notes des pads par le fil du scheduler ; puis l'essai sur
  le Push de l'utilisateur.
- 2026-10-17, **adaptateur câblé**, garde-fou 13/13, pas encore essayé dans Live (Live fermé, l'utilisateur absent) :
  interrupteur « Push » dans la bande (908, 148, la ligne de la chaîne ramenée à 24 px), `deviceon` depuis
  `live.thisdevice`, la piste sélectionnée depuis un observateur de `live_set view`, 19 `live.observer` pour la
  matrice, les boutons et les encodeurs ; prise sur la deuxième surface qui liste pads et encodeurs ; palette en
  SysEx ; couleurs renvoyées trois fois à 150 ms après la prise. À voir dans Live : le format de ce que
  `live.observer` rapporte de la matrice (lu ici comme vélocité, colonne, rangée), les boutons blancs à 127, la prise
  rendue par Live quand le device est supprimé (rien n'est rendu à la destruction), et la latence d'une frappe de
  Track (fil principal puis scheduler).
- 2026-10-17, **corrections de la revue de l'adaptateur** (658bd57), garde-fou 13/13, toujours pas essayé dans Live :
  les messages du patch (interrupteur, device actif, piste sélectionnée, valeurs des pads, boutons et encodeurs)
  arrivent dans les notifications de Live, où Live refuse un changement et où un appel LOM déplacerait
  `live.object` au milieu de ceux d'une impression (la piste qu'elle crée peut changer la sélection) : ils sont mis
  en file et traités par un qelem à part, `pushWork`, qui fait aussi les rallumages et les rafraîchissements
  qu'`emit()` demande. Les notes d'un pad de Track partent avant les appels LOM des couleurs. Un pad de pas relâché
  est reconnu à son pad et agit sur la Track et le pas où il a été frappé : tourner la page ou changer de Track en
  le tenant ne le laisse plus tenu ni ne retire un autre pas. `Editor::showTrack` renvoie l'onglet de Track de la
  bande, qui suit donc un pad de Track.
- 2026-10-17, **build de dev prêt pour l'essai** : `build/dbg/device/MD Drums Seq.amxd` avec `mdseq_dbg30` (9c66f21, qui écrit chaque valeur reçue de Push dans le log).
  Ordre de l'essai dans Live, set de test, transport arrêté d'abord :
  1. Interrupteur « Push » allumé, piste du device sélectionnée : `%TEMP%\mdseq-log.txt` dit « push: taken on
     surface N », les pads prennent la disposition (pas en haut, Tracks en bas à gauche, Patterns en bas à droite).
  2. Un pad de pas : Hit posé ; retapé vite : retiré ; tenu + encodeur 1 : le Lock bouge dans la fenêtre.
  3. Un pad de Track : le son, l'onglet de la bande suit ; Mute tenu + pad : coupe.
  4. Page gauche/droite, Double, Delete, Duplicate, Accent, pages de Locks sous l'écran : allumés comme prévu.
  5. Autre piste sélectionnée : Push rendu à Live (« push: given back ») ; retour : repris.
  6. Device supprimé avec Push pris : Push revient-il à Live sans rien faire ?
  Si la matrice ne répond pas, lire dans le log ce que l'observateur rapporte (ordre vélocité, colonne, rangée
  supposé).
- 2026-10-19, **premier essai dans Live 12.2.1**, sur une piste temporaire « MDSEQ PUSH » du set de l'utilisateur
  (MoDule et le device de dev) :
  - Les 20 `live.observer` du patch ne disaient rien : ni la piste sélectionnée, ni la matrice, ni les boutons, ni
    les encodeurs. Cause : `live.observer` n'a aucun attribut (`<attributelist/>` dans sa référence, comme
    `live.path` et `live.object`), et « @property value » ne réglait rien. Corrigé : la propriété passe en argument
    (« live.observer value »), et `check_device.py` refuse maintenant un attribut sur ces trois objets.
  - Avec `mdseq_dbg33` : Push pris sur la surface 2, pads allumés selon la disposition. La matrice rapporte bien
    vélocité, colonne, rangée (« 127 1 1 », puis « 0 1 1 » au relâchement). Encodeurs reçus ; essai 2 (Hit posé,
    retiré, Lock à l'encodeur) jugé bon par l'utilisateur.
  - Demande de l'utilisateur : les Mutes directement sur Push. Choisi : Mute verrouillé. Une pression brève sur Mute,
    sans pad de Track tapé entre-temps, fait passer le bloc des Tracks en mode Mute (un pad coupe ou rétablit sa
    Track, sans note), le bouton allumé ; une autre pression en sort ; tenir Mute et taper des pads reste
    momentané ; rendre Push l'enlève.
  - Avec `mdseq_dbg34` (3bb0271), validé par l'utilisateur : Mute verrouillé, pads de Track (117 frappes), Page ←/→,
    Double ; Push rendu à Live quand une autre piste est sélectionnée, repris au retour (« given back » puis « taken »).
  - Pas vus dans le log, donc non essayés : Delete, Duplicate, Accent, les pages de Locks sous l'écran ; la
    suppression du device pendant que Push est pris (la piste MDSEQ PUSH reste dans le set de l'utilisateur, non
    enregistré, à supprimer après enregistrement : une suppression par la LOM a fait quitter Live le 2026-10-17).
- 2026-10-21, **le reste essayé dans Live** avec `mdseq_dbg41` (l'arbre de 1b33edb : la v12, les noms des banques utilisateur), sur une piste de test
  d'un set neuf, devant MoDule ; l'utilisateur, après la liste d'essais : « tout semble ok ».
  Delete, Duplicate, Accent, les pages de Locks sous l'écran, et la suppression du device pendant que Push est
  pris. Tout le mode Push est vu dans Live : résolu. La piste MDSEQ PUSH du set de l'utilisateur reste à supprimer
  à la main.
