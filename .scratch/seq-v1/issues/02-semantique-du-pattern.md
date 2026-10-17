# Sémantique du Pattern

Type: grilling
Status: resolved
Assignee: c0remusic (session du 2026-10-09)
Blocked by: —

## Question

Que contient exactement un pas, et comment se joue-t-il ? Règles des Conditions : sur quoi compte `A:B` (Passes depuis
le début, depuis la dernière bascule de Pattern ?), portée de `PRE` (Condition précédente de la même Track ?), de `NEI`
(Track précédente, même pas ?), ce que redémarre `1ST`, comment `X%` tire. Ratchets : nombre, vitesse, courbe de
vélocité, et les Locks qu'ils portent. Micro-timing : plage et résolution. Vélocité par défaut. Locks par pas : combien,
et comment s'appliquent-ils à un ratchet. Ce qu'un pas sans Hit peut porter (rien, puisque MD Drums ignore un Lock sans
Hit ?).

## Answer

Grilling du 2026-10-09 avec l'utilisateur. La sémantique d'Elektron partout où elle existe, pour que les réflexes
Machinedrum et Digitakt marchent tels quels.

### Un pas

- **Hit** : oui ou non ; seul le Hit fait sonner. Vélocité 1-127, 100 par défaut.
- **Condition** : une au plus par pas.
- **Micro-timing** : de −23 à +23 en 1/24 de pas (384 par ronde, comme Elektron) ; avec des ratchets, tout le groupe
  se décale.
- **Ratchets** : 1 à 8 Hits répartis également dans le pas (façon pulses Cirklon, sans vitesse ni longueur séparées),
  avec une courbe de vélocité de −100 % à +100 % (descente ou montée sur les N). La Condition du pas décide pour tout le
  groupe.
- **Locks** : 0 à 24 par pas, aucun sans Hit (MD Drums les ignore). Valeurs 0-127 sauf 63, que l'échelle de l'ADR 0003
  ne peut pas écrire : un bouton de Lock saute de 62 à 64, si bien que ce qui joue en direct est toujours ce qui
  s'imprime. Un Lock égal à la valeur du Kit reste un Lock (il fige la valeur si le Kit change) ; seul l'Alt-clic retire
  un Lock.

### Conditions

- **`A:B`** : joue au Pass A d'un cycle de B Passes ; B de 2 à 8, A de 1 à B. Les Passes se comptent depuis le
  démarrage du Pattern : départ du transport ou bascule de Pattern remettent à 1.
- **`1ST` / `!1ST`** : le premier Pass depuis ce démarrage, ou les autres.
- **`PRE` / `!PRE`** : le résultat de la dernière Condition évaluée sur la même Track, à n'importe quel pas ; les pas en
  `PRE`, `!PRE`, `NEI`, `!NEI` et les pas sans Condition ne le mettent pas à jour.
- **`NEI` / `!NEI`** : le résultat de la dernière Condition évaluée sur la Track précédente (N−1). À chaque pas, les
  Tracks s'évaluent de 1 à 16, donc la Track N voit le résultat de N−1 au même pas. La Track 1 n'a pas de voisine :
  `NEI` n'y joue jamais, `!NEI` toujours ; pas de bouclage vers la 16.
- **`X%`** : la liste d'Elektron (1, 2, 4, 6, 9, 13, 19, 25, 33, 41, 50, 59, 67, 75, 81, 87, 91, 94, 96, 98, 99) ; un
  tirage indépendant à chaque passage du pas, tiré d'une graine par Pass, pour qu'une impression puisse rejouer un
  tirage.
- **`FILL` / `!FILL`** : le Fill est momentané en v1 (actif tant qu'il est tenu, lu à chaque pas) ; le verrou jusqu'à
  la fin du Pass viendra avec Push 2.
- **Track muette** : elle évalue ses Conditions (compteurs, `PRE`, `NEI` restent justes) et n'émet rien.

### Temps

Le pas joué suit la position de Live : temps de la chanson modulo la longueur du Pattern (× l'échelle), ancré au temps 0
de la chanson, puis au moment de chaque bascule de Pattern. Le device reste ainsi sur la grille de l'Arrangement, et une
capture tombe juste. Le compteur de Passes, lui, repart à 1 au départ du transport et à chaque bascule.

### Émission et impression

- Chaque Hit, ratchet compris, part avec ses notes de Lock au même instant, sur le canal de sa Track : MD Drums revient
  au Kit à tout Hit qui n'en a pas (ADR 0003).
- Dans un clip imprimé, un Hit et ses notes de Lock durent un pas (ou une tranche de ratchet) : ils se lisent comme un
  bloc dans le piano roll, et le clip reste éditable à la main.

### Le Kit vu du device

La Machine de chaque Track et les valeurs du Kit (valeur fantôme sur les boutons de Lock, noms de SYN1-8) sont lues dans
MD Drums par LOM, en lecture seule, si Live expose ces paramètres ; sinon « SYN1-8 » génériques et pas de fantôme. À
vérifier : « Paramètres de MD Drums visibles par LOM ».
