# Persistance des Patterns par Blob

Type: task
Status: resolved
Assignee: c0remusic (session du 2026-10-13)
Blocked by: 13

## Question

Le ticket « Structure du moteur en Max natif » sauve chaque Pattern dans un `pattr` Blob *Stored Only*, écrit à la fin
de chaque geste d'édition (`research/08-max-natif.md`, section 1). Mesurer dans Live, avant de construire la banque de
16 Patterns :

- un Blob dense (environ 55 000 entiers, le pire cas d'un Pattern) se sauve et se relit intact ;
- le temps de sauvegarde et de chargement du set avec 16 Blobs denses ;
- combien d'entrées d'annulation crée une écriture, en 12.2.1 puis en 12.4 ;
- si Ctrl+Z dans Live rend l'ancien Blob et que le device recharge ce Pattern ;
- quand la valeur sauvée arrive, par rapport au bang de `live.thisdevice`.

## Comments

- 2026-10-13 : construit pour un Pattern (la banque de 16 viendra avec son ticket).
  - Noyau (`mdseqStore.*`) : le Pattern en entiers tous sous 2^31 (Live peut garder un nombre en double sans perte) :
    un mot magique qui porte la version, longueur et échelle, puis chaque pas avec un Hit en deux mots, six de plus
    s'il a des Locks. Le pire Pattern tient en 8194 mots, pas 55 000. La lecture refuse tout ce que le format n'a pas
    écrit (autre version, champ hors bornes, bits inutilisés, mots en trop ou manquants) sans toucher au Pattern ;
    test : 200 Patterns au hasard aller-retour, le pire, dix copies cassées.
  - Device : un `pattr pattern1 @thru 2`, paramètre Blob *Stored Only* de valeur initiale 0 (refusée comme « pas de
    Pattern »). `parameter_enable` doit être dans `saved_object_attributes` d'un `newobj`, comme les `pattr` Blob
    d'Ableton (Dynamical, Waveshaper) : au niveau de la boîte, Max l'ignore, et Live n'a rien gardé (vu en dupliquant
    la piste). L'external écrit le Pattern 500 ms après la dernière édition, pour qu'un glissé ou un bouton tourné
    fasse une seule écriture ; il ignore l'écho de sa propre écriture (le `pattr` la renvoie malgré `thru 2`).
  - Relecture : au chargement du device ou par l'annulation de Live, l'Editor redessine tout, et le Sequencer reçoit
    le Pattern entier par un pointeur alloué et libéré sur le fil principal (rien d'alloué sur le fil audio, test).
- 2026-10-13, mesuré dans Live 12.2.1 (build de dev `mdseq_dbg15`, Pattern de démonstration `dense`, le pire) :
  - Le Blob de 8194 mots revient intact dans la copie quand la piste est dupliquée (même empreinte FNV,
    `f2551fd1877fae12`), avant le bang de `live.thisdevice` (795 ms contre 818 ms après la création) ; un device neuf
    reçoit la valeur initiale 0, refusée, 128 ms après sa création.
  - Une écriture du pire Pattern prend 5,4 à 5,7 ms sur le fil principal (écho compris).
  - Pas mesuré : la sauvegarde et la réouverture d'un set avec 16 Blobs denses (ni sauvegarde ni ouverture de set par
    LOM, et le set ouvert est celui de l'utilisateur), le nombre d'entrées d'annulation par écriture et Ctrl+Z (une
    annulation dans le set de l'utilisateur pourrait défaire son travail). À faire sur un set de test qu'il ouvre.
  - Live a planté pendant ces essais ; d'abord imputé à une copie dans la User Library, le journal de Windows le place
    35 s avant elle, dans Max for Live, au départ du transport avec deux instances qui jouaient le Pattern `dense`
    (ticket 24). Copier un build pendant que Live tourne reste possible.
- 2026-10-13, banque de 16 et annulation, mesuré dans Live 12.2.1 à 126 BPM, sur une piste « MDSEQ TEST » d'une session
  neuve (Live relancé après installation, Live fermé), build de dev `mdseq_dbg17`, Patterns 1 et 2 de la démonstration :
  - La valeur initiale du Blob (`0`) se lit maintenant comme un Pattern vide (`NoPatternStored`), plus comme un
    refus : une annulation jusqu'à elle vide le Pattern dans le device, qui sinon garderait un Pattern que Live n'a
    plus. La démonstration des builds de dev s'applique au bang de `live.thisdevice`, après les valeurs du set, et
    passe par l'Editor comme l'interface (elle n'appelle plus le Sequencer depuis le fil principal).
  - Dupliquer la piste : la copie relit les 16 Blobs, Patterns 1 et 2 avec leurs empreintes, les 14 autres vides.
  - Annulation (`undo` par LOM, dans cette session de test seulement) : les deux Blobs écrits ensemble 500 ms après
    les éditions reviennent à 0 d'un seul coup, une seule entrée d'annulation, et le device recharge les deux Patterns
    vides ; Live renvoie les 16 Blobs, ceux qui n'ont pas changé sont ignorés. `redo` les rend avec leurs empreintes.
  - Reste non mesuré : sauvegarder puis rouvrir un set (ni l'un ni l'autre par LOM) ; la duplication et
    l'annulation passent par le même état du device.
- 2026-10-14, après la seconde revue : un Blob qui contient des mots que ce build ne sait pas lire (un format plus
  récent, ou l'annulation de Live jusqu'à eux) vide son Pattern dans le device, comme la réouverture du set le
  donnerait, et la console Max l'écrit (« Pattern 3: the set holds words this version cannot read… ») ; la prochaine
  édition écrit par-dessus. Avant, le device gardait un Pattern que le set n'avait plus.
- Question ouverte : l'external rend chaque Pattern relu au fil du scheduler par `schedule_delay(loadIntoSequencer)`,
  puis le libère par `defer_low(freeLoaded)` ; jusqu'à 16 passages en attente au chargement d'un set. Si l'objet est
  détruit avant, `freeLoaded` est sans danger (au pire le Pattern fuit), mais `loadIntoSequencer` (comme
  `applyToSequencer` pour une édition) lit l'objet détruit si Max l'appelle encore. Le SDK ne dit pas ce que Max fait
  d'un appel en attente sur un objet libéré, et `destroy` ne peut pas l'annuler. Pour trancher : un build instrumenté,
  hors de Live, qui programme un `schedule_delay`, libère l'objet avant que le scheduler tourne, et journalise si
  l'appel arrive. Sinon, remplacer le passage par des cases atomiques préallouées que l'horloge du scheduler vide et que
  `destroy` draine.

## Answer

Construit et vérifié dans Live (2026-10-13 et 14) : un `pattr` Blob par Pattern, écrit une demi-seconde après la dernière édition, rendu au chargement du set et par Ctrl+Z ; la banque de 16 au ticket 21. Une chaîne de Patterns y est gardée aussi depuis le 2026-10-17 (ticket 31).
