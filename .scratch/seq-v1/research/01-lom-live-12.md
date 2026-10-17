# Ce que LOM permet dans Live 12 : recherche du ticket 01

Recherche du 2026-10-09 pour `issues/01-lom-live-12.md`. Une section par question du ticket ; chacune finit par ce
qu'elle implique pour le device (lookahead, annulation, fils d'exécution, capture).

## Cadre : quelles versions

- **Live installé sur la machine : 12.2.1 Suite**, version lue sur `C:\ProgramData\Ableton\Live 12 Suite\Program\Ableton
  Live 12 Suite.exe`. **Max embarqué : 9.0.7**, version lue sur `Resources\Max\Max.exe`. Les notes de version de Live
  12.2 confirment que Live 12.2 a embarqué Max 9.0.7.
- **Dernière version publique : Live 12.4.6** (15 septembre 2026), qui embarque Max 9.1.5 depuis Live 12.4.5.
- **La référence LOM en ligne de Cycling '74 décrit Live 12.4.5** (elle l'écrit en tête de son index). Certains appels
  qu'elle liste n'existent donc pas dans la 12.2.1 installée : `Track.insert_device` arrive en 12.3, l'ouverture des
  fenêtres de plug-in par LOM en 12.4.3.

Légende utilisée plus bas :

- **[doc]** : documenté par Cycling '74 ou Ableton, avec l'URL dans la liste des sources en fin de fichier.
- **[notes]** : notes de version d'Ableton ou de Cycling '74.
- **[observé]** : constaté dans les fichiers de Live 12.2.1 installés sur la machine, sans lancer Live (aide locale de
  Max, noms présents dans le bytecode du pont Max-Live `_MxDCore`).
- **[communauté]** : rapporté sur les forums ; à vérifier.
- **[déduit]** : raisonnement de ma part à partir des sources citées, pas une source en soi.
- **[inconnu]** : aucune source trouvée.

## 1. Écrire, modifier et effacer des notes dans un clip Session en lecture

### L'API des notes (classe `Clip`, clips MIDI seulement) [doc]

- **`add_new_notes`** prend un dictionnaire dont la clé `notes` contient une liste de notes. Chaque note exige `pitch`
  (entier 0-127), `start_time` (flottant, en temps du clip, absolu) et `duration` (flottant, en temps). Les clés
  optionnelles sont `velocity` (flottant 0-127, 100 par défaut), `mute` (booléen, 0 par défaut), `probability`
  (flottant 0-1, 1 par défaut), `velocity_deviation` (flottant -127 à 127, 0 par défaut : la vélocité jouée est tirée
  entre `velocity` et `velocity + velocity_deviation`, bornée à 0-127) et `release_velocity` (flottant, 64 par
  défaut). L'appel **rend la liste des `note_id`** des notes ajoutées. Disponible depuis Live 11.0.
- **`get_notes_extended`** (`from_pitch`, `pitch_span`, `from_time`, `time_span`, temps en temps musicaux) rend les
  notes dont le **début** tombe dans la zone, sous forme de dictionnaires contenant `note_id`, `pitch`, `start_time`,
  `duration`, `velocity`, `mute`, `probability`, `velocity_deviation` et `release_velocity`. On peut passer un seul
  dictionnaire avec une clé optionnelle `return` qui limite les propriétés rendues, ce que la doc recommande pour les
  performances. Depuis 11.0. Variantes : `get_all_notes_extended` (toutes les notes, quels que soient les marqueurs et
  la boucle, depuis 11.1), `get_notes_by_id` (11.0), `get_selected_notes_extended` (11.0).
- **`remove_notes_extended`** (mêmes quatre paramètres) supprime les notes qui **commencent** dans la zone. Depuis 11.0.
- **`remove_notes_by_id`** supprime les notes d'une liste d'identifiants, qui doivent exister. Depuis 11.0.
- **`apply_note_modifications`** prend `{notes: [...]}` avec des dictionnaires tels que rendus par
  `get_notes_extended`. La liste peut n'être qu'une partie des notes du clip, mais **l'appel entier est ignoré si une
  seule note n'est pas dans le clip**. Depuis 11.0 ; remplace le couple `remove_notes` puis `set_notes`.
- Autres : `duplicate_notes_by_id` (11.1.2), `select_notes_by_id` (11.0.6), `duplicate_region`, `quantize`. Les anciens
  `get_notes`, `set_notes`, `remove_notes` et `replace_selected_notes` ne sont plus décrits que comme remplacés.
- La propriété **`notes`** s'observe : l'observateur émet un bang à chaque changement de la liste de notes (y compris,
  [déduit], les changements que le device fait lui-même).
