# Paramètres de MD Drums visibles par LOM : recherche du ticket 12

Recherche du 2026-10-09 pour `issues/12-parametres-md-drums-lom.md`. Une section par question du ticket, puis ce que
cela implique pour le device. Cadre des versions (voir `research/01-lom-live-12.md`) : Live 12.2.1 Suite et Max 9.0.7
sur la machine ; dernière version publique Live 12.4.6. Le dépôt gearmulator-md-drums a été lu sans être modifié.

Légende :

- **[doc]** : documenté par Cycling '74 ou Ableton (URL dans les sources en fin de fichier).
- **[notes]** : notes de version d'Ableton.
- **[repo]** : lu dans les fichiers de gearmulator-md-drums (chemin donné).
- **[observé]** : constaté sur la machine sans lancer Live (fichiers de sets `.als`, pont Max-Live installé).
- **[communauté]** : rapporté sur les forums ; à vérifier.
- **[déduit]** : raisonnement de ma part à partir des sources citées.
- **[inconnu]** : aucune source trouvée.

## 1. Quels paramètres d'un VST3 un `PluginDevice` expose par LOM

### Ce que disent les sources

- **[doc]** La classe `Device` porte la liste `parameters` (get, observe) avec cette restriction : seuls les paramètres
  automatisables sont accessibles. `PluginDevice` hérite de `Device` et n'ajoute que `is_editor_open` (get, set,
  observe ; ouverture de la fenêtre du plug-in, arrivée en Live 12.4.3 selon les notes de version), `presets` (get,
  observe) et `selected_preset_index` (get, set, observe). La référence LOM ne parle ni du mode Configure, ni d'une
  limite de nombre.
- **[observé]** Dans le pont Max-Live de la 12.2.1 installée (`_MxDCore/LomTypes.pyc`), `presets` et
  `selected_preset_index` figurent, `is_editor_open` non : cohérent avec son arrivée en 12.4.3.
- **[doc]** Manuel de Live 12, « Plug-Ins in the Device View » : un plug-in qui a jusqu'à 64 paramètres modifiables
  voit tous ses paramètres dans le panneau de Live ; au-delà de 64, le panneau s'ouvre vide et l'utilisateur choisit
  les paramètres à montrer.
- **[doc]** Manuel de Live 12, « Plug-In Configure Mode » : le bouton Configure de l'en-tête du device fait entrer dans
  ce mode ; un clic sur un paramètre dans la fenêtre du plug-in l'ajoute au panneau (pour certains plug-ins, il faut en
  changer la valeur) ; certains plug-ins ne publient pas tous leurs paramètres. Les paramètres choisis sont propres à
  chaque instance et sauvés avec le set. Sans entrer en Configure : toucher un paramètre dans la fenêtre du plug-in crée
  une entrée **temporaire** dans les sélecteurs d'automation et d'enveloppe, effacée dès qu'on touche un autre
  paramètre, rendue permanente si on édite son automation ; l'automation enregistrée et le mode de mapping MIDI, clavier
  ou Macro ajoutent aussi les paramètres touchés au panneau.
- **[doc]** Base de connaissances, « Options.txt file » : l'option non officielle `-_PluginAutoPopulateThreshold=X`
  remplit d'office le panneau d'un plug-in qui a au plus X paramètres ; défaut 64, minimum 1, **maximum 128** ; la
  valeur 128 remplit toujours le panneau avec au plus 128 paramètres, quel que soit leur nombre. L'article prévient
  que ces options sont expérimentales et peuvent disparaître sans avertissement.
- **[doc]** Base de connaissances, « Saving plug-in parameter configurations » : au-delà de 64 paramètres, il faut
  configurer à la main ; un clic droit sur la barre de titre du plug-in, « Save as Default Configuration », fait de la
  configuration le défaut de ce plug-in (elle ne sauve pas son état). Les notes de Live 9.5 situent ces configurations
  dans `User Library/Defaults/Plug-In Configurations` [notes]. Ce dossier n'existe pas encore sur la machine
  [observé].
