# Ce que LOM permet dans Live 12

Type: research
Status: resolved
Assignee: —
Blocked by: —

## Question

Le device écrit en avance de la tête de lecture dans des clips que Live joue (cadrage du 2026-10-08). Que permet le
Live Object Model de Live 12 (Suite installée sur la machine), depuis Max for Live, pour ça ?

- Écrire, modifier et effacer des notes dans un clip Session **en cours de lecture**, en avance de la tête : API
  (`add_new_notes`, `get_notes_extended`, `remove_notes_extended`, `apply_note_modifications`, identifiants de note),
  propriétés de note (vélocité, `probability`, `velocity_deviation`, `release_velocity`, `mute`), limites connues
  (une note ajoutée juste devant la tête est-elle jouée ?).
- Créer un clip MIDI en **Arrangement** depuis M4L et y écrire des notes pendant la lecture ; dans quelle version de
  Live 12 ces appels sont apparus.
- **Historique d'annulation** : chaque écriture LOM y crée-t-elle une entrée ? Peut-on les regrouper ou les éviter ?
- **Fils d'exécution** : d'où un appel LOM peut partir (JS `v8`/`js`, `live.object`), sur quel fil il s'exécute, ce
  qui est interdit depuis une notification ou le fil haute priorité, `deferlow`.
- **Horloge** : comment suivre la position du transport depuis M4L (`plugphasor~`, `plugsync~`, `transport`,
  `current_song_time`) et à quelle résolution.
- **Max embarqué** dans Live 12 : version, disponibilité de `v8`, version de JavaScript, et comment partager un module
  entre `v8` et Node (pour les tests du moteur).
- **Mise en place** : LOM peut-il créer les 16 pistes MIDI et régler leur routage de sortie (vers la piste de MD Drums,
  canal N) ? Un External Instrument se règle-t-il par LOM ?
- **Affichage** : hauteur d'un device dans la chaîne (169 px ?), ouverture d'une fenêtre flottante depuis un device,
  objets d'UI disponibles (`live.grid`, `jsui`, `v8ui`, `live.*` et le thème de Live).

Sources : référence LOM de Cycling '74 pour Live 12, notes de version d'Ableton, documentation de Max 9 ; distinguer
ce qui est documenté de ce qui est rapporté par la communauté.

## Answer

La machine est en Live 12.2.1 avec Max 9.0.7 embarqué ; la référence LOM en ligne décrit Live 12.4.5. L'API `Clip`
couvre l'écriture dans un clip en lecture (`add_new_notes` rend les `note_id`, suppression par identifiant ou par zone,
`apply_note_modifications`, toutes les propriétés de note), mais aucune source ne dit à quelle distance devant la tête
une note écrite est encore jouée : le ticket 04 doit mesurer le lookahead. Les appels LOM passent par le fil principal
de Live (jamais depuis la haute priorité, jamais depuis une notification, `deferlow` entre les deux). Ils entrent dans
l'historique d'annulation, sans moyen de les regrouper ou de les exclure depuis Max for Live. `Track.create_midi_clip`
(Live 12.1.10) et `duplicate_clip_to_arrangement` permettent la capture en Arrangement ; le crash de création n'est
corrigé qu'en 12.4. `v8` (V8 10.1, ES2022) charge des modules CommonJS par `require`, pas de modules ES. Un External
Instrument ne se règle pas par LOM, mais le routage de sortie d'une piste MIDI vers le canal N, si. Un device fait 169
pixels de haut. Détail et sources : `research/01-lom-live-12.md`.
