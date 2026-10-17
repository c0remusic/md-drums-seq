# Structure du moteur en Max natif

Type: grilling
Status: resolved
Assignee: c0remusic (session du 2026-10-10)
Blocked by: 02

## Question

Pas de JS dans le device (ADR 0001) : comment s'organise le moteur en Max natif (skill `codebase-design`) ?

- Où vit le Pattern (`dict`, `coll`, `table`, paramètres `live.*`) et comment il se sauve dans le set ;
- qui tire les Conditions (`A:B`, `PRE`, `NEI`, `X%` avec sa graine par Pass, `1ST`, Fill) et à quel moment : au pas
  même, sur le scheduler haute priorité, ou un peu en avance ;
- comment le résultat d'un pas devient des notes (Hit, notes de Lock sur le canal de la Track, ratchets, micro-timing)
  émises à l'heure ;
- quelles abstractions (une par Track ? une pour le tirage, une pour l'émission ?) et quelle frontière avec l'UI et
  avec la LOM de l'impression ;
- quel garde-fou sans tests Node : un patch de test dans Max qui joue des Patterns connus et compare ce qui sort, un
  enregistrement MIDI comparé hors ligne, ou autre ?

## Answer

Grilling du 2026-10-10 en trois rounds, sur les faits de `research/08-max-natif.md` ; les douze décisions sont dans les
commentaires ci-dessous, et l'ADR 0002 les résume.

- **Où vit la logique** : dans un external C++ (`mdseq.mxe64`), code natif compilé, sur le fil du scheduler. Son noyau,
  une bibliothèque C++17 sans en-tête Max, tient le Pattern, le tirage des Conditions, les événements de chaque pas, le
  rendu de N Passes pour l'impression, le journal de la capture et le dessin des cases. L'adaptateur (C SDK,
  `max-sdk-base`) tient l'horloge ITM en ticks, sort les octets MIDI vers `midiout`, et fait passer les éditions de
  l'UI au fil du scheduler (`schedule()`) et les sorties vers le fil principal (`defer_low`). Le patch garde l'UI
  (`live.*`, `lcd`), `live.object` et `midiout`, et ne fait que router.
- **Quand** : le pas n se planifie à la frontière du pas n−1 (un pas d'avance, qu'impose le micro-timing −23/24) ;
  Mute se lit à l'émission, Fill au tirage ; un Pass neuf chaque fois que le pas joué recule ou reste ; `SEEK` et arrêt
  vident la file avec leurs note-off. `X%` tire hash(graine du Pass, Track, pas), sans état, la graine de départ étant
  tirée au départ du transport.
- **Comment ça devient des notes** : le flux joué est celui qu'imprime l'impression (Hit et Locks durent un pas ou une
  tranche de ratchet, note-off avant note-on, Locks avant Hit), d'où la propriété « impression = jeu ».
- **Persistance** : 16 `pattr` Blob *Stored Only*, un par Pattern, écrits à la fin de chaque geste ; à mesurer avant
  la banque de Patterns (ticket « Persistance des Patterns par Blob »).
- **Garde-fou** : build Release puis `ctest` sur le noyau, en temps virtuel, avant chaque commit et push ; l'adaptateur,
  le patch et la LOM par une checklist Live par ticket.

Esquisse d'interface retenue pour démarrer, à laisser évoluer sous les tests (quatre esquisses comparées, celle qui
optimise l'appelant le plus fréquent gardée) :

```cpp
namespace mdseq
{
	struct Transport { double ticks; bool playing; bool jumped; };
	struct MidiEvent { double tick; uint8_t bytes[3]; };
	struct Wake { const MidiEvent* begin; const MidiEvent* end; double next; Position at; };

	class Sequencer	// construit sur le fil principal ; ensuite, fil du scheduler seulement
	{
	public:
		Wake wake(const Transport& _now);	// tout ce qui est dû, et le tick du prochain réveil
		Touched input(const Input& _in);	// une édition, Fill, Mute, bascule
		...
	};

	TrackNotes print(const Pattern& _pattern, uint64_t _startSeed, int _passes);	// pur, fil principal
	CellOps drawCell(const Step& _step, ...);	// pur : primitives lcd dans l'ordre de Design/HANDOFF.md
}
```

Restent à trancher en construisant, chacun dans son ticket : ce que veut dire « le dernier Lock de ce paramètre sur la
Track » (le pas précédent le plus proche qui en porte un, ou la dernière édition), le moment d'une bascule immédiate
(premier pas non encore planifié), et si la mémoire de `PRE`/`NEI` repart à zéro avec le compteur de Passes.

## Comments

- 2026-10-10, après la tranche verticale : la décision 1 disait qu'une édition pendant le pas n−1 compte au pas
  n+1. En jouant, l'utilisateur l'a trouvé gênant (« il faudrait que le premier soit pris en compte ») : une édition
  du pas planifié et pas encore commencé replanifie maintenant sa Track, et s'entend sur ce Pass. Un pas commencé
  n'est jamais joué en retard. Fill reste lu au tirage.
- 2026-10-13, avec le micro-timing (ticket 19) : les Tracks d'un pas commencent maintenant à des instants
  différents ; la replanification se décide par Track. Une Track qui a déjà commencé le pas planifié garde ce qu'elle
  joue et donne à NEI et PRE son résultat ; les autres sont replanifiées.

- 2026-10-09 : ce ticket était « Interface du moteur JS » ; l'utilisateur a écarté le JS (non natif, et `js`/`v8` ne
  tournent que sur le fil basse priorité, d'où de la gigue sur le jeu en direct).
- 2026-10-10, grilling, round 1 (recommandations acceptées par l'utilisateur) :
  1. **Tirage un pas d'avance** : à la frontière du pas n−1, les 16 Tracks du pas n s'évaluent de 1 à 16, Conditions
     tirées et Fill lu, puis tous les événements du pas n sont programmés à leur heure (grille + micro-timing + tranche
     de ratchet). Le micro-timing −23/24 l'impose ; une édition ou un Fill pendant le pas n−1 compte au pas n+1.
  2. **Frontière de Pass** : un Pass neuf chaque fois que le pas joué recule ou reste (fin de Pattern, boucle, saut
     arrière, saut avant qui franchit la fin) ; un saut avant dans le Pass saute des pas. Événements en attente jetés à
     chaque saut et à l'arrêt, avec leurs note-off.
  3. **`X%`** : graine tirée au départ du transport et gardée pour l'impression ; graine du Pass = hash(graine de départ,
     numéro de Pass) ; tirage d'un pas = hash(graine du Pass, Track, pas), sans état, donc insensible aux éditions
     ailleurs ; un tirage pour tout un groupe de ratchets.
  4. **Flux joué = flux imprimé** : Hit et Locks durent un pas (ou une tranche de ratchet), raccourcis si la même note
     revient plus tôt (micro-timing) ; au même instant, note-off avant note-on, puis Locks avant Hit. La capture
     enregistre le flux tel quel.
  5. **Mute lu à l'émission** (coupe aussi les événements déjà programmés) ; Fill lu au tirage.
- 2026-10-10, round 2 (faits : `research/08-max-natif.md` ; recommandations acceptées) :
  6. **La logique vit dans un external C++** : un noyau C++17 sans en-tête Max (Pattern, tirage, événements d'un pas en
     ticks, rendu de N Passes), testé sous ctest ; un adaptateur `mdseq.mxe64` mince en C SDK (`max-sdk-base`), sur le
     fil du scheduler ; le patch garde l'UI (`live.*`, `lcd`), `live.object` et `midiout`. Écartés : le patch pur
     (logique illisible en boîtes, garde-fou seulement dans Max) et `gen~` (sortie MIDI un vecteur en retard, aucun test
     hors de Max) ; Min écarté pour l'adaptateur (`threadsafe::yes` partout, dépôt moins suivi). L'ADR 0001 gagne une
     ligne : un external compilé est natif.
  7. **16 `pattr` Blob *Stored Only*, un par Pattern**, liste d'entiers au format versionné, écrits à la fin de chaque
     geste d'édition : une entrée d'annulation par geste, et Ctrl+Z dans Live recharge probablement l'ancien Pattern. Un
     ticket de mesure le vérifie avant la construction (Blob dense relu intact, temps de sauvegarde, entrées
     d'annulation en 12.2.1 et 12.4, Ctrl+Z, ordre de rappel par rapport à `live.thisdevice`).
- 2026-10-10, round 3 (recommandations acceptées) :
  8. **L'adaptateur tient l'horloge** : un réveil ITM en ticks sur le prochain instant utile (frontière de pas ou
     événement) ; à la frontière du pas n−1, le noyau planifie le pas n dans une file préallouée triée en ticks
     flottants ; à chaque réveil, tout ce qui est dû sort en octets MIDI vers `midiout`. Pas joué calculé depuis les
     ticks du transport (ancre, longueur, échelle) ; `SEEK` et arrêt vident la file avec leurs note-off ; le tempo qui
     change entre tirage et émission est suivi. Écartés : `metro @quantize` + un `pipe` par événement, et
     `plugsync~` + `edge~` (un vecteur de retard).
  9. **Noyau confiné au fil du scheduler** : les messages de l'UI y passent par `schedule()`, les sorties vers l'UI, le
     Blob et la LOM repartent par `defer_low` sur des copies ; aucun verrou. Le travail lourd (rendu de N Passes,
     sérialisation) se fait sur le fil principal à partir d'une copie du Pattern prise côté scheduler (environ 32 Ko
     dense).
  10. **Garde-fou** : un `CMakeLists.txt` à la racine (noyau, tests, external `mdseq` avec `max-sdk-base` en
      FetchContent épinglé sur un commit) ; build Release puis `ctest` vert avant chaque commit et push. Tests du noyau
      en temps virtuel : Pattern + script de transport contre la liste exacte des événements, propriété « impression =
      jeu » (même graine, mêmes N Passes), aller-retour de sérialisation et versions du format, dessin de cases de
      référence. L'adaptateur dans Max, le patch et la LOM : checklist Live par ticket. Un exécutable de test par
      module, `main` simples, sans dépendance de test ; conventions de code de gearmulator-md-drums.
  11. **Le noyau calcule le dessin** : une fonction pure (pas, joué, sélectionné) rend les primitives `lcd` dans l'ordre
      de `Design/HANDOFF.md`, que l'adaptateur sort sur le fil principal ; le panneau du pas reçoit ses valeurs par
      `set` ; les règles d'édition (Lock qui part du dernier Lock du paramètre sinon de 64, 63 sauté, Alt-clic) vivent
      dans le noyau. Le patch route.
  12. **Frontière LOM** : le noyau rend les notes de chaque Track (impression : N Passes depuis une graine ; capture :
      journal des événements émis dans un anneau préalloué) ; l'adaptateur en fait un `dict` nommé par Track
      (`dictobj`) et sort `dictionary <nom>` ; le patch crée le clip puis envoie `call add_new_notes dictionary <nom>`
      par `live.object`, une Track à la fois. Le reste (Session ou Arrangement, durée de capture, taille du journal) :
      ticket 07 ; la syntaxe `dictionary` est à vérifier dans Live.
