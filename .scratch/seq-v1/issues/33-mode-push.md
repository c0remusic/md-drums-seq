# Mode Push du device

Type: grilling
Status: open
Assignee: —
Blocked by: — (séance avec l'utilisateur)

## Question

La ligne « Push 2 » de la carte (grille de pas, Track choisie, Locks au pas tenu) et la piste que l'utilisateur a
ouverte au ticket 25 le 2026-10-14 : un mode Push « à la Tuple », la grille de pads prise par `grab_control`, sans
paramètre Live, donc sans double automation. Que montre la grille, comment choisir Track et Pattern, que deviennent
les Locks et l'écran ?

## Comments

- 2026-10-17 : recherche faite dans le device Tuple de l'utilisateur, qui joue déjà Push 2 :
  [research/33-push.md](../research/33-push.md). Ce qui marche (prise de `Button_Matrix` sur la deuxième surface,
  `send_value col row couleur`, palette par SysEx, appuis observés), ce qui manque (écran inaccessible depuis M4L,
  appuis non enregistrables par Live, pads qui lancent encore des clips en vue Session), et six questions pour la
  séance. Rien de construit ; à trancher avec l'utilisateur (grilling).
