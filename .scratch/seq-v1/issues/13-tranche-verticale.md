# Tranche verticale : le device joue des Hits

Type: task
Status: resolved
Assignee: c0remusic (session du 2026-10-10)
Blocked by: 08

## Question

Le plus court chemin de bout en bout : un device qui joue MD Drums dans Live depuis l'external `mdseq` (ADR 0002).

- Le dépôt gagne son build (`CMakeLists.txt` à la racine, Visual Studio 2022) et son garde-fou : build Release puis
  `ctest` vert, écrit dans `CLAUDE.md`.
- Le noyau, sans Max : un Pattern de 16 Tracks × 16 pas (Hit et vélocité), le pas joué calculé depuis les ticks du
  transport, le pas n planifié à la frontière du pas n−1, des note-on et note-off d'un pas, les Mutes lus à l'émission,
  la file vidée avec ses note-off à l'arrêt et aux sauts. Testé en temps virtuel.
- L'adaptateur `mdseq.mxe64` (C SDK, `max-sdk-base` en FetchContent épinglé) : réveil ITM en ticks, octets MIDI vers
  `midiout`, éditions passées au fil du scheduler.
- Un device `.amxd` minimal, écrit par script comme ceux de `measure/` : `mdseq`, une grille de 16 × 16 cases pour les
  Hits, les Mutes, `midiout`.
- Checklist Live pour l'utilisateur : le device avant MD Drums sur sa piste, des Hits posés à la souris s'entendent,
  le timing mesuré contre un clip avec `measure/onsets.py`.

Hors tranche : Locks, Conditions, ratchets, micro-timing, 64 pas, échelle, banque de Patterns et sauvegarde, impression,
UI de la v4.

## Answer

Le device joue MD Drums dans Live, au même échantillon qu'un clip et sans gigue mesurable (ticket 04, commentaire du
2026-10-10). Vérifié par le pont `TupleMCP` dans le set de l'utilisateur, après la correction du transport (le
device lisait le transport interne de Max, pas celui de Live : ticket 15) : sortie de MD Drums entre 0,77 et 0,88
pendant 4 s de lecture, 62 notes envoyées. Ce que le pont ne voit pas (l'aspect et les clics) reste à la checklist de
l'UI v4 (ticket 15), qui a remplacé l'interface provisoire de cette tranche.

## Comments

- 2026-10-10 : construit. `CMakeLists.txt` à la racine, garde-fou écrit dans `CLAUDE.md` ; le noyau
  (`source/mdseq/mdseqSequencer.*`) et ses 14 scénarios en temps virtuel (`mdseqSequencerTest`, vert, aucune allocation
  pendant dix Passes de 256 Hits) ; l'adaptateur `mdseq.mxe64` (ne dépend que de `MaxAPI.dll`) ; le device
  `MD Drums Seq.amxd`, écrit à côté de lui dans `build/device` par `tools/make_device.py`. Le SDK ne documente aucune
  notification de départ ou de saut du transport : l'adaptateur regarde l'ITM chaque milliseconde (départ, arrêt, saut
  au-delà d'un demi-pas d'écart) et se réveille sur un minuteur ITM au tick que demande le noyau ; un départ ou un saut
  vu jusqu'à 20 ms en retard envoie encore les Hits du pas atteint.

### Checklist pour l'utilisateur

1. Construire si besoin (`cmake --build build --config Release`), puis ajouter `C:\dev\md-drums-seq\build\device` aux
   Places du navigateur de Live.
2. Sur la piste de MD Drums, glisser `MD Drums Seq` **devant** MD Drums. Ouvrir la fenêtre Max (clic droit sur le
   device, ou Max Console) : si elle dit `mdseq: No such object`, copier `mdseq.mxe64` dans
   `Documents\Max 9\Packages\md-drums-seq\externals\` et recharger le device ; noter lequel des deux a marché.
3. Lancer la lecture. Dans la grille, une colonne est un pas (16), une rangée une Track (Track 1 en haut) : un clic
   pose ou retire un Hit. Vérifier : les Hits s'entendent à leur place ; la rangée du haut suit le pas joué ; un Hit posé
   sur le pas qui va jouer s'entend au Pass suivant ; la colonne de gauche coupe une Track tout de suite ; Stop coupe
   tout ; la boucle de l'Arrangement et un clic plus loin dans la timeline repartent du bon pas.
4. Timing, comme la checklist du ticket 04 mais avec ce device à la place de `md-seq-probe` : piste « Ref » avec un
   clip de note 36 à chaque double croche vers un Operator à attaque nette ; piste « Probe » avec `MD Drums Seq`
   (Track 1 sur les 16 pas) vers un Operator identique ; enregistrer les deux en audio 4 mesures depuis la mesure 1,
   métronome coupé, et donner les chemins des WAV pour `measure/onsets.py --bpm 120`.
