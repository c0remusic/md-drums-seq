# Timing et canaux du MIDI joué en direct

Type: task
Status: resolved
Assignee: c0remusic (session du 2026-10-09)
Blocked by: —

## Question

Le device joue en direct par `midiout` (décision du 2026-10-09, voir la carte). Mesurer dans Live, avec MD Drums :

- la gigue d'une note émise par `midiout` (scheduler en interruption audio, horloge `plugsync~` ou `transport`) contre
  la même note jouée depuis un clip, à l'arrivée dans MD Drums ou à sa sortie Out ;
- si le canal MIDI de chaque note arrive intact jusqu'à MD Drums, dans les deux placements possibles du device : en
  effet MIDI sur la piste de MD Drums, ou sur sa propre piste dont la sortie va vers MD Drums (Live garde-t-il les 16
  canaux, ou ramène-t-il tout sur un seul ?). Les notes de Lock en dépendent (canal N = Track N).

Un device de mesure jetable, avec une checklist pour l'utilisateur si l'agent ne peut pas piloter Live lui-même. Les
Locks eux-mêmes ne s'entendent qu'avec « Locks de MD Drums poussés » ; le canal se vérifie sans eux (MIDI Monitor).

## Comments

- 2026-10-09 : ce ticket mesurait la latence d'écriture LOM pour un lookahead ; le jeu en direct par `midiout` l'a
  remplacé (l'écriture LOM ne sert plus qu'à l'impression, hors temps réel).
- 2026-10-09, outils dans `measure/` à côté de la carte : `make_devices.py` écrit `md-seq-probe.amxd` (note 36 à chaque
  double croche pendant que Live joue, canal 1 à 16 tour à tour) et `md-seq-channels.amxd` (une case allumée par canal
  reçu, bouton clear) au format du « Max MIDI Effect.amxd » de Live 12 ; `onsets.py` mesure l'écart de chaque frappe
  enregistrée à une grille régulière (validé sur un WAV synthétique : écart de 38 échantillons injecté, 38 retrouvé).

### Checklist pour l'utilisateur

Préparation : tempo 120, noter la fréquence d'échantillonnage et la taille de buffer (Préférences, Audio). Ajouter le
dossier `C:\dev\md-drums-seq\.scratch\seq-v1\measure` aux Places du navigateur de Live.

1. **Canaux, device sur la piste de l'instrument** : une piste MIDI avec `md-seq-probe`, puis `md-seq-channels`, puis
   un instrument (Operator suffit). Lancer la lecture deux mesures. Combien de cases allumées dans `md-seq-channels` :
   16 (canaux gardés) ou 1 (tout ramené sur un canal) ?
2. **Canaux, device sur sa propre piste** : piste « Source » avec `md-seq-probe` seul ; piste « Dest » avec
   `md-seq-channels` puis l'instrument. Sur « Source », MIDI To = « Dest », « Track In ». Clear, lecture deux mesures,
   même question.
3. **Timing** : piste « Ref » avec un clip de 4 mesures, note C1 (36) à chaque double croche, vélocité 100, vers un
   Operator à attaque nette ; piste « Probe » avec `md-seq-probe` vers le même Operator (copie de la piste). Deux pistes
   audio armées, Audio From « Ref » et « Probe » (Post FX). Enregistrer 4 mesures depuis la mesure 1 en Arrangement,
   métronome coupé. Donner les chemins des deux WAV (dossier `Samples/Recorded` du projet).
4. Facultatif : refaire 3 avec MD Drums à la place d'Operator, pour ajouter le plancher du moteur (0 à 31 échantillons
   tant que son ticket 19 n'est pas fait).
- 2026-10-10 : la tranche verticale (ticket 13) joue par l'external `mdseq` ; sa checklist mesure le timing réel avec
  `onsets.py`. La partie canaux de cette checklist reste à faire, pour les Locks.
- 2026-10-10, **timing mesuré** dans le set de l'utilisateur par le pont `TupleMCP` (scripts dans `measure/live/`),
  Live 12.2.1, 44,1 kHz, 126 BPM. Piste PROBE : `MD Drums Seq` (build de développement, Pattern de démonstration :
  notes 36 sur les temps, 37 sur 2 et 4, 42 sur les croches, une double croche chacune) puis Operator par défaut ;
  piste REF : un clip d'une mesure avec les mêmes notes, puis le même Operator ; chacune enregistrée en audio (Post
  FX) en Session, lancées ensemble à la barre, 4 mesures. `onsets.py --bpm 126 --division 8`, 35 attaques chacune :
  - décalage moyen à la grille : PROBE 34,3 échantillons, REF 34,5 ; écart des résidus 32 échantillons pour les deux ;
  - par attaque, les résidus de PROBE répètent mesure après mesure le même motif à 1 échantillon près
    (−2 8 −16 16 −13 13 −16 16), celui de REF aussi (−13 13 −16 16 …) : le motif vient du détecteur sur des sons
    différents (accords de sinus de hauteurs différentes), pas du MIDI ;
  - seules différences entre PROBE et REF, au premier temps de chaque mesure et à la croche suivante : +11 et −5
    échantillons (moins de 0,25 ms), d'une mesure à l'autre identiques, ce que donne la phase libre des oscillateurs
    d'Operator selon ce qui a joué avant (le clip boucle, le device non).

  Conclusion : `midiout` dans un device garde la position sous le vecteur jusqu'à l'instrument. Des événements calés
  sur les vecteurs de 64 échantillons varieraient d'une mesure à l'autre de jusqu'à 64 échantillons (une croche vaut
  10 500 échantillons, qui ne tombent pas sur les vecteurs) ; ici rien ne varie. Le device joue au même échantillon
  qu'un clip, sans gigue mesurable. Reste la partie canaux, pour les Locks.
