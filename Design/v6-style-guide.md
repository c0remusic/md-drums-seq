# md-drums-seq v6 : guide de style

`v5-style-guide.md`, avec des couleurs par rôle pour les `live.text` allumés :

| Rôle | Valeur | Où |
|---|---|---|
| contenu choisi | #ffa52e, texte #1e1e1e (l'orange de Live) | Pattern, Track et page choisis, Hit, moitié montrée |
| mode | #7cc6ff, texte #1e1e1e (le bleu de la sélection et du pas joué) | Follow, Chain, Clips only, Push, Fill edit : ce qui change ce que font les mains |
| réglage du device | #5e5e5e, texte #dadada | Check for updates |
| hors longueur | fond #232323, texte #555 ; puits du lcd #161616 | pages et pas au-delà de la longueur du Pattern |
| fin du Pattern | #a2a2a2, 2 px | trait dans la grille après le dernier pas |

Les couleurs d'un `live.text` se fixent par ses attributs (`activebgoncolor`, `activetextoncolor`), comme celles des
Mutes en v5 ; les autres `live.*` gardent le thème de Live.

Groupes : une étiquette `live.comment` au-dessus de chaque groupe de la bande (Play, Output), un filet `panel` entre
les groupes de la barre du haut, sans cadre ni fond de groupe.
