# Max natif sans JS : persistance, externals, timing, tests, LOM

Recherche du 2026-10-10 pour `issues/08-structure-du-moteur-natif.md` (moteur sans `js`, `v8`, `jsui`, `v8ui` ni
`node.script`, ADR 0001). Une section par question ; chacune finit par ce qu'elle implique pour le device, sans
trancher la conception.

## Cadre : quelles versions, quels fichiers

- **Machine : Live 12.2.1 Suite, Max 9.0.7 embarqué** (voir `research/01-lom-live-12.md`). Max est dans
  `C:\ProgramData\Ableton\Live 12 Suite\Resources\Max\`, qui contient `Max.exe`, `MaxRT.exe` et `MaxRT_nocef.exe`
  (environ 32 Mo chacun) [observé].
- **Documentation locale qui correspond exactement à 9.0.7** [observé] : pages de référence
  `Resources\Max\resources\docs\refpages\{max-ref,msp-ref,m4l-ref}\*.maxref.xml`, guide utilisateur
  `resources\docs\userguide\content\**\*.json`, aides `resources\help\{max,m4l}\*.maxhelp`, externals Max for Live
  `resources\externals\m4l\*.mxe64`. Les citations [doc] plus bas viennent de ces fichiers sauf mention contraire ;
  l'URL donnée en fin de fichier est la version en ligne, qui décrit Max 9.1.x.
- **Pas de Max autonome installé** (`C:\Program Files\Cycling '74` absent). Le dossier utilisateur
  `Documents\Max 9\` existe (avec `Packages` vide, `Library`, `Max for Live Devices`) et les réglages sont dans
  `AppData\Roaming\Cycling '74\Max 9\` [observé].
- **Dernière version publique : Live 12.4.6 avec Max 9.1.5.** Ce qui est daté 9.1.x plus bas n'existe pas sur la
  machine.

Légende :

- **[doc]** : documenté par Cycling '74 ou Ableton (URL dans les sources en fin de fichier).
- **[notes]** : notes de version d'Ableton ou de Cycling '74.
- **[observé]** : constaté dans les fichiers installés sur la machine (aides, pages de référence, binaires, sets
  `.als`), ou dans le code source des dépôts GitHub cités, sans lancer Live ni Max.
- **[communauté]** : rapporté sur les forums ; à vérifier.
- **[déduit]** : raisonnement de ma part à partir des sources citées.
- **[inconnu]** : aucune source trouvée.

## 1. Sauver des données arbitraires par instance dans le set, en natif

### a. `pattr` en mode paramètre, type Blob

- **[doc]** Un paramètre Max for Live a l'un de quatre types : Float, Int, Enum ou **Blob**. Le guide « Parameter
  Mode » de 9.0.7 : un Blob ne s'automatise ni ne se module, mais se sauve dans les presets ; il peut contenir
  « single values, lists, dictionaries, arrays or symbols ». Le guide Max for Live plus ancien (« Device Parameters »)
  ne cite encore que valeurs, listes et chaînes.
- **[doc]** Visibilité : *Automated and Stored* (set, presets, automation), *Stored Only* (set et presets, sans
  automation), *Hidden* (« it will neither be stored nor available for automation »). Un Blob est *Stored Only* par
  défaut (page `pattr` en ligne). Le guide « Max for Live Limitations » : un `pattr` n'est sauvé avec le set que si
  Parameter Mode Enable est coché et la visibilité *Automated and Stored* ou *Stored Only*.
- **[doc]** `pattr` accepte le message `dictionary` : il garde une copie du dictionnaire et la ressort ; lié (bindto)
  à un `dict`, il met à jour sa copie quand le `dict` change. L'aide de `pattr` (onglet « MaxForLive ») : « Store
  complex sets of data with Parameter Type "Blob" ».
