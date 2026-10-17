# Routage External Instrument vers MD Drums

Type: task
Status: resolved
Assignee: —
Blocked by: 03

## Question

Vérifier dans Live, avec le MD Drums qui joue les Locks : 16 pistes MIDI, chacune avec un External Instrument vers la
piste de MD Drums sur le canal N et retour de Out N. Le canal N arrive-t-il bien à MD Drums comme Track N ? Un Lock
dans un clip de la piste N s'entend-il sur le Hit de la Track N et seulement sur lui ? Le son revient-il sur la bonne
piste ? Noter tout réglage nécessaire (Out de chaque Track activé, MPE désactivé, etc.).

## Comments

- 2026-10-11 : deux faits changent ce ticket.
  - Live ramène sur le canal 1 les notes d'un device Max for Live (ticket 04). Pour les clips imprimés, le canal
    viendrait de l'External Instrument (canal N), non mesuré ; MD Drums lie aussi un Lock du canal 1 au Hit reçu juste
    avant lui (ADR 0003 amendée, `cf3385cd` de gearmulator-md-drums).
  - MD Drums n'a plus que Main et 15 Outs (`812551fa`) : Live n'ouvre que 16 sorties par plug-in, piste comprise.
    Le paramètre Out de chaque Track choisit Main (0) ou Out 01-15, et un Out ne porte que les Tracks qui y sont
    envoyées (avant, Out n portait toujours la Track n). Le retour « Out N » par Track devient un choix à faire dans
    MD Drums, et une Track au moins partage une sortie.
- 2026-10-17 : fermé, dépassé. Aucun External Instrument dans ce que la carte a construit. Le jeu en direct passe par
  le device posé devant MD Drums sur sa propre piste (ticket 13), les Locks liés par la règle du canal 1 (ticket 16,
  mesuré). L'impression (tickets 07 et 22) écrit chaque Track sur une piste MIDI dont la sortie va droit à la piste de
  MD Drums sur « N-MD Drums » (le canal gardé), vérifiée dans Live le 2026-10-16 ; depuis un clip imprimé, MD Drums
  sonne au même échantillon qu'en direct (ticket 04). Le son de chaque Track revient par le paramètre Out de
  MD Drums (Main ou Out 01-15), un réglage de MD Drums que le device ne touche pas.
