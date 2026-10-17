# Jeu en direct par midiout

Type: grilling
Status: open
Assignee: —
Blocked by: 04

## Question

Comment le device joue-t-il MD Drums en direct : où il se place (effet MIDI sur la piste de MD Drums, ou piste à lui),
sur quelle horloge il cale ses pas (`plugsync~`, `transport`, `plugphasor~`) et avec quelle avance il émet pour
tenir le micro-timing et les ratchets, comment le Fill, les mutes et la bascule de Pattern prennent effet (au pas
suivant ?), et ce qu'il fait quand le transport de Live s'arrête, repart ou saute. Les clips imprimés jouent pendant ce
temps sur les pistes Track : le device et eux peuvent-ils jouer ensemble, et qui l'emporte ?

## Comments

- 2026-10-09 : ce ticket portait sur l'écriture en avance dans les clips Session ; la recherche « Ce que LOM permet
  dans Live 12 » a montré que chaque écriture LOM entre dans l'historique d'annulation, et l'utilisateur a rappelé le
  but : jouer en direct, imprimer à la demande. Le jeu passe donc par `midiout`.
- 2026-10-10 : « Structure du moteur en Max natif » a répondu à l'horloge (ITM en ticks dans l'external), à l'avance
  (un pas), au Fill, aux Mutes et au transport (arrêt, sauts) ; la tranche verticale (ticket 13) pose le device en
  effet MIDI devant MD Drums. Restent : le placement définitif quand les Locks exigent les canaux (ticket 04), et le jeu
  du device en même temps que des clips imprimés.
