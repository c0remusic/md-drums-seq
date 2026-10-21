# Swing, Accent et Slide façon Machinedrum

Type: task
Status: resolved
Assignee: c0remusic (session du 2026-10-15)
Blocked by: Slide, les notes 89-114 côté MoDule (contrat du 2026-10-19, voir plus bas)

## Question

L'utilisateur, le 2026-10-15 : « on a pas le swing, ni les trig apparemment », puis « Trigs Accent/Slide/Swing » et
« Construire façon MD ». Le cadrage du 2026-10-08 (carte) les avait mis plus tard ; rien n'était spécifié.

Sur la Machinedrum (OS 1.63, format de pattern décodé dans `mdProtocol/mdsysexautomation.*` de MD Drums, recherches
`research/06-groupes.md`) :

- **Swing** : une quantité par pattern, 50 % (droit) à 80 %, mot `swingWord` = (s - 50) × 16384 / 50, donc un retard de
  (2 × s - 1) pas ; il s'applique aux pas de la rangée Swing (un masque de 64 pas, « EDIT ALL » pour toutes les pistes,
  ou un par piste), par défaut un pas sur deux.
- **Accent** : une quantité par pattern (0-127) et une rangée de pas. Un pas accentué joue la piste à une vélocité de
  128 + 2 × quantité (`HostModel::trigger(track, velocity, accent)`, VOL gain = ((LEV² >> 8) × VEL >> 17) × …) ; le
  séquenceur joue sinon ses pas à 128. Une note MIDI plafonne à 127 et l'OS met l'accent à 0 pour une note MIDI : MD
  Drums n'a aujourd'hui aucune entrée qui porte un accent.
- **Slide** : une rangée de pas ; les valeurs lockées glissent du pas marqué jusqu'au suivant. MD Drums n'accepte un Lock
  qu'avec le Hit de son bloc (ADR 0003) : pas de valeur entre deux Hits.

## Answer

—

## Comments