- **[notes]** Live 10.1 : les plug-ins VST3 de moins de 65 paramètres sont configurés d'office à l'instanciation.
  L'article « Push 1: Using Third Party Plug-Ins » cite encore un seuil de 32, qui était l'ancien défaut [doc,
  ancien].
- **[déduit]** Dans Live, un paramètre de plug-in n'est « automatisable » que s'il figure dans le panneau : le manuel
  ne le rend disponible dans les sélecteurs d'automation qu'une fois configuré (ou temporaire). Avec la règle de la
  référence LOM, **la liste `parameters` d'un `PluginDevice` ne contient que les paramètres configurés**, pas les 576 de
  MD Drums. C'est ce que rapporte un utilisateur dans une demande de fonction du forum d'Ableton (29 mai 2026) : tant
  qu'un paramètre de VST3 ou d'AU n'est pas ajouté en Configure, il est invisible pour la LOM, et aucune API ne permet
  de lister ni de configurer les paramètres d'un plug-in [communauté]. L'ADR 0003 de MD Drums l'affirme déjà (« Live
  exposes only configured parameters to LOM, not 576 ») [repo]. Un message non daté du forum de Cycling '74 dit au
  contraire avoir lu les noms des paramètres d'un VST « sans configurer », sans dire de quel plug-in il s'agissait
  (probablement un plug-in sous le seuil de 64) [communauté].
- **[doc]** Aucune fonction de la LOM n'ajoute un paramètre au panneau d'un plug-in : `Device` et `PluginDevice` n'en
  ont pas. La configuration reste un geste de l'utilisateur.
- **[inconnu]** Si les entrées temporaires (paramètre touché dans la fenêtre du plug-in) apparaissent dans
  `parameters`.
- **[communauté]** Le premier élément de `parameters` de tout device est l'interrupteur « Device On » ; usage courant,
  non vérifié ici.

### Ce qui a changé au fil de Live 12

Rien n'est documenté entre 12.0 et 12.4.6 sur le seuil de remplissage, le mode Configure ou l'exposition des
paramètres de plug-in à la LOM [notes]. Changements voisins : `DeviceParameter.display_value` arrive en 12.2 ; Live 12.4
corrige les noms de paramètres de plug-in automatisés qui n'étaient pas restaurés au chargement d'un set ; Live 12.4.3
ouvre et ferme les fenêtres de plug-in par la LOM ; Live 12.3 rend plus fluide l'interface des VST3 quand leurs
paramètres changent [notes].

### Ce que ça implique pour le device

MD Drums a 576 paramètres, bien plus que 64 : dans un set neuf, son panneau est vide et **la LOM ne voit aucun de ses
paramètres** tant que l'utilisateur n'en a pas configuré. Le device ne peut pas le faire à sa place.

## 2. Ce que donne un `DeviceParameter` pour un paramètre de plug-in

### Propriétés de la classe [doc]

| Nom | Accès | Ce qu'il donne |
|---|---|---|
| `name` | get | nom court, celui du sélecteur d'automation fermé |
| `value` | get, set, observe | valeur interne, entre `min` et `max` |
| `min`, `max` | get | bornes de `value` |
| `display_value` | get, set, observe | valeur telle que l'interface l'affiche (depuis Live 12.2 [notes]) |
| `is_quantized` | get | 1 pour les booléens et les listes, 0 pour les entiers et flottants |
| `value_items` | get | les valeurs possibles, seulement si `is_quantized` vaut 1 |
| `default_value` | get | valeur par défaut, seulement si `is_quantized` vaut 0 |
| `is_enabled`, `state`, `automation_state` | get (observe pour les deux derniers) | modifiable ou non, actif ou non, automation |
| `str_for_value(value)` | call | texte d'une valeur donnée |
| `__str__` | call | texte de la valeur courante |

Tous ces noms figurent dans le pont Max-Live de la 12.2.1 installée, `display_value` compris [observé].

### Nom

