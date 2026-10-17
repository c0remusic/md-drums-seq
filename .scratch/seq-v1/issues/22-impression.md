# Impression

Type: task
Status: resolved
Assignee: c0remusic (session du 2026-10-14)
Blocked by: —

## Question

Construire l'impression telle que « Impression et capture » (ticket 07) l'a tranchée : N Passes du Pattern affiché,
un clip Session par Track sur sa piste MIDI routée vers « N-MD Drums », pistes retrouvées par leur routage et créées
si elles manquent, scène sélectionnée ou neuve, bords du clip, ligne d'état.

Pistes de construction (ticket 08, décision 12) : le noyau rend les notes de chaque Track sur N Passes depuis une
graine, testé sous ctest contre le flux du Sequencer (mêmes notes, mêmes temps, en battements) ; l'adaptateur en fait un
`dict` par Track ; le patch parle à la LOM par `live.path` et `live.object` (lecture des routages, `create_midi_track`,
`create_scene`, `create_clip`, `add_new_notes dictionary`), sur le fil principal. À vérifier dans Live : la syntaxe
`add_new_notes` avec un dictionnaire, et que « N-MD Drums » joue la Track N avec ses Locks sur le canal N.

## Comments

- 2026-10-13 : la règle du canal 1 dont dépendent les Locks de la Track 1 est dans MD Drums depuis `73c45ab8`
  (ticket 07, décision 4).
- 2026-10-14, **construit**. Le noyau rend les notes (`mdseqPrint.*`, depuis 4d8de80) et place les clips
  (`mdseqPrintPlan.*`, `mdseqPrintPlanTest`) : piste de la Track N trouvée par sa sortie (piste du device, canal
  « N-… »), la première du set si deux, avec un avertissement ; « MD nn » créée après la piste du device sinon (ce qui
  décale les pistes suivantes) ; scène sélectionnée, ou neuve dessous si un clip gêne sur une piste trouvée ; clip
  « P01 ×4 » ; ligne d'état. L'external lit le set et exécute le plan par `live.path`/`live.object` ; la fenêtre a
  Print, le nombre de Passes (×1-×16, ×4 au départ) et la ligne d'état au-dessus de l'aide des Locks.
- 2026-10-14, **formats LOM mesurés dans Live** (sonde `%TEMP%\mdseq-lomprobe.txt` d'un build de dev) : un chemin
  répond `id n` ; `get` répond `<propriété> <valeurs>`, une propriété de routage `<propriété> dictionary <nom>` ;
  `create_midi_track` et `create_clip` répondent `id n` ; `add_new_notes` prend `dictionary <nom>` (notes
  `{"notes":[{pitch, start_time, duration, velocity}]}`) ; un routage se règle avec le dictionnaire d'une entrée de sa
  liste `available_…` (`{"display_name", "identifier"}`). Une piste MIDI créée par la LOM sort sur « No Output ».
- 2026-10-14, **une course trouvée et corrigée** : juste après le réglage du type de sortie d'une piste créée, sa liste
  de canaux ne contient que « Track In » (deux essais sur quatre : « MD 03 could not be sent »). Live la remplit après
  le retour de l'appel. L'impression se fait donc en deux temps : pistes créées et type de sortie d'abord, puis canal,
  scène et clips 50 ms plus tard, réessayés jusqu'à 2 s ; mesuré ensuite : canal pris au premier essai.
- 2026-10-14, **vérifié dans Live** (`measure/live/live-print-run.mjs --play`, pistes temporaires supprimées ensuite) :
  Pattern de démonstration (Track 1 aux pas 1 et 9, Track 3 aux pas 5 et 13, Lock VOL 0 au pas 5), 2 Passes. Track 1
  sur la piste existante « MDSEQ T1 » (1-MD Drums), Track 3 sur « MD 03 » créée et routée sur 3-MD Drums, clips de 8
  temps en boucle, notes au temps près (Lock 81 de vélocité 1 avec son Hit). Une seconde impression va dans une scène
  neuve sous la sélectionnée. Device coupé, les clips joués et enregistrés depuis la piste de MD Drums : la Track 3
  ne monte pas aux temps 2 et 6 (−2,5 dB, la traîne de la Track 1 seule) et monte de 20 dB aux temps 4 et 8 : le Lock
  passe par le canal 3. La ligne d'état est vue dans la fenêtre (capture `PrintWindow`).
- Reste à voir : le clic sur Print dans le device publié (les essais passent par la sonde, qui appelle la même
  fonction) ; les Locks de la Track 1 sur le canal 1 avec la DLL de MD Drums qui a `73c45ab8` (le Live en cours peut
  garder l'ancienne).
- 2026-10-14, d4466fc : le plan et les deux temps de l'impression sont dans `mdseqPrinter.*` (`Printer` sur `LiveSet`,
  testé par `mdseqPrinterTest` contre un set en mémoire) ; `mdseqPrintPlan.*` et `mdseqPrintPlanTest` sont supprimés,
  leurs cas repris. L'external ne garde que `LomSet` (les formats LOM au-dessus de `lomValue`), la graine et l'horloge
  qui rappelle `Printer::resume`. Statuts nouveaux : « Not printed: Live made no clip in scene N for Track nn » quand
  Live refuse tous les clips (avant : « Nothing to print ») ; « Not printed: Live did not create MD nn » quand
  `create_midi_track` ne répond pas `id n`. Après la revue (ff0b481) : aucun clip quand une piste du plan n'est plus à
  son index au second temps (« Not printed: tracks moved while printing: … ») ; une création refusée après d'autres
  n'écrit aucun clip, mais les pistes déjà créées reçoivent leur canal (« Not printed: Live did not create MD 03. 1
  track added. »).
- Reste à voir aussi : une impression qui crée des pistes ne doit pas afficher « Not printed: Live did not create MD
  nn » (contrôle de l'id rendu par `create_midi_track`, d4466fc) ; une piste glissée pendant l'impression donne « Not
  printed: tracks moved while printing » et aucun clip.
- 2026-10-14, après la seconde revue : une piste trouvée se reconnaît au second temps par sa sortie (le device, sur le
  canal de sa Track), plus par son nom, que Live renumérote (« 2-MIDI », un nom avec « # ») quand une piste est créée
  avant elle ; une piste créée, par son nom « MD nn ». Une piste créée vue ailleurs pendant les essais de canal est
  réessayée, et routée si elle revient ; laissée sur « Track In », la ligne d'état la nomme.
- Reste à voir : une impression qui crée « MD nn » avant une piste trouvée au nom par défaut (« 2-MIDI » routée sur
  1-MD Drums) écrit ses clips (« Printed … »), et la piste est renommée « 3-MIDI ».
- 2026-10-16, **vérifié dans Live** (`measure/live/live-print-auto.mjs`, build de développement au commit 3ec536e,
  pistes temporaires supprimées ensuite) : la piste de la Track 1 garde le nom que Live lui donne (« 8-MIDI »), sortie
  sur 1-MD Drums ; Hits sur les Tracks 1, 2 et 3. L'impression crée « MD 02 » et « MD 03 » après le device sans dire
  « Live did not create » ; la piste trouvée devient « 10-MIDI » et reçoit son clip (Hits de la Track 1, Lock VOL au pas
  9) : « Printed P01 ×2 in scene 3: 3 clips, 2 tracks added ». Le cas qui finissait sur « tracks moved while printing »
  avant `a2b94f4`. Restent à la main : le clic sur Print, une piste glissée pendant l'impression.
