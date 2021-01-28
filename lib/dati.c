//K: numero posizioni dell'array (rappresenta il numero delle casse)
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <time.h> 
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/select.h>
#include "dati.h"
#include "random.h"
#define DEBUG 0




item** crea( int K, item** coda, int* statocassa, int casseaperte, int* codaclienti, timecassa* tempocassa, elemcassa* lista) {
    //condizioni
    if (K<1) {
        fprintf(stderr,"K=%d non concesso \n",K);
        exit(EXIT_FAILURE);
    }
    if (statocassa==NULL) {
        fprintf(stderr,"statocassa non inizializzato \n");
        exit(EXIT_FAILURE);
    }
    if (casseaperte<1 || casseaperte>K) {
        fprintf(stderr,"non è possibile aprire %d casse \n",casseaperte);
        exit(EXIT_FAILURE);
    }
    if (casseaperte==K) {
        fprintf(stderr,"all'apertura del supermercato non possono essere aperte tutte le casse \n");
        exit(EXIT_FAILURE);
    }
    if (codaclienti==NULL) {
        fprintf(stderr,"codaclienti non inizializzato \n");
        exit(EXIT_FAILURE);      
    }

    #if DEBUG
    printf("** apertura di %d cassa/e: ", casseaperte);
    #endif

    //crea lista
    initlista(K);


    //inizializza codaclienti
    for (int i=0;i<K;i++) {
        codaclienti[i]=0;
    }


    //apro casse in un numero pari a "casseaperte" 
    for(int i=0;i<K;i++) {
        if (i<casseaperte) {
            inseriscicassa(i);
            statocassa[i]=1;
            #if DEBUG
            printf("[%d] ",i+1);
            #endif
            //set tempocassa(se cassa aperta)
            tempocassa[i].inizio=current_time_millisecond();
            tempocassa[i].fine=0;
            tempocassa[i].periodo=0;
        }
        else {
            //set tempocassa(se cassa chiusa)
            statocassa[i]=0;
            tempocassa[i].inizio=0;
            tempocassa[i].fine=0;
            tempocassa[i].periodo=0;           
        }
    }

    #if DEBUG
    printf("**\n");
    #endif

    //inizializza coda
    coda=(item **) malloc(K*sizeof(item*));
    for(int i=0;i<K;i++) 
        coda[i]=NULL;


    return coda;
}




item** inserisci(item** coda, int K, int cassa, long value, int prodotti, int tempoperacquisti,int* statocassa,int* codaclienti, double arrivoincoda) {
    //condizioni
    if (cassa<1 || cassa>K) {
        fprintf(stderr,"cassa=%d non concesso \n", cassa);
        exit(EXIT_FAILURE);
    }
    if (codaclienti==NULL) {
        fprintf(stderr,"codaclienti non inizializzato \n");
        exit(EXIT_FAILURE);      
    }  
    if (tempoperacquisti<10) {
        fprintf(stderr,"tempoperacquisti=%d non concesso \n", tempoperacquisti);
        exit(EXIT_FAILURE);      
    }
    if (K<1) {
        fprintf(stderr,"K=%d non concesso \n",K);
        exit(EXIT_FAILURE);
    }
    if (coda==NULL) {
        fprintf(stderr,"coda non allocata. Prova con: coda=create(K,coda) \n");
        exit(EXIT_FAILURE);
    }

    if (statocassa[cassa-1]==0) {
        fprintf(stderr,"cassa %d chiusa. impossibile inserire (inserisci) \n",cassa-1);
        return coda;
    }
    if (prodotti<0) {
        fprintf(stderr,"numero prodotti=%d non valido \n",prodotti);
        exit(EXIT_FAILURE);
    }
    
    item* testa=coda[cassa-1];
    //creo oggetto (nodo)
    item* oggetto=malloc(sizeof(item));
    oggetto->timeiniziocoda=arrivoincoda; 
    oggetto->timefinecoda=0;
    oggetto->next=NULL;
    oggetto->value=value;
    oggetto->prodotti=prodotti;
    oggetto->changecoda=0;
    oggetto->tempoacquisti=tempoperacquisti;

    //aggiorno codaclienti
    codaclienti[cassa-1]=codaclienti[cassa-1]+1;

    //coda vuota
    if (testa==NULL) {
        coda[cassa-1]=oggetto;
        return coda;
    }
    //altrimenti
    while(testa->next != NULL) {
        testa=testa->next;
    }
    
    testa->next=oggetto;
    return coda;
}