- **[repo]** Le nom qu'un paramètre de MD Drums donne à l'hôte est « Track n » ou « Master », une espace, puis son
  `displayName` : `jucePluginLib/parameter.cpp` (constructeur, ligne 35) et le formateur de
  `mdDrumsController.cpp` (lignes 70-73). Exemples : « Track 1 Machine », « Track 16 SYN8 », « Track 3 LFO Track »,
  « Master Echo TIME ». Le test `mdDrumsProcessorTest.cpp` (lignes 217-224) vérifie « Track 1 Machine » en premier et
  « Master Echo TIME » en 545ᵉ position.
- **[repo]** Le wrapper VST3 de JUCE passe ce nom comme titre (128 caractères) et en coupe un titre court de 8
  caractères (« Track 1 », illisible) : `JUCE/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp`,
  `updateParameterInfo`.
- **[observé]** Live garde le titre complet : dans un set 12.2.1 de la machine
  (`Documents/Ableton/User Library/PROJECTS/test.als`), une instance de Gearmulator MD, plug-in de la même famille,
  stocke des noms comme « Track 15 Machine Parameter 1 ». `name` devrait donc rendre « Track n Machine » [déduit].

### Plage de valeurs

- **[repo]** Le wrapper VST3 de JUCE expose chaque paramètre en valeur normalisée 0-1, et sa valeur « plain » est la
  même (`toPlain` rend la valeur telle quelle).
- **[observé]** Dans le set cité, tous les paramètres de plug-in (9 instances VST2 et VST3) sont stockés en flottants
  0-1 (`MidiControllerRange` et `LastInternalRange` de 0 à 1). Un paramètre discret 0-127 de Gearmulator MD y vaut
  0.692913413, soit 88/127.
- **[déduit]** Pour MD Drums, `min` vaut 0, `max` vaut 1 et `value` est normalisée : entier = arrondi de
  `value × max_json` (191 pour la Machine, 127 pour les 24 paramètres). À confirmer en lisant `min` et `max` au premier
  essai [inconnu tant que non lu dans Live].
- **[communauté]** Les réponses du forum de Cycling '74 traitent les paramètres de plug-in comme des échelles 0-1.

### Texte

