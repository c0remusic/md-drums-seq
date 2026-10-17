# Map : md-drums-seq v1

Label: wayfinder:map

## Destination

md-drums-seq v1 : un device Max for Live qui joue MD Drums en direct dans Live, et qui imprime à la demande ses Hits et
leurs Locks dans des clips Live, lesquels rejouent MD Drums à l'identique sans le device. Fini quand : joué par
l'utilisateur dans Live, timing mesuré (en direct et depuis un clip imprimé), garde-fou vert, poussé sur `main`. Pas de
tag.

## Notes

- **Exécution dans la carte** (dérogation à « plan, don't do », décidée au cadrage du 2026-10-08, comme pour la carte
  éditeur de MD Drums) : les tickets `task` construisent. Un ticket par session, sauf la recherche.
- Domaine : `CONTEXT.md` (Track, Hit, Lock, Kit repris de MD Drums ; Pattern, Pass, Condition, Fill). Contrat avec le
  plug-in : ADR 0003 de gearmulator-md-drums (`docs/adr/0003-locks-travel-as-notes.md`) : Hits = notes 36-51, Locks =
  notes 64-87 sur le canal MIDI de leur Track, vélocité 1-63 → valeurs 0-62, 64-127 → elles-mêmes. Amendé le
  2026-10-11 (ticket « Locks ») : Live ramène sur le canal 1 les notes d'un device, alors un Lock sur le canal 1 se lie
  au dernier Hit reçu avant lui au même échantillon ; le device envoie chaque Hit suivi de ses Locks.
- Skills : `grilling` + `domain-modeling` par défaut ; `codebase-design` pour l'interface du moteur ;
  `plugin-gui-design` pour les maquettes de l'UI (versionnées, validées par l'utilisateur) ; `tdd` pour les tickets
  `task` qui construisent ; `research` pour la recherche.
- Recherches : un doc par ticket dans `research/` à côté de cette carte (pas de branche `research/*`), comme pour MD
  Drums.
- Garde-fou (décidé par « Structure du moteur en Max natif ») : build Release puis `ctest` vert sur le noyau C++, en
  temps virtuel, avant chaque commit et push dès que la tranche verticale l'a posé ; l'adaptateur, le patch et la LOM
  par une checklist Live par ticket. Push sans demander une fois vert. Jamais de tag.
- Construction (décidée le 2026-10-10, après « Structure du moteur en Max natif ») : tranche verticale d'abord (le
  device joue des Hits), puis un ticket `task` par ajout, sans grilling : Locks, impression, Conditions, ratchets,
  micro-timing, 64 pas et échelle, banque de Patterns et sauvegarde, UI de la v4.
- **Cadrage (2026-10-08)** :
  - destination = le device lui-même (exécution dans la carte) ;
  - v1 : Hits, vélocité, Locks sur les 24 paramètres, toutes les Conditions (`X%`, `A:B`, `FILL`/`!FILL`,
    `PRE`/`!PRE`, `NEI`/`!NEI`, `1ST`), micro-timing, ratchets, Fill, mutes ; Accent, Swing et Slide globaux plus tard
    dans la carte ;
  - un seul device, sur sa propre piste MIDI, qui écrit dans les clips des 16 pistes Track ; dans Live, chaque Track a
    sa piste MIDI avec un External Instrument vers MD Drums (canal N, retour Out N) ;
  - le device tient le Pattern (vérité) ; les clips reçoivent ce qu'il joue, Conditions tirées ;
  - ~~Live joue les clips : le device écrit en avance de la tête de lecture~~ ; remplacé le 2026-10-09 (l'écriture
    LOM noie l'historique d'annulation, et le but est de jouer en direct) : **le device joue en direct par `midiout`,
    et n'écrit par LOM que pour imprimer, sur demande et d'un seul lot** (N Passes, ou la capture après coup d'une
    performance) ;
  - jusqu'à 64 pas communs aux 16 Tracks, pas = double-croche par défaut, échelle globale façon MKII (1/8× à 2×) ;
  - 16 Patterns par device, bascule mise en file à la fin du Pass (option immédiate), pas de chaîne ;
  - Patterns stockés dans le set Live seulement ;
  - ~~logique en module JS pur testé sous Node~~ ; remplacé le 2026-10-09 : **pas de JS, tout en Max natif** (logique,
    UI, timing). `js` et `v8` ne tournent que sur le fil basse priorité de Max : sur le chemin du jeu en direct, ils
    créent de la gigue (ADR 0001) ;
  - Push 2 et l'enregistrement live en fin de carte ; v1 d'abord à la souris ;
  - dépôt privé `c0remusic/md-drums-seq`, tickets en markdown local.

