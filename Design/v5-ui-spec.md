# md-drums-seq v5 : spec d'UI

v4 (`v4-ui-spec.md`) telle que Max la construit, mesurée dans Live le 2026-10-16 (critique de design et sondes
`live.tab` du ticket 15), avec ce que MD Drums dit au device depuis l'ADR 0003 : le nom de chaque Machine, ceux de
SYN1-8 et les valeurs du Kit. Géométrie : celle de `HANDOFF.md` (fenêtre de 1440 × 868, panneau du pas à partir de
y 592), que la rangée Swing a décalée de 28 px depuis la v4.

## Bande

- **Tracks** : un `live.tab` de 16 items sur une rangée (`multiline` 0 : à 1, le défaut, il pose deux fois
  `num_lines` rangées, d'où les deux rangées de 8 du build), à 172, 22, 636 × 24 : des cases de 36 × 24 au pas de
  40, chacune au-dessus de son Mute. Un item tient sur une ligne : le numéro et le nom court de la Machine (« 01 BD » ;
  sans le préfixe de famille, ROM-14 en « R14 »), le numéro seul tant que MD Drums ne dit rien. Le nom complet
  (« 01 TRX-BD ») est coupé dans 36 px.
- **Mutes** : l'activateur de piste de Live, jaune tant que la Track joue, gris une fois muette (la v4 le voulait ; le
  build l'allume à l'inverse).
- **Patterns** : 4 × 4 par `multiline` 1 et `num_lines` 2 (la règle des sondes, à voir dans Live).

## Pas de texte d'aide

L'utilisateur, le 2026-10-16 : « pas besoin du texte ». Ni ligne d'aide sous les pas de la bande (l'onglet de Track et
celui de la page disent déjà ce qu'ils montrent), ni « Arrangement when Live records », ni aide dans le panneau du pas,
ni ligne au bas de la fenêtre. Restent les textes qui disent une valeur : l'état du Pattern et sa longueur, les
numéros de pas, le pas sélectionné, le micro-timing et la courbe des ratchets ; et le retour d'une impression, qui
n'apparaît qu'après elle.

## Fenêtre

- **Pattern** et **Half** : une rangée chacun (`multiline` 0).
- **Rangée Swing** sous la grille, et son menu : comme construits, la marque d'un pas de la rangée plus sourde que
  l'accent de la sélection et du pas joué.
- **Titre du panneau** : « Track 01 TRX-BD », sans virgule.
- **Micro-timing** et **Ratchet velocity** : sans le nombre du slider ; le texte dessous dit la valeur (« +6 of 23,
  late », « −50 %, falling over the 3 »).
- **Locks** : à côté du bouton, le nom puis la valeur. Sans Lock, le bouton est inactif, son aiguille pâle sur la
  valeur du Kit, la valeur se lit « Kit 40 » ; le tourner crée le Lock à partir de là ; le double-clic le rend au Kit.
  Avec un Lock, arc, aiguille et valeur dans l'accent. Un SYN que la Machine n'utilise pas : « unused », sans valeur.
- **Étiquettes** dans la casse de Live (« Pattern », « Length »), comme `live.comment` les écrit.

## Pour construire

- Les noms courts et le titre sans virgule changent ce que l'Editor envoie (`tracknames`, `tracktitle`).
- La valeur du Kit sur les Locks : une clé de panneau de plus (la valeur à afficher, par paramètre), le bouton inactif
  posé sur la valeur du Kit par `set`. Le Kit reste la position du milieu du bouton (63), que le double-clic redonne :
  le device lit alors « pas de Lock » et repose le bouton sur la valeur du Kit.
- `multiline` 0 sur Track, Pattern de la fenêtre et Half ; `num_lines` 2 sur les Patterns de la bande.
- Les commentaires d'aide de `make_device.py` partent ; celui du retour d'impression (`---printstatus`) commence
  vide.
