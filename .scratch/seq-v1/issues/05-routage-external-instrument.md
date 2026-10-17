# Routage External Instrument vers MD Drums

Type: task
Status: open
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