- Position et boucle : `playing_position` (get/observe, en temps du clip ; 0 si le clip est arrêté), `start_time`
  (pour un clip Session, l'instant de lancement en temps de chanson ; peut être négatif), `loop_start`, `loop_end`,
  `looping`, `start_marker`, `end_marker` (get/set/observe), `length` (get).
- Création d'un clip Session : `ClipSlot.create_clip(length)`, sur un emplacement vide d'une piste MIDI [doc].

Notes de version utiles :

- Live 12.0.5 : les appels avec de gros dictionnaires en entrée, `apply_note_modifications` cité en exemple, sont
  devenus beaucoup plus rapides [notes].
- Max 9.0.1 : `LiveAPI` en JavaScript accepte des objets JS comme arguments de `call()` et `set()` [notes]. On peut donc
  passer `{notes: [...]}` directement depuis `v8`.
- Live 12.2 : `Clip.start_time` devient observable [notes].

### Limites connues

- **Aucune source ne dit si une note ajoutée juste devant la tête de lecture d'un clip en lecture est jouée, ni à quelle
  distance minimale.** Ni la référence LOM, ni le manuel, ni les notes de version n'en parlent [inconnu].
- [communauté] La Live API tourne à basse priorité par conception, avec une latence imprévisible qui dépend de ce que
  fait le reste du set (fil « Getting the notes currently played in a clip / latency »).
- [communauté] L'observateur de `playing_position` n'émet qu'environ toutes les 60 ms, mesure rapportée dans le fil
  « Recording MIDI notes into Live Clip - timing issues ». Dans ce même fil, des notes placées dans un clip en lecture
  d'après `playing_position` tombaient au mauvais endroit ; la position calculée avec `plugsync~` donnait un meilleur
  résultat.
- [doc] `live.object` est conçu pour imiter les gestes de l'utilisateur et s'ajoute à l'historique d'annulation (voir
  la section 3).
- [inconnu] Ce qui arrive quand on supprime ou modifie une note en train de sonner (note-off immédiat ou non), et quand
  on écrit dans le tampon audio que Live est déjà en train de calculer.
- L'ADR 0003 de MD Drums notait déjà, parmi les options écartées, des réglages par LOM sur le fil principal de Live avec
  des dizaines de millisecondes de gigue.

### Ce que ça implique pour le device

L'API couvre tout le besoin : ajout par lots avec identifiants rendus, modification, suppression par identifiant ou par
zone, et toutes les propriétés de note (vélocité 1-127 pour les Hits et les Locks de l'ADR 0003). Mais **le lookahead
minimal n'est documenté nulle part** : c'est au ticket 04 de le mesurer. En attendant :

- écrire par lots, un appel `add_new_notes` par Track et par fenêtre, plutôt qu'un appel par note ;
- garder les `note_id` rendus pour effacer ou modifier sans relire le clip ;
- ne pas déduire la position de la tête de `playing_position` (environ 60 ms de résolution), mais de l'horloge Max
  (section 5) ;
- écrire `probability` à 1 et `velocity_deviation` à 0, pour que le clip rejoue exactement ce que le device a tiré ;
- ignorer, dans l'observateur `notes`, les changements que le device a lui-même provoqués.

## 2. Créer un clip MIDI en Arrangement et y écrire pendant la lecture

- **`Track.create_midi_clip(start_time, length)`** crée un clip MIDI vide dans l'Arrangement, aux temps donnés en temps
  musicaux [doc]. L'appel échoue sur une piste non MIDI ou gelée, hors de l'intervalle [0, 1576800], **ou quand la piste
  est en cours d'enregistrement** [doc]. **Apparu dans Live 12.1.10** (25 février 2025) [notes]. **Live 12.4 corrige un
  crash** qui pouvait survenir en créant des clips Arrangement avec `create_audio_clip` et `create_midi_clip` [notes].
- `TakeLane.create_midi_clip`, `Track.create_take_lane` et l'accès aux clips des take lanes : Live 12.2 [notes ; doc].
- `Track.arrangement_clips` (get/observe, depuis 11.0) liste les clips Arrangement d'une piste ; `Clip.is_arrangement_clip`
  les distingue ; pour un clip Arrangement, `start_time` est sa position et `end_time` son bord droit [doc].
- **`Track.duplicate_clip_to_arrangement(clip, destination_time)`** copie un clip Session (ou Arrangement) dans
  l'Arrangement au temps donné [doc]. La doc ne dit pas depuis quelle version ; le nom est présent dans le pont Max-Live
  de la 12.2.1 installée [observé]. Live 12.3 et 12.4 corrigent chacun un crash de cet appel [notes].
- `Track.delete_clip(clip)` [doc]. Depuis Live 12.3, les identifiants de clip restent stables quand on déplace les
  clips dans l'Arrangement [notes].
- Côté chanson [doc] : `Song.record_mode` (get/set/observe) est le bouton Arrangement Record ; `punch_in`, `punch_out`,
  `loop_start`, `loop_length`, `arrangement_overdub` et `back_to_arranger` sont accessibles ; `Track.back_to_arranger`
  existe par piste.
- Écrire des notes dans un clip Arrangement pendant la lecture passe par la même API `Clip` ; aucune restriction liée à
  la lecture n'est documentée [doc]. Qu'une note écrite juste devant la tête soit jouée reste inconnu, comme en Session
  [inconnu].
- Manuel de Live [doc] : sur une piste, le clip Session et l'Arrangement s'excluent, un seul joue à la fois. Quand
  Arrangement Record est actif, Live consigne les clips lancés et les changements de leurs propriétés, et copie les
  clips lancés dans l'Arrangement aux bonnes positions.
- [inconnu] Si des notes réécrites par LOM dans un clip Session en lecture se retrouvent dans la copie que Live fait en
  Arrangement, et dans quel état (au lancement, à la fin de la prise).
- [inconnu] Si une piste qui joue un clip Session pendant qu'Arrangement Record est actif compte comme « en cours
  d'enregistrement » pour `create_midi_clip`.

### Ce que ça implique pour le device

La capture est possible par trois voies, que le ticket 07 doit départager en les essayant :

1. laisser Live enregistrer lui-même les lancements Session dans l'Arrangement (rien à écrire, mais fidélité inconnue
   pour un clip réécrit en continu) ;
2. créer un clip Arrangement par Track (`create_midi_clip`) et y écrire les notes au fil des Pass ;
3. copier le clip Session dans l'Arrangement à la fin de chaque Pass (`duplicate_clip_to_arrangement`).

Les voies 2 et 3 exigent Live 12.1.10 ou plus récent, ce que la 12.2.1 installée satisfait. Mais le crash de création
de clips Arrangement n'est corrigé qu'en 12.4 : **passer la machine en 12.4.x avant de construire la capture**.

## 3. Historique d'annulation

- [doc] La classe `Song` de la LOM n'offre que `undo`, `redo`, `can_undo` et `can_redo`. **Aucune fonction pour ouvrir
  ou fermer un pas d'annulation** (`begin_undo_step`, `end_undo_step` ou équivalent) n'apparaît dans la référence LOM.
- [observé] `begin_undo_step` et `end_undo_step` existent dans l'API Python de Live : les scripts de surfaces de contrôle
  livrés avec Live s'en servent (par exemple `MIDI Remote Scripts/ableton/v2/control_surface/components/transport.pyc`).
  Mais ces noms sont absents de `MIDI Remote Scripts/_MxDCore/LomTypes.pyc`, qui contient la table des membres exposés
  à Max (`AVAILABLE_TYPE_PROPERTIES`), alors que `undo`, `redo`, `can_undo`, `create_midi_clip` ou `add_new_notes` y
  figurent. Conclusion probable : ces deux fonctions ne sont pas accessibles depuis Max for Live. Non vérifié par un
  appel dans Live.
- [doc] Le guide Max for Live dit que `live.object` imite les interactions de l'utilisateur avec le set, et qu'il
  s'ajoute donc à l'historique d'annulation.
- [doc] `live.remote~` règle un paramètre de device en temps réel sans toucher l'historique d'annulation ni
  l'automation. Mais il ne s'applique qu'aux `DeviceParameter`, pas aux notes.
- [notes] Live 12.4 (Max 9.1.4) permet de rendre des paramètres Max for Live non annulables (attribut « Undo When
  Visible »). Là encore, cela ne concerne que les paramètres.
- [communauté] Andrew Pask (Cycling '74) confirme dans le fil « Undo History » que les actions faites par `live.object`
  vont dans la pile d'annulation de Live. Un utilisateur y rapporte que des actions successives du device, sans geste
  souris ou clavier entre elles, sont fusionnées en une seule entrée d'annulation.
- [inconnu] Le comportement exact de `add_new_notes` et `remove_notes_by_id` en flux continu : une entrée par appel,
  une fusion par paquet, ou une fusion jusqu'au prochain geste de l'utilisateur ? Et ce que fait un Ctrl+Z de
  l'utilisateur pendant la lecture : défait-il les écritures du device, ou un geste de l'utilisateur fusionné avec
  elles ?
- Outil de mesure : la vue **Undo History** (menu View, Ctrl+Alt+Z), apparue dans Live 12.1, liste les pas
  d'annulation [notes].

### Ce que ça implique pour le device

On ne peut ni regrouper explicitement les écritures de notes, ni les tenir hors de l'historique depuis Max for Live. Le
risque n'empêche pas de jouer, mais il touche l'usage : historique pollué, ou Ctrl+Z qui défait l'impression. À
mesurer avec la vue Undo History pendant le ticket 04 ou 06. Écrire moins souvent et par lots réduit le nombre
d'écritures ; reste à voir si cela réduit aussi le nombre d'entrées.

## 4. Fils d'exécution

- [doc] La Live API s'exécute sur le fil principal de Live, et tous les messages vers et depuis l'API sont différés
  automatiquement (pages de référence de `live.object`, `live.path` et `live.observer`).
- [doc] Le moteur JavaScript de Max exécute toujours le code sur le fil basse priorité : un message qui arrive d'un objet
  haute priorité (MIDI, `metro`) est différé vers la file basse priorité (guide JavaScript de Max 9).
- [doc] L'objet `LiveAPI` ne peut être ni créé ni utilisé sur le fil haute priorité ; il faut passer par `defer` ou
  `deferlow`. Il ne peut pas non plus servir dans le code global du script : attendre le bang de `live.thisdevice`, qui
  signale que le device et la Live API sont prêts (référence JS de `LiveAPI`).
- [doc] **On ne peut pas modifier le set depuis une notification**, c'est-à-dire depuis une sortie spontanée d'un
  `live.observer`, de la sortie du milieu de `live.path`, ou d'un callback d'observation. Max affiche alors l'erreur
  « Changes cannot be triggered by notifications ». La doc indique qu'un `deferlow` entre la notification et la
  modification règle souvent le problème (guide « Live API Overview », page de `live.observer`).
- [communauté] `deferlow` est en général plus fiable que `defer` (Tyler Mazaika, fil « you will need to defer your
  response ») ; Andrew Pask (Cycling '74) recommande aussi `deferlow` (fil « live.object Changes cannot be triggered by
  notifications »). Il ne suffit pas toujours : quand la notification arrive déjà du fil basse priorité, un `defer` peut
  ne rien changer, et un délai d'environ 1 ms sert alors de repli (Iain Duncan, fil « Deferlow issues ») ; un autre
  utilisateur n'a réglé le problème qu'en désactivant ses observateurs. Iain Duncan conseille aussi de ne pas observer
  et modifier dans la même chaîne de messages, et de préférer `plugsync~` à l'observation par la Live API quand c'est
  possible.
- [communauté] Dans un device Max for Live hors édition, Max tourne comme avec Overdrive et « Scheduler in Audio
  Interrupt » activés, avec un vecteur de 64 échantillons, non modifiable (fil « Scheduler in Max For Live questions »).
- [notes] Live 12.2 (Max 9.0.6-9.0.7) : correctif amélioré pour les problèmes de callbacks d'observateurs `LiveAPI` dans
  `v8`.
- `v8` et `js` donnent tous deux accès à `LiveAPI` ; `js` garde l'ancien moteur, maintenu pour compatibilité [doc].
- [déduit] `get` et `call` rendent leur valeur tout de suite : l'appel est synchrone vu du JavaScript et occupe le fil
  principal de Live pendant sa durée. Un gros lot coûte donc du temps d'interface.

### Ce que ça implique pour le device

Chaîne à respecter : l'horloge en haute priorité (`plugsync~` ou `transport`), puis `deferlow`, puis `v8`, qui contient
le moteur et fait les appels LOM en basse priorité. Les callbacks d'observateurs (`record_mode`, `is_playing`, `notes`)
ne doivent pas écrire directement : passer par un `deferlow` dans le patch ou par une `Task` en JS. Les écritures
dépendent du fil principal de Live, que le redessin de l'interface et les autres devices occupent aussi : c'est la
raison du lookahead, et le ticket 04 doit mesurer dans ces conditions-là.

## 5. Horloge : suivre le transport depuis Max for Live

- **`plugphasor~`** [doc] : rampe de 0 à 1 par temps, synchronisée à l'échantillon près sur le séquenceur de Live ;
  signal nul quand le transport est arrêté.
- **`plugsync~`** [doc] : sort l'état du transport, la mesure, le temps, la fraction de temps, la signature, le tempo
  (en échantillons par temps), le nombre de temps en 1 PPQ avec partie fractionnaire, le compteur d'échantillons, et un
  masque qui dit quelles sorties sont valides. Sa sortie est continue tant que le scheduler tourne ; un bang l'interroge.
  Max 9.0.7, celui de la 12.2.1 installée, corrige sa sortie dans Max for Live [notes].
- **`transport`** [doc] : dans un device, il est lié au transport de Live ; un bang donne son état, mais il ne peut pas
  piloter le transport de Live. Sa résolution est de 480 PPQ, et la précision finale reste celle du scheduler de Max. Les
  objets `transport` sans nom suivent Live par défaut.
- **`Song.current_song_time`** [doc] : get/set/observe, en temps musicaux. La cadence de son observateur n'est pas
  documentée [inconnu] ; celle de `playing_position`, qui passe par le même mécanisme d'observation, est d'environ 60 ms
  selon une mesure rapportée [communauté]. `Song.is_playing` et `Song.tempo` s'observent aussi [doc].
- `Clip.playing_position` et `Clip.start_time` (observable depuis 12.2) [doc ; notes].
- [communauté] Le scheduler d'un device tourne une fois par vecteur de 64 échantillons : un événement attend la
  frontière suivante, soit au plus environ 1,45 ms de gigue à 44,1 kHz, qui ne s'accumule pas (Iain Duncan, fil
  « metro for syncing to Live transport / quantized MIDI note output »).

### Ce que ça implique pour le device

Suivre la tête en haute priorité avec `plugsync~` (ou `transport` avec `timepoint`/`when`) pour décider quand écrire.
Les écritures visent des temps absolus du clip et ne dépendent donc pas de l'instant exact de l'appel ; la seule
contrainte est d'arriver avant la tête avec la marge mesurée au ticket 04. Pour un clip Session, la position dans le
clip vaut le temps de chanson moins `Clip.start_time`, ramené dans la boucle [déduit]. Les observateurs de
`current_song_time` et `playing_position` ne servent qu'à l'affichage.

## 6. Max embarqué, `v8`, et un module partagé avec Node

- **Live 12.2.1 embarque Max 9.0.7** [observé ; notes]. Live 12.0 et 12.1.x embarquaient Max 8.6 (8.6.0, 8.6.2, puis
  8.6.5), sans `v8` [notes]. **`v8` n'existe donc dans Live qu'à partir de 12.2.** Ensuite : Max 9.0.8 (12.2.7), 9.0.9
  (12.3), 9.0.10 (12.3.5), 9.1.4 (12.4), 9.1.5 (12.4.5) [notes].
- **`v8` et `v8ui`** utilisent le moteur V8, JavaScript ES6 et au-delà [doc]. L'aide locale de `v8` dans Max 9.0.7
  annonce **V8 10.1.69, soit le langage de Chrome 101 et de Node 18 (ES2022 plus une partie d'ES2023)** [observé]. Max
  9.2.0, version autonome, passe V8 en 14.6 et ajoute `setTimeout`, `setInterval` et des API réseau de style Node
  [notes] ; aucune version de Live, jusqu'à 12.4.6, ne l'embarque encore.
- `js` et `jsui` gardent le moteur Mozilla 1.8.5 (ES5), maintenu pour compatibilité [doc].
- [doc] Le guide JavaScript livré avec Max 9.0.7 dit que `setTimeout` et `setImmediate` n'existent pas et qu'il faut
  passer par `Task`.
- **`require`** [doc] : `v8` inclut d'autres fichiers JavaScript avec `require` ; **le fichier inclus doit être un module
  CommonJS**, qui exporte par `exports` ou `module.exports`. Max 9.0.1 a amélioré ce support [notes].
- **Modules ES** : `import` ne fonctionne pas dans `v8` « at this time », alors que `require` (CommonJS) et `include`
  fonctionnent, selon Joshua Kit Clayton sur le forum de Cycling '74, vers mars 2025 [communauté]. `node.script`
  accepte les modules ES, mais il fait tourner Node dans un processus séparé [doc Max 8].
- [notes] Max 8.0.8 a corrigé `require` dans un device gelé pour `js` ; pour `v8`, rien n'est documenté [inconnu]. Un
  fil du forum rapporte des `require` qui ne marchaient qu'avec l'éditeur ouvert [communauté]. La résolution exacte des
  chemins de `require` (dossier du projet, chemin de recherche de Max) n'est pas documentée non plus [inconnu].

### Ce que ça implique pour le device

Le plan tient : écrire le moteur en **CommonJS** (`module.exports`), le charger par `require` dans `v8` et dans les
tests Node. Contraintes à poser dès le ticket 08 :

- syntaxe limitée à **ES2022** : Node 26, installé sur la machine, accepte plus que V8 10.1 ; un lint réglé sur
  `ecmaVersion: 2022` attrape l'écart ;
- aucun global de Node (`process`, `Buffer`, `setTimeout`) ni de Max (`post`, `outlet`, `Dict`, `LiveAPI`) dans le
  module pur ;
- pas de `"type": "module"` dans le `package.json` ;
- Live 12.2 au minimum pour `v8` ;
- vérifier tôt que `require` trouve le module dans un device gelé.

## 7. Mise en place : pistes et routage par LOM

- `Song.create_midi_track(index)` crée une piste MIDI (-1 pour la fin de la liste) [doc]. `Track.name` et
  `Track.color` se règlent [doc].
- **`Track.output_routing_type` et `Track.output_routing_channel`** sont en get/set/observe ; les valeurs admises sont
  celles de `available_output_routing_types` et `available_output_routing_channels`, sous forme de dictionnaires
  `display_name`/`identifier` [doc]. Même chose pour `input_routing_type` et `input_routing_channel`.
- Le manuel de Live décrit l'usage d'un instrument multitimbral : chaque piste MIDI qui alimente une partie met son
  Output Type sur la piste qui contient l'instrument et son Output Channel sur le canal MIDI visé [doc]. **La sortie
  d'une piste MIDI vers MD Drums sur le canal N se règle donc par LOM, sans External Instrument** [déduit de ces deux
  documents ; les libellés exacts sont à lire dans `available_output_routing_channels`].
- **External Instrument : la LOM n'a pas de classe pour lui.** Sa liste de classes (référence 12.4.5) ne le contient
  pas. Un `Device` générique n'expose que ses paramètres automatisables, et la classe `DeviceIO`, qui porte un routage,
  n'existe que pour un `MaxDevice` (ses `audio_inputs`, `audio_outputs`, `midi_inputs` et `midi_outputs`) [doc]. **Les
  choix MIDI To et Audio From d'un External Instrument ne se règlent donc pas par LOM** [déduit de la doc]. Sur le forum
  de Cycling '74, la question a été posée (« LiveAPI: discovering properties of External Instrument ») sans réponse
  [communauté].
- `Track.insert_device(nom, index)` insère un device natif par son nom ; ni les devices Max for Live ni les plug-ins ne
  sont acceptés. L'appel n'existe que **depuis Live 12.3** [doc ; notes], donc pas dans la 12.2.1 installée, où son nom
  est absent du pont Max-Live [observé].
- Retour audio : une piste audio peut prendre en entrée une sortie auxiliaire de MD Drums, puisque
  `input_routing_type` et `input_routing_channel` se règlent par LOM [doc ; libellés à vérifier].
- `live.routing` ne règle que les entrées et sorties du device Max for Live lui-même [doc].

### Ce que ça implique pour le device

LOM peut créer les 16 pistes MIDI et envoyer leur MIDI vers MD Drums sur le canal N, mais **il ne peut pas régler un
External Instrument**. La mise en place cadrée (un External Instrument par Track, réglé automatiquement) n'est donc pas
automatisable telle quelle. Options pour le ticket 05 :

- **(a)** un modèle de set prêt, où les External Instrument sont réglés à la main une fois ;
- **(b)** le routage de sortie des pistes MIDI vers le canal N par LOM, avec 16 pistes audio dont l'entrée prend Out N,
  elle aussi par LOM ;
- **(c)** une mise en place manuelle guidée par une checklist.

## 8. Affichage

- **La hauteur de tout device dans Live est fixe : 169 pixels** [doc, guide « User Interfaces in Max for Live »]. La
  largeur est fixe (View > Set Device Width) ou suit le contenu du patch ; le mode Présentation sépare l'interface de la
  logique.
- **Fenêtre flottante** : Max documente l'ouverture et la fermeture d'une fenêtre de sous-patch avec `pcontrol`, et le
  passage d'une fenêtre au premier plan avec le message `front` de `thispatcher` [doc]. Dans Live, l'usage est rapporté
  par la communauté dans le fil « Floating Window? » [communauté] : `window flags float` puis `window exec` à
  `thispatcher` fonctionnent chez certains (avec `front` et `presentation 1`), d'autres n'y arrivent pas ; la fenêtre
  peut passer derrière Live, le message `topmost` marche chez l'un et pas chez l'autre, et deux participants trouvent
  peu fiable de changer des fenêtres pendant que le transport de Live tourne.
- **Objets d'interface** [doc] :
  - `live.grid` : grille de pas avec mode matrice, contraintes et directions, conçue pour piloter `chucker~` ;
  - `live.step` : séquenceur pas à pas qui affiche plusieurs séquences de hauteur, vélocité et durée, plus deux valeurs
    libres ;
  - les objets `live.*` usuels : `live.dial`, `live.numbox`, `live.tab`, `live.text`, `live.toggle`, `live.menu`,
    `live.slider`, entre autres ;
  - `jsui`, avec l'ancien moteur, et `v8ui`, avec le moteur V8, offrent dessin et souris ; `v8ui` reçoit le clavier
    (`onkeydown`) depuis Max 9.1.
- **Thème** [doc] : dans Max for Live, les couleurs dynamiques sont activées par défaut pour la plupart des objets
  Max for Live, et celles de la catégorie Live suivent le thème choisi dans les préférences de Live. `live.colors` rend
  la valeur RGBA de chaque couleur du thème et émet un bang quand le thème change : un `v8ui` peut donc dessiner aux
  couleurs de Live.

### Ce que ça implique pour le device

Les 169 pixels sont confirmés. Seize Tracks de 64 pas n'y tiennent pas de façon lisible : il faudra une vue condensée
dans le device, plus une fenêtre flottante pour l'édition détaillée, à valider en maquette (tickets 09 et 10).
`v8ui` avec `live.colors` permet une grille dessinée aux couleurs du thème de Live ; `live.grid` et `live.step`
existent mais sont pensés pour d'autres usages.

## Ce qui contredit ou fragilise le plan

1. **Écriture en avance dans des clips en lecture.** Rien ne l'interdit, mais rien ne garantit le délai entre l'appel
   et le moment où Live peut jouer la note. Le fil principal de Live impose une gigue que la communauté chiffre en
   dizaines de millisecondes : le lookahead doit être mesuré (ticket 04). Un geste joué en direct (Fill, mute) qui
   arrive moins d'un lookahead avant son pas sera en retard (ticket 06).
2. **Annulation.** Les écritures LOM entrent dans l'historique d'annulation de Live, et Max for Live n'a aucun moyen de
   les regrouper ou de les en exclure. Ce n'est pas bloquant pour jouer, mais c'est un risque d'usage à mesurer.
3. **Capture en Arrangement.** Elle est possible (`create_midi_clip` depuis 12.1.10, `duplicate_clip_to_arrangement`).
   Mais `create_midi_clip` refuse une piste en cours d'enregistrement, et son crash n'est corrigé qu'en 12.4. On ignore
   aussi comment elle cohabite avec l'enregistrement Session vers Arrangement que Live fait seul.
4. **Module JS partagé avec Node.** Le plan tient en CommonJS, pas en modules ES ; la syntaxe doit rester en ES2022, et
   `v8` exige Live 12.2 ou plus, ce que la 12.2.1 installée satisfait.
5. **External Instrument.** Il ne se règle pas par LOM : la mise en place automatique cadrée n'est pas possible telle
   quelle. Le routage de piste par LOM peut le remplacer.
6. **Versions.** La machine est en 12.2.1 ; la doc LOM décrit 12.4.5. Passer en 12.4.x ferait coïncider la doc et la
   machine, et apporterait les correctifs de crash et de lenteur de l'API (12.3.2, 12.4).

## Sources

Cycling '74, documentation (en ligne ; les pages du guide sont aussi livrées avec Live dans
`Resources\Max\resources\docs`) :

