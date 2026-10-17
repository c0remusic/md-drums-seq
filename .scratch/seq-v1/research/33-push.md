# Push depuis le device : ce que Tuple a appris

Recherche du 2026-10-17 pour la ligne « Push 2 » de la carte et la piste que l'utilisateur a ouverte au ticket 25 :
un mode Push « à la Tuple », par `grab_control`, sans paramètre Live (donc sans double automation). Lu dans
`C:\dev\tuple` (le device de l'utilisateur qui joue déjà Push 2), sans essai sur le matériel ici. Clés des chemins :
**S** `device/tuple_push2_spike.js`, **D** `docs/decisions.md`, **B** `docs/bugs.md`, **R** `docs/roadmap.md`,
**C** `CLAUDE.md`, **TF** `device/tests/push-focus.test.mjs`, **TN** `device/tests/push-noteoff.test.mjs`.
`push/README.md` (« Parked, grab_control doesn't take control ») est périmé : D:1033, D:1041-1051, B:180-183.

## Ce que fait Tuple

- **Un `[js]`** (ES5, fil basse priorité de Max), par `LiveAPI` ; ni external, ni `v8`, ni `live.object`.
- **Prise de la grille** (S:339-383, D:1054-1065) : parcourir `control_surfaces 0..19`, garder celles dont
  `get_control_names` liste `Button_Matrix`, prendre la **deuxième** (deux surfaces l'exposent ; prendre la première
  ne fait rien, la seconde éteint le Push, D:1057-1058) ; `get_control Button_Matrix` donne l'id ; `release_control`
  par précaution puis `grab_control`, et observer `value`. Rendue par `release_control`. Un `new LiveAPI("chemin")` à
  un argument est pris pour un callback et échoue sans bruit : c'était le « grab qui ne marche pas » (D:1048-1051).
- **Seule la grille de pads** est prise ; encodeurs, transport et écran restent à Live, par règle (D:1088-1090).
- **Allumer un pad** : `send_value col row couleur`, la couleur en dernier (D:1064-1065) ; 64 appels par redessin.
  Palette réécrite par SysEx à travers la surface (`send_midi 240 0 33 29 1 1 3 …`, S:448-453), Push 2 seulement,
  à renvoyer après que la prise est active (S:384-386).
- **Appui** : l'observateur reçoit `[valeur, vélocité, col, ligne, 1]` ; vélocité 0 = relâché ; ligne 0 en haut.
- **Écran** : jamais écrit, « impossible depuis M4L (protocole USB propriétaire) » (D:1358-1359). Tuple y montre
  8 paramètres Live rangés par `parameterbanks` : justement ce que le mode voulu évite.
- **Focus** (S:285-300, TF:127-204) : prise seulement si demandée, device actif (`is_active`, pas `active`), piste du
  device sélectionnée (piste retrouvée en remontant `canonical_parent`) ; quitter la piste rend la grille, revenir la
  reprend. L'état « pris » ne se lit pas par LOM (C:301). Pas d'observateur sur l'arrivée d'un Push.

## Écueils mesurés par Tuple

- En vue Session, les pads lancent encore des clips malgré la prise : bug ouvert (B:29-56).
- Pads noirs juste après la prise (la prise n'est pas active dans le même tour) : relance 6 fois toutes les 150 ms
  (S:371-396, D:1552-1559).
- Notes tenues au relâchement : note-off des pads tenus avant de rendre la grille (S:398-412, TN:82-124).
- Rendre la grille à la suppression du device est obligatoire (D:1060-1061), et Tuple ne le fait pas encore.
- Un appui pris par le device n'arrive pas à l'entrée de la piste : Live ne peut pas l'enregistrer (D:1075-1082).
  Pour md-drums-seq : l'enregistrement live (ligne de la carte) serait celui du device lui-même.
- Pads pris moins sensibles à la vélocité qu'en mode User (D:3141).
- Push 3 : palette SysEx propre au Push 2, nom `Button_Matrix` et format des appuis à revérifier (R:2321-2327).

## Ce que ça veut dire pour md-drums-seq (faits, pas des choix)

- Le device parle déjà à la LOM par son `live.path` et son `live.object` (`tools/make_device.py:614-630`), sur le
  fil principal, réponses dans l'appel. `grab_control`, `send_value` et `release_control` passeraient par là ;
  observer la grille demande un `live.observer` de plus, qui renverrait les appuis à l'external. Aucun `js`.
- Le fil principal de Max s'est arrêté jusqu'à 6,4 s quand Live était en arrière-plan (ticket 32) : un mode Push
  qui y vit se figerait autant. Live au premier plan, rien de tel n'a été signalé.
- L'écran de Push ne montrerait rien du device sans paramètres Live.
- Encodeurs pour les Locks : Tuple ne les prend pas ; `grab_control` d'un encodeur n'est mesuré nulle part.

## Questions pour la séance avec l'utilisateur

1. Ce que montre la grille : 32 pas d'une Track (4 × 8), 64 pas, ou 16 pas × Tracks ?
2. Choisir la Track et le Pattern depuis Push : avec quels boutons, sans écran ?
3. Les Locks au pas tenu : prendre aussi les encodeurs (non mesuré), ou rester à la souris ?
4. L'écran vide : acceptable, ou quelques paramètres Live malgré tout (et leur automation) ?
5. Session view : les pads lancent encore des clips (bug de Tuple) ; bloquant ?
6. Push 3 dans le périmètre ?
