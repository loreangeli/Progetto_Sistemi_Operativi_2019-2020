#!/bin/bash

exec 3<filelog.txt

while read -u 3 linea ; do
    tmp=${linea/id_cliente:/id cliente:}
    tmp=${tmp/prodottiacquistati:/n.prod.acquistati:}
    tmp=${tmp/tempototalenelsupermercato:/t.tot.nelsupermercato:}
    tmp=${tmp/tempototalespesoincoda:/t.tot.spesoincoda:}
    tmp=${tmp/n.codevisitate:/n.codevisitate:}
    tmp=${tmp/tempocolcassiere(temposerviziodiogniclienteservito):/t.concassiere:}
    tmp=${tmp/cassa/id cassa}
    tmp=${tmp/il numero di prodotti elaborati è:/n.prod.elaborati:}
    tmp=${tmp/numero clienti serviti/n.diclienti}
    tmp=${tmp/periodo apertura totale cassa (s)/tempo.tot.diapertura}
    tmp=${tmp/tempo medio di servizio della cassa/tempomediodiservizio}
    tmp=${tmp/numero di volte in cui la cassa è stata chiusa/n.dichiusure}
    echo ${tmp}

done