- 2026-10-15, **Swing construit** dans le device (rien à demander à MD Drums : c'est du temps). `Pattern::swing` (50-80)
  et `Pattern::swingSteps` (64 bits, un pas sur deux par défaut, pour toutes les Tracks : l'EDIT ALL de la machine) ;
  le Sequencer retarde les Hits des pas de la rangée de `swingSubticks` (arrondi au 840e de pas : 504 à 80 %, 269 à
  66 %), avant leur micro-timing, ratchets compris ; l'impression met au début du clip un dernier pas que le retard
  pousse au-delà de la fin. La file garde quatre pas (un pas peut finir 2,56 pas après sa place). Format du Blob : la
  quantité dans l'en-tête, la rangée en trois mots de 22 bits quand elle n'est pas celle par défaut ; un Pattern sans
  swing écrit les mêmes mots qu'avant. Fenêtre : une rangée « Swing » sous la grille (un clic bascule un pas, un glissé
  peint), un menu 50 % à 80 % à sa gauche ; la fenêtre gagne 28 px (`Design/HANDOFF.md`). Tests : `mdseqSequencerTest`
  swing (et la file sans allocation, prouvée par la capacité de trois pas), `mdseqStoreTest` swingWords,
  `mdseqPrintTest` swing, `mdseqEditorTest` swingFromTheWindow, `mdseqProtocolTest`.
- 2026-10-16, **swing vérifié dans Live** (`measure/live/live-swing-run.mjs`, pistes temporaires supprimées ensuite) :
  build de développement au commit 3ec536e, démonstration Track 1 sur les 16 pas à 80 %, 126 BPM ; une piste MIDI
  enregistre ce que le device envoie (MDSEQ PRINT, Post FX) pendant deux mesures. 35 notes : les pas 1, 3, 5… à 0, 0,5,
  1,0… temps, les pas 2, 4, 6… à 0,4, 0,9, 1,4… temps, exactement 0,15 temps (0,6 pas) en retard, sur toute la durée ; une
  note swinguée finit au Hit suivant de la même Track (0,1 temps), comme un clip le ferait.
- 2026-10-16, vu dans Live (ticket 15, même jour) : le menu Swing et la rangée montrent le swing du Pattern affiché,
  66 % et les pas pairs pour un Pattern, 50 % et la rangée grisée pour l'autre, et suivent le passage de l'un à l'autre.
- Reste à voir dans Live : le menu et la rangée cliqués à la main, un set enregistré puis rouvert qui garde le swing.
- **Accent et Slide** : à construire quand MD Drums aura une entrée pour eux (demande à la session MD Drums). Piste pour
  l'accent : une note de Lock de plus (88) sur le canal de la Track, dans le bloc du Hit, dont la vélocité porte la
  quantité, et MD Drums appelle `trigger(track, velocity, true)` avec `setAccentAmount`. Piste pour le Slide : des notes
  de Lock sans Hit sur le canal de la Track, que MD Drums applique à la voix qui sonne, envoyées par le device à chaque
  pas de temps entre le pas marqué et le suivant ; ou un temps de glissement que MD Drums interpole lui-même.
- 2026-10-17, **Accent décidé avec l'utilisateur** (séance du ticket 33, questions 12 à 15 et 17) :
  - Façon Machinedrum : une quantité par Pattern (0-127, 0 par défaut comme la machine) et une rangée Accent commune
    à toutes les Tracks, calquée sur la rangée Swing (l'EDIT ALL). Chaque Hit d'un pas accentué joue à
    128 + 2 × quantité (`HostModel::trigger(track, velocity, true)` après `setAccentAmount`).
  - Transport : la note 88 (après les Locks 64-87), juste après le Hit, sur le canal de sa Track, vélocité = la
    quantité à l'échelle des Locks (0-62 → 1-63, 64-127 elles-mêmes) ; MD Drums la lie au Hit comme un Lock (règle du
    canal 1). Jouée en direct, elle finit aussitôt comme un Lock (ticket 24) ; imprimée, elle garde sa longueur.
    Amendement de l'ADR 0003 de MD Drums, construit côté MoDule ; proposition envoyée à sa session le 2026-10-17.
  - Fenêtre : une rangée « Accent » sous la rangée Swing (un clic bascule un pas, un glissé peint), le menu de la
    quantité à sa gauche ; un pas accentué marqué dans la grille et dans la bande. Sur Push (ticket 33) : Accent tenu
    + pad de pas bascule l'accent. La quantité seulement dans la fenêtre.
  - Ordre : l'Accent d'abord (device et MoDule), le mode Push ensuite.
  - Slide : rien de décidé, toujours en attente d'une entrée dans MD Drums.
- 2026-10-17, **Accent construit côté device**, garde-fou 12/12 :
  - `Pattern::accent` (0-127, 0 par défaut) et `Pattern::accentSteps` (64 bits, vide par défaut) ; `SetAccent`,
    `SetAccentStep` ; ×2 recopie la rangée, Clear la vide.
  - Sequencer : la note 88 (`AccentNote`, paramètre 24 après les Locks, son propre id de note) après les Locks de
    chaque Hit d'un pas accentué, à chaque ratchet, vélocité `lockVelocity(accent)` ; jouée en direct, elle finit
    aussitôt comme un Lock ; imprimée, elle garde sa longueur ; un arrêt la coupe si elle sonne. Tests
    `mdseqSequencerTest` accents, `mdseqPrintTest` accent.
  - Blob : le bit 18 de l'en-tête dit qu'un Accent suit (après la rangée Swing) : la quantité dans un mot, la rangée
    en trois mots de 22 bits ; un Pattern sans Accent écrit les mêmes mots qu'avant (`mdseqStoreTest` accentWords).
  - Fenêtre : la rangée Accent sous la rangée Swing (un clic bascule, un glissé peint), un `live.numbox` 0-127 dans
    une case à sa gauche ; la fenêtre gagne 28 px (1440 × 896) et le panneau du pas descend d'autant
    (`Design/HANDOFF.md`). Bande : une bande #ff6b5b de 3 px dans la marge du haut des cases accentuées. Tests
    `mdseqEditorTest` accentFromTheWindow, `mdseqViewTest` accentMarks, `mdseqProtocolTest`.
  - Ordre réel : Hit, Locks, puis la note 88 (et non avant les Locks comme écrit dans la proposition à MoDule) ; MD
    Drums la lie au Hit dans le même bloc, quel que soit l'ordre.
  - À ne pas publier avant que MD Drums lise la note 88 : sans elle, MoDule la compte comme une note qui ne joue
    rien (sa ligne d'état le dit cinq secondes). Puis mesurer dans Live un Hit accentué contre la vélocité 127.
- 2026-10-17, **note 88 dans MoDule** (`88fc90a`, son garde-fou 20/20, ADR 0003 amendé « après les Locks ») et
  **Accent vérifié dans Live 12.2.1** : MoDule.vst3 copié par l'utilisateur dans `Common Files\VST3` à la place du lien
  `MD Drums.vst3` ; build de dev `mdseq_dbg29`, Track 1 sur les quatre temps à 127, temps 3 et 4 accentués à 64 ;
  sortie de la piste enregistrée (fader à zéro), `measure/accent_levels.py` : temps simples 0,169 à 0,175, temps
  accentués 0,348 à 0,355 sur deux mesures, rapport 2,06 (+6,3 dB) pour 256/127 = 2,02.
  - Live a quitté à 20:46:59 pendant la suppression par LOM de la piste de mesure (le device puis MoDule) :
    `0xc0000005` dans le code de Live (offset `0x3072654`), sur un fil de travail de Live sans Max ni MoDule sur sa
    pile ; les fils de MoDule encore là. Un seul cas, cause non établie ; signalé à la session MoDule.
  - Reste : la demande de l'utilisateur du même soir, le Slide « de la même manière que swing et accent », avec un
    sélecteur pour une seule ligne sous la grille (à trancher avec lui).
- 2026-10-17, **Slide et rangée des trigs décidés avec l'utilisateur** (questions 18 à 20) :
  - Une seule rangée sous la grille, la rangée des trigs, et un menu à sa gauche (Swing, Accent, Slide) ; à côté, le
    réglage de la rangée choisie (le menu du swing, le nombre de l'Accent, rien pour le Slide) ; la fenêtre revient à
    1440 × 868 ; la bande garde la barre rouge de l'Accent quelle que soit la rangée montrée.
  - Slide façon Machinedrum : une rangée commune à toutes les Tracks, sans quantité ; sur un pas marqué, les valeurs
    lockées de chaque Track glissent vers celles de son Hit suivant.
  - Transport : à la session MoDule de chercher (la routine de glissement de l'OS 1.63 que son moteur pourrait
    appeler) et de proposer un contrat, une note « ce pas glisse » ou des Locks sans Hit pendant un glissement ;
    demandé le 2026-10-17, recherche lancée de son côté.
- 2026-10-17, **construit côté device**, garde-fou 12/12 : `Pattern::slideSteps` (`SetSlideStep`, ×2, Clear ; bit 19 de
  l'en-tête du Blob, la rangée en trois mots après l'Accent ; rien de joué) ; la rangée des trigs dans l'Editor
  (`Editor::TrigRow`, `showTrigRow`, message et clé `trigrow`), dessinée en bleu (Swing), rouge (Accent) ou vert
  (Slide, #7ee08a), `drawMarkCell` pour l'Accent et le Slide ; dans le patch, le menu « Trig Row » et les deux
  réglages superposés qu'un `thispatcher` montre ou cache. Tests `mdseqEditorTest` trigRowMenu, `mdseqStoreTest`
  slideWords, `mdseqSequencerTest`, `mdseqViewTest`. Pas encore vu dans Live : le menu qui cache le bon réglage.
- 2026-10-17, **proposition de MoDule pour le Slide**, en attente de l'accord de l'utilisateur (note de recherche :
  `C:\dev\MModule\.scratch\slide\research-01-os-slide.md`, lue dans le code de l'OS 1.63, rien mesuré) :
  - L'OS n'a pas de routine de Slide appelable : c'est du code dans le gestionnaire d'horloge du séquenceur. Un pas
    après, pour chaque paramètre locké d'un pas qui a son Hit et le Slide, il calcule un incrément
    (fin − début) × 128 / horloges, début = le Lock du pas, fin = le Lock du Hit suivant de la Track ou la valeur du
    Kit, horloges = horloges par pas (6, 3, 8 ou 4 selon l'échelle) × pas jusqu'au Hit suivant, corrigé du swing ; puis
    à chaque horloge de 24 ppqn, en lecture seulement, il ajoute l'incrément à la cible de lissage, sans dépasser la
    fin ; le Hit suivant finit le glissement. Un paramètre sans Lock sur le pas ne glisse pas. MoDule porterait ces
    quelques lignes dans `HostModel`, armées là où `applyLocks` agit.
  - Contrat (a) : avec le Hit d'un pas de Slide, après ses Locks, la note 89, sa vélocité = le nombre d'horloges de
    24 ppqn jusqu'au Hit suivant de la Track (1-127) ; puis les notes 90-113, les Locks du Hit suivant comme cibles,
    même ordre et même échelle que 64-87 (un paramètre locké sans cible glisse vers le Kit) ; toutes liées au Hit
    comme un Lock, refusées sans Hit ; aucun Hit ne recule.
  - Pas (b) : en direct, sur le canal 1, un Lock sans Hit ne peut pas nommer sa Track.
  - Ouvert : 127 horloges = 21 pas à 1×, quand l'OS va jusqu'à 384 ; le « Hit suivant » après des ratchets, et la
    phase de l'horloge pour un Hit swingué ou décalé, sont à définir côté device ; MoDule n'a pas de transport et
    l'OS gèle un glissement à l'arrêt ; un bouton ou une automation sur un paramètre qui glisse n'est pas tracé.
- 2026-10-19, **contrat du Slide tranché avec l'utilisateur** (grilling, Q1 à Q8, recommandations prises) :
  - Contrat (a) de MoDule. Avec le Hit d'un pas de Slide, après ses Locks et l'Accent, liées au Hit comme un Lock,
    refusées sans Hit :
    - note 89, vélocité = horloges 24 ppqn jusqu'au Hit suivant ;
    - notes 90-113, cibles ;
    - note 114 au-delà de 127 horloges : horloges = vélocité 89 + 127 × vélocité 114, ce qui couvre la plus longue
      (64 pas à 1/8×, 3072 horloges).
  - Cibles : la note 90+p seulement pour un paramètre p locké sur les deux pas. Locké sur le pas de Slide seulement :
    MoDule le fait glisser vers le Kit. Locké sur le Hit suivant seulement : ne glisse pas, comme sur l'OS.
  - Hit suivant structurel : prochain pas de la Track qui porte un Hit dans le Pattern courant, en bouclant, quelle
    que soit sa Condition ou son Fill. Le seul Hit d'une Track glisse vers lui-même, un Pattern plus loin. Une bascule
    de Pattern avant lui termine le glissement au premier Hit du nouveau Pattern, côté MoDule.
  - Ratchets : notes de Slide avec le dernier ratchet, durée comptée depuis lui.
  - Durée sur le temps joué : des deux Hits, swing et micro-timing compris, arrondie à l'horloge, au moins 1.
  - Un Hit de Slide qui ne sonne pas (Condition fausse, Mute) n'envoie rien.
  - En direct, note-off immédiat (ticket 24). Imprimé, longueur gardée : le clip rejoue le glissement.
  - Demandé à MoDule : figer les glissements quand le transport de l'hôte s'arrête, si son contexte VST3 le donne ;
    l'écart est accepté sinon.
- 2026-10-19, **construit côté device**, garde-fou 13/13 :
  - `SlideNote`, `FirstSlideTargetNote`, `LongSlideNote`, `TicksPerClock` dans `mdseqTypes.h` ;
    `Sequencer::stageRatchet` envoie le Slide, `gridOf` et `groupBegin` partagés avec `planStep`.
  - Tests : `mdseqSequencerTest` slides (cibles, ratchets et micro-timing, swing, FILL, Hit seul, 1/8× avec note
    114, Mute) ; `mdseqPrintTest` slide (longueur gardée).
  - Le Pattern le plus lourd porte maintenant Accent et Slide sur chaque pas : 0,13 ms au plus long réveil, 17 notes
    tenues au plus en direct.
  - Reste : la part de MoDule, puis l'essai dans Live.
- 2026-10-21, **Slide essayé dans Live** avec `mdseq_dbg41` (l'arbre de 1b33edb : la v12, les noms des banques utilisateur), sur une piste de test
  d'un set neuf, devant MoDule ; l'utilisateur, après la liste d'essais : « tout semble ok ».
  Le Slide côté MoDule est sur son `main` (4ab931b, c1fb60e), vérifié contre le séquenceur du firmware
  (`mdSlideFirmwareTest`), et part dans MoDule 0.1.2 avec l'Accent (88fc90a). Swing, Accent et Slide construits et
  vus dans Live : résolu.
