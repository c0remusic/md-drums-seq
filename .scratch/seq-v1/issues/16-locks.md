# Locks

Type: task
Status: resolved
Assignee: c0remusic (session du 2026-10-10)
Blocked by: 13, 15

## Question

Les Locks dans le device : 24 par pas, émis avec chaque Hit en notes 64-87 sur le canal de sa Track (ADR 0003 de
MD Drums), édités dans le panneau du pas, montrés dans les cases (point de Lock de la passation).

## Comments

- 2026-10-10 : construit.
  - Noyau : un pas tient 24 valeurs, `NoLock` (63, que l'échelle de l'ADR 0003 ne peut pas transporter) là où le Kit
    joue. `SetLock` (paramètre, valeur, 63 pour revenir au Kit) n'est accepté que sur un pas qui a un Hit ; retirer
    le Hit retire ses Locks (MD Drums ignore un Lock sans Hit). Le Sequencer émet, au même instant que le Hit et
    avant lui, une note par Lock (64 + paramètre, vélocité 1-63 pour 0-62, la valeur au-delà), d'un pas de long,
    note-off avant note-on ; Mute et arrêt les traitent comme le Hit. Une édition de Lock sur le pas planifié et pas
    encore commencé replanifie sa Track, comme les Hits.
  - Case : le point de la passation (disque de 5 px couleur accent cerclé de 7 px couleur panneau, près du coin haut
    droit), dans la bande et la grille.
  - Panneau : chaque bouton de Lock est un `live.dial` à 128 positions, chaque position la valeur qu'elle verrouille,
    sauf celle du milieu, à la place de 63, qui lit « Kit ». Tourner depuis le Kit part de 62 ou 64 ; le
    double-clic de Live (retour à la valeur par défaut) rend le paramètre au Kit ; un pas sans Hit refuse et le
    bouton revient au Kit. Le bouton s'allume tant que le pas tient un Lock.
  - Écart à la passation, décidé ici : la passation voulait qu'un Lock parte « du dernier Lock de ce paramètre sur la
    Track, sinon de 64 » et qu'Alt-clic le retire. Un `live.dial` ne transmet ni les clics sans mouvement ni les
    touches de modification, et ne connaît pas l'historique : le Kit au milieu des valeurs donne le départ de 64 sans
    mémoire, et le double-clic remplace Alt-clic.
  - Builds de développement : le Pattern de démonstration pose la Track 3 sur les temps avec un Lock de VOL à 0 sur
    2 et 4, la Track 7 sur les contretemps. Dans Live, la Track 3 doit se taire sur 2 et 4 ; si Live ramenait tous
    les canaux sur 1, ces Locks iraient à la Track 1 et la Track 3 jouerait les quatre temps (question des canaux du
    ticket 04).
- 2026-10-10, **dans Live** : les Locks de la Track 1 marchent (VOL 0 fait taire ses temps 2 et 4, le Kit revient au
  temps 3), ceux des autres Tracks non, parce que Live ramène les notes d'un device sur le canal 1 (ticket 04). En
  passant, un bug du device trouvé et corrigé : les contrôles du panneau envoyaient leur valeur initiale au chargement
  (`hit 0` retirait le Hit du pas sélectionné, `length 16` coupait le Pattern) ; ils n'en ont plus, l'Editor les montre
  au redessin, et `tools/check_device.py` refuse un contrôle d'édition qui en aurait une. Les builds de développement
  lisent leur Pattern de démonstration dans `%TEMP%\mdseq-demo.txt`.
- 2026-10-11, **décidé par l'utilisateur** : le Lock se lie à son Hit. Amendement de l'ADR 0003 de MD Drums, construit
  par sa session : un Lock reçu sur le canal 1 s'applique au dernier Hit reçu avant lui au même échantillon, quelle que
  soit sa Track ; sur les canaux 2-16, il garde sa Track par le canal. Écarté : un device de sortie par piste Track
  avec un External Instrument au canal N (17 devices, un set modèle) et les Locks en direct sur la seule Track 1. Le
  device envoie donc, à un même tick, les note-off d'abord, puis Track par Track chaque Hit suivi de ses Locks, chacun
  toujours sur le canal de sa Track : que Live garde le canal ou non, la liaison tombe juste. Reste à mesurer dans Live
  une fois MD Drums construit avec l'amendement, et, au ticket de l'impression, l'ordre dans lequel Live joue les
  notes simultanées d'un clip (la Track 1, sur le canal 1, en dépend).
- 2026-10-13, avec les ratchets (ticket 18) : l'adaptateur passait une édition au Sequencer en 4 champs, sans
  `param` ; un Lock posé au panneau partait donc sur le paramètre 0 côté jeu, l'interface le montrant sur le bon. La
  démonstration, appliquée directement, ne le voyait pas. Corrigé par `pack`/`unpack` dans le noyau, testés par
  `mdseqEditorTest`.
- 2026-10-13 : prouvé dans Live 12.2.1 à 126 BPM, sur une piste « MDSEQ TEST » du set de l'utilisateur (MD Drums
  construit le 2026-10-13 à 17:05, après cf3385cd et 812551fa ; build de dev `mdseq_dbg9` puis `mdseq_dbg10` sur le
  Pattern de `measure/steps.py`), sortie de la piste enregistrée par `live-record-track.mjs`, analysée par `steps.py`.
  Track 3 sur les quatre temps, VOL verrouillé à 0 sur les temps 2 et 4 : temps 2 et 4 à
  −inf dB, temps 1 et 3 entre −12,2 et −13,3 dB, sur trois enregistrements. Le Lock part sur le canal 1 (Live efface
  le canal) juste après son Hit, et MD Drums l'y lie.
