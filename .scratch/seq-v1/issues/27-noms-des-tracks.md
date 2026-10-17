# Noms des Tracks par MD Drums

Type: task
Status: resolved
Assignee: c0remusic (session du 2026-10-16)
Blocked by: —

## Question

L'utilisateur, le 2026-10-16 : « on a toujours pas les noms des pistes ». La bande et la fenêtre numérotent les Tracks
de 01 à 16 ; la passation v4 les voulait nommées par leur Machine (« numéro et nom court de la Machine ») et SYN1-8
nommés par la Machine de la Track. Le ticket 12 passait par LOM, sur des paramètres que l'utilisateur configure dans
Live : 16 des 128 places que la liste Push de MD Drums (« a page per Track ») remplit déjà, et jamais les valeurs du
Kit. L'utilisateur a choisi que MD Drums les publie.

## Comments

- 2026-10-16, contrat : ADR 0003 (`docs/adr/0003-md-drums-tells-the-device-its-kit.md`). Le device tire un id de 24
  bits à sa création et l'annonce dans le MIDI qu'il envoie déjà : quatre notes 124-127 dont les vélocités portent
  l'id, au démarrage, puis chaque seconde tant qu'aucun fichier ne répond, toutes les cinq secondes ensuite. MD Drums
  écrit `<temp>/md-drums-seq/<id>.kit` (texte, version 1 : nom du Kit, Machine et noms de SYN1-8 par Track, 24
  valeurs du Kit par Track) et l'efface quinze secondes après le dernier hello. La session MD Drums accepte le
  format tel quel et le construit (amendement de son ADR 0003 : les notes 124-127 deviennent le hello, ne déclenchent
  rien et ne comptent plus comme « note qui ne joue rien »).
- 2026-10-16, côté device construit : `mdseqKit.*` (lecture du fichier, octets du hello, `KitLink` qui dit quand
  saluer et quand le Kit change, `mdseqKitTest`), l'Editor (`setKit` : onglets de la bande « 01 TRX-BD » par
  `_parameter_range` puis l'onglet choisi remis, colonne Machine des rangées de la fenêtre, titre du panneau
  « Track 04, TRX-CP », noms de SYN1-8 de la Machine du pas sélectionné ; numéros et « SYN1 » sans Kit,
  `mdseqEditorTest` kitNames), quatre clés de panneau (`tracknames`, `trackindex`, `trackname`, `lockname`),
  l'external (horloge de 500 ms, lecture sur le fil principal, hello du fil du scheduler par `schedule_delay`).
  Garde-fou 11/11.
- Les valeurs du Kit sont lues mais pas encore montrées (fantôme sur les boutons de Lock : à faire).
- 2026-10-16, **vérifié dans Live** avec MD Drums 035a88e5 (qui écrit le fichier) et un build de développement à
  c7448e9, sur une piste temporaire « MDSEQ KIT » (le device puis MD Drums), supprimée ensuite :
  - le hello arrive et MD Drums répond : `bf630b.kit` (Kit « TRX UW »), lu par le device 0,4 s après sa démonstration
    (journal) ;
  - la bande nomme ses onglets « 01 TRX-B2 » … « 16 ROM-29 » par `_parameter_range`, l'onglet choisi reste 01 ; la
    fenêtre nomme ses rangées, le titre dit « Track 01, TRX-B2 », SYN1-8 lisent PTCH DEC RAMP HOLD TICK NOIS DIRT
    DIST (captures) ;
  - MD Drums retiré de la piste : son fichier part aussitôt, les onglets reprennent leurs numéros, les rangées se
    vident (le symbole vide s'affiche vide), le titre redit « Track 01 » ;
  - MD Drums remis sur la piste : son fichier revient 1,3 s après le chargement ;
  - un second device sur un second MD Drums, le temps de l'essai : deux fichiers, deux ids. Le produit reste un seul
    device par set (l'utilisateur, le 2026-10-16 : « je veux un seul device ») ; l'id sert à trouver son MD Drums
    parmi plusieurs.
- Les onglets de la bande coupent les noms complets dans une case de 36 px (ticket 15, sonde des onglets du
  2026-10-16) : la maquette v5 les veut courts (« 01 B2 »).

### Checklist dans Live

1. Fait le 2026-10-16 (ci-dessus), sauf : changer la Machine d'une Track dans MD Drums renomme son onglet en une
   seconde (un clic dans MD Drums, à la main).

## Answer

Vérifié dans Live le 2026-10-16 (ci-dessus) et, le 2026-10-17, par l'utilisateur : changer la Machine d'une Track dans MD Drums renomme son onglet dans la seconde. Les valeurs du Kit sur les boutons de Lock sont venues au ticket 28.