item** copiaincodadiversa(item** coda, int K, int cassa, item* cliente,int* statocassa, int* codaclienti) {

    //condizioni
    if (cassa<1 || cassa>K) {
        fprintf(stderr,"cassa=%d non concesso (copiaincodadiversa) \n", cassa);
        exit(EXIT_FAILURE);
    }
    if (codaclienti==NULL) {
         fprintf(stderr,"codaclienti non allocata.\n");
        exit(EXIT_FAILURE);       
    }
    if (K<1) {
        fprintf(stderr,"K=%d non concesso (copiaincodadiversa) \n",K);
        exit(EXIT_FAILURE);
    }
    if (coda==NULL) {
        fprintf(stderr,"coda non allocata. Prova con: coda=create(K,coda) \n");
        exit(EXIT_FAILURE);
    }

    if (statocassa[cassa-1]==0) {
        fprintf(stderr,"cassa %d chiusa. impossibile inserire (copiaincodadiversa) \n",cassa-1);
        return coda;
    }

    item* testa=coda[cassa-1];
    //creo oggetto (nodo)
    item* oggetto=malloc(sizeof(item));
    oggetto->next=NULL;
    oggetto->value=cliente->value;
    oggetto->prodotti=cliente->prodotti;
    oggetto->changecoda=(cliente->changecoda)+1;
    oggetto->timeiniziocoda=(cliente->timeiniziocoda);
    oggetto->timefinecoda=(cliente->timefinecoda);
    oggetto->tempoacquisti=(cliente->tempoacquisti);

    //aggiorno il numero di clienti in coda alla cassa
    codaclienti[cassa-1]=codaclienti[cassa-1]+1;

    //coda vuota
    if (testa==NULL) {
        coda[cassa-1]=oggetto;
        return coda;
    }
    //altrimenti
    while(testa->next != NULL) {
        testa=testa->next;
    }
    
    testa->next=oggetto;

    return coda;
}



item rimuovi(item** coda, int K, int cassa, int* statocassa, int* codaclienti) {
    // inizializzo default-client
    item copiacliente;
    copiacliente.value=-1;
    copiacliente.prodotti=-1;
    copiacliente.next=NULL;
    copiacliente.changecoda=-1;
    copiacliente.timeiniziocoda=-1;
    copiacliente.timefinecoda=-1;
    copiacliente.tempoacquisti=-1;

    //condizioni
    if (cassa<1 || cassa>K) {
        fprintf(stderr,"cassa=%d non concesso \n", cassa);
        exit(EXIT_FAILURE);
    }
    if (codaclienti==NULL) {
        fprintf(stderr,"codaclienti non allocata. \n");
        exit(EXIT_FAILURE);
    }
    if (coda==NULL) {
        fprintf(stderr,"coda non allocata. Prova con: coda=create(K,coda) \n");
        exit(EXIT_FAILURE);
    }
    if (K<1) {
        fprintf(stderr,"K=%d non concesso \n",K);
        exit(EXIT_FAILURE);
    }

    if ( (statocassa[cassa-1]==0) ) {
        printf("cassa %d chiusa. impossibile rimuovere \n",cassa);
        return copiacliente;
    }

    // se cassa vuota 
    if (coda[cassa-1]==NULL) {    
        return copiacliente;
    }

    //aggiorno codaclienti
    codaclienti[cassa-1]=codaclienti[cassa-1]-1;

    // altrimenti 
    //tolgo primo elemento dalla cassa
    item* pun=coda[cassa-1];
    item* succ=coda[cassa-1]->next;
    coda[cassa-1]=succ;
    //copio cliente in un oggetto prima di fare la free su di esso, in modo da poterlo restituire
    copiacliente.next=NULL;
    copiacliente.changecoda=pun->changecoda;
    copiacliente.prodotti=pun->prodotti;
    copiacliente.value=pun->value;
    copiacliente.timeiniziocoda=pun->timeiniziocoda;
    copiacliente.timefinecoda=current_time_millisecond();
    copiacliente.tempoacquisti=pun->tempoacquisti;

    free(pun);

    return copiacliente;
}




int opencassa(item** coda,int cassa ,int K,int* statocassa, timecassa* tempocassa, elemcassa* lista) {
    //condizioni
    if (coda==NULL) {
        fprintf(stderr,"coda non allocata. Prova con: coda=create(K,coda) \n");
        exit(EXIT_FAILURE);
    }
    if (K<1) {
        fprintf(stderr,"K=%d non concesso \n",K);
        exit(EXIT_FAILURE);
    }
    if (statocassa==NULL) {
        fprintf(stderr,"statocassa non inizializzato \n");
        exit(EXIT_FAILURE);
    }

    //se cassa chiusa
    if (statocassa[cassa]==0) { 
        statocassa[cassa]=1; //apertura cassa
        inseriscicassa(cassa);
        //gestione tempocassa
        if (tempocassa[cassa].periodo==0 && tempocassa[cassa].inizio==0 && tempocassa[cassa].fine==0) { //apro la cassa per la prima volta
            tempocassa[cassa].inizio=current_time_millisecond();
        }
        else { //cassa aperta anche in precedenza
            double fine=tempocassa[cassa].fine;
            double inizio=tempocassa[cassa].inizio;
            double tmp=fine-inizio;
            tempocassa[cassa].periodo=tempocassa[cassa].periodo+tmp;
            tempocassa[cassa].inizio=current_time_millisecond();
        }

        return 1;
    }

    //cassa già aperta
    return 0;
}




