# Swing, Accent et Slide façon Machinedrum

Type: task
Status: open
Assignee: c0remusic (session du 2026-10-15)
Blocked by: Accent et Slide attendent une entrée dans MD Drums (voir plus bas)

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
