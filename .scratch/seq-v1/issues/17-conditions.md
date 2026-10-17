# Conditions

Type: task
Status: resolved
Assignee: c0remusic (session du 2026-10-12)
Blocked by: 13, 15

## Question

Les Conditions dans le device, comme le ticket « Sémantique du Pattern » et le round 1 de « Structure du moteur en
Max natif » les ont fixées : une par pas, tirées quand le pas est planifié, `X%` par hash de la graine du Pass, `A:B`
compté en Passes depuis le départ, `PRE` sur la même Track, `NEI` sur la Track N−1, `1ST`, `FILL` et le Fill momentané.

## Comments

- 2026-10-12 : construit.
  - Noyau (`mdseqCondition.*`) : une liste de 65, celle du menu (aucune, les 21 `X%` d'Elektron, les 35 `A:B` de
    B = 2 à 8, puis FILL, !FILL, PRE, !PRE, NEI, !NEI, 1ST, !1ST) ; un pas tient l'indice de la sienne. Le tirage d'un
    pas est hash(graine du Pass, Track, pas), sans état ; la graine du Pass vient de celle du départ du transport, que
    le Sequencer tire d'une source (l'entropie de la machine dans le device, une constante dans les tests). La mémoire
    de PRE et NEI garde, par Track, le résultat de la dernière Condition évaluée ; PRE, NEI et les pas sans Condition
    la laissent ; elle repart de rien à chaque départ du transport, avec le compteur de Passes (question ouverte du
    ticket 08, tranchée ici). Une Track muette évalue toujours ses Conditions.
  - Une Condition demande un Hit, et retirer le Hit la retire. Une édition du pas planifié et pas encore commencé le
    tire à nouveau, toutes Tracks, depuis la mémoire d'avant lui : PRE et NEI restent justes (test qui casse sans la
    restauration).
  - Fill : lu quand le pas est planifié, pas une édition du Pattern ; les boutons FILL de la bande (paramètre Live) et
    de la fenêtre l'envoient tenu et lâché.
  - Case : la bande violette de 3 px dans la marge basse et le libellé en gras de 10 px centré au-dessus, comme la
    passation ; l'external le dessine par `frgb`, `font Arial 10`, `textface bold`, `moveto`, `write`.
  - Panneau : le menu Condition, 65 entrées, sans valeur initiale (contrôle d'édition).
- À vérifier dans Live : que `font Arial 10` est accepté par `lcd` (sa page parle d'un numéro de police) et que le
  bouton FILL en mode bouton envoie 1 à l'appui et 0 au relâché.
- 2026-10-13, vérifié dans Live 12.2.1 à 126 BPM, sur une piste « MDSEQ TEST » du set de l'utilisateur (MD Drums
  construit le 2026-10-13 à 17:05, après cf3385cd et 812551fa ; build de dev `mdseq_dbg9` puis `mdseq_dbg10` sur le
  Pattern de `measure/steps.py`), sortie de la piste enregistrée par `live-record-track.mjs`, analysée par `steps.py` :
  - FILL ne marchait pas : en mode bouton, `live.text` envoie un seul `bang` au clic et rien au relâché (sa référence
    Max) ; `prepend fill` en faisait `fill bang`, que l'external lit 0. Le Fill n'a jamais tenu, même par son
    paramètre. Corrigé : FILL est une bascule (`live.text` en mode toggle, paramètre Live : un contrôleur la bascule),
    sous un `ubutton` transparent qui lui donne 1 à l'appui de la souris et 0 au relâché ; la bande et la fenêtre se
    montrent l'état l'une de l'autre (`---fillstate`).
  - Mesuré avec le paramètre Fill réglé par LOM : relâché, le pas `FILL` est muet (−inf dB) ; tenu, il sonne
    (−12,2 dB, comme ses voisins). Le message `fill 1` arrive à l'external (journal de dev).
  - Restent pour l'œil et la souris : le texte des cases (`font Arial 10`) et FILL tenu à la souris.

## Answer

Construites et vérifiées dans Live le 2026-10-13 : toutes les Conditions de la carte, tirées à chaque passage du pas ; le mode Fill pour les poser (ticket 30).
