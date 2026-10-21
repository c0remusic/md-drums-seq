# md-drums-seq : contrôles du device

Ce que le device montre et ce qu'on y touche. Les identifiants sont ceux des éléments de `vN-test.html`. « Paramètre
Live » : l'objet est un `live.*` avec `parameter_enable`, donc automatisable et assignable à un contrôleur (Push 2,
MIDI) ; c'est le cas de tout ce qui sert à jouer.

## Bande (dans la chaîne de devices, pour jouer)

| Id | Contrôle | Type | Valeurs | Paramètre Live |
|---|---|---|---|---|
| `pattern-1` … `pattern-16` | Pattern joué / mis en file | boutons radio | 1-16 ; un clic met en file pour la fin du Pass | oui (un seul paramètre « Pattern », 16 valeurs) |
| `track-1` … `track-16` | Track montrée | boutons radio | 1-16 | non |
| `mute-1` … `mute-16` | Mute de la Track | bascules | joue / muette | oui (16 paramètres) |
| `step-1` … `step-16` | Pas de la page montrée, pour la Track montrée | bascules | Hit ou rien ; dessinés par un `lcd` (comme la grille), position de lecture encadrée | non |
| `page-1` … `page-4` | Page de 16 pas montrée | boutons radio | 1-16, 17-32, 33-48, 49-64 ; celles au-delà de la longueur inactives | non |
| `fill` | Fill | momentané | tenu = Fill | oui |
| `chain` | Mode Chain | bascule | allumé : les Patterns cliqués font une chaîne | non |
| `output` | Clips only : le device ne joue plus MoDule, ses clips le jouent | bascule | éteint / allumé | oui (« Output ») |
| `capture` | La dernière prise en clips de Session | bouton | actif quand la prise a une note | non |
| `edit` | Ouvre la fenêtre d'édition | bouton | | non |
| `push` | Push 2 joue le device tant que sa piste est sélectionnée | bascule | éteint au chargement | non |

## Fenêtre d'édition

| Id | Contrôle | Type | Valeurs | Paramètre Live |
|---|---|---|---|---|
| `win-pattern-1` … `win-pattern-16` | Pattern édité | boutons radio | 1-16 | non (lié à `pattern-*`) |
| `length` | Longueur du Pattern | menu | 1-64 pas | non |
| `scale` | Valeur d'un pas | menu | 1/8×, 1/4×, 1/2×, 3/4×, 1×, 3/2×, 2× | non |
| `win-page-1`, `win-page-2` | Moitié du Pattern montrée | boutons radio | 1-32, 33-64 | non |
| `follow` | La moitié montrée suit la lecture | bascule | | non |
| `win-fill` | Fill | momentané | | même paramètre que `fill` |
| `print-source` | Ce que Print écrit | menu | le Pattern affiché sur 1-16 Passes, ou la dernière prise | non |
| `print` | Imprimer dans un clip de Session par Track | bouton | | non |
| `win-mute-1` … `win-mute-16` | Mute de la Track | bascules | | mêmes paramètres que `mute-*` |
| `cell-<t>-<s>` | Pas `s` (1-64) de la Track `t` (1-16) | case d'un `lcd` | vide ou Hit ; clic = Hit, sélection pour le panneau | non |
| `step-hit` | Hit du pas sélectionné | bascule | | non |
| `step-velocity` | Vélocité | bouton rotatif | 1-127, 100 par défaut | non |
| `step-condition` | Condition | menu | aucune, `X%` (1, 2, 4, 6, 9, 13, 19, 25, 33, 41, 50, 59, 67, 75, 81, 87, 91, 94, 96, 98, 99), `A:B` (2 ≤ B ≤ 8, 1 ≤ A ≤ B), `FILL`, `!FILL`, `PRE`, `!PRE`, `NEI`, `!NEI`, `1ST`, `!1ST` | non |
| `step-microtiming` | Micro-timing | glissière | -23 à +23, en 1/24 de pas | non |
| `step-ratchets` | Ratchets | menu | 1-8 Hits répartis dans le pas | non |
| `step-ratchet-curve` | Courbe de vélocité des ratchets | glissière | -100 % à +100 % | non |
| `lock-1` … `lock-24` | Lock sur le paramètre 1-24 de la Track (SYN1-8 nommés selon sa Machine, puis AMD … SRR, DIST … LFOM) | boutons rotatifs | 0-127, 63 sauté (ADR 0003 de MD Drums)  ; sans Lock, bouton inactif marqué « Kit » (le device ne connaît pas la valeur du Kit) ; tourner crée le Lock, Alt-clic le retire | non |
| `updates` | Chercher une nouvelle version une fois par jour | bascule | réglage du device, pour tous les sets | non |

Sémantique des pas et des Conditions : ticket « Sémantique du Pattern » (`.scratch/seq-v1/issues/02-semantique-du-pattern.md`).
Pas de JS (ADR 0001) : les pas de la bande et la grille sont des `lcd`, tout le reste des objets `live.*`.