- 2026-10-10, **canaux mesurés** (set vide, pistes temporaires, pont `TupleMCP`) : le canal qu'un device Max for Live
  met sur ses notes ne passe pas. Une sonde (`measure/md-seq-channel-probe.amxd` : canal du dernier Hit et du dernier
  Lock vus, en paramètres Live que la LOM lit) placée juste après `MD Drums Seq` lit le canal 1 pour des notes envoyées
  sur les canaux 3 et 7 ; sur une autre piste qui prend son MIDI de celle-là (MIDI From, Post FX), encore le canal 1.
  MD Drums le confirme à l'oreille : les Locks VOL de la Track 3 (canal 3) restent sans effet, la Track 3 joue ses
  quatre temps (−7,9 à −8,8 dBFS) dans les deux placements, alors que ceux de la Track 1 (canal 1) la font taire. Le
  guide Max for Live le dit : par convention, les devices MIDI travaillent sur le canal 1 ; et le mode MPE de Live
  attribue lui-même les canaux, il ne garde pas ceux de la source. La session MD Drums a vérifié son côté : un Lock VOL
  sur le canal 3 fait taire la Track 3 dans son test VST3. Le contrat de l'ADR 0003 (canal = Track) ne tient donc pas
  pour le jeu en direct depuis un device ; il tient pour des clips sur 16 pistes avec un External Instrument au canal
  N (non mesuré).
- 2026-10-14, **MD Drums par le device contre MD Drums par un clip**, à la question de l'utilisateur : deux grosses
  caisses sur le même pas (un 909 en sample sur « 1 KICK », MD Drums sur « BASS ») ne se calent pas, et MD Drums
  semble ne pas avoir la même latence joué par le séquenceur ou par Live. `measure/live/live-kick-latency.mjs` dans le
  set de l'utilisateur (pistes temporaires « MDSEQ … », supprimées ensuite), 44,1 kHz, 126 BPM. SEQ : `MD Drums Seq`
  dbg17 (démonstration : Track 1 aux pas 1 et 9, Track 2 aux pas 5 et 13) puis MD Drums ; CLIP : MD Drums et un clip
  d'une mesure aux mêmes notes (36 aux temps 1 et 3, 37 aux temps 2 et 4) ; MD Drums sur le Kit 1 d'usine (Track 1 :
  TRX-B2), sortie Main, latence annoncée 51 échantillons. Enregistrés ensemble en Post FX, 2 mesures :
  - Track 1, aux 5 temps : premier échantillon au-dessus de −60 dBFS 2 échantillons après la grille, sur SEQ comme sur
    CLIP ; Track 2 : décalage 0 entre les deux par corrélation, temps par temps ;
  - enveloppes identiques à 1 dB près ; les échantillons diffèrent jusqu'à −9 dBFS, ce que font les voix de deux
    instances (bruit, phases libres), pas le timing ;
  - le sample 909 (Simpler, début 0, sans latence) : au-dessus de −60 dBFS à l'échantillon 5, de −20 dBFS au 107, crête
    au 222 (5 ms) ;
  - TRX-BD pas mesuré dans Live : la LOM ne voit qu'un paramètre de MD Drums, pas de Machine à régler. Le moteur le
    fait sonner 32 échantillons après les autres machines (`mdDrumsEngineTest`), soit 34 après la grille.

  Conclusion : le séquenceur joue MD Drums au même échantillon qu'un clip. Le décalage entendu vient des sons : TRX-BD
  arrive 0,73 ms après la grille sur Main (à MD Drums de le corriger : faire attendre toutes les voix comme TRX-BD et
  annoncer 32 échantillons de plus), et le 909 monte 5 ms jusqu'à sa crête. Le bus « DRUM BUS » (1196 échantillons sur
  le chemin de « 1 KICK ») est compensé par Live tant que ses plug-ins annoncent leur latence ; non mesuré.
