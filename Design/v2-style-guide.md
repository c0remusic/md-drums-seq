# md-drums-seq v2 : guide de style

Famille A de `v1-style-guide.md` (look natif de Live, objets `live.*`), plus les couleurs fixes des deux `lcd`.

## Couleurs du lcd

`lcd` ne suit pas le thème de Live et n'a pas de transparence : le patch lui donne des couleurs opaques, mélangées
d'avance. Thème Dark de Live 12 approché :

| Rôle | Hex | Mélange |
|---|---|---|
| puits d'une case | #1e1e1e | |
| puits d'un temps (tous les 4 pas) | #262626 | |
| pas joué | puits + 18 % de #7cc6ff | |
| Hit | #1e1e1e → #ffa52e | 35 % + 65 % × vélocité / 127 |
| Lock, cadres du pas joué et sélectionné | #7cc6ff | |
| texte de Condition | #1e1e1e | sur le Hit |
| trait de ratchet | couleur du fond de la case | |

## Typographie

Comme v1 (police de Live, 10 / 11 / 13 / 16 px) ; dans le `lcd`, la Condition en Arial gras 10 px.

## États

| État | Rendu |
|---|---|
| Lock absent | `live.dial` inactif, valeur « Kit » en italique discret |
| Lock posé | arc et valeur en accent, nom du paramètre en texte |
| moitié montrée, Follow | bouton plein comme une option choisie |
| ratchets | traits dans le Hit ; la courbe se lit dans le panneau du pas |

Le reste est inchangé depuis v1.
