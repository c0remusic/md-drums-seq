# Locks réglables depuis n'importe quel contrôleur, sans mapping

Type: task
Status: open
Assignee: —
Blocked by: —

## Question

Aujourd'hui les Locks se règlent à la souris dans la fenêtre d'édition, ou sur Push 2 en tenant un pas (ticket 33).
Un autre contrôleur n'a aucun moyen de les atteindre : les boutons de Lock de la fenêtre ne sont pas des paramètres
Live, et un mapping Ctrl+M par pas et par paramètre est impossible.

L'idée de l'utilisateur (2026-10-21) : quand on sélectionne une Track ou un pas, ses paramètres s'affichent là où un
contrôleur les prend, comme les 32 relais de MoDule qui suivent la Track montrée (ticket 27 de MoDule). Et sans que
l'utilisateur ait à mapper : ni un device Max ni le LOM ne peuvent créer un mapping MIDI de Live, mais la « main
bleue » le rend inutile. Un contrôleur doté d'un script Live (Push, Launchkey, Launch Control, APC...) règle les
banques de paramètres du device sélectionné, que `live.banks` définit.

Proposition :

- des réglages « Lock 1 » à « Lock n », des `live.dial`, qui visent le pas sélectionné (la Track montrée, la valeur
  du Kit, quand aucun pas ne l'est) ; tourner l'un d'eux écrit le Lock de ce pas, comme un encodeur de Push tenu ;
- des banques `live.banks` dont la première page porte ces réglages, nommés d'après le paramètre qu'ils visent
  (SYN1-8 selon la Machine, puis EFX et ROUTING) ;
- Push 2 garde son mode du ticket 33 quand l'interrupteur Push est allumé.

## À trancher

1. Le contrôleur voit-il les banques de `live.banks` d'un device Max for Live, et sous quels noms ? Avec Push 2 (main
   bleue, interrupteur Push éteint) et avec un autre script si l'utilisateur en a un. Première étape, à mesurer dans
   Live avant de construire quoi que ce soit.
2. 8 réglages, une page qui suit la page de Locks choisie, ou 24 en trois pages ?
3. Le réglage suit la sélection : écrire sa valeur depuis le device, est-ce une édition aux yeux de Live
   (automation enregistrée pendant un enregistrement, entrée dans l'historique d'annulation) ? MoDule a réglé le même
   cas pour ses relais (ticket 34 de MoDule : changement sans geste). À mesurer dans Live, armement et
   enregistrement compris.
4. Sans pas sélectionné : le réglage montre la valeur du Kit de la Track et ne fait rien, ou il règle le Kit lui-même
   (ce qui serait un paramètre de MoDule, pas un Lock) ?
5. Un réglage tourné sur un pas sans Hit : rien, ou pose un Hit (les Locks demandent un Hit, `applyEdit`) ?
6. Comment le device donne aux réglages la valeur du pas sélectionné sans écrire d'automation ni d'annulation ?
   Piste vue dans Stepic for Live (Devicemeister, lu le 2026-10-22 dans son `.amxd`) : ses 8 séquenceurs de
   modulation pilotent des paramètres de Live par `live.remote~`, chacun visant le paramètre qu'un `live.object`
   lui désigne. `live.remote~` règle un paramètre sans automation ni historique d'annulation. Ici, il réglerait les
   réglages « Lock » du device lui-même quand la sélection change, pendant que le geste de l'utilisateur sur le
   contrôleur passe, lui, par le paramètre comme d'habitude. À mesurer dans Live avec la question 3.

## Answer

—