- **[observé]** Format dans le set : chaque instance de device a **un seul** bloc
  `<BlobSlot><Value><MxDBlob><Blob>…</Blob>` dans le `.als`, qui contient un objet JSON encodé en hexadécimal ; ses
  clés sont les Long Names des paramètres Blob du device. Exemple décodé du « Sequencers Demo Set » (pack Sequencers
  d'Ableton) : 839 octets, `"u954000445[1]" : [ 98, 70, 92, … ]`. L'hexadécimal double la taille avant la
  compression gzip du `.als`.
- **[observé]** C'est ce que font les séquenceurs d'Ableton : *Rhythmic Steps* range ses pas dans 20 `multislider`
  (sous-patch `p StepsStore`) en `parameter_type 3` (Blob), `parameter_invisible 1` et valeur initiale activée ;
  *Step Arp* utilise 6 `pattr` configurés de même. Que `parameter_invisible 1` désigne *Stored Only* est [déduit]
  (les valeurs se retrouvent dans le Blob du set). Max 9.0.0 règle d'ailleurs un `multislider` à plus d'un curseur en
  Blob automatiquement [notes].
- **Ordre au chargement.** [doc] « Patcher Lifecycle » : Parameter Initialization, puis pattr Restoration, puis
  `loadbang`/`loadmess`, puis `live.thisdevice`. La page de `live.thisdevice` : bang « when the Max Device is opened
  and completely initialized », et à nouveau à chaque chargement de preset ou sauvegarde du device. [inconnu] Le
  moment exact où Live injecte la valeur sauvée dans le set par rapport à ce bang : aucune page ne le dit pour un
  device dans Live. [communauté] L'ordre d'initialisation diffère entre Max autonome et Max for Live (fil
  « initialization order inconsistency »).
- **Taille.** [inconnu] Aucune limite documentée pour un Blob dans un set. [communauté] En 2013, un Blob tenait au
  plus 32 767 caractères par symbole (`ATOM_MAX_STRLEN`, Jeremy de Cycling '74 et parkingsun), l'inspecteur n'en
  montrant que 2048 ; Jeremy conseillait déjà de mettre l'état dans un dictionnaire. [observé] Un set de
  l'utilisateur (`PROJECTS\BODY.als`) contient un Blob de 35 015 octets (l'état entier d'un `pattrstorage`, en JSON
  imbriqué) : la sauvegarde au-delà de 32 Ko existe, la relecture intacte n'est pas vérifiée.
- **Annulation.**
  - [doc] Lignes directrices Ableton : un paramètre *Automated and Stored* ou *Stored Only* piloté par le patch
    « inonde » l'historique d'annulation ; le remède proposé est *Hidden*, qui ne sauve plus rien.
  - [doc] `pattr` : le message `assign` suivi d'un flottant, sur un paramètre *Stored Only*, ne crée pas d'entrée
    d'annulation (page locale 9.0.7 et page en ligne). Rien ne dit qu'`assign` marche pour une liste ou un
    dictionnaire [inconnu].
  - [notes] Live 12.4 (Max 9.1.4) ajoute les visibilités *Visible* et *Visible (Not Stored)* et l'attribut « Undo
    When Visible », **pour Float, Int et Enum seulement**. Max 9.1.4 note aussi : « Parameters: Only "Visible" and
    "Visible (Not Stored)" permit non-undoability ». Lecture probable : à partir de 9.1.4, *Stored Only* n'échappe
    plus à l'annulation, `assign` compris [déduit, formulation ambiguë]. Un Blob n'a donc aucun moyen documenté de
    rester hors de l'historique en 12.4.
  - [inconnu] Combien d'entrées crée une mise à jour de Blob, et si Live les fusionne.

### b. `dict`

- **[doc]** La page de `dict` en 9.0.7 porte `parameter_enable` (« Enables use of this object with Max for Live
  Parameters ») et `parameter_mappable`. Le guide `pattr` : « dictionaries, strings, and arrays can be included in the
  pattr system ».
- **[communauté]** Tyler Mazaika, fil « Store arbitrary number of values within a Live Set » (pièce jointe datée de
  2025) : « [dict] with @parameter_enable is what you want », avec un `dict` de tête qui range chaque état comme
  sous-dictionnaire. Fil « Automatically storing Dict's… » (pièce jointe de 2022) : 11OLSEN confirme la sauvegarde
  et le rappel avec le set ; Martin, que cela ne marche qu'avec un nom de `dict` stable ; vichug explique pourquoi :
  Live retrouve la valeur par le nom du paramètre, qui change si le nom du `dict` change.
- **[communauté]** Simon East (fil « Saving a Dict as part of a Live Set with Dict not being updated from C++ min
  API ») : un `dict @parameter_enable 1` rempli par un external Min n'est pas sauvé avec le set, même après
  `touch()`. Contournement adopté : un `pattr` lié à l'external, ou l'état sérialisé en JSON vers un symbole.
- **`@embed 1`** [doc] : « Save Data With Patcher », et il l'emporte sur les autres `dict` du même nom. Dans un
  device, c'est le `.amxd` qui est sauvé, par l'éditeur ; ce que l'utilisateur change dans Live n'y entre pas
  [communauté, fil `coll`, même attribut] : **pas de sauvegarde par instance** [déduit].
- **[doc]** L'espace des noms Max est global entre devices ; un nom qui commence par `---` est rendu unique au
  device à l'initialisation (guide « Using Symbols in Max for Live »). Ce nom change d'une ouverture à l'autre
  [communauté : withakay, fil `coll` ; Florent Ghys, fil « Coll, Dict & Pattr »], ce qui rejoint le problème de nom
  stable ci-dessus.
- [inconnu] Le comportement d'un `dict` paramétré vis-à-vis de l'annulation, et le moment où Live lit son contenu
  (à chaque changement ou à la sauvegarde du set).

### c. `pattrstorage` en mode paramètre

- **[doc]** En mode paramètre, la valeur de `pattrstorage` est **son état de stockage entier** (tous les presets, ce
  qui irait sinon dans un fichier) ; Live le sauve dans le set. `@savemode 0` coupe le fichier externe ; avec une
  valeur initiale, `savemode` et `autorestore` sont ignorés. Attribut propre au mode paramètre : « Auto-update
  Parameter Initial Value ». `autopattr` ne participe pas au système de paramètres.
- **[observé]** Le Blob de 35 Ko de `BODY.als` est exactement cela : `"paramstore" : [ { "pattrstorage" : { "name"
  : "paramstore", "slots" : { "1" : { … } } } } ]`.
- **[communauté]** Un `pattrstorage` en mode paramètre a fait planter Live pendant des interpolations (fil
  « Pattrstorage, recall and line to interpolate… ») ; des rappels répétés y ralentissaient de 4 à 19 s, avec du JS
  soupçonné de recréer ses objets LiveAPI à chaque appel (fil « pattrstorage responds slower with each recall »).

### d. Les autres objets

| Objet | Ce que dit la page 9.0.7 | Par instance dans le set ? |
|---|---|---|
| `coll` | `@embed` seulement (« Save Data With Patcher »), pas de `parameter_enable` [observé] | Non [communauté : Andrew Pask conseille `pattr` + `matrixctrl` à la place] |
| `table` | `@embed` (1 par défaut) et `parameter_enable` [doc] | Par `parameter_enable` [déduit, non testé] |
| `array`, `string` | `parameter_enable` [doc] | Documenté ; un essai avec `array` a échoué chez chapelier fou (2025) [communauté] |
| `live.step`, `live.grid` | Objets `live.*` : paramètres toujours actifs, sauvés dans le set [doc] ; `live.grid` entre dans le système `pattr` [doc] | Oui [doc] ; mais `live.step` ne connaît que hauteur, vélocité, durée, extra1, extra2 par pas, en `nseq` séquences [doc] |
| `multislider` | Blob automatique au-delà d'un curseur (Max 9.0.0) [notes] | Oui, c'est le choix d'Ableton [observé] |

### e. Ce que recommandent les développeurs, limites connues

- Andrew Pask (Cycling '74) : `pattr` avec `matrixctrl` et l'état de `pattrstorage` sauvé avec le device
  [communauté]. Jeremy (Cycling '74, 2013) : mettre l'état dans un dictionnaire porté par `pattr` [communauté]. Tyler
  Mazaika (2025) : un `dict @parameter_enable` [communauté]. Ableton : Long Names stables, sinon le rappel casse ; un
  seul pas d'annulation à l'ajout du device et jamais d'historique inondé (checklist des lignes directrices) [doc].
- Performances : `dict` « lent à sortir son contenu » par rapport à `coll` (chapelier fou) [communauté] ; Max 9.1.0
  améliore la conversion de chaînes de `dict` et la sûreté entre fils de `dict`, `array` et `string` [notes].
- Ordre de grandeur pour ce device [déduit] : au pire, 16 Patterns × 16 Tracks × 64 pas = 16 384 pas ; à 6 champs plus
  24 Locks en couples (paramètre, valeur), au plus 54 nombres par pas, soit environ 885 000 nombres, environ 3 Mo de
  JSON et 6 Mo d'hexadécimal dans le XML du set avant gzip. Le cas creux typique est bien plus petit.

### Ce que ça implique pour le device

Trois voies natives sauvent un Pattern par instance : un `pattr` Blob qui porte un dictionnaire, un `dict
@parameter_enable` à nom stable, un `pattrstorage` en mode paramètre ; la quatrième, celle d'Ableton, découpe l'état en
plusieurs Blobs (`multislider` ou `pattr`). Toutes finissent dans le même Blob JSON par device. Ce qui reste à mesurer
avant de choisir, sur la machine :

- la relecture intacte d'un Blob de quelques Mo (le pire cas) et le temps de chargement et de sauvegarde du set ;
- le nombre d'entrées d'annulation que crée une mise à jour, en 12.2.1 puis en 12.4 (le correctif 9.1.4 change la
  donne pour *Stored Only*) ;
- l'instant où la valeur sauvée arrive par rapport à `live.thisdevice` ;
- le nom du `dict` : `---` le rend unique au device mais instable d'une ouverture à l'autre.

Écrire dans le Blob à chaque édition remplit l'historique ; écrire rarement (au changement de Pattern, sur geste)
risque de perdre la dernière édition si l'utilisateur sauve entre deux. Le choix est à faire au ticket 08.

## 2. Externals C/C++ dans Max for Live

### a. Chargement, gel, emplacement

- **[doc]** Un external Windows est un `.mxe64` ; il s'installe en le mettant dans le chemin de recherche de Max.
  Ordre de recherche : dossier du patch ouvert, projet du patch, dossier Max 9, puis chemins configurés ;
  `Documents\Max 9\Library` et les paquets de `Documents\Max 9\Packages` y sont. « Max for Live devices are just
  projects » et suivent ces règles.
- **[doc]** Geler un device y embarque les externals tiers (« Objects created by third parties are always
  included ») ; les versions des autres plates-formes sont prises si elles sont dans le chemin de recherche ; à
  l'ouverture, **les fichiers du device gelé passent avant** ceux du disque. Un device non gelé ne contient que son
  patch principal, le reste est cherché dans le chemin de recherche. Dégeler écrit les fichiers dans un dossier dédié
  (`Unfrozen Max Device Files` selon le guide Max ; `Documents/Max/Max for Live Devices/` selon Ableton).
- **[doc]** Lignes directrices Ableton : un external hors distribution standard doit être compilé à part pour Mac,
  PC et Push 3 (Linux).
- **[observé]** `Documents\Max 9\Max for Live Devices\` contient déjà `tuple Project`, `tupline Project` et `Dyad
  Project`.

### b. SDK et Min : état des dépôts

État relevé sur l'API GitHub le 2026-10-10 [observé] :

| Dépôt | Dernière étiquette | Dernier commit | Remarques |
|---|---|---|---|
| `Cycling74/max-sdk` | v8.2.0 (2021-10-12) | 2026-05-12, « Use new max-sdk-base CMake style with FetchContent » | README : CMake ≥ 3.25, `cmake -G "Visual Studio 17 2022" ..`, un `CMakeLists.txt` minimal par external (`set_property(DIRECTORY PROPERTY max::external YES)`) |
| `Cycling74/max-sdk-base` | v1.0.0 (2021) | 2026-06-17, option de paquet hors arbre | Cibles importées `Max::Max`, `Max::MSP`, `Max::Jitter` ; FetchContent ou sous-module |
| `Cycling74/min-api` | v0.6.0 (release), étiquettes jusqu'à v0.6.11 | 2026-03-24 | CI « Test » (macOS et `windows-latest`, build puis `ctest`) verte le 2026-03-24 et le 2025-11-20, rouge le 2025-09-25 ; 61 tickets ouverts |
| `Cycling74/min-devkit` | v0.6.0 (2021-10-21) | 2025-05-20 (workflow CI) | README : Visual Studio 2017 ou 2019, CMake ≥ 3.19 ; cite le dossier Packages de « Max 8 (or Max 9) » ; 70 tickets ouverts |
| `Cycling74/mock` | aucune | 2025-11-20 | Noyau simulé des tests Min (2d) |
| `Cycling74/min-lib` | v0.6.0 | 2022-10-20 | |

- [inconnu] Aucune déclaration officielle de compatibilité Max 9 dans ces dépôts. [doc] Le guide « Externals » de
  Max 9 renvoie au SDK C et à Min pour écrire des externals. [communauté] Des externals tiers déclarent marcher sous
  Max 8 et Max 9 (Spat, IRCAM).
- [déduit] Les deux SDK sont maintenus à bas régime : `max-sdk` vient de refaire son CMake, `min-api` reçoit des
  correctifs et sa CI passe sous Windows ; `min-devkit` (le paquet d'exemples) est en retard sur les deux.

### c. Fils d'exécution

- **[doc]** Un événement du scheduler (MIDI, `metro`, `delay`) est traité en envoyant un message à un objet, qui en
  envoie à d'autres, en profondeur d'abord : ce sont des appels de fonction. La méthode d'un external appelée par un
  `metro` s'exécute donc **sur le fil qui sert le scheduler** [doc + déduit]. Avec « Scheduler in Audio Interrupt »,
  ce fil est le fil audio, servi une fois par vecteur.
- **[doc]** SDK 8.2, chapitre « Threading » : Max protège entre le fil du scheduler et le fil principal « many of its
  common operations », dont l'envoi de messages et les **outlets** ; une routine `perform` audio ne peut parler à Max
  que par un `clock`. Chapitre « Scheduler » : éviter sur le fil haute priorité les opérations longues ou imprévisibles
  (accès fichiers) et tout ce qui bloque ; ouvrir une boîte de dialogue y plante probablement. `defer()` diffère vers
  le fil principal seulement si l'appel vient du scheduler ; `defer_low()` diffère toujours ; `isr()`,
  `systhread_istimerthread()` et `systhread_isaudiothread()` disent où l'on est.
- **[doc]** Guide « Scheduler » : avec SIAI, un calcul coûteux dans le scheduler fait perdre des vecteurs audio.
- [inconnu] Le SDK ne dit rien de l'allocation mémoire sur ce fil. [déduit] Les règles temps réel ordinaires
  s'appliquent (pas d'allocation ni de verrou bloquant), puisque le fil est celui de l'audio de Live.
- **[observé] Piège de Min** : dans `c74_min_message.h`, « By default, all messages are assumed to not be threadsafe
  and thus will defer themselves to the main thread if input has come from another thread ». Un message reçu hors du
  fil principal part dans une file de **2** places (`fifo<deferred_message> m_deferred_messages{ 2 }`, `try_enqueue`)
  puis `max::defer`. Il faut déclarer `message<threadsafe::yes>` ou la classe `object<T, threadsafe::yes>` pour
  rester sur le fil du `metro`. [déduit] Sans cela, un external Min reçoit le pas sur le fil basse priorité, comme
  `js` ; et au-delà de deux messages en attente, l'appel est perdu.
- **[doc]** Max 9.0 ajoute l'objet `schedule`, qui ramène n'importe quel message sur le fil haute priorité,
  éventuellement retardé.

### d. Tests unitaires hors de Max

- **[observé]** `min-api/test/` contient `catch.hpp`, `main.cpp` et `min-object-unittest.cmake` ; `test/mock` est un
  sous-module vers `Cycling74/mock` (« A mocked/stubbed library for linking unit tests of Max externals built with
  the Min-DevKit »). Pour un external `foo.cpp`, si `foo_test.cpp` existe, CMake construit l'exécutable `foo_test`
  avec `-DMIN_TEST`, le lie à la bibliothèque `mock_kernel` et l'enregistre par `add_test` : **`ctest` le lance**.
- **[observé]** Un test Min (exemples `min.hello-world_test.cpp`, `min.xfade_tilde_test.cpp`) : `#include
  "c74_min_unittest.h"`, `#include "foo.cpp"`, `ext_main(nullptr)`, `test_wrapper<foo>`, appel direct des messages,
  puis lecture de ce qu'a reçu l'outlet par `c74::max::object_getoutput(obj, 0)` (une liste de messages enregistrés).
  Les objets signal s'appellent comme une fonction (`my_object(0.0, 1.0)`).
- **[observé] Limites du noyau simulé** :
  - les outlets enregistrent les messages ;
  - `clock_fdelay` tourne sur un vrai fil, en temps réel (`std::chrono::steady_clock`), arrondi à la milliseconde
    entière : pas de temps virtuel ;
  - `systhread_ismainthread()` rend toujours vrai et `defer` ne fait rien ;
  - toutes les fonctions de dictionnaire (`dictobj_register`, `dictionary_new`, etc.) sont des souches qui rendent
    `nullptr` ou 0 ;
  - `time_getms` rend 0 et rien de l'API ITM (transport, ticks) n'est simulé.
- **[observé]** Ça compile encore : CI de `min-api` verte sous `windows-latest` le 2026-03-24 (build Debug et
  Release, `ctest -C Debug` et `-C Release`).
- **[doc]** Le SDK C n'a pas d'équivalent ; son README renvoie à `max-test` (section 4b). [déduit] `mock_kernel`
  exporte des symboles C (`gensym`, `outlet_new`, `clock_new`…), donc un external C pourrait s'y lier ; personne ne le
  documente.

### e. Un external qui remplit un `dict` pour `live.object`

- **[doc]** Le SDK permet de créer un dictionnaire, de l'enregistrer sous un nom (`dictobj_register`) et de sortir
  `dictionary <nom>`, que tout objet Max reconnaît.
- **[observé]** `live.object.mxe64` importe `dictobj_findregistered_retain`, `dictobj_release`, `atomisdictionary`,
  `dictionary_getdictionary`, `dictobj_register` et un `jsonreader`, et contient les chaînes `dictionary`,
  `serializeblob` et « live_object_parse_output: truncated message greater than 32k ». [déduit] `live.object` retrouve
  un `dict` enregistré par son nom, le sérialise pour Live, enregistre en `dict` les réponses JSON de Live, et tronque
  les autres réponses au-delà de 32 Ko.
- [déduit] Puisque `live.object` diffère tout vers le fil principal de Live, le `dict` de l'external doit rester
  enregistré et inchangé jusqu'à ce que l'appel soit servi.
- [communauté] Le contre-exemple de Simon East (1b) : un `dict` paramétré rempli par un external Min ne se sauvait
  pas dans le set.

### Ce que ça implique pour le device

Un external C ou C++ est viable dans Max for Live : chargé par Max, embarqué au gel, construit par CMake et Visual
Studio 2022, testable sous `ctest` avec le noyau simulé de Min. Trois réserves pèsent sur la conception :

- en Min, chaque message reçu du `metro` part par défaut sur le fil basse priorité, dans une file de deux places :
  `threadsafe::yes` est obligatoire sur le chemin du jeu, et le code doit alors être réellement sûr entre fils ;
- le noyau simulé ne simule ni le temps, ni les dictionnaires, ni le transport : ce qui doit être testé sous `ctest`
  (tirage des Conditions, ordonnancement des événements d'un pas) doit pouvoir tourner sans eux ;
- un external est un binaire par plate-forme à reconstruire et à geler, et ce qu'il écrit dans un `dict` paramétré
  ne se sauve pas forcément dans le set.

## 3. Primitives de temps natives

### a. Temps relatif au tempo

- **[doc]** 480 ticks par noire ; valeurs de note (`16n` = 120 ticks, `16nt` = 80…), `bars.beats.units`, et
  `translate` pour convertir. `@quantize` accepte ticks, bbu ou valeurs de note et ne fait rien avec une unité fixe.
  Une valeur bbu est une position pour `timepoint`, un intervalle pour `@quantize` (guide « Time Value Syntax »).
- **[doc]** `metro` : intervalle relatif au tempo, `@quantize`, `@transport`, `@autostart`/`@autostarttime` ;
  intervalle minimal 1 µs depuis Max 9.0 [notes]. `delay` et `pipe` : en unités relatives, « the object will not
  operate unless the transport is running » ; un nouveau délai ne change pas ce qui attend déjà ; `delay` gagne un
  attribut `transport` en 9.1.0 [notes]. `timepoint` : bang quand le transport atteint l'instant, en ticks entiers ou
  flottants, transport en marche et « moving forward ». `transport` : 480 PPQ, sorties unités, tempo et ticks bruts
  en flottants.
- **[observé]** Dans l'API ITM du SDK (`ext_itm.h`), les ticks sont des `double` (`itm_getticks`, `itm_tickstoms`) et
  le transport notifie `TEMPO`, `SEEK`, `UPDATESTATE` et `TIMESIG`. [doc] Le chapitre ITM du SDK : « An event based
  in ticks will occur when the transport reaches the specified tick value » ; un objet de temps reçoit `reschedule`
  quand le transport change. [déduit] Un `delay` ou un `pipe` en ticks suit donc un changement de tempo pendant
  l'attente ; aucune page d'objet ne l'écrit [inconnu pour `metro`, `delay`, `pipe` précisément]. Max 9.0.0 améliore
  la synchro ITM au changement de position et de signature [notes].
- **[doc] Dans Max for Live** : les `transport` sans nom suivent Live, même en mode preview ; un `transport` nommé ne
  suit Live qu'avec `@clocksource live` ; deux devices qui partagent un nom de transport tournent indépendamment ; le
  `transport` ne peut pas piloter Live. La page de `transport` prévient : la conversion d'une position en temps ne
  tient compte que du tempo global et se trompe si le set a des changements de tempo.
- **Sauts et boucle.** [observé] La notification `SEEK` existe. [communauté] Les objets calés sur le transport
  « peuvent dérouter » quand Live revient au début de la boucle (redhexagonal, 2014). [inconnu] Ce que font
  précisément `timepoint`, `metro @quantize` et les `delay` en attente quand la tête recule. `plugsync~` et
  `plugphasor~` restent les références à l'échantillon près (voir `01-lom-live-12.md`, section 5).

### b. Le sous-vecteur arrive-t-il jusqu'à Live ?

- **[doc]** Avec SIAI, Max traite avant chaque vecteur tous les événements prévus pendant ce vecteur ; `sig~`,
  `click~`, `line~`, `curve~` et **`vst~`, pour le MIDI**, en tiennent compte à l'échantillon près (guide « Sample
  Accurate Messages »). Le scheduler suit alors le compteur d'échantillons et non l'heure réelle.
- [inconnu] **Aucune déclaration officielle** de Cycling '74 ou d'Ableton sur la précision du MIDI qu'un device
  envoie à Live par `midiout`, ni sur la conservation d'un décalage à l'intérieur du vecteur.
- [communauté] Iain Duncan : un événement attend la frontière de vecteur suivante, soit au plus environ 1,45 ms à
  44,1 kHz, sans cumul (déjà cité au ticket 01). broc (2013) : une latence constante et sans gigue par les routages
  standard de Live. redhexagonal (2013) : de la gigue aux grandes tailles de tampon.

### c. Du signal au MIDI

- **[doc]** Avec SIAI, `snapshot~`, `edge~`, `peakamp~`, `spike~`, `what~` et `subdiv~` produisent des événements du
  scheduler à l'échantillon près, en retard d'exactement un vecteur mais avec leur timing relatif intact.
- [inconnu] Aucun objet natif documenté n'envoie du MIDI vers Live depuis le domaine signal en contournant le
  scheduler : le chemin passe par un événement du scheduler, puis `midiout`, dont la précision vers Live est
  l'inconnue de 3b.

### Ce que ça implique pour le device

Le tirage d'un pas d'avance (ticket 08, round 1) peut se programmer en ticks flottants sur le transport de Live, avec
le micro-timing en fraction de tick ; les objets ITM suivent le tempo pendant l'attente [déduit]. Deux mesures
décident du reste et relèvent du ticket 04 :

- ce que deviennent les événements en attente à un saut ou une boucle (le ticket 08 veut les jeter) ;
- si un événement prévu au milieu d'un vecteur de 64 échantillons arrive à MD Drums au bon échantillon ou à la
  frontière. MD Drums joue ses Hits à l'échantillon près (ticket 19 du dépôt gearmulator-md-drums), donc son rendu
  montre l'offset qu'il a reçu.

## 4. Tester sans Node ni JS

### a. Le Max de Live en ligne de commande

- **[observé]** `Max.exe` contient les options `--mfl`, `--new-instance`, `--nocef`, `--log`, `--authsystem`,
  `--authuser`, `--sms` et `--from-max`, et le message « Max: unable to open %s » près de leur analyse ; [déduit]
  Max ouvre les fichiers passés en argument. Aucune de ces options n'est documentée. `MaxRT.exe` et
  `MaxRT_nocef.exe` sont aussi livrés (un Max Runtime, d'après le nom [déduit]).
- **[doc]** Cycling '74 (page de support, lue par le moteur de recherche : accès direct refusé) : le Max livré avec
  Live est « a different product » et « not intended to be used outside the context of Live ». Sans autorisation, Max
  7 à 9 ouvre, fait tourner et modifie des patchs, mais ne sauve pas ; ouvrir et utiliser un patch ne demande pas de
  licence.
- **[doc]** Autorisé par Live seulement, Max n'utilise pas ses propres pilotes MIDI ni audio : le MIDI passe par le
  device en cours d'édition, en mode preview ; un patch ouvert hors édition de device n'a **pas d'entrée-sortie
  MIDI**.
- **[doc]** Messages à `max` : `quit`, `clean` (pas de dialogue de sauvegarde avant `quit`), `openfile` (ouvrir un
  patch sous un nom de référence), `sendapppath`. [communauté] Sous Windows, `; max quit` laisse parfois le processus
  vivant ; un `taskkill` sert de repli, au prix des préférences non écrites.
- [inconnu] Un mode sans interface : rien de documenté (`--nocef` coupe sans doute le moteur web intégré [déduit du
  nom]). [inconnu] Le comportement réel du `Max.exe` de Live lancé seul sur cette machine (demande d'autorisation,
  essai, mode non autorisé) : à essayer une fois.

### b. `max-test` de Cycling '74

- **[doc]** Paquet « Automated Test Harness for Max ». Un test est un patch `*.maxtest.maxpat` (ou un projet
  `.maxtest.maxzip`) qui démarre seul (`loadbang`), contient au moins un `test.assert` et se termine par un bang à
  `test.terminate`. Autres objets : `test.equals` (flottants), `test.sample~`, `test.log`, `test.string.equals`.
- **[observé]** Les objets `test.*` sont des externals C++ réunis dans l'external `oscar` (`source/projects/oscar` :
  `test.assert.cpp`, `test.runner.c`, `test.db.cpp`…), déclarés par `init/testpackage-init.txt` (`max objectfile
  test.assert oscar test.assert`). Il faut les compiler avec le SDK (CMake depuis la v1.2.0).
- **[observé]** Le lanceur `ruby/test.rb` démarre `Max.exe` sans argument, lui parle en OSC sur UDP (4791 et 4792,
  à activer par `misc/max-test-config.json`), attend la base de fichiers, laisse Max lancer tous les tests trouvés
  dans le chemin de recherche, envoie `max quit`, puis lit les résultats dans une base SQLite (gem `sqlite3`
  requise).
- **[observé]** Dernière étiquette v1.2.1 (2021-12-16) ; derniers commits 2025-06 à 2025-09, qui ajustent des tests de
  `dict` : [déduit] Cycling '74 s'en sert encore avec un Max récent. Compatibilité Max 9 non déclarée [inconnu].

### c. Autres approches

- **[observé]** `twhiston/mUnit` (« Unit testing for Max 9 », commit du 2026-09-29) : objets d'assertion bâtis sur
  `v8` et `node.script`. Le JS n'y est que dans les patchs de test, pas dans le device.
- **[doc]** `Ableton/maxdevtools` : le *Load Time Tester* (un patch Max lance une version de Live avec un set, et un
  device `DoneLoading.amxd` signale la fin du chargement), le *CPU Reporter*, `maxdiff` (diffs lisibles des `.amxd`) et
  les lignes directrices de production (checklist d'annulation et de rappel des valeurs).
- **[doc]** `ideoforms/AbletonOSC` : un script de surface de contrôle qui expose la LOM en OSC (ports 11000 et
  11001), pilotable depuis un programme externe pour des tests d'intégration dans Live.
- **[observé]** Le Max de Live contient `live.testhelper.mxe64`, non documenté (chaînes `devicestate`, `isinlive`,
  `learningType`) ; son usage est [inconnu].
- **[déduit]** En dehors de Live, `transport` pilote le transport global de Max (marche, position, tempo) et
  `live.thisdevice` fait office de `loadbang` : un patch de test peut faire jouer la logique du device sans Live,
  mais pas `live.object` (qui ne fonctionne que dans un device, d'après son aide) ni le MIDI sortant (4a).

### Ce que ça implique pour le device

Trois niveaux de garde-fou sont possibles sans JS dans le device :

1. `ctest` sur la logique compilée en external, avec le noyau simulé de Min, si la logique n'a pas besoin du temps,
   des dictionnaires ni du transport ;
2. des patchs `*.maxtest.maxpat` lancés par `max-test` dans le Max de Live, s'il tourne seul et si `oscar` se compile
   avec le SDK actuel ; sans MIDI sortant, ils doivent comparer les messages avant `midiout` ;
3. un set de test dans Live, ouvert par un lanceur (comme le *Load Time Tester*), qui enregistre ce que joue le device
   et le compare hors ligne.

Le premier essai à faire est de lancer le `Max.exe` de Live seul, sous garde de temps, avec un patch en argument et
`; max clean, quit` à la fin.

## 5. Appels LOM depuis `live.object` natif

- **[doc]** `live.object` reçoit `call <fonction> <paramètres…>`, avec les types de la LOM, et rend « function value »
  par sa sortie gauche. Le guide « Live API Overview » ne liste pas de type dictionnaire (bool, symbol, int, float,
  double, beats, time, list).
- **[doc]** `Clip.add_new_notes` prend un dictionnaire `{notes: [ {pitch, start_time, duration, velocity, …} ]}` et
  rend la liste des `note_id` ; `Track.create_midi_clip(start_time, length)` existe depuis 12.1.10 ;
  `ClipSlot.create_clip(length)` pour la Session (détails au ticket 01).
- **[observé]** Ni la page locale de `live.object` (9.0.7), ni la page en ligne ne décrivent comment passer un
  dictionnaire. Le binaire, lui, sait le faire (2e). Le pont Python (`_MxDCore/MxDCore.pyc`) contient « Malformatted
  dictionary argument », « Expecting 'notes' as the main dictionary's key » et `dict_from_args`.
- **[communauté]** dfwaudio (fil « Ableton Live11 - add_new_notes/apply_note_modifications (dictionary methode) »,
  Live 11) : `call apply_note_modifications { "notes" : [ … ] }` en JSON brut dans une boîte de message est accepté ;
  un dictionnaire d'une seule note sans tableau donne « invalid arguments ». Mattijs Kneppers (commentaire de
  l'article « What's new in Live 11, part 2 ») : une limite de taille des dictionnaires échangés avec Live, levée
  ensuite.
- **[déduit]** La syntaxe native la plus probable : `[dict ---notes]` sort `dictionary <nom>`, un `[prepend call
  add_new_notes]` en fait `call add_new_notes dictionary <nom>`. Pour une piste : `call create_midi_clip 0. 16.`
  (début et longueur en temps) ; pour un emplacement de clip : `call create_clip 16.`. Non vérifié dans Live.
- **[observé]** L'aide de `live.miditool.out` construit un dictionnaire de notes sans JS : `dict.pack pitch:
  start_time: duration:0.25` pour chaque note, ajoutée à un `array @name ---notes`, puis `dict.pack notes:`. Un
  `array` d'un seul élément reste un tableau, ce qui contourne le problème d'une seule note [déduit].
- [inconnu] La forme exacte de la réponse d'`add_new_notes` à la sortie de `live.object` (liste d'identifiants ou
  `dictionary <nom>`).

### Ce que ça implique pour le device

L'impression peut se faire sans JS : `dict.pack` et `array` construisent le dictionnaire, `live.object` l'envoie.
Avant de s'appuyer dessus, un essai dans Live doit fixer la syntaxe `dictionary <nom>` ou JSON brut, la forme de la
réponse, et la taille d'un appel (une Track et une fenêtre par appel, comme le ticket 01 le proposait déjà).

## Ce qui reste ouvert et se mesure

1. Persistance : relecture d'un gros Blob (Mo), temps de sauvegarde et de chargement, entrées d'annulation en 12.2.1
   et en 12.4, ordre de la valeur sauvée par rapport à `live.thisdevice`.
2. Externals : `threadsafe::yes` obligatoire en Min sur le chemin du jeu ; le noyau simulé ne couvre ni temps, ni
   dictionnaires, ni transport.
3. Timing : offset sous le vecteur jusqu'à MD Drums, et comportement des événements en attente à un saut ou une
   boucle.
4. Tests : comportement du `Max.exe` de Live lancé seul, absence de MIDI sortant hors édition.
5. LOM : syntaxe exacte de `call add_new_notes` avec un dictionnaire.

## Sources

Cycling '74, documentation (les pages du guide et de référence sont aussi livrées avec Live dans
`Resources\Max\resources\docs`) :

- Device Parameters in Max for Live : https://docs.cycling74.com/userguide/m4l/live_parameters/
- Parameter Mode : https://docs.cycling74.com/userguide/parameter_mode/
- Using pattr in Live Devices : https://docs.cycling74.com/userguide/m4l/live_pattr/
- pattr (guide) : https://docs.cycling74.com/userguide/pattr/
- Max for Live Limitations : https://docs.cycling74.com/userguide/m4l/live_limitations/
- Freezing Max for Live Devices : https://docs.cycling74.com/userguide/m4l/live_freezing/
- Unfreezing Devices : https://docs.cycling74.com/userguide/m4l/live_unfreezing/
- Using Symbols in Max for Live : https://docs.cycling74.com/userguide/m4l/live_symbols/
- Timing and Synchronization in Max for Live : https://docs.cycling74.com/userguide/m4l/live_timing/
- Live API Overview : https://docs.cycling74.com/userguide/m4l/live_api_overview/
- MIDI Tools : https://docs.cycling74.com/userguide/m4l/live_miditools/
- Patcher Lifecycle : https://docs.cycling74.com/userguide/patcher_lifecycle/
- Externals : https://docs.cycling74.com/userguide/externals/
- Search Path : https://docs.cycling74.com/userguide/search_path/
- Scheduler and Priority : https://docs.cycling74.com/userguide/scheduler/
- Sample Accurate Messages : https://docs.cycling74.com/userguide/sample_accurate_messages/
- Transport : https://docs.cycling74.com/userguide/transport/
- Time Value Syntax : https://docs.cycling74.com/userguide/time_value_syntax/
- Références d'objets : https://docs.cycling74.com/reference/pattr/, https://docs.cycling74.com/reference/pattrstorage/,
  https://docs.cycling74.com/reference/dict/, https://docs.cycling74.com/reference/coll/,
  https://docs.cycling74.com/reference/table/, https://docs.cycling74.com/reference/array/,
  https://docs.cycling74.com/reference/string/, https://docs.cycling74.com/reference/metro/,
  https://docs.cycling74.com/reference/delay/, https://docs.cycling74.com/reference/pipe/,
  https://docs.cycling74.com/reference/timepoint/, https://docs.cycling74.com/reference/transport/,
  https://docs.cycling74.com/reference/schedule/, https://docs.cycling74.com/reference/max/,
  https://docs.cycling74.com/reference/live.object/, https://docs.cycling74.com/reference/live.thisdevice/,
  https://docs.cycling74.com/reference/live.step/, https://docs.cycling74.com/reference/live.grid/
- LOM : https://docs.cycling74.com/apiref/lom/clip/, https://docs.cycling74.com/apiref/lom/track/,
  https://docs.cycling74.com/apiref/lom/clipslot/
- SDK 8.2, Threading : https://sdk.cdn.cycling74.com/max-sdk-8.2.0/chapter_threading.html
- SDK 8.2, Scheduler : https://sdk.cdn.cycling74.com/max-sdk-8.2.0/chapter_scheduler.html
- SDK 8.2, module threading : https://sdk.cdn.cycling74.com/max-sdk-8.2.0/group__threading.html
- SDK 5.1.7, ITM : https://sdk.cdn.cycling74.com/max-sdk-5.1.7/chapter__itm.html
- Support, Max for Live : https://support.cycling74.com/hc/en-us/articles/360049845974-Common-questions-about-Max-for-Live
- Support, patchs sans licence : https://support.cycling74.com/hc/en-us/articles/360049930694-Do-I-have-to-purchase-Max-to-open-and-use-a-patcher
- Support, autorisation Max 7-9 : https://support.cycling74.com/hc/en-us/articles/360049995834-Max-7-9-Authorization
- Article « What's new in Live 11, part 1 » : https://cycling74.com/articles/what's-new-in-live-11-part-1
- Article « What's new in Live 11, part 2 » : https://cycling74.com/articles/what's-new-in-live-11-part-2

Dépôts GitHub (README, code et API GitHub, lus le 2026-10-10) :

- https://github.com/Cycling74/max-sdk
- https://github.com/Cycling74/max-sdk-base (dont `c74support/max-includes/ext_itm.h`)
- https://github.com/Cycling74/min-api (dont `include/c74_min_message.h`, `include/c74_min_object.h`, `test/`)
- https://github.com/Cycling74/min-devkit (exemples `source/projects/*/*_test.cpp`)
- https://github.com/Cycling74/mock
- https://github.com/Cycling74/min-lib
- https://github.com/Cycling74/max-test (dont `ruby/test.rb`, `init/testpackage-init.txt`, `source/projects/oscar`)
- https://github.com/Ableton/maxdevtools (lignes directrices de production, Load Time Tester, CPU Reporter, maxdiff)
- https://github.com/twhiston/mUnit
- https://github.com/ideoforms/AbletonOSC

Notes de version :

- Live 12 (12.4 : visibilités Visible, Undo When Visible) : https://www.ableton.com/en/release-notes/live-12/
- Max 9.0.0 : https://cycling74.com/releases/max/9.0.0
- Max 9.1.0 : https://cycling74.com/releases/max/9.1.0
- Max 9.1.4 : https://cycling74.com/releases/max/9.1.4

Communauté :

- What is the maximum size blob I can store in a pattr? : https://cycling74.com/forums/what-is-the-maximum-size-blob-i-can-store-in-a-pattr
- Automatically storing Dict's inside a Ableton Session… : https://cycling74.com/forums/automatically-storing-dict's-inside-a-ableton-session-inside-a-max-for-live-device-again
- Saving a Dict as part of a Live Set (C++ Min) : https://cycling74.com/forums/saving-a-dict-as-part-of-a-live-set-with-dict-not-being-updated-from-c-min-api
- Store arbitrary number of values within a Live Set : https://cycling74.com/forums/store-arbitrary-number-of-values-within-a-live-set
- Storing the data in a coll object in a Live Set : https://cycling74.com/forums/storing-the-data-in-a-coll-object-in-a-live-set
- Coll, Dict & Pattr within Max for Live : https://cycling74.com/forums/coll-dict-pattr-within-max-for-live
- Pattrstorage, recall and line to interpolate… : https://cycling74.com/forums/pattrstorage-recall-and-line-to-interpolate-between-presets-is-glitching-why
- pattrstorage responds slower with each recall : https://cycling74.com/forums/pattrstorage-responds-slower-with-each-recall-of-parameters
- initialization order inconsistency : https://cycling74.com/forums/initialization-order-inconsistency
- add_new_notes/apply_note_modifications (dictionary) : https://cycling74.com/forums/ableton-live11-add_new_notesapply_note_modifications-dictionary-methode
- Appending to an array of dictionaries : https://cycling74.com/forums/appending-to-an-array-of-dictionaries
- midi jitter : https://cycling74.com/forums/midi-jitter
- Best method for tight syncing to Live transport in M4L : https://cycling74.com/forums/best-method-for-tight-syncing-to-live-transport-in-m4l
- metro for syncing to Live transport / quantized MIDI note output : https://cycling74.com/forums/metro-for-syncing-to-live-transport-quantized-midi-note-output
- ;max quit does not quit cleanly : https://cycling74.com/forums/max-quit-does-not-quit-cleanly
- Spat et Max 9 (IRCAM) : https://discussion.forum.ircam.fr/t/spat-and-max9/140729

Observé sur la machine (Live 12.2.1 Suite, `C:\ProgramData\Ableton\Live 12 Suite`) :

- pages de référence 9.0.7 : `Resources\Max\resources\docs\refpages\max-ref\{pattr,pattrstorage,dict,coll,table,array,string,metro,delay,pipe,timepoint,transport,schedule,max}.maxref.xml`,
  `…\m4l-ref\{live.object,live.thisdevice,live.step,live.grid}.maxref.xml` ;
- guide 9.0.7 : `Resources\Max\resources\docs\userguide\content\` (`parameter_mode.json`, `pattr.json`,
  `patcher_lifecycle.json`, `scheduler.json`, `sample_accurate_messages.json`, `transport.json`,
  `time_value_syntax.json`, `externals.json`, `search_path.json`, `m4l\*.json`) ;
- aides : `Resources\Max\resources\help\max\pattr.maxhelp`, `dict.maxhelp`, `…\help\m4l\live.miditool.out.maxhelp` ;
- binaires : `Resources\Max\Max.exe` (options de ligne de commande), `Resources\Max\resources\externals\m4l\live.object.mxe64`,
  `live.testhelper.mxe64`, `Resources\MIDI Remote Scripts\_MxDCore\MxDCore.pyc` ;
- devices d'Ableton : `Documents\Ableton\Factory Packs\Sequencers\{Rhythmic Steps,SQ Sequencer,Step Arp}\Ableton Folder Info\*.amxd` ;
- sets : `Documents\Ableton\Factory Packs\Sequencers\Sequencers Demo Set.als`, `Documents\Ableton\User Library\PROJECTS\BODY.als`
  (Blobs `MxDBlob` décodés).
