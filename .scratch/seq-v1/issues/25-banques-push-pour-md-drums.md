# Banques Push pour MD Drums

Type: grilling
Status: open
Assignee: —
Blocked by: — (décision de l'utilisateur)

## Question

L'utilisateur veut régler MD Drums depuis Push 2/3, dans des banques nommées, et surtout enregistrer ces gestes en
automation. Un VST3 ne nomme pas ses banques et ne choisit pas celle que Push affiche (ticket 02 de MD Drums) ; un
device Max for Live le peut, avec ses propres paramètres et `live.banks`. Choisi le 2026-10-14 : les banques vivent
dans MD Drums Seq, pas dans un device à part.

Deux chemins vers MD Drums sont sur la table, avec la session MD Drums :

- **LOM** : les paramètres de MD Drums Seq posent les 32 relais de MD Drums (`9_0_0`-`9_0_31`, « la Track que
  l'éditeur montre ») et ses 32 paramètres de master par `live.object`. Exige « Prepare Push list » puis « Save as
  Default Configuration » dans Live (608 paramètres : la LOM ne voit que ceux configurés). Une automation enregistrée
  vise « la Track montrée » : rejouée après un changement de Track, elle bouge la mauvaise.
- **NRPN** : MD Drums prendrait des NRPN sur tout canal (MSB = Track, LSB = paramètre, data entry = valeur) et dirait
  à Live une édition de son vrai paramètre (« Track 3 FLTF »), que Live enregistre. Plus de LOM ni de configuration.

## Mesuré le 2026-10-14 : Push ne montre que des paramètres automatisables

`measure/md-seq-banks-probe.amxd` (`measure/make_devices.py`) : trois `live.numbox`, un par visibilité de paramètre
(Automated, Stored Only, Hidden), et deux banques écrites dans le registre `parameters` du patcher (`parameterbanks`) :
« Probe » (les trois) et « Not automated » (Stored Only et Hidden). `measure/live/live-banks-probe.mjs` le charge sur
une piste temporaire et lit ce que les scripts de Push lisent d'un device Max (Live 12, surface Push2 active) :

- `Device.parameters` : Device On, Auto Knob. Ni Stored Knob ni Hidden Knob ;
- `get_bank_count` 2 ; `get_bank_name` « Probe » et « Not automated » : les noms écrits dans le fichier passent ;
- `get_bank_parameters` : « Probe » [1, -1, -1, -1, -1, -1, -1, -1], « Not automated » huit -1.

Un paramètre que Live n'automatise pas laisse donc sa case vide sur Push. Tout encodeur de Push qui règle MD Drums par
MD Drums Seq tourne un paramètre automatisable de MD Drums Seq, que Live enregistre quand il enregistre.

## Conséquence pour le chemin NRPN

Si MD Drums dit aussi à Live une édition de « Track 3 FLTF », un geste enregistre deux automations (le paramètre de
MD Drums Seq et celui de MD Drums), qui se battent à la lecture. Pistes :

1. **Paramètres par Track dans MD Drums Seq** : « T03 FLTF » pour chaque Track et paramètre (16 × 24 moteur, plus
   Level, Mute, Solo, Out, Machine, LFO), automatisables ; `live.banks` (`edit` en temps réel) réaffecte les banques
   aux paramètres de la Track choisie sur Push. L'automation enregistrée nomme sa Track, la lecture envoie le NRPN de
   cette Track, et MD Drums ne dit rien à Live. L'automation vit sur MD Drums Seq, pas sur MD Drums.
2. **Un seul jeu de paramètres** (la Track choisie), et MD Drums dit l'édition à Live : double automation, à exclure.

## Answer

—

## Comments

- 2026-10-14 : en attente. L'utilisateur, par la session MD Drums : le contrôle en direct et l'automation qu'il veut
  sont dans le plug-in MD Drums lui-même, pas dans un device Max. Rien à construire ici pour l'instant, pas d'entrée
  NRPN côté MD Drums. Piste ouverte par l'utilisateur le même jour : un mode Push à la Tuple (`grab_control` sur les
  pads et les encodeurs, sans paramètre Live, donc sans double automation), qui rejoint la ligne « Push 2 » de la carte.
