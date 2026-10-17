# Paramètres de MD Drums visibles par LOM

Type: research
Status: resolved
Assignee: —
Blocked by: —

## Question

Le device veut lire, en lecture seule, la Machine de chaque Track et les valeurs du Kit dans MD Drums (« Sémantique du
Pattern »). MD Drums est un VST3 à 576 paramètres hôte (34 par Track, puis 32 d'effets master ; identifiants
`page_part_index`, fichier `parameterDescriptions_mddrums.json` de gearmulator-md-drums ; la Machine est la page 5).
Depuis Max for Live, dans Live 12.2 et 12.4 :

- quels paramètres d'un VST3 posé sur une autre piste un `PluginDevice` expose-t-il par LOM (`parameters`) : tous,
  seulement ceux configurés dans Live (« Configure »), avec quelle limite ?
- leurs noms et leurs valeurs (normalisées 0-1, ou dans l'unité du plug-in ?), et peut-on les observer sans les écrire ?
- un set peut-il garder une configuration de 16 + 16 × 24 paramètres, ou faut-il choisir ?

Sources : référence LOM de Cycling '74 (`PluginDevice`, `DeviceParameter`), manuel de Live sur la configuration des
plug-ins, base de connaissances d'Ableton ; et le JSON des paramètres de MD Drums pour ce qu'il déclare.

## Answer

La LOM ne montre d'un plug-in que ses paramètres automatisables, c'est-à-dire ceux du panneau de Live. Au-delà de 64
paramètres, le panneau s'ouvre vide : sans configuration, le device ne voit aucun des 576 paramètres de MD Drums, et
aucun appel LOM ne peut en configurer. Les sets 12.2.1 de la machine réservent 128 places par instance de plug-in, et
le forum parle d'un plafond de 128 : 16 + 16 × 24 = 400 ne tiennent pas, il faut choisir (16 Machines et 112 autres au
plus). Une valeur se lit normalisée 0-1 (Machine : identifiant = valeur × 191), s'observe sans écriture, et son texte
vient de MD Drums (« TRX-BD », « PTCH 64 »). `is_quantized` pour un VST3 reste à vérifier. L'utilisateur doit
configurer les 16 « Track n Machine » une fois, puis « Save as Default Configuration » ; les noms de SYN1-8 viennent
alors d'une table embarquée. Trou probable dans MD Drums : SYN1-8 remis par un changement de Machine ne sont pas
signalés à l'hôte. Détail et sources : `research/12-parametres-md-drums-lom.md`.
