# 64 pas et échelle

Type: task
Status: resolved
Assignee: c0remusic (session du 2026-10-13)
Blocked by: 13, 15

## Question

Le cadrage (carte, 2026-10-08) : jusqu'à 64 pas communs aux 16 Tracks, un pas valant une double-croche par défaut, et
une échelle globale à la manière de la MKII (1/8x à 2x). « Sémantique du Pattern » : le pas joué suit la position de
Live, temps de la chanson modulo la longueur du Pattern (× l'échelle).

## Comments

- 2026-10-13 : construit.
  - Les 64 pas existaient déjà : longueur 1-64 (menu Length), quatre pages de 16 dans la bande, deux moitiés de 32
    dans la fenêtre.
  - Échelle : un index par Pattern (gardé dans le set avec lui, ticket 14), sept valeurs de la MKII. Un pas dure 960
    ticks à 1/8x (une blanche), 480, 240 (une croche), 160 à 3/4x (un triolet de croches), 120 à 1x, 80 à 3/2x (un
    triolet de doubles), 60 à 2x (une triple croche). Les sous-ticks restent 840 par pas, quelle que soit l'échelle :
    ratchets et micro-timing gardent leurs proportions.
  - Changer d'échelle en jouant change le pas que désigne la position de Live : le Sequencer le traite comme un saut,
    au réveil suivant, depuis la position (les notes tenues finissent, le pas commencé ne joue pas en retard, le
    suivant tombe sur la nouvelle grille). Test, et mutation : sans ce recalage, le test casse.
  - Fenêtre : le menu Scale est actif (sans valeur initiale, contrôle d'édition) ; la ligne d'état de la bande dit la
    note d'un pas (« 16 steps, 1/32 » à 2x).
- À vérifier dans Live : un Pattern à 2x et à 3/4x contre la grille de Live.
- 2026-10-13 : vérifié dans Live 12.2.1 à 126 BPM, sur une piste « MDSEQ TEST » d'une session neuve (Live relancé après
  installation, Live fermé), build de dev `mdseq_dbg17` : un Pattern de 8 pas à 2x joue 32 attaques par mesure, une par
  triple croche (`measure/onsets32.py`).
