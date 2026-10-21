# Capture d'une performance et Arrangement

Type: grilling
Status: open
Assignee: —
Blocked by: 11 (la prise vers l'Arrangement seulement)

## Question

Ce que le ticket 07 a renvoyé à plus tard : noter ce que le device a réellement joué (Conditions tirées, Fill, Mutes,
bascules de Pattern) et l'écrire après coup ; imprimer et capturer en Arrangement, à la position jouée ou au point
d'insertion. Créer un clip en Arrangement par LOM plante avant Live 12.4 (ticket 11). À trancher : quand la capture
écrit (arrêt du transport, bouton), où (Arrangement à la position jouée quand Live enregistre, Session sinon), combien
de temps le journal garde, et comment la bande montre l'état de capture (passation : « Arrangement when Live
records »).

## Answer

Grilling du 2026-10-19 avec l'utilisateur, deux rounds, recommandations prises (Q1 à Q10).

1. **Contenu** : le flux réellement joué en direct. Conditions telles que tirées, Fill tel que tenu, Patterns et
   chaînes tels que basculés, positions exactes. Une Track coupée n'a pas de notes pendant sa coupure. Mêmes notes
   que l'impression (Hits, Locks, Accent, Slide) et même destination : un clip par Track sur sa piste « MD nn »,
   retrouvée par son routage ou créée (ticket 07).
2. **Quand** : les deux.
   - Quand Arrangement Record est allumé, le device note la prise depuis le départ du transport et l'écrit à l'arrêt,
     aux positions jouées.
   - Sinon, un bouton « Capture » de la bande écrit après coup la dernière prise : du dernier départ au dernier arrêt,
     ou jusqu'à maintenant pendant la lecture.
3. **Où** :
   - prise enregistrée par Live : dans l'Arrangement, aux positions jouées ;
   - capture au bouton : en Session, comme l'impression (scène sélectionnée si ses emplacements sont libres, sinon
     une scène neuve dessous). Un clip par Track, de la longueur de la prise, nommé « Take n », sans boucle ; rien
     n'est écrasé.
4. **Double jeu** : un interrupteur « Device / Clips » dans la bande, paramètre automatisable, Device par défaut,
   gardé avec le set. Sur Clips, le device n'envoie plus de note-on à MD Drums ; ses note-off passent toujours, pour
   que Live ne garde aucune note tenue (ticket 24). Son Sequencer tourne, la tête de lecture et Push le suivent, et
   il note toujours la prise.
5. **Journal** : un tampon fixe d'un million de notes par device (environ 16 Mo), écrit sur le fil du scheduler sans
   allocation ; un Pattern ordinaire envoie de l'ordre de 1000 notes par mesure. Tampon plein : la capture au bouton
   garde les plus récentes, la prise enregistrée par Live s'arrête de noter avec un avertissement dans la bande.
6. **Boucles et sauts** :
   - vers l'Arrangement, chaque note va à sa position de morceau, et là où des passages se recouvrent, le dernier
     gagne ;
   - au bouton, la prise est le temps joué bout à bout, sauts compris.
7. **Arrangement déjà rempli** : une prise enregistrée par Live remplace la plage jouée sur les pistes « MD nn »
   seulement, comme un enregistrement de Live. L'impression, elle, n'écrase jamais rien.
8. **Bande** : un bouton « Capture », allumé quand une prise est disponible, avec sa longueur (« Take 12 bars ») ;
   « Recording to Arrangement » quand Live enregistre ; le résultat dans la ligne d'état, comme pour l'impression.
   Rien de plus dans la fenêtre d'édition.
9. **Ordre de construction** : d'abord la capture au bouton vers la Session et l'interrupteur, qui marchent en Live
   12.2.1 (l'impression crée déjà des clips de Session) ; la prise vers l'Arrangement après la mise à jour en 12.4
   (ticket 11).

## Comments

- 2026-10-20, **capture vers la Session et interrupteur Device / Clips construits**, garde-fou 14/14, pas encore
  essayés dans Live :
  - `mdseqCapture.*` : le `Journal` (anneau d'un million de notes, deux mots atomiques par note, écrit sur le fil du
    scheduler, lu sur le fil principal en jetant ce qui a été écrasé pendant la copie), en temps de prise depuis la
    barre avant le départ, sauts joints ; `takeClips` (un clip par Track, Locks, Accent et Slide aussi longs que leur
    Hit, clip arrondi à la mesure) ; `takeText` (« Take 12 bars », « No take »).
  - `Wake` dit le départ, le saut (depuis où) et l'arrêt ; `Printer::capture` place la prise comme une impression,
    « Take n », sans boucle (`LiveSet::createClip` prend la boucle). L'état d'une impression conseille maintenant
    « Set the device to Clips, or each Hit plays twice. »
  - Bande : « Capture » (908, 70), actif quand la prise a une note, sa longueur au-dessus (908, 52) ; « Device /
    Clips » (828, 148), paramètre Live « Output ». Sur Clips, les note-on restent dans le device, ses note-off sortent.
    Les frappes de pads de Track sur Push sortent toujours, et ne vont pas dans la prise.
  - Tests : `mdseqCaptureTest` (journal, anneau, clips, `Wake`, et une Pass capturée égale la Pass imprimée de la
    même graine), `mdseqPrinterTest` captures, `mdseqProtocolTest`.
  - La case « Capture » de la fenêtre (1328, 16) reste en lecture seule, « Session » : rien de plus dans la fenêtre.
- 2026-10-21, **capture vers la Session essayée dans Live** avec `mdseq_dbg41` (l'arbre de 1b33edb : la v12, les noms des banques utilisateur), sur une piste de test
  d'un set neuf, devant MoDule ; l'utilisateur, après la liste d'essais : « tout semble ok ».
  - Capture (clips « Take n » dans une scène neuve), Clips only, Print avec « LAST TAKE » puis « PATTERN ×2 ».
  - Clips only mesuré la veille par la LOM : crête du mètre de la piste 0,921 éteint, 0,000 allumé.
  - La v12 a remplacé « Device / Clips » par la bascule « Clips only » (même paramètre « Output ») et la case
    « Session » de la fenêtre par le menu de Print, dont « LAST TAKE » écrit la prise comme Capture.
  - Reste ouvert : la capture vers l'Arrangement (Live 12.4).