## Decisions so far

- [Ce que LOM permet dans Live 12](issues/01-lom-live-12.md) : l'API `Clip` couvre l'écriture dans un clip en lecture,
  mais sans borne connue sur le délai (lookahead à mesurer) ; chaque écriture LOM entre dans l'historique d'annulation,
  sans regroupement possible depuis Max ; capture en Arrangement par `create_midi_clip` (12.1.10), crash corrigé en
  12.4 ; `v8` charge du CommonJS ES2022 ; un External Instrument ne se règle pas par LOM ; machine en Live 12.2.1.
- [Direction visuelle du device](issues/09-direction-visuelle.md) : bande de 169 px pour jouer, grande fenêtre pour
  éditer ; look natif de Live ; grille de 32 pas visibles avec badges de Condition.
- [Sémantique du Pattern](issues/02-semantique-du-pattern.md) : Conditions à la manière d'Elektron (compteur depuis le
  démarrage du Pattern, `PRE` sur la même Track, `NEI` sur N−1, liste `X%` d'Elektron), ratchets 1-8 répartis dans le
  pas, micro-timing ±23/24, Locks réémis avec chaque Hit, 63 sauté ; pas calé sur la position de Live.
- [Paramètres de MD Drums visibles par LOM](issues/12-parametres-md-drums-lom.md) : la LOM ne voit que les paramètres
  configurés dans Live (128 au plus, aucun par défaut au-delà de 64) ; le device peut lire les 16 Machines une fois
  configurées (noms de SYN1-8 par table embarquée), mais pas les valeurs du Kit des 384 paramètres de Track.
- [Locks de MD Drums poussés](issues/03-locks-md-drums-pousses.md) : sur `main` de gearmulator-md-drums (`4d8e4a62`),
  fusionnés avec les Hits à l'échantillon près et les effets master, garde-fou 32/32 ; Out 01 à 1,52 ms sans gigue.
