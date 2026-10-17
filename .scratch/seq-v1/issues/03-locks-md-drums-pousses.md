# Locks de MD Drums poussés

Type: task
Status: resolved
Assignee: —
Blocked by: —

## Question

Pousser la branche `md-drums-locks` de gearmulator-md-drums (commit `49c6e0de` : Locks en notes, ADR 0003) une fois son
garde-fou vert. Bloqué le 2026-10-08 par le disque C: plein (`LNK1180`) : il faut environ 15 Go libres, que
l'utilisateur libère. Puis build dans le worktree `.claude/worktrees/locks`, garde-fou complet, push de la branche, et
fusion dans `main` selon les règles de ce dépôt-là.

## Answer

Fait le 2026-10-09. `main` de gearmulator-md-drums avait avancé (Hits à l'échantillon près du ticket 19, effets master
du ticket 24, Kits du ticket 26) : fusion de `main` dans les Locks sur un HEAD détaché (`4d8e4a62`, six conflits
résolus en gardant les deux côtés), garde-fou complet sur l'arbre fusionné : 32 tests sur 32 en 354 s. Sur cet arbre,
`mdDrumsDeviceTest` passe ses Locks (Hit locké égal au Kit avec ces valeurs, dans les deux ordres ; retour au Kit ;
orphelins ignorés) et mesure la note au son à 44,1 kHz : Out 01 à 1,52 ms sans gigue, Main à 1,93 ms (après les effets
master). Branche `md-drums-locks` poussée, puis `main` avancé sans force sur le même commit gaté (`f223e2b1..4d8e4a62`).
MD Drums joue donc les notes de Lock 64-87 sur le canal de leur Track.
