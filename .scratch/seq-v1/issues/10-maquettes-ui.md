# Maquettes de l'UI

Type: prototype
Status: resolved
Assignee: c0remusic (session du 2026-10-09)
Blocked by: 02, 09

## Question

Maquettes versionnées de l'UI du device, validées par l'utilisateur (skill `plugin-gui-design`, comme les maquettes v1
à v22 de MD Drums) : la grille de pas, l'édition d'un Lock (pas tenu + bouton), l'édition d'une Condition, le Fill, les
mutes, la bascule de Pattern, l'impression. Aboutit à une maquette approuvée et à sa passation pour la construction.

Point de départ (« Direction visuelle du device ») : la v1 de `Design/`, famille A (look natif de Live), grille de 32
pas visibles avec badges, 64 pas en deux pages.

Apporté depuis : « Sémantique du Pattern » remplace la vitesse des ratchets par une courbe de vélocité et fait sauter 63
aux boutons de Lock ; « Paramètres de MD Drums visibles par LOM » montre que le device connaît les Machines (noms de
SYN1-8) mais pas les valeurs du Kit : la v1 dessine une valeur fantôme du Kit (« 52 (kit 40) ») qu'il faudra retirer ou
remplacer. Et pas de JS (ADR 0001) : la grille ne peut pas être dessinée par `jsui`/`v8ui`, les maquettes ne montrent
que ce que des objets natifs savent afficher.

## Answer

v4 approuvée par l'utilisateur le 2026-10-10 ; passation dans `Design/HANDOFF.md` (placement des deux surfaces, objet
Max de chaque contrôle, dessin d'une case au `lcd`, couleurs, comportements). Le chemin :

- **v2** : famille A à 32 pas, les pas de la bande et la grille en `lcd` (couleurs opaques, vélocité en teinte), Locks
  sans valeur du Kit (« Kit » sur un bouton inactif), courbe de vélocité des ratchets.
- **v3** : l'utilisateur prenait les traits des ratchets pour des Conditions ; les ratchets deviennent des crans dans le
  bord haut du Hit, la Condition une bande violette sous la case avec son texte ; une légende « Lire une case ».
- **v4** : l'utilisateur prenait le décalage du micro-timing pour une case mal centrée ; le Hit reste centré, le
  micro-timing devient une flèche dans la marge (gauche en avance, droite en retard), dessinée par-dessus les cadres.