- **[doc]** `str_for_value` et `__str__` rendent le texte d'une valeur.
- **[repo]** Pour un VST3 JUCE, l'hôte obtient ce texte du plug-in lui-même (`Param::toString`, qui appelle
  `getText`). Dans MD Drums :
  - la Machine rend le nom de la liste `machines` du JSON (« TRX-BD », « EFM-BD », ou « #6 » pour un identifiant que
    le Machinedrum n'a pas) ;
  - SYN1-8 rendent le nom du paramètre selon la Machine courante de la Track, suivi de la valeur (« PTCH 64 ») ; « -- »
    suivi de la valeur pour une case sans nom ; la valeur seule si la Machine n'a pas de table de noms
    (`mdDrumsController.cpp`, `DrumsParameter::getText`, lignes 44-58). Le test du processeur vérifie que SYN1 commence
    par « PTCH » sur EFM-BD (lignes 245-250) ;
  - les autres rendent la valeur, ou Off/On, Main/Out, les listes du LFO.
- **[déduit]** `__str__` sur SYN1-8 donnerait donc le nom par Machine, mais seulement pour des SYN configurés. Les
  mêmes noms sont dans le dépôt : `md::machines::parameterNames` et sa table `g_mdParameterNames`
  (`source/elektron/md/mdProtocol/mdmachines.cpp`, table de l'OS 1.63).
- **[communauté]** Pour un paramètre continu, `str_for_value` ne marche pas toujours ; un utilisateur s'est rabattu
  sur `__str__` après avoir posé chaque valeur, ce qui écrit (fil « LOM devices parameters value unit »).
- **[inconnu]** Si Live met en cache le texte d'un paramètre de plug-in ou le redemande à chaque appel ; ce que
  `display_value` rend pour un paramètre de plug-in.

### `is_quantized`

- **[repo]** MD Drums déclare tous ses paramètres discrets ; JUCE en fait un `stepCount` VST3 : 191 pour la Machine,
  127 pour les paramètres 0-127, 1 pour Mute, Solo et Out, 15, 23, 5 et 2 pour les listes du LFO
  (`juce_audio_plugin_client_VST3.cpp`, lignes 864-872 ; `jucePluginLib/parameter.cpp`, intervalle 1).
- **[inconnu]** Si Live traduit ce `stepCount` en `is_quantized` = 1, avec `value_items` et une valeur en index.
  Indices contraires : le set stocke tous les paramètres de plug-in comme `PluginFloatParameter`, discrets compris
  [observé] ; selon le forum, le type dépend du format et un VST2 ne rend jamais `is_quantized` = 1 [communauté].
- **[déduit]** Le device doit donc lire `is_quantized`, `min` et `max` avant de convertir, ou passer par le texte :
  `__str__` sur la Machine rend son nom, que la liste `machines` ramène à l'identifiant.

### Valeur par défaut

`default_value` n'existe que pour un paramètre non quantifié [doc]. Celle que MD Drums déclare (Machine : 16, TRX-BD)
n'est pas la valeur du Kit : au démarrage, la Track 1 porte TRX-B2, identifiant 28, celui du Kit d'usine 1 [repo, test
du processeur, lignes 226-229]. Seule `value` donne le Kit.

### Observer sans écrire

- **[doc]** `live.observer` écoute une propriété et émet sa valeur à chaque changement ; il ne modifie rien. La Live
  API tourne sur le fil principal de Live, et tous les messages vers elle et depuis elle sont différés. On ne peut pas
  modifier le set depuis une notification. Observer `value` d'un paramètre de MD Drums est donc une lecture pure.
- **[inconnu]** La cadence des notifications d'un paramètre n'est pas documentée. Repères [communauté] : environ 60 ms
  pour `playing_position` (voir la recherche 01) ; un utilisateur a préféré interroger `__str__` toutes les 50 ms
  parce que l'observation lui donnait des erreurs de récursion.
- **[repo]** Les valeurs du Kit sont celles des paramètres hôte : les Locks ne les changent jamais (ADR 0003, section
  « Consequences »). Lire `value` donne bien la valeur du Kit, celle que le fantôme doit montrer.

### Quand Live apprend-il un changement fait dans MD Drums ?

D'après le code de MD Drums et de JUCE [repo, déduit] :

- un réglage fait dans l'éditeur de MD Drums (Machine, bouton) part avec un geste et un `performEdit` vers l'hôte :
  `value` suit ;
- le chargement d'un état (set, preset) passe par `Processor::notifyHostOfProgramChange`, qui renvoie la valeur de
  chaque paramètre puis demande à l'hôte de tout relire (`restartComponent(kParamValuesChanged)`) ;
- **trou probable** : quand la Machine change, le Device remet SYN1-8 aux valeurs de la nouvelle Machine et les renvoie
  par `Controller::parseControllerMessage` avec l'origine `PresetChange` (`mdDrumsController.cpp`, ligne 213).
  `Parameter::setValueFromSynth` ne prévient pas l'hôte pour cette origine et compte sur un `updateHostDisplay` de
  l'appelant (`parameter.cpp`, lignes 317-342), que `parseControllerMessage` ne fait pas. Live garderait alors les
  anciennes valeurs de SYN1-8, dans son panneau comme dans la LOM, jusqu'au prochain chargement d'état. À vérifier
  dans Live ; la correction, si elle est nécessaire, appartient à MD Drums.

## 3. Un set peut-il garder 16 + 16 × 24 = 400 paramètres configurés ?

- **[observé]** Dans le set 12.2.1 de la machine, chaque `PluginDevice` (9 instances, VST2 et VST3) a exactement 128
  emplacements `PluginFloatParameter` dans sa `ParameterList`, qu'il ait 0, 2, 7, 12, 25 ou 26 paramètres configurés ;
  les emplacements libres ont `ParameterId` à -1. Chaque configuré porte son identifiant de paramètre VST et sa place
  dans le panneau (`VisualIndex`). Le format réserve donc 128 places par instance de plug-in.
- **[doc]** Le seuil de remplissage automatique plafonne lui aussi à 128 (article « Options.txt file »).
- **[communauté]** Forum d'Ableton : en 2012, Configure limité à 128 paramètres par plug-in pour l'automation et le
  contrôle MIDI ; en 2017, un plug-in d'environ 400 paramètres dont ceux au-delà de 128 restaient inaccessibles. Un
  souvenir d'une limite de « 400 paramètres au total » (forum de Cycling '74) n'est étayé par rien.
- **[notes]** Aucune version de Live 12, jusqu'à 12.4.6, ne mentionne de changement.
- **[déduit]** Plafond probable : **128 paramètres configurés par instance de MD Drums**. 400 n'y tiennent pas : il
  faut choisir. Avec les 16 Machines, il reste 112 places, soit par exemple les 24 paramètres de 4 Tracks, ou 7 par
  Track. À confirmer par un essai : en Configure, toucher un 129ᵉ paramètre.
- **[déduit]** Le remplissage automatique ne règle rien : avec le seuil à 128, Live prendrait les 128 premiers
  paramètres dans l'ordre du plug-in, soit les Tracks 1 à 3 entières (3 × 34) et les 26 premiers de la Track 4
  (Machine, les 24, LFO Track). Et l'option est non officielle.
- **[déduit]** L'ordre de `parameters` suit celui du panneau, que l'utilisateur fixe en configurant : le device doit
  retrouver chaque paramètre par son `name`, pas par son index.

## 4. Ce que MD Drums déclare

Sources : `source/elektron/md/mdDrumsPlugin/parameterDescriptions_mddrums.json`, `Design/parameter-spec.md`,
`mdDrumsController.cpp` et `.h`, `mdDrumsProcessorTest.cpp`, `jucePluginLib/controller.cpp` et `parameter.cpp`, le
wrapper VST3 de JUCE 7.0.10 du dépôt [repo].

- **Nombre et ordre** : 576 paramètres hôte. D'abord 34 par Track, Track n aux index 34 × (n − 1) à 34 × n − 1, dans
  cet ordre : Machine ; SYN1-8 ; AMD, AMF, EQF, EQG, FLTF, FLTW, FLTQ, SRR ; DIST, VOL, PAN, DEL, REV, LFOS, LFOD,
  LFOM ; LFO Track, LFO Param, LFO Shape 1, LFO Shape 2, LFO Mode ; Level ; Mute ; Solo ; Out. Puis, de l'index 544 à
  575, les 32 effets master : Echo (8), Reverb (8), EQ (8), Dynamix (8). Les 24 paramètres de Track suivent l'ordre des
  notes de Lock 64-87 de l'ADR 0003 : le paramètre k (0-23) d'une Track est la note 64 + k.
- **Identifiants** : `page_part_index`, figés depuis le premier push (« append parameters, never renumber ») : Track 1
  Machine `5_0_0`, Track 1 Out `7_0_1`, Master Echo TIME `8_0_0`. JUCE en tire l'identifiant VST3 par hachage
  (`JUCE_FORCE_USE_LEGACY_PARAM_IDS` garde sa valeur par défaut, 0). La LOM n'expose aucun de ces identifiants : `DeviceParameter`
  n'en a pas [doc].
- **Visibles par l'hôte et automatisables : tous.** `isPublic` vaut `true` par défaut dans le JSON et aucun paramètre ne
  le change ; JUCE marque chaque paramètre `kCanAutomate`, puisque `isAutomatable` n'est redéfini nulle part
  (`juce_audio_plugin_client_VST3.cpp`, ligne 881).
- **Ce que le wrapper ajoute** : un paramètre « Bypass » (`kIsBypass`) en fin de liste, puisque MD Drums n'en déclare
  pas, soit 577 paramètres VST3 ; et, `JUCE_VST3_EMULATE_MIDI_CC_WITH_PARAMETERS` valant 1 par défaut, 16 × 130
  paramètres « MIDI CC » et 16 « MIDI PC » non automatisables (drapeaux à 0). Pas de paramètre de programme
  (`getNumPrograms` rend 1). Comment Live compte ces ajouts est inconnu ; sans effet ici, MD Drums dépassant 64 de toute
  façon.
- **Groupes** : « Track 1 » à « Track 16 », puis « Global » pour le master (`controller.cpp`, `registerParams`). JUCE en
  fait des unités VST3 ; la LOM ne les montre pas [déduit, aucune propriété correspondante].
- **La Machine** : page 5, index 0 ; plage 0-191, discrète, défaut déclaré 16 ; texte par la liste `machines` de 192
  noms. **La valeur est l'identifiant de machine du Machinedrum** (l'index dans la liste est l'identifiant) :
  GND 0-5, NFX 7-9, TRX 16-29, EFM 32-39, E12 48-63, P-I 64-72, INP 80-87, MID-01 à 16 aux identifiants 96-111, CTR
  112-113 et 120-123, ROM-01 à 32 aux identifiants 128-159, RAM 160-163 et 165-168, ROM-33 à 48 aux identifiants
  176-191 ; un nom « #n » marque un identifiant que le Machinedrum n'a pas, et le choisir n'envoie rien. Vue de
  l'hôte : valeur normalisée = identifiant / 191.
- **Les 24 paramètres** : 0-127 ; SYN1-8 nommés par Machine dans leur texte (voir la section 2). EQG et PAN sont
  bipolaires (sans effet sur la plage). Level 0-127 ; Mute et Solo Off/On ; Out Main/Out ; listes du LFO.
- **Valeurs** : celles du Kit, que les Locks ne touchent pas (ADR 0003).

## Ce que ça implique pour le device

**Machine des 16 Tracks : oui, si l'utilisateur configure les 16 paramètres « Track n Machine ».** Valeurs du Kit pour
les 24 paramètres des 16 Tracks : **non, pas par la LOM** ; 400 paramètres ne tiennent pas dans les 128 places d'une
instance. Au mieux les 16 Machines plus 112 paramètres du Kit choisis.

Noms de SYN1-8 : la Machine suffit. Le device peut embarquer la table de noms de `mdmachines.cpp` (192 identifiants
× 8 noms, OS 1.63) et nommer SYN1-8 sans configurer un seul SYN. `__str__` sur un SYN configuré donne le même nom, mais
dépend de ce que Live met en cache.

Mise en place que l'utilisateur doit faire, une fois :

1. ouvrir la fenêtre de MD Drums et cliquer Configure dans l'en-tête du device ;
2. toucher la Machine de chacune des 16 Tracks dans l'éditeur de MD Drums (et les paramètres du Kit choisis pour le
   fantôme, s'il y en a) ;
3. sortir de Configure ;
4. clic droit sur la barre de titre, « Save as Default Configuration », pour que chaque nouvelle instance de MD Drums
   arrive configurée ; ou garder un modèle de set prêt.

Côté device :

- trouver MD Drums par `class_name` = `PluginDevice` et `class_display_name` = « MD Drums » ;
- observer `parameters`, dont la liste change quand l'utilisateur configure ;
- retrouver chaque paramètre par son `name` (« Track n Machine », « Track n SYN1 »…), jamais par index ;
- lire `is_quantized`, `min` et `max` avant de convertir ; à défaut, lire `__str__` de la Machine et la ramener à son
  identifiant par la liste `machines` ;
- observer `value` sans jamais l'écrire ;
- sans configuration, revenir à « SYN1-8 » génériques et sans fantôme, comme le ticket 02 le prévoit.

À vérifier dans Live avant de construire, avec un set de test :

1. le plafond de 128 paramètres configurés (toucher un 129ᵉ) ;
2. ce que la LOM rend pour la Machine de MD Drums : `min`, `max`, `is_quantized`, `value_items`, `__str__` ;
3. le trou de la section 2 : SYN1-8 périmés dans Live après un changement de Machine fait dans MD Drums ;
4. la cadence des notifications de `value`.

Un fantôme complet (24 paramètres × 16 Tracks) demanderait un autre canal que la LOM, que MD Drums publierait au
séquenceur : c'est une décision à prendre ailleurs, ce ticket n'a pas exploré ce canal. L'autre voie est de réduire le
fantôme à un sous-ensemble configuré.

## Sources

Cycling '74 :

- Device : https://docs.cycling74.com/apiref/lom/device/
- PluginDevice : https://docs.cycling74.com/apiref/lom/plugindevice/
- DeviceParameter : https://docs.cycling74.com/apiref/lom/deviceparameter/
- `live.observer` : https://docs.cycling74.com/reference/live.observer
- Guide « Live API Overview » : https://docs.cycling74.com/userguide/m4l/live_api_overview/

Ableton :

- Manuel de Live 12, « Plug-Ins in the Device View » et « Plug-In Configure Mode » :
  https://www.ableton.com/en/manual/working-with-instruments-and-effects/
- Options.txt file : https://help.ableton.com/hc/en-us/articles/6003224107292-Options-txt-file
- Saving plug-in parameter configurations : https://help.ableton.com/hc/en-us/articles/209073089-Saving-plug-in-parameter-configurations
- Push 1: Using Third Party Plug-Ins : https://help.ableton.com/hc/en-us/articles/209071489-Push-1-Using-Third-Party-Plug-Ins
- Automating plug-in parameters that can't be configured : https://help.ableton.com/hc/en-us/articles/209067009-Automating-plug-in-parameters-that-can-t-be-configured
- Notes de version : Live 12 https://www.ableton.com/en/release-notes/live-12/ ; Live 10 (10.1 : VST3, seuil de 65)
  https://www.ableton.com/en/release-notes/live-10/ ; Live 9 (9.5 : configurations par défaut)
  https://www.ableton.com/en/release-notes/live-9/

Communauté :

- Forum d'Ableton, « [Feature Request] Live API: auto-discover all plugin parameters names + configure devices
  parameters » (mai 2026) : https://forum.ableton.com/viewtopic.php?p=1837238
- Forum d'Ableton, « Unlimited Vst Parameter "Configure" » (2012, 2020) : https://forum.ableton.com/viewtopic.php?t=179472
- Forum d'Ableton, « Can't access certain VST parameters for MIDI mapping » (2017) :
  https://forum.ableton.com/viewtopic.php?t=224828
- Forum d'Ableton, « Way to configure VST parameters without manually selecting? » (2013) :
  https://forum.ableton.com/viewtopic.php?t=194951
- Forum de Cycling '74, « accessing VST-parameters of VSTs loaded outside of Max (For Live) » :
  https://cycling74.com/forums/accessing-vst-parameters-of-vsts-loaded-outside-of-max-for-live
- Forum de Cycling '74, « LOM devices parameters value unit » : https://cycling74.com/forums/lom-devices-parameters-value-unit
- Forum de Cycling '74, « Getting the real value of the VST parameter for live.dial object » :
  https://cycling74.com/forums/getting-the-real-value-of-the-vst-parameter-for-livedial-object

Dépôt gearmulator-md-drums (`C:\dev\gearmulator-md-drums`, lu sans modification) :

- `source/elektron/md/mdDrumsPlugin/parameterDescriptions_mddrums.json`
- `source/elektron/md/mdDrumsPlugin/Design/parameter-spec.md`
- `source/elektron/md/mdDrumsPlugin/mdDrumsController.cpp`, `mdDrumsController.h`, `mdDrumsProcessorTest.cpp`
- `source/jucePluginLib/controller.cpp`, `parameter.cpp`, `parameter.h`, `processor.cpp`
- `source/JUCE/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp` (JUCE 7.0.10)
- `source/elektron/md/mdProtocol/mdmachines.cpp` (noms de SYN1-8 par Machine)
- `docs/adr/0003-locks-travel-as-notes.md`

Observé sur la machine :

- set de test dans la User Library (`PROJECTS\test.als`), créé par Live 12.2.1 (lu, décompressé en
  mémoire, non modifié) ;
- pont Max-Live : `C:\ProgramData\Ableton\Live 12 Suite\Resources\MIDI Remote Scripts\_MxDCore\LomTypes.pyc`.
