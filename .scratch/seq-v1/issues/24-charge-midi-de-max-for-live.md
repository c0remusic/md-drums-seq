# Charge MIDI que Max for Live supporte

Type: task
Status: resolved
Assignee: —
Blocked by: —

## Question

Live a planté le 2026-10-13 à 18:11:56 (journal d'applications de Windows, Event 1000 : `Ableton Live 12 Suite.exe`,
module `ucrtbase.dll`, exception `0xc0000409`, code d'arrêt 7, `abort()` après une exception C++ non attrapée ; dump
complet dans `%LOCALAPPDATA%\CrashDumps\Ableton Live 12 Suite.exe.22168.dmp` : sur la pile du fil fautif,
`MaxPlug.dll`, `VCRUNTIME140_1.dll`, `ucrtbase.dll`, aucun module `mdseq`). Une seconde avant, le transport venait de
partir avec deux instances du build de dev `mdseq_dbg15` sur la même piste, chacune sur le Pattern `dense` (16 Tracks,
8 ratchets, 24 Locks à chaque pas) : jusqu'à 800 événements MIDI chacune au même tick, 1600 dans un même vecteur ;
le journal de dev montre leurs 400 premiers événements chacune.

Hypothèse : la sortie MIDI d'un device Max for Live a une capacité par vecteur, et la dépasser fait lever une
exception à Max. À mesurer dans une session de test que l'utilisateur accepte de voir planter : combien d'événements
par tick et par vecteur passent, une instance puis deux. Puis décider : étaler les événements d'un même tick sur
plusieurs vecteurs, plafonner ce que le device émet par réveil, ou refuser les Patterns qui dépassent.

Le Pattern le plus lourd n'est pas un cas d'usage, mais un plantage de Live l'est toujours.

## Comments

- 2026-10-17 : la cause lue dans le dump, sans relancer Live (le set ouvert était celui de l'utilisateur, non
  sauvegardé : aucun essai qui puisse planter). Lecteur de minidump écrit pour l'occasion (`dumpread.py`,
  `dumpobj.py`, `pertti.py`, `pecode.py` du scratchpad de la session ; aucun débogueur installé).
  - Sur la pile du fil fautif, un enregistrement d'exception C++ (`0xE06D7363`) lancée par
    `Ableton Live 12 Suite.exe` lui-même ; la vtable de l'objet lancé nomme sa classe par le RTTI du binaire sur
    disque : `boost::container::bad_alloc` (bases `boost::container::exception`, `std::exception`). Boost.Container la
    lance quand un conteneur de capacité fixe déborde. Non rattrapée dans `MaxPlug.dll` : `terminate`, `abort`, le
    `0xc0000409` du journal.
  - Les deux adresses de retour gardées sous l'objet mènent au site : un appel au `throw_bad_alloc` de Live (RVA
    `0xa25980`, qui charge la vtable de `bad_alloc`) à la fin d'une fonction (RVA ~`0x3229b00`) qui tient deux
    ensembles de 128 éléments de 16 octets (clé 64 bits, valeur 32 bits), choisis par un drapeau : elle ajoute une clé
    absente (`cmp r11, 0x80` puis le throw quand l'ensemble est plein), ne fait rien pour une clé présente, et retire
    une clé par échange avec la dernière, puis prévient un abonné. Elle est appelée par le gestionnaire de messages
    MIDI de Live (RVA ~`0x11a57c7`, table de sauts sur l'octet de statut 0x80-0xF0). Lecture : l'ensemble des notes
    tenues que Live garde pour la sortie MIDI d'un device Max, 128 au plus ; un note-on de trop le fait quitter.
  - Le device tenait chaque Lock jusqu'au ratchet suivant : le Pattern `dense` joué en direct tient jusqu'à 399
    notes à la fois (`heldNotes` de `mdseqSequencerTest`), deux instances le double (la seconde fait passer la
    sortie de la première).
  - Corrigé : joué en direct, un Lock finit aussitôt (note-off juste après son note-on, son note-off prévu
    abandonné) ; MD Drums lit un Lock à son note-on et ignore son note-off. Imprimé (`prepareRender`), il garde sa
    longueur, dont la note d'un clip a besoin. Le même Pattern tient désormais 17 notes au plus (16 Hits et le Lock
    entre ses deux messages). Garde-fou 12/12.
  - Vérifié dans Live 12.2.1 le même soir (build de dev `mdseq_dbg28`, Pattern de contrôle de `measure/steps.py`,
    piste MDSEQ KIT enregistrée Post FX par `live-record-track.mjs`, son fader à zéro le temps de l'enregistrement par
    `live-track-volume.mjs`, aucun autre clip dans le set) : un Lock qui finit aussitôt arrive à MD Drums comme
    avant. Temps 2 et 4, VOL verrouillé à 0 : −inf dB ; temps 1 et 3 : −13,3 dB (les chiffres du 2026-10-13) ;
    ratchets −13,3/−15,7/−19,3/−25,3 dB et −42,4/−19,3/−13,2 dB ; micro-timing à 1 échantillon de ±12/24 ; pas FILL
    muet sans le Fill.
  - 2026-10-17, 17:24 et 17:32, **le scénario du plantage rejoué** dans Live 12.2.1, sur un set que l'utilisateur a
    accepté de perdre (`measure/live/live-crash-test.mjs` : piste « MDSEQ CRASH », fader à zéro, MD Drums, deux
    devices de dev sur le Pattern `dense`, transport lancé 20 s).
    - Build corrigé (`mdseq_dbg28`) : 20 s, 42 temps, environ 598 000 événements MIDI envoyés par chaque device,
      Live vivant.
    - Témoin, build d'avant (`mdseq_dbg27`, Locks tenus) : Live quitte 2 s après le départ (Event 1000 à 17:32:07,
      `ucrtbase.dll`, `0xc0000409`, offset `0xa527e`, comme le 2026-10-13). Le nouveau dump donne le même objet
      lancé (vtable de `boost::container::bad_alloc`, RVA `0x6526f78`) et les deux mêmes adresses de retour
      (`0x3229c34`, l'ensemble de 128 ; `0x11a57c7`, le gestionnaire MIDI).
  - Ouvert, non mesuré : les clips imprimés d'un Pattern dense qui jouent ensemble vers MD Drums (16 pistes, 25
    notes tenues chacune). On ne sait pas si l'ensemble de 128 est celui de la sortie d'un device Max ou d'une
    entrée de piste ; le chemin du plantage passe par `MaxPlug.dll`, ce qui penche pour le premier.