- 2026-10-16, **les deux grosses caisses sur Main**, l'utilisateur entendant toujours le kick du séquenceur décalé du
  kick d'un clip. `measure/live/live-three-kicks.mjs` dans le set ouvert (pistes et clip temporaires, supprimés
  ensuite), 44,1 kHz, 126 BPM, Main enregistré en Resampling, une mesure d'écart entre les coups : le 909 de « 1 KICK »
  joué par un clip (172 échantillons de plug-ins sur sa piste, 1024 dans DRUM BUS, désactivés compris), MD Drums joué
  par un clip, MD Drums joué par MD Drums Seq (build de développement au commit 3ec536e, après la refonte du transport ;
  Kit 1 d'usine, Track 1 : TRX-B2). Analyse : `measure/three_kicks.py`, `measure/three_envelopes.py`.
  - premier échantillon au-dessus de −60 dBFS : 909 à 8, MD Drums par clip à 2, par le séquenceur à 1 ; la compensation
    de Live tient pour le 909 et ses 1196 échantillons ;
  - MD Drums par clip et par le séquenceur : mêmes enveloppes à 1 dB près, à 6 dB de leur crête à 3,6 et 3,3 ms ;
  - les deux sons diffèrent : le 909 claque entre 2 et 5 ms (−8,8 dBFS) et s'éteint avant 200 ms ; TRX-B2 commence à
    −22 dBFS, reste vers −16 dBFS et atteint sa crête (−13,6 dBFS) entre 100 et 200 ms, encore à −15,5 dBFS à 300 ms.

  Le séquenceur et un clip jouent MD Drums au même instant, après la refonte comme avant. Ce qui s'entend décalé tient
  aux sons et à leur chaîne : la grosse caisse TRX-B2 d'usine met sa plus grande énergie 100 ms après le temps, le 909
  la sienne dans les 5 premières ms. Le set ouvert n'a pas MD Drums sur BASS (NodalRed2x) : le set où l'utilisateur
  l'entend n'est pas mesuré.
- 2026-10-16, **clip et séquenceur sur la même piste, en même temps**, à la demande de l'utilisateur (son usage : MD
  Drums Seq et MD Drums sur une piste, kick TRX-BD). `measure/live/live-same-track.mjs` : sur « MDSEQ PRINT » (MD Drums
  Seq dbg23 au commit 3ec536e, puis MD Drums), un clip joue la Track 2 (note 37) sur chaque temps pendant que le device
  joue la Track 1 (note 36) sur les mêmes temps ; une seconde piste, « MDSEQ MIDIREC », qui ne joue rien, enregistre ce
  que la première envoie à MD Drums (Post FX). Sur deux mesures, les 9 temps : les notes du device et du clip au même
  instant, 0,00 échantillon d'écart. Ce qui arrive à MD Drums est donc simultané ; un écart audible vient de ce que MD
  Drums fait de chaque note : TRX-BD sonne 32 échantillons après les autres machines (ticket 36 de MD Drums). Le Kit
  d'usine 15 « RESAMPLING » est le seul à avoir TRX-BD (Track 1) ; Kit 1, chargé par défaut, n'en a pas
  (`measure/bank_machines.py`).