- [Maquettes de l'UI](issues/10-maquettes-ui.md) : v4 approuvée, passation dans `Design/HANDOFF.md` ; bande et fenêtre
  en objets `live.*`, pas et grille en `lcd` (vélocité en teinte, crans de ratchets, bande et texte de Condition, point
  de Lock, flèche de micro-timing). v5 approuvée le 2026-10-16 (la v4 telle que Max la construit, avec les valeurs du
  Kit, sans textes d'aide) et construite au [ticket 28](issues/28-ui-v5.md), vue dans Live le 2026-10-17 et approuvée par l'utilisateur.
- [Structure du moteur en Max natif](issues/08-structure-du-moteur-natif.md) : la logique vit dans un external C++
  (`mdseq.mxe64`, ADR 0002) dont le noyau sans Max est testé sous ctest ; pas n planifié à la frontière de n−1, horloge
  ITM en ticks dans l'adaptateur, noyau confiné au fil du scheduler ; flux joué = flux imprimé ; 16 `pattr` Blob, un
  par Pattern ; recherche dans `research/08-max-natif.md`.
- [Tranche verticale](issues/13-tranche-verticale.md) : le device joue MD Drums dans Live au même échantillon qu'un
  clip, sans gigue mesurable (mesure au ticket 04) ; dans un device, le transport de Live est l'ITM du minuteur
  (`'live'`), pas `itm_getglobal()`. Vérifications dans Live par le pont `TupleMCP` (`measure/live/`).
- [Impression et capture](issues/07-impression-et-capture.md) : un clip Session par Track, sur sa piste MIDI routée
  vers la piste du device sur « N-MD Drums » (droit au plug-in, canal gardé, sans External Instrument) ; pistes
  retrouvées par leur routage, créées si elles manquent ; N Passes du Pattern affiché depuis la graine du dernier
  départ ; scène sélectionnée ou neuve, rien n'est écrasé ; la règle du canal 1 de MD Drums à restreindre au Hit de ce
  canal. Construction au ticket 22 ; capture et Arrangement au ticket 23, après Live 12.4.
- [Routage External Instrument](issues/05-routage-external-instrument.md) et
  [Jeu en direct par midiout](issues/06-jeu-en-direct.md) : fermés le 2026-10-17, dépassés par la construction ; le
  device joue devant MD Drums sur sa piste, les clips imprimés vont droit à MD Drums, les deux jouent ensemble.
- [Charge MIDI que Max for Live supporte](issues/24-charge-midi-de-max-for-live.md) : Live garde les notes tenues
  par un device Max dans un ensemble de 128 et quitte quand il déborde (lu dans le dump du plantage du 2026-10-13 :
  `boost::container::bad_alloc` lancée par Live). Joué en direct, un Lock finit aussitôt : 17 notes tenues au plus
  au lieu de 399, Locks toujours reçus par MD Drums (mesuré). Scénario rejoué le 2026-10-17 : deux instances denses,
  le build corrigé tient 20 s, celui d'avant plante en 2 s au même endroit. Livré en 0.1.1.

## Not yet specified

- La construction après la tranche verticale (ticket 13) : un ticket `task` par ajout (voir Notes), créé quand on y
  arrive ; la banque de Patterns attend « Persistance des Patterns par Blob » (ticket 14).
- Accent, Swing et Slide façon MD (ticket 26) : Swing construit le 2026-10-15 ; Accent et Slide attendent une entrée dans
  MD Drums (un accent au-delà de la vélocité 127, des Locks sans Hit ou un glissement).
- Push 2 : grille de pas, Track choisie, encodeurs pour les Locks pendant qu'un pas est tenu, et la bascule avec le mode
  contrôleur de MD Drums (sa carte éditeur le prévoit). Les banques de paramètres de MD Drums : ticket 25. Mode
  « à la Tuple » par `grab_control` : [ticket 33](issues/33-mode-push.md), recherche faite le 2026-10-17, six
  questions pour la séance.
- Enregistrement live : Hits joués en temps réel, quantisés, Locks pris des mouvements d'encodeurs.
- Format de stockage des Patterns dans le set (taille, versions du format).
- ~~Mise en place dans Live : set modèle ou réglage à la main des External Instruments~~ : plus de question depuis
  l'impression (ticket 22), qui crée elle-même les pistes et les route droit à MD Drums ; les Machines viennent du
  fichier de MD Drums (ticket 27).
- [Noms des Tracks par MD Drums](issues/27-noms-des-tracks.md) : construit et vérifié dans Live le 2026-10-16 avec
  MD Drums 035a88e5 (hello, fichier, onglets, rangées, titre et SYN1-8 nommés) ; le changement de Machine dans MD
  Drums vérifié par l'utilisateur le 2026-10-17. Les valeurs du Kit sur les boutons de Lock : ticket 28. Les noms qui
  revenaient aux numéros (fichier occupé pendant que MD Drums le remplace, hellos retardés par le fil principal de
  Max) : [ticket 32](issues/32-noms-qui-clignotent.md), corrigé le 2026-10-17.
- [Copier un Pattern, doubler sa longueur](issues/29-copie-et-double.md) : menu « Copy to », bouton « ×2 »,
  puis Clear (2026-10-17) ; vérifiés dans Live par l'utilisateur.
- [Mode Fill et états des Patterns](issues/30-mode-fill-et-etats.md) : construits et vérifiés dans Live le
  2026-10-17.
- [Chaînes de Patterns](issues/31-chaines.md) : construites le 2026-10-17 (mode Chain, une Pass par Pattern,
  menées par l'Editor), vérifiées dans Live ; gardées dans le set par un Blob.
- ~~Charge LOM avec 16 pistes réécrites en continu~~ : sans objet depuis le 2026-10-09, la LOM n'écrit que pour
  imprimer.
- Mesure de bout en bout : faite le 2026-10-17 ([ticket 04](issues/04-timing-midiout.md)) : en direct, depuis
  un clip imprimé et depuis un clip fait à la main, MD Drums sonne 2 échantillons après la grille, décalage 0 entre
  les trois, sans gigue mesurable.

## Out of scope

- Polymétrie (longueur par Track ou par rangée) : écartée au cadrage.
- Scènes de Patterns : écartées au cadrage. Les chaînes, écartées aussi, ont été construites à la demande de
  l'utilisateur (ticket 31).
- Pas méta façon Cirklon (saut, sens de lecture) : écartés au cadrage.
- Kit par Pattern : le séquenceur ne change jamais le Kit, et la SysEx ne traverse pas une piste Live.
- Export et import de Patterns en fichiers.
- Locks portés par MPE : écartés par l'ADR 0003 de MD Drums.