int closecassa(item** coda,int cassa,int K, int* statocassa, timecassa* tempocassa, int* nchiusure, elemcassa* lista) {
     //condizioni
    if (coda==NULL) {
        fprintf(stderr,"coda non allocata. Prova con: coda=create(K,coda) \n");
        exit(EXIT_FAILURE);
    }
    if (K<1) {
        fprintf(stderr,"K=%d non concesso \n",K);
        exit(EXIT_FAILURE);
    }
    if (statocassa==NULL) {
        fprintf(stderr,"statocassa non inizializzato \n");
        exit(EXIT_FAILURE);
    }   

    //controllo se c'è una sola cassa aperta in quel momento (ritorno 0 senza far nulla)
    int open=0;
    for(int i=0;i<K;i++)
        if (statocassa[i]==1)
            open++;
    if (open==1) {
        return 0;
    }
    //controllo se la cassa passata come argomento è già chiusa
    if (statocassa[cassa]==0) {
        return 0;
    }

    //chiudi cassa e sposta i clienti dalla cassa che voglio chiudere a un'altra cassa
    rimuovicassa(cassa);
    nchiusure[cassa]++;
    codaclienti[cassa]=0; //azzero i clienti della cassa
    statocassa[cassa]=0; //chiudo cassa
    /*gestisco tempocassa */
    tempocassa[cassa].fine=current_time_millisecond();
    double fine=tempocassa[cassa].fine;
    double inizio=tempocassa[cassa].inizio;
    double temp=fine-inizio;
    tempocassa[cassa].periodo=tempocassa[cassa].periodo+temp;
    tempocassa[cassa].inizio=0;
    tempocassa[cassa].fine=0;  

    /* cerco cassa aperta j dove spostare i clienti della cassa "cassa" */
    int trovata=0;
    int j; //cassa aperta dove spostare i clienti
    while(trovata==0) {
        for (int h=0;(h<K) && (trovata==0);h++) {
            if (h!=cassa) {
                pthread_mutex_lock(&mutex[h]); 
                if (statocassa[h]==0) { //cassa chiusa
                    pthread_mutex_unlock(&mutex[h]);
                }
                else {
                    trovata=1;
                    j=h;
                }
            }
        } 
        trovata=1; 
    }

        //copio i clienti dalla cassa "cassa" alla cassa "j"
        item* tmp=coda[cassa];
        item* elimina=tmp;
        while(tmp!=NULL) {    
            //funzione che aggiunge 1 alla variabile changecoda del cliente in modo da aggiornare tale
            //variabile per ricordarsi quanti spostamenti di cassa effettua. 
            copiaincodadiversa(coda,K, j+1, tmp, statocassa, codaclienti);
            tmp=tmp->next;
            free(elimina);
            elimina=tmp;
        }
           
        coda[cassa]=NULL;
        pthread_mutex_unlock(&mutex[j]); 

        return 1;

    fprintf(stderr,"errore, non dovrei essere arrivato qui \n");

    return 0;
}




void dealloca(item** coda, int K, int* statocassa, int* codaclienti) {
    //condizioni
    if (coda==NULL) {
        fprintf(stderr,"coda non allocata. Prova con: coda=create(K,coda) \n");
        exit(EXIT_FAILURE);
    }
    if (K<1) {
        fprintf(stderr,"K=%d non concesso \n",K);
        exit(EXIT_FAILURE);
    }

    //deallocazione
    for (int i=0;i<K;i++) {
        item* punt=coda[i];
        item* succ=NULL;
        while (punt!=NULL)
        {
            succ=punt->next;
            free(punt);
            punt=succ;;
        }
    }

    free(codaclienti);
    free(statocassa);
    free(coda);
    return;
}




int incoda(item** coda, int K, int cassa, int* statocassa) {
        //condizioni
    if (coda==NULL) {
        printf("coda non allocata. Prova con: coda=create(K,coda) \n");
        exit(EXIT_FAILURE);
    }
    if (K<1) {
        printf("K=%d non concesso \n",K);
        exit(EXIT_FAILURE);
    }
    if (cassa<1 || cassa>K) {
        printf("cassa=%d non concesso \n", cassa);
        exit(EXIT_FAILURE);
    }
    
    if (statocassa[cassa-1]==0) {
        //printf("cassa %d chiusa. impossibile controllare (incoda) \n",cassa);
        return -1;
    }

    int cont=codaclienti[cassa-1];

    return cont;
}




