# Noms des Tracks qui clignotent

Type: task
Status: resolved
Assignee: c0remusic (session du 2026-10-17)
Blocked by: —

## Question

Vu le 2026-10-17 en faisant les captures d'écran de 0.1.0 pour le site de MoDule : les onglets de la bande montraient
les numéros des Tracks au lieu de leurs Machines, et le journal du build de dev alternait « kit … : TRX UW » et
« kit … : none » toutes les quelques secondes. Le Kit disparaît, et avec lui les noms des Tracks, les SYN1-8 nommés et
les valeurs du Kit sur les boutons de Lock, jusqu'à la lecture suivante.

## Comments

- 2026-10-17 : deux causes, mesurées dans Live 12.2.1 avec MD Drums sur la piste du device, transport arrêté.
  - **Un fichier occupé lu comme absent.** MD Drums remplace son fichier par un renommage atomique
    (`writeFileAtomic`, `MoveFileExW` avec `MOVEFILE_REPLACE_EXISTING`). Windows refuse d'ouvrir un fichier pendant
    qu'un renommage le remplace : un lecteur contre un fichier remplacé 567 fois en 10 s a vu 923 ouvertures refusées
    (`PermissionError`), aucune lecture tronquée (script `replace_race.py` du scratchpad de la session). Le device
    prenait ce refus pour « pas de fichier ». Désormais `readKitFile` distingue un fichier absent
    (`std::filesystem::status` dit `not_found`) d'un fichier occupé, et `KitLink::poll` garde le Kit sur
    `KitFile::busy()`.
  - **Des hellos en retard.** Le fichier disparaissait vraiment, 5 à 12 s, environ une fois par minute (trois fois en
    4 minutes, `watch_kit.py` toutes les 100 ms) : MD Drums efface le fichier d'un device qu'il n'a pas entendu depuis
    15 s (`KitFiles::ForgetMs`). Le hello partait du sondage du fichier, sur le fil principal de Max, et ce fil s'arrête
    plusieurs secondes dans Live : sondages en retard de 3,7 s et 6,4 s pendant que l'horloge du scheduler tournait
    (12 tops en 6,4 s), 13 s entre deux hellos. Le rythme du hello passe dans `HelloPace`, appelé par l'horloge sur le
    fil du scheduler, qui lit l'état du dernier sondage dans un atomique ; le sondage du fichier reste sur le fil
    principal.
  - Vérifié dans Live avec `mdseq_dbg27` : 7 minutes, fichier jamais effacé, aucun « none », malgré deux arrêts du fil
    principal (6,4 s et 4,1 s). Avant : trois effacements en 4 minutes (`mdseq_dbg18`).
  - Ouvert : pourquoi le fil principal de Max s'arrête plusieurs secondes dans ce set (fenêtre d'édition ouverte,
    fenêtre de MD Drums ouverte, Live lui-même ?). Le reste de ce que fait le fil principal (Kit, panneau, blink) attend
    autant ; rien n'y est perdu, seulement retardé. Pendant toutes ces mesures, Live n'était pas au premier plan (une
    autre application l'était) : piste à vérifier, Windows 11 ralentit les minuteurs d'une application en arrière-plan.
    L'utilisateur, qui joue Live au premier plan, n'a signalé aucune interface figée.
