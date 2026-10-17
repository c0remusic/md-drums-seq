# Impression et capture

Type: grilling
Status: resolved
Assignee: c0remusic (session du 2026-10-13)
Blocked by: —

## Question

Imprimer, c'est écrire par LOM, sur demande et d'un seul lot, ce que le device joue dans des clips Live. Deux gestes :

- **Imprimer N Passes** du Pattern, Conditions tirées : vers quels clips (un par piste Track, dans quel emplacement
  Session ou à quelle position de l'Arrangement), avec quel tirage (le prochain, ou une graine choisie) ?
- **Capturer une performance** : le device note ce qu'il a réellement joué en direct (positions exactes, Conditions
  tirées, Fill, mutes, bascules de Pattern) et l'écrit après coup. Quand (à l'arrêt du transport, sur un bouton ?), où
  (Session, Arrangement à la position jouée), et combien de temps garde-t-il le journal ?

Contraintes apportées par « Ce que LOM permet dans Live 12 » : chaque écriture entre dans l'historique d'annulation
(d'où un seul lot, jamais pendant le jeu) ; `Track.create_midi_clip` (depuis 12.1.10) refuse une piste en cours
d'enregistrement, et un crash à la création de clips Arrangement n'est corrigé qu'en 12.4 (« Live 12.4 installé »).

## Answer

Grilling du 2026-10-13 avec l'utilisateur (recommandations acceptées, trois rounds). Le ticket ne livre que
l'impression ; la capture d'une performance et l'Arrangement attendent Live 12.4 (ticket 23).

Faits pris en compte : un clip Live ne garde pas de canal MIDI, la piste le donne ; LOM règle la sortie MIDI d'une piste
mais pas un External Instrument ; chaque écriture LOM fait une entrée d'annulation, sans regroupement ; créer un clip
en Arrangement par LOM plante avant 12.4 (machine en 12.2.1) ; mesuré le 2026-10-13
(`measure/live/live-routing-probe.mjs`)
: une piste MIDI qui sort vers la piste du device propose « Track In » ou « 1-MD Drums » … « 16-MD Drums », ce second
choix allant droit au plug-in sur le canal N, sans passer par le device (où Live efface le canal, ticket 04).

1. **Destination** : un clip par Track, sur sa propre piste MIDI, dont la sortie va à la piste du device sur le canal
   « N-MD Drums ». Canal gardé, réglé par LOM, sans External Instrument ; le son reste sur la piste de MD Drums (Main
   et Outs, ticket 05). Un seul clip pour toutes les Tracks est écarté : tout passerait sur le canal 1 et dépendrait de
   l'ordre, non mesuré, dans lequel Live joue des notes simultanées, et le clip serait illisible.
2. **Pistes retrouvées par leur routage** : la piste de la Track N est celle dont la sortie MIDI va à la piste du
   device, canal « N-MD Drums » ; elle résiste aux renommages et aux déplacements. Deux pistes sur le même canal : la
   première du set l'emporte, avec un avertissement dans la fenêtre.
3. **Pistes manquantes créées par l'impression** : pour chaque Track qui a des notes et pas de piste, une piste MIDI
   « MD nn » insérée après celle du device, sortie sur « N-MD Drums ». L'impression ne supprime ni ne reroute jamais
   rien.
4. **Locks de la Track 1** : sur le canal 1, MD Drums lie un Lock au Hit juste avant lui au même échantillon, quelle
   que soit sa Track ; un Hit d'une autre piste glissé entre le Hit de la Track 1 et ses Locks les volerait. La session
   MD Drums est priée de restreindre la règle au Hit du canal 1 juste avant : rien ne change pour le jeu en direct
   (tout y arrive sur le canal 1), les clips deviennent sûrs.
5. **Contenu** : N Passes (1 à 16) du Pattern affiché, tirés de la graine du dernier départ du transport (ce qu'on a
   entendu ; une graine neuve si le transport n'est jamais parti), Passes comptés depuis 1. Fill coupé, sauf s'il est
   tenu au clic. Mutes ignorées (on coupe une piste dans Live). Une Track sans note sur les N Passes n'a pas de clip.
   Les notes sont le flux du device (« flux joué = flux imprimé », ticket 08) : Hit et Locks d'un pas ou d'une tranche
   de ratchet de long, micro-timing, courbe de vélocité, note raccourcie si la même revient plus tôt.
6. **Bords du clip** : un Hit du premier pas en avance passe à la fin du clip (en boucle, il sonne avant chaque
   reprise, comme le device) ; une fin de note qui déborde est coupée à la fin du clip.
7. **Emplacement** : Session seulement en v1. Scène sélectionnée si les emplacements des Tracks imprimées y sont
   libres, sinon une scène neuve insérée juste dessous ; rien n'est écrasé. Clip nommé « P01 ×4 », en boucle sur ses N
   Passes.
8. **Moment et annulation** : impression à tout moment, même en lecture, d'un seul lot sur le fil principal ; une
   entrée d'annulation par piste créée, par clip et par lot de notes, acceptées (défaire une impression : supprimer
   ses clips ou sa scène).
9. **Device** : l'impression n'y touche pas ; la ligne d'état de la fenêtre dit le résultat (« Printed P01 ×4 in
   scene 3: 5 clips », ou ce qui a manqué) et rappelle de couper le device ou les clips, qui sinon jouent deux fois
   chaque Hit. Une bascule Device / Clips pourra venir avec le ticket 06.
10. **Interface** : « Print » et le nombre de Passes (1-16) dans la barre du haut de la fenêtre, comme la passation ;
    rien dans la bande.

Construction : ticket 22. Capture et Arrangement : ticket 23.

## Comments

- 2026-10-09 : ce ticket était « Capture en Arrangement » pendant que Live armait l'enregistrement ; il couvre
  maintenant l'impression et la capture après coup, puisque le jeu passe par `midiout`.
- 2026-10-13 : décision 4 faite côté MD Drums, `73c45ab8` de gearmulator-md-drums (ADR 0003, deuxième amendement «
  the last channel-1 Hit ») : un Lock du canal 1 va au dernier Hit du canal 1 reçu juste avant lui au même
  échantillon, sinon à la Track 1 ; canaux 2-16 inchangés. Testé là-bas : un Hit du canal 5 glissé entre le Hit du
  canal 1 et son Lock ne le prend plus. Le Live en cours garde l'ancienne DLL jusqu'à son redémarrage ; la préversion
  publique md-drums-v0.1.0 a encore l'ancienne règle.
