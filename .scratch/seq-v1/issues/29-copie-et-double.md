# Copier un Pattern, doubler sa longueur

Type: task
Status: resolved
Assignee: c0remusic (session du 2026-10-16)
Blocked by: —

## Question

L'utilisateur, le 2026-10-16 : « on a toujours pas la possibilité de copier un pattern vers un autre, ni de copier
une partie du pattern dans la suite du pattern (par exemple 16 pas vers 32 etc) ». Choisi le même jour, entre deux
gestes pour chacun : un menu « Copy to » plutôt qu'un Copy / Paste à la Elektron, et un bouton « ×2 » comme
« Dupl. Loop » d'un clip Live plutôt qu'une copie de page.

## Comments

- 2026-10-16 : construit, garde-fou 11/11.
  - Deux éditions de plus (`Input::Kind`) :
    - `CopyPattern` : le Pattern `value` copié sur celui que l'édition nomme (`pattern`), si bien que le
      `PatternStore` écrit le Blob de ce dernier, et que l'annulation de Live (Ctrl+Z) le rend comme toute écriture
      de Blob (ticket 14). `applyEdit` ne voit qu'un Pattern : l'Editor et le Sequencer copient dans leur banque, le
      Sequencer par `load`, qui rejoue le pas planifié si le Pattern écrit est celui qui joue ;
    - `DoubleLength` : la longueur doublée, 64 au plus, et les pas 1-L recopiés après eux sur chaque Track (Hits,
      vélocités, Locks, Conditions, ratchets, micro-timing) avec leur place dans la rangée swing ; de 40 pas, 64, les
      pas 41-64 étant 1-24. Ce qui suivait la longueur est écrasé. Le pas planifié d'avance ne change pas : la
      position suit celle de Live, et la copie lui donne le même contenu.
  - Messages `copyto i` (l'index du menu, 0 son étiquette) et `double` ; clé de panneau `copyto 0`, qui remet le menu
    sur « Copy to » après chaque choix et à chaque redessin.
  - Fenêtre : le menu à 612 après les Patterns, ×2 à côté de Length ; la barre du haut resserrée (`HANDOFF.md`).
  - Pas de retour écrit d'une copie : le menu revient, et le Pattern copié se voit en cliquant son onglet.

### Checklist dans Live

1. Fenêtre : Copy to 05 depuis le Pattern 01 ; l'onglet 05 montre la même grille ; le menu relit « Copy to ».
2. Ctrl+Z dans Live : le Pattern 05 redevient ce qu'il était (vide).
3. Length 16, ×2 : Length 32, les pas 17-32 rejouent 1-16, Locks et Conditions compris ; en lecture, rien ne saute
   (la position, celle de Live modulo la longueur, peut passer dans la copie, qui joue la même chose).
4. ×2 à 64 pas : rien ne change.

## Answer

Vérifié dans Live par l'utilisateur le 2026-10-17 : Copy to (et Ctrl+Z qui rend le Pattern écrasé), ×2. Ajouté le même jour, à sa demande : Clear, un bouton au-dessus des rangées de la fenêtre, à côté de Fill edit, qui rend le Pattern affiché vide, sa longueur, son échelle et son swing compris (`Input::Kind::ClearPattern`, message `clear` ; le Sequencer le charge par `load`, comme une copie). Ctrl+Z dans Live le rend, par son Blob.