int infocassa(int K, int* statocassa, int cassa) {
    if (K<1) {
        printf("K=%d non concesso \n",K);
        exit(EXIT_FAILURE);
    }
    if (statocassa==NULL) {
        printf("statocassa non inizializzato \n");
        exit(EXIT_FAILURE);
    }     

    if (statocassa[cassa]==0) { //cassa chiusa 
        return -1;  
    }
           
    return 1; //cassa aperta
}




void stampa(item** coda, int K, int* statocassa) {
    //condizioni
    if (coda==NULL){
        perror("coda non allocata \n");
        exit(EXIT_FAILURE);
    }
    if (K<1) {
        printf("K=%d non concesso \n", K);
        exit(EXIT_FAILURE);
    }

    item* pun=NULL;
    for(int i=0;i<K;i++) {
        pun=coda[i];
        printf("cassa %d con %d clienti in coda: ", i+1, codaclienti[i]);
        //cassa aperta
        if (statocassa[i]==1) {
            while(pun!=NULL) {
                printf("[%ld %d %d %f %f %f] ",pun->value, pun->prodotti, pun->changecoda, pun->timeiniziocoda, pun->timefinecoda, pun->tempoacquisti);
                pun=pun->next;
            }
        }
        //cassa chiusa
        else {
            printf("chiusa");
        }
        printf("\n");
    }

    return;
}




void initlista(int K) {
    _K=K;
    lista=NULL;

}




void inseriscicassa(int indexcassa) {

    //controlli
    if ( (indexcassa<0) || (indexcassa >(_K-1)) ) {
        fprintf(stderr, "valore di cassa errato: %d \n",indexcassa);
        exit(EXIT_FAILURE);
    }

    pthread_mutex_lock(&locklista);
    
    elemcassa* elem=malloc(sizeof(elemcassa));
    elem->indexcassa=indexcassa;
    elem->next=NULL;


    //lista vuota
    if (lista==NULL) {
        lista=elem;

        pthread_mutex_unlock(&locklista);
        return;
    }


    //inserimento in coda
    elemcassa* p=lista;

    while (p->next != NULL) {
        p=p->next;
    }


    p->next=elem;
    pthread_mutex_unlock(&locklista);
    return;
}




void rimuovicassa(int indexcassa) {
    //controlli
    if ( (indexcassa<0) || (indexcassa >(_K-1)) ) {
        fprintf(stderr, "valore di cassa errato: %d \n",indexcassa);
        exit(EXIT_FAILURE);
    }


    pthread_mutex_lock(&locklista);

    elemcassa* prec=NULL;
    elemcassa* succ=lista;

    //trovato indexcassa nel primo elemento
    if (succ->indexcassa==indexcassa) {
        lista=succ->next;
        free(succ);

        pthread_mutex_unlock(&locklista);
        return;
    }

    prec=succ;
    succ=succ->next;

    while(succ!=NULL) {
        if (succ->indexcassa==indexcassa) {
            prec->next=succ->next;
            free(succ);

            pthread_mutex_unlock(&locklista);
            return;
        }

        prec=succ;
        succ=succ->next;

    }

    pthread_mutex_unlock(&locklista);
    return;
}




int sizelista() {
    int cont=0;
    elemcassa* tmp=lista;


    while (tmp!=NULL) {
        cont++;
        tmp=tmp->next;
    }

    return cont;  
}




int generacassa() {
    //condizioni
    if (lista==NULL) {
        fprintf(stderr, "lista non inizializzata \n");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_lock(&locklista);

    int dimlista= sizelista(lista); //numero di elementi della lista
    //genera valore casuale tra [0,dimlista-1]
    unsigned int seed=(unsigned int)current_time_millisecond()+(unsigned int) pthread_self();
    int tmp;
    tmp=rand_r(&seed);
    tmp=tmp%dimlista;
    
    //cerco elemento che corrisponde a quell'indice nella lista
    elemcassa* pun=lista;
    int cont=0;
    while (tmp!=cont) {
        pun=pun->next;
        cont++;
    }

    pthread_mutex_unlock(&locklista);
    return pun->indexcassa;
}




void stampalista() {
    elemcassa* tmp=lista;

    while (tmp!=NULL) {
        printf("[%d] ",tmp->indexcassa);
        tmp=tmp->next;
    }
    printf("\n");
    printf("elementi: %d \n",sizelista(lista));
}




void deletelista() {
    elemcassa* tmp=NULL;
    elemcassa* succ=lista;

    while (succ!=NULL) {
        tmp=succ;
        succ=succ->next;
        free(tmp);
    }

    lista=NULL;

    return;
}