- Référence LOM, index (Live 12.4.5) : https://docs.cycling74.com/apiref/lom/
- Clip : https://docs.cycling74.com/apiref/lom/clip/
- ClipSlot : https://docs.cycling74.com/apiref/lom/clipslot/
- Track : https://docs.cycling74.com/apiref/lom/track/
- TakeLane : https://docs.cycling74.com/apiref/lom/takelane/
- Song : https://docs.cycling74.com/apiref/lom/song/
- Device : https://docs.cycling74.com/apiref/lom/device/
- MaxDevice : https://docs.cycling74.com/apiref/lom/maxdevice/
- DeviceIO : https://docs.cycling74.com/apiref/lom/deviceio/
- `LiveAPI` en JavaScript : https://docs.cycling74.com/apiref/js/liveapi/
- Guide « Live API Overview » : https://docs.cycling74.com/userguide/m4l/live_api_overview/
- Guide « Creating Devices that use the Live API » : https://docs.cycling74.com/userguide/m4l/live_api/
- Guide « User Interfaces in Max for Live » : https://docs.cycling74.com/userguide/m4l/live_userinterfaces/
- Guide « Timing and Synchronization in Max for Live » : https://docs.cycling74.com/userguide/m4l/live_timing/
- Guide JavaScript : https://docs.cycling74.com/userguide/javascript/
- Guide Transport : https://docs.cycling74.com/userguide/transport/
- Guide Scheduler : https://docs.cycling74.com/userguide/scheduler/
- Guide Dynamic Colors : https://docs.cycling74.com/userguide/dynamic_colors/
- Références d'objets : https://docs.cycling74.com/reference/live.object, https://docs.cycling74.com/reference/live.observer,
  https://docs.cycling74.com/reference/live.path, https://docs.cycling74.com/reference/live.routing,
  https://docs.cycling74.com/reference/plugsync~/, https://docs.cycling74.com/reference/plugphasor~/,
  https://docs.cycling74.com/reference/v8/, https://docs.cycling74.com/reference/v8ui/,
  https://docs.cycling74.com/reference/live.colors, https://docs.cycling74.com/reference/live.grid,
  https://docs.cycling74.com/reference/live.step, https://docs.cycling74.com/reference/pcontrol/,
  https://docs.cycling74.com/reference/thispatcher/