- 2026-10-16, **sur la piste de l'utilisateur**, « MD DRUMS SEQ » (la release au commit 3ec536e, puis MD Drums ; Track 1
  passée en TRX-BD par l'utilisateur, son clip sur la Track 2, note 37, à chaque temps). `measure/live/live-user-kicks.mjs`
  enregistre Main trois fois deux mesures (ensemble, clip seul device coupé, device seul) ; `live-trxbd-clip.mjs` y ajoute
  la Track 1 jouée par un clip temporaire, device coupé. Attaques par `measure/attacks.py`, enveloppes par
  `measure/one_envelope.py` :
  - Track 2 (Kit 1 : TRX-SD) par le clip : attaque à +3 échantillons sur les 8 temps ;
  - Track 1 en TRX-BD par le séquenceur : +42 sur les 8 temps ; par le clip : +42 sur les 8 temps, même enveloppe au
    dixième de dB (−22,8 dBFS la première ms, crête −9,6 dBFS entre 20 et 50 ms) ;
  - avant le changement de machine, la Track 1 (TRX-B2) partait à +2, crête entre 100 et 200 ms.

  Le séquenceur joue TRX-BD au même échantillon qu'un clip. TRX-BD sonne 39 échantillons (0,88 ms) après une autre
  machine sur Main, ce que `mdDrumsEngineTest` donne (32 de plus que les autres machines) au découpage du détecteur près :
  c'est le ticket 36 de MD Drums. Le reste de ce qui s'entend décalé tient à l'enveloppe de chaque machine.
- 2026-10-16, **boucle courte à l'échelle lente** (`measure/live/live-loop-run.mjs`, build de développement au commit
  3ec536e, qui a la marge de saut à 30 ticks de 1cf5eb8) : Pattern à 1/8x (un pas de deux temps), Track 1 au pas 1,
  boucle d'arrangement d'un temps, ce que le device envoie enregistré (Post FX) 4 s à 126 BPM : le pas 1 rejoue à chaque
  tour, 9 fois, à 1,000 temps ± 0,002 d'écart. Avec la marge d'un demi-pas (a38d1e0), une boucle plus courte que 480 ticks
  ne passait pas pour un saut et ne rejouait rien. Le nettoyage du script remet la boucle par paliers : Live refuse une
  boucle ou une position au-delà de la longueur du morceau, qu'une boucle courte vient de raccourcir.
- 2026-10-17, **mesure de bout en bout** (Live 12.2.1, 44,1 kHz, 126 BPM ; MD Drums d'adb536eb, latence annoncée
  83 échantillons, que Live compense ; build de développement dbg22 au commit 771f177). Pattern de démonstration :
  Track 1 (TRX-B2) aux pas 1 et 9, Track 2 (TRX-SD) aux 5 et 13, vélocité 127. Trois enregistrements Post FX depuis une
  barre, pistes temporaires supprimées ensuite :
  - **en direct** : MD Drums joué par le device (`live-kick-latency.mjs`, 4 mesures, 18 attaques) ;
  - **depuis un clip imprimé** : le Pattern imprimé par le device (4 Passes, `live-print-run.mjs --play`, la sonde
    `%TEMP%\mdseq-lomprobe.txt` faisant « passes 4 » puis « print »), device coupé, MD Drums joué par les clips
    imprimés (notes aux temps exacts, vélocité 127) ;
  - **référence** : un clip fait à la main, mêmes notes, vélocité 120.

  `onsets.py --bpm 126 --division 4` :
  - au seuil de −20 dBFS (défaut), la caisse claire tombe 2 à 3 échantillons après la grille dans les trois, à 1
    échantillon près d'un coup à l'autre (sur 9 coups en direct, 5 imprimés) ; le kick TRX-B2, qui monte lentement, passe
    ce seuil à des instants qui varient (de −19 à +58 échantillons), pareil dans les trois : le son, pas le moment ;
  - au seuil de −30 dBFS, chaque attaque détectée est exactement 2 échantillons après la grille, dans les trois
    enregistrements, sans aucun écart d'un coup à l'autre.

  `measure/compare.py` (nouveau : la différence la plus grande de deux enregistrements partant de la même barre, et le
  décalage de −8 à +8 échantillons où ils s'accordent le mieux) : décalage 0 pour les trois paires (direct contre
  référence, direct contre imprimé, référence contre imprimé). Les différences de niveau qui restent viennent de la
  vélocité du clip de référence (120 contre 127) et des sons eux-mêmes.

  Conclusion : de bout en bout, le device en direct et ses clips imprimés jouent MD Drums au même échantillon qu'un clip,
  2 échantillons (0,05 ms) après la grille une fois la latence de MD Drums compensée, sans gigue mesurable. Le critère
  de timing de la carte est rempli.

## Answer

Le device joue MD Drums au même échantillon qu'un clip, sans gigue mesurable (2026-10-10), et de bout en bout, en direct comme depuis ses clips imprimés, 2 échantillons après la grille, décalage 0 entre les sources (2026-10-17). Les canaux : Live ramène sur le canal 1 les notes d'un device, d'où l'amendement de l'ADR 0003 de MD Drums (un Lock sur le canal 1 se lie au dernier Hit avant lui).
