# Charge MIDI que Max for Live supporte

Type: task
Status: open
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
