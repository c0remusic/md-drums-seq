# Chaînes de Patterns

Type: task
Status: resolved
Assignee: c0remusic (session du 2026-10-17)
Blocked by: —

## Question

L'utilisateur, le 2026-10-17 : « et un moyen de chain les patterns aussi ». Les chaînes avaient été écartées au
cadrage. Il a choisi le même jour une liste simple, chaque Pattern une Pass, plutôt qu'une liste avec répétitions
ou l'automation de Live seule, et un mode Chain où les clics sur les boutons Pattern construisent la chaîne.

## Comments

- 2026-10-17 : construit, garde-fou 11/11.
  - L'Editor tient la chaîne (16 Patterns au plus) et la joue : à chaque Pass qui commence (le pas joué revient en
    arrière), il met en file le Pattern suivant comme un clic le ferait, rien quand c'est le même Pattern encore ; la
    chaîne reboucle. Le Sequencer ne change pas : sa file et ses retours en arrière sur un saut restent ceux d'un
    Pattern choisi, et la mise en file a une Pass entière pour arriver.
  - Mode Chain (`setChainMode`, message `chain`, clé `chainmode`) : allumé, une nouvelle chaîne, la précédente
    s'arrête, et chaque bouton Pattern cliqué (bande ou fenêtre) l'allonge (`chainAdd`, message `chainadd`) au lieu
    de choisir ; éteint, deux Patterns ou plus jouent, un seul est choisi, aucun ne fait rien. À l'arrêt, son premier
    Pattern s'affiche aussitôt et le suivant part en file au départ de Live ; en lecture, elle commence à la fin de
    la Pass (aussitôt si son premier Pattern est celui qui joue). Un Pattern choisi à la main (bouton, paramètre
    « Pattern », automation) arrête la chaîne. Live qui repart reprend à l'étape du Pattern affiché.
  - Ligne de la chaîne (`chaintext`) : « Chain: 01 03 » pendant la construction, « Chain 2/4: 01 03 03 05 » pendant
    qu'elle joue, vide sinon.
  - Patch : le bouton Chain et la ligne dans la bande, sous FILL ; les boutons Pattern envoient leur clic à
    `---patternclick`, qu'une porte réglée par `chainmode` mène au paramètre « Pattern » ou à `chainadd`.
  - Gardée dans le set (ajout du 2026-10-17) : un Blob « Pattern Chain » reçoit `chainstore` à chaque début ou fin
    de chaîne, et la rend par `restorechain` à la réouverture ou par Ctrl+Z.

### Checklist dans Live

1. Chain allumé, cliquer 01 03 03 05 : la ligne lit « Chain: 01 03 03 05 », le Pattern affiché ne change pas.
2. Chain éteint, lecture : 01 une Pass, 03 deux, 05 une, puis 01 ; la ligne suit (« Chain 2/4 … »).
3. Cliquer 07 à la main : la chaîne s'arrête, la ligne se vide, 07 part à la fin de la Pass.

## Answer

Vérifié dans Live par l'utilisateur le 2026-10-17 : une chaîne construite au mode Chain joue chaque Pattern une Pass et reboucle, un Pattern choisi à la main l'arrête ; Clear (ticket 29) aussi. Reste ouvert : garder la chaîne dans le set.