- `node.script` et modules ES (Max 8) : https://docs.cycling74.com/max8/vignettes/12_n4m_esmodules

Notes de version :

- Live 12 (12.0 à 12.4.6) : https://www.ableton.com/en/release-notes/live-12/
- Max 8.0.8 : https://cycling74.com/releases/max/8.0.8
- Max 9.0.0 : https://cycling74.com/releases/max/9.0.0
- Max 9.0.1 : https://cycling74.com/releases/max/9.0.1
- Max 9.0.7 : https://cycling74.com/releases/max/9.0.7
- Max 9.1.0 : https://cycling74.com/releases/max/9.1.0
- Max 9.2.0 : https://cycling74.com/releases/max/9.2.0

Manuel de Live 12 :

- Routing and I/O, instruments multitimbraux : https://www.ableton.com/en/manual/routing-and-i-o/
- External Instrument : https://www.ableton.com/en/manual/live-instrument-reference/
- Recording Sessions into the Arrangement : https://www.ableton.com/en/manual/session-view/
- Recording New Clips : https://www.ableton.com/en/manual/recording-new-clips/

Communauté (forum de Cycling '74) :

- Undo History : https://cycling74.com/forums/undo-history
- Recording MIDI notes into Live Clip - timing issues : https://cycling74.com/forums/recording-midi-notes-into-live-clip-timing-issues
- Getting the notes currently played in a clip / latency : https://cycling74.com/forums/getting-the-notes-currently-played-in-a-clip-latency
- Poor performance of LiveAPI in JS object : https://cycling74.com/forums/poor-performance-of-liveapi-in-js-object
- live.object Changes cannot be triggered by notifications : https://cycling74.com/forums/live-object-changes-cannot-be-triggered-by-notifications
- you will need to defer your response : https://cycling74.com/forums/you-will-need-to-defer-your-response
- Deferlow issues : https://cycling74.com/forums/deferlow-issues
- Scheduler in Max For Live questions : https://cycling74.com/forums/scheduler-in-max-for-live-questions-overdrive-audio-interrupt-task-objects-etc
- metro for syncing to Live transport / quantized MIDI note output : https://cycling74.com/forums/metro-for-syncing-to-live-transport-quantized-midi-note-output
- v8 : import et require : https://cycling74.com/forums/v8-do-the-javascript-import-and-require-statements-work-as-intended-with-v8
- Max for Live device, require() sans l'éditeur ouvert : https://cycling74.com/forums/max-for-live-device-javascript-require-doesnt-work-without-editor-open
- LiveAPI: discovering properties of External Instrument : https://cycling74.com/forums/liveapi-discovering-properties-of-external-instrument
- Floating Window? : https://cycling74.com/forums/floating-window-2

Observé sur la machine (Live 12.2.1 Suite, `C:\ProgramData\Ableton\Live 12 Suite`) :

- versions des binaires `Program\Ableton Live 12 Suite.exe` (12.2.1) et `Resources\Max\Max.exe` (9.0.7) ;
- aide de `v8` : `Resources\Max\resources\help\max\v8.maxhelp` (V8 10.1.69) ;
- noms exposés à Max : `Resources\MIDI Remote Scripts\_MxDCore\LomTypes.pyc` ; usage de `begin_undo_step` :
  `Resources\MIDI Remote Scripts\ableton\v2\control_surface\components\transport.pyc`.
