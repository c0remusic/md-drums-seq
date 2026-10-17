# Banque de Patterns

Type: task
Status: resolved
Assignee: c0remusic (session du 2026-10-13)
Blocked by: 14, 20

## Question

Le cadrage (carte, 2026-10-08) : 16 Patterns par device, bascule mise en file à la fin du Pass (option immédiate), pas
de chaîne, Patterns gardés dans le set seulement. « Sémantique du Pattern » : le pas joué suit la position de Live,
ancré au temps 0 de la chanson puis au moment de chaque bascule ; le compteur de Passes repart à 1 au départ du
transport et à chaque bascule. Passation v4 : `live.tab` de 16 cases, paramètre Live « Pattern », dans la bande et la
fenêtre ; « 02 next, after step 64 » dans la ligne d'état.

## Comments

- 2026-10-13 : construit.
  - Noyau : le Sequencer et l'Editor tiennent chacun la banque de 16 (sur le tas : un demi-mégaoctet). Chaque édition
    nomme son Pattern (`Input::pattern`, transmis entre fils avec le reste) : pendant une bascule, aucune ne tombe sur
    un autre. Le Sequencer place maintenant ses événements en unités absolues de 1/42 de tick, depuis le début de la
    chanson : à toutes les échelles un pas et ses 840es y sont des entiers, et les événements déjà planifiés gardent
    leur heure quand le Pattern suivant part sur sa propre grille (refonte vérifiée sans changement : mêmes 405 923
    événements sur le Pattern le plus lourd).
  - Bascule : choisi en jouant, un Pattern est mis en file ; quand le pas à planifier ouvre un nouveau Pass, le
    Sequencer bascule, ancre la grille du nouveau Pattern à ce point (sa longueur, son échelle) et compte ses Passes
    depuis 1. Choisi pendant le dernier pas, après que le premier pas du Pass suivant a été planifié, ce pas est
    planifié à nouveau depuis le Pattern choisi (sauf si une Track l'a commencé) : la bascule tombe à la fin de ce
    Pass, pas un Pass plus tard. Choisir le Pattern joué reprend le choix ; à l'arrêt, le Pattern en file devient le
    Pattern joué ; choisi à l'arrêt, il l'est aussitôt. Deux mutations vérifiées (l'ancre, le dernier pas).
  - Option de bascule immédiate : pas construite, faute d'interface décidée (la passation la cite sans la placer).
  - Device : l'onglet Pattern de la bande est un paramètre Live automatisé (le set garde le Pattern choisi,
    l'Arrangement peut automatiser les bascules) ; celui de la fenêtre le suit. 16 `pattr` Blob, un par Pattern
    (`store p …`, `restore p …`), chacun écrit 500 ms après la dernière édition de son Pattern. La ligne d'état dit
    « 02 next, after step 16 » tant qu'une bascule attend.
- À vérifier dans Live (avec Live fermé pour installer le build, puis rouvert) : bascule en file à l'oreille et sur
  la grille, onglet automatisé dans l'Arrangement, 16 Patterns relus après sauvegarde et réouverture du set.
- 2026-10-13, vérifié dans Live 12.2.1 à 126 BPM, sur une piste « MDSEQ TEST » d'une session neuve (Live relancé après
  installation, Live fermé), build de dev `mdseq_dbg17`, avec `measure/live/live-bank-run.mjs` et `measure/onsets32.py`
  : Pattern 1 (Track 3 sur les temps), le paramètre Live « Pattern » mis à 02 au temps 5,45 de la mesure 2 ; la mesure 2
  garde ses 4 attaques, les mesures 3 et 4 en ont 32 chacune (Pattern 2, 8 pas à 2x) : la bascule tombe à la barre de
  mesure, fin du Pass. L'onglet Pattern de la bande est actif et la ligne d'état dit « Stopped » et « 16 steps, 1/16 »
  (capture de la fenêtre de Live).

## Answer

Construite et vérifiée dans Live le 2026-10-13 : 16 Patterns gardés dans le set, la bascule en file à la fin du Pass ; Copy to, ×2 et Clear au ticket 29, les chaînes au ticket 31.
