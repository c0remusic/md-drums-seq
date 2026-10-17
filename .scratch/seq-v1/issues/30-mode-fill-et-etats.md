# Mode Fill et états des Patterns

Type: task
Status: resolved
Assignee: c0remusic (session du 2026-10-17)
Blocked by: —

## Question

L'utilisateur, le 2026-10-17 : « et comment on edit le fill ? », puis « oui fais le mode fill puis les états ». Un
Fill se fait comme sur les machines Elektron, par la Condition des pas (FILL joue pendant le Fill tenu, !FILL se tait
alors), choisie pas à pas dans le menu Condition. Le mode Fill en fait un geste. Les états des boutons de Pattern
sont ceux de la maquette v5, le dernier écart visible avec elle.

## Comments

- 2026-10-17 : construit, garde-fou 11/11.
  - Mode Fill (`Editor::setFillEdit`, message et clé `filledit`) : un bouton « Fill edit » au-dessus des rangées de
    la fenêtre. Allumé, un Hit posé à la souris dans la grille ou la bande, au clic ou au glisser, prend la Condition
    FILL ; un Hit retiré part comme toujours. La grille et la bande redessinent les Hits ni FILL ni !FILL à 35 % de
    leur couleur (`CellLook::faded`).
  - États (`PatternState`, clé `patternstate p état`, envoyée avec le Pattern choisi, pour un Pattern que le set
    rend ou qu'une copie écrit) : vide, utilisé (un Hit au moins), joué, en file. `Editor::blink`, appelé par
    l'external à chaque lecture du Kit (500 ms) comme `settle`, fait clignoter le contour du Pattern en file. Dans le
    patch, un message par état pose la valeur et les couleurs du bouton, dans la bande et dans la fenêtre.
  - Vu dans Live (dbg24, démonstration avec des Hits dans les Patterns 1, 3 et 5) : 01 allumé, 03 et 05 clairs, les
    autres pâles, dans la bande et la fenêtre ; le bouton Fill edit en place.

### Checklist dans Live

1. Fill edit allumé : un clic dans la grille pose un Hit marqué FILL, les autres Hits pâlissent ; éteint, tout
   revient.
2. En lecture, choisir un autre Pattern : son bouton clignote, contour bleu, jusqu'à la fin du Pass, puis s'allume.

## Answer

Vérifié dans Live par l'utilisateur le 2026-10-17 : le mode Fill (un Hit posé prend FILL, les autres pâlissent) et le Pattern en file qui clignote jusqu'à la fin du Pass.
