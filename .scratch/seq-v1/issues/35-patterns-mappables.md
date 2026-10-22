# Une commande mappable par Pattern

Type: task
Status: open
Assignee: —
Blocked by: —

## Question

Le choix de la Pattern est un seul paramètre Live, « Pattern » (16 valeurs) : un mapping Ctrl+M le relie à un
bouton qui fait défiler les valeurs, ou à un knob, pas à seize pads. Sur un contrôleur à pads, choisir la Pattern 7
demande donc de passer par la souris, par Push 2 (ticket 33) ou par une automation.

Stepic for Live (Devicemeister, lu le 2026-10-22 dans son `.amxd`) donne à chacune de ses 16 Patterns son propre
`live.text`, un paramètre Live chacun (« pattern1 » à « pattern16 »), en plus d'un `live.numbox` « PatternNo » : un
Ctrl+M par pad, et chaque Pattern a sa touche.

Proposition : seize paramètres « Pattern 1 » à « Pattern 16 », des boutons dont l'appui choisit la Pattern comme un
clic sur sa touche du strip (en file pendant la lecture, ajoutée à la chaîne en mode Chain), le paramètre « Pattern »
restant celui que le set garde et que l'automation joue.

## À trancher

1. Les touches existantes du strip deviennent-elles ces paramètres (aujourd'hui des `live.text` sans paramètre Live,
   reliés au paramètre « Pattern »), ou seize paramètres cachés de plus ?
2. Leur valeur : un bouton qui revient à 0 (rien d'enregistré dans l'automation qui gêne), ou un état allumé pour la
   Pattern jouée ? Un bouton allumé se lit sur un contrôleur à LED, mais une automation qui l'enregistre rejouerait
   des choix.
3. Ce qu'un tel bouton fait pendant l'enregistrement d'automation de Live : une automation du paramètre « Pattern »
   suffit-elle, ou faut-il que les seize n'enregistrent rien (`parameter_invisible` 1, comme l'interrupteur Push du
   ticket 33) ?
4. Changement immédiat (au pas suivant) en plus du changement en fin de Pass, comme le « Retrigger » de Stepic ?

## Answer

—
