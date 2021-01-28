#define _POSIX_C_SOURCE 200112L
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
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include "./lib/parsing.h"
#include "./lib/dati.h"
#include "./lib/random.h"
#define DEBUGC 0 //DEBUG cliente
#define DEBUGCA 0 //DEBUG cassiere
#define DEBUGD 0 //DEBUG direttore
#define DEBUGM 0 //DEBUG main


/* VARIABILI GLOBALI */

//variabili estratte da config.txt
int K, C, E, T, P, S1, S2, KLIM, TP, TIM; char filelog[15];


//file dove inserire le statistiche del supermercato
FILE* file;
pthread_mutex_t filemtx=PTHREAD_MUTEX_INITIALIZER; //mutua esclusione per la scrittura su file


//numero dei clienti attivi nel supermercato
int nclienti;
pthread_mutex_t lockclienti=PTHREAD_MUTEX_INITIALIZER; //mutua esclusione per la variabile "nclienti" 


//direttore (gestione uscita senza acquisti)
int permesso; //FLAG: 0->il direttore non ha dato il via libera di uscire ai clienti con 0 prodotti. 1->altrimenti
pthread_mutex_t lockuscita=PTHREAD_MUTEX_INITIALIZER; //mutua esclusione per la variabile "permesso" 
pthread_cond_t condcliente=PTHREAD_COND_INITIALIZER; //v.condizione dove si ferma il cliente con 0 acquisti in attesa del permesso da parte del direttore di poter uscire


//cassiere-direttore (gestione comunicazione cassiere-direttore sul numero dei clienti in coda alle casse)
pthread_mutex_t locknotify=PTHREAD_MUTEX_INITIALIZER; //mutua esclusione per l'array "notifica"
pthread_cond_t attendinotify=PTHREAD_COND_INITIALIZER; //v.condizione dove si ferma il cliente in attesa della notifica da parte del direttore
int* notifica; //array di bit (0/1): quando un cassiere i invia il dato al direttore viene settato notifica[i]=1 
int* cassenotify; //array di appoggio che contiene lo stato in cui si trova ogni cassa (aperta,chiusa)
int* clientinotify; //array di appoggio che contiene il numero dei clienti presenti in coda ad ogni cassa


//FLAG per i SEGNALI
volatile sig_atomic_t segnalesq; //SIGQUIT
volatile sig_atomic_t segnalesh; //SIGHUP



/* nclienti */

//incrementa variabile "nclienti"
void  inc(int* nclienti) {
    pthread_mutex_lock(&lockclienti);
    *nclienti=*nclienti+1;
    pthread_mutex_unlock(&lockclienti);
    return;
}

//decrementa variabile "nclienti"
void  dec(int* nclienti) {
    pthread_mutex_lock(&lockclienti);
    *nclienti=*nclienti-1;
    pthread_mutex_unlock(&lockclienti);
    return;
}

//restituisce valore variabile "nclienti"
int get(int* nclienti) {
    pthread_mutex_lock(&lockclienti);
    int clienti=*nclienti;
    pthread_mutex_unlock(&lockclienti);

    return clienti;
}



/* notifica */

// return 1 if tutti i bit di "notifica" sono a 1 else return 0
int getnotifica(int* notifica) {
    int flag=1;

    for (int i=0;i<K;i++) {
        if (notifica[i]==0)
            flag=0;
    }

    return flag;
}

//imposta array a 0
void setnotifica(int* notifica) {
    for (int i=0;i<K;i++)
        notifica[i]=0;

    return;
}



/*gestione SEGNALI */

static void gestoreSIGQUIT (int signum) {
    printf("gestione SIGQUIT \n");
    segnalesq=1;

    //risveglia eventuali casse chiuse 
    for (int i=0;i<K;i++) {
        pthread_mutex_lock(&mutex[i]);
        pthread_cond_signal(&cond[i]);     
        pthread_mutex_unlock(&mutex[i]);
    }

    return;
}

static void gestoreSIGHUP (int signum) {
    printf("gestione SIGHUP \n");
    segnalesh=1;

    //risveglia eventuali casse chiuse 
    for (int i=0;i<K;i++) {
        pthread_mutex_lock(&mutex[i]);
        pthread_cond_signal(&cond[i]);     
        pthread_mutex_unlock(&mutex[i]);
    }

    return;
}



/* thread di gestione notifica del cassiere */

static void* notifythread (void* arg) {
    int* cassa=(void*) arg; //numero cassa

    while(segnalesh==0 && segnalesq==0) {
        ms_sleep(TIM);
        pthread_mutex_lock(&locknotify);
        pthread_mutex_lock(&mutex[(*cassa)-1]);
        cassenotify[(*cassa)-1]=statocassa[(*cassa)-1];
        clientinotify[(*cassa)-1]=codaclienti[(*cassa)-1];
        pthread_mutex_unlock(&mutex[(*cassa)-1]);
        notifica[(*cassa)-1]=1;
        pthread_cond_signal(&attendinotify);
        pthread_mutex_unlock(&locknotify); 
    } 

    pthread_exit((void*)0);
} 





static void* cliente (void* arg) {
    long id_cliente=(long) arg;
    inc(&nclienti); //incremento variabile che conta il numero di clienti attivi

    #if DEBUGC
    printf("*cliente %ld entra nel supermercato \n", id_cliente);
    #endif

    int timeacquisti=randomtimeacquisticliente(T); //genera tempo per acquisti (millisecondi)

    #if DEBUGC
    printf("cliente %ld sta facendo acquisti \n", id_cliente);
    #endif

    ms_sleep(timeacquisti); //il cliente sta facendo acquisti
    double timeacquistisecondi=(double) timeacquisti*(0.001); //ms to second
    int prodotti=randomprodotti(P); 

    //if cliente senza prodotti: attendi il permesso di uscire dal direttore
    if (prodotti==0) {
        pthread_mutex_lock(&lockuscita);
        while (permesso==0) {
            pthread_cond_wait(&condcliente,&lockuscita);
        }
        //se ho ricevuto segnale SIGHUP: il direttore da il via libera di uscire a tutti i clienti 
        if (segnalesh==0)
            permesso=0;

        //cliente senza acquisti (nprodotti=0)
        pthread_mutex_lock(&filemtx);
        fprintf(file,"id_cliente:%ld prodottiacquistati:%d tempototalenelsupermercato:%.2f tempototalespesoincoda:%d n.codevisitate:%d tempocolcassiere(temposerviziodiogniclienteservito):%d \n",id_cliente,0,timeacquistisecondi,0,0,0);
        pthread_mutex_unlock(&filemtx);
        pthread_mutex_unlock(&lockuscita);

        #if DEBUGC
        printf("cliente %ld esce dal supermercato senza acquisti \n", id_cliente);
        #endif
        
        dec(&nclienti); //decremento variabile che conta il numero di clienti attivi

        pthread_exit((void*)0);
    }
    
    //genera cassa aperta dove il cliente può pagare ("cassa" compresa tra [0,K-1]) 
    int trovato=0; int cassa=0;
    while (trovato==0) {
        cassa=generacassa(lista);
        pthread_mutex_lock(&mutex[cassa]);
        if (statocassa[cassa]==0) { //cassa chiusa
            pthread_mutex_unlock(&mutex[cassa]);
        }
        else { //cassa aperta
            trovato=1;
        }
    }

    //gestione segnale SIGQUIT (il cliente esce senza prodotti senza mettersi in coda)
    if (segnalesq==1) {
        //il cliente esce senza acquisti (dovuto al segnale SIGQUIT)
        pthread_mutex_lock(&filemtx);
        fprintf(file,"id_cliente:%ld prodottiacquistati:%d tempototalenelsupermercato:%2.f tempoattesacoda:%d n.codevisitate:%d tempocolcassiere(temposerviziodiogniclienteservito):%d \n",id_cliente,0,timeacquistisecondi,0,0,0);
        pthread_mutex_unlock(&filemtx);
        pthread_cond_signal(&cond[cassa]);
        pthread_mutex_unlock(&mutex[cassa]);
        dec(&nclienti); //decremento variabile che conta il numero di clienti attivi
        pthread_exit((void*)0);
    }

    double arrivoincoda=current_time_millisecond();
    inserisci(coda, K, (1+cassa),id_cliente, prodotti, timeacquisti, statocassa, codaclienti, arrivoincoda); 

    #if DEBUGC
    printf("cliente %ld si mette in coda alla cassa %d \n",id_cliente,cassa);
    #endif

    pthread_cond_signal(&cond[cassa]);
    pthread_mutex_unlock(&mutex[cassa]);

    #if DEBUGC
    printf("cliente %ld esce dal supermercato \n", id_cliente);
    #endif

    pthread_exit((void*)0);
}



static void* cassiere (void* arg) {
    long* ncassatmp=(long*) arg;
    int* ncassa=(int*) ncassatmp; //numero cassa 

    #if DEBUGCA 
    printf("inizializza cassa %d \n", *ncassa);
    #endif

    //avvio thread di "supporto" per gestire le notifiche al direttore sui clienti in coda alle casse
    pthread_t tidnotify; pthread_attr_t atth;
    
    if (pthread_attr_init(&atth) != 0){
        fprintf(stderr, "errore pthread_attr_init\n");
        exit(EXIT_FAILURE);
    }
    
    if (pthread_attr_setdetachstate(&atth, PTHREAD_CREATE_DETACHED) != 0){
        fprintf(stderr,"errore pthread_attr_setdetachstate\n");
        pthread_attr_destroy(&atth);
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&tidnotify,&atth,notifythread, arg) != 0){
            fprintf(stderr,"errore pthread_create\n");
            exit(EXIT_FAILURE);
    }


    while(segnalesq==0) {
        pthread_mutex_lock(&mutex[(*ncassa)-1]);
        int tempofisso=generatempofissocassiere(*ncassa); 
        int tprodotto=TP; //tempo gestione del singolo prodotto

        //attendo finchè: coda vuota(0) o cassa chiusa (-1) o arriva segnale SIGQUIT/SIGHUP
        //SIGQUIT: tale segnale chiude la cassa a prescindere dal fatto che ci siano clienti in coda
        //SIGHUP: tale segnale non chiude la cassa immediatamente ma serve ogni cliente in coda e poi la chiude
        while( (segnalesq==0) && (segnalesh==0) && (incoda(coda, K, (*ncassa), statocassa) <= 0) ) {
            pthread_cond_wait(&cond[(*ncassa)-1], &mutex[(*ncassa)-1]);
        }
        
        //segnale SIGQUIT
        if (segnalesq==1) {
            //aggiorna periodo apertura cassa (il periodo viene aggiornato se la cassa è aperta)
            if (statocassa[(*ncassa)-1]==1) {  
                tempocassa[(*ncassa)-1].fine=current_time_millisecond();
                double fine=tempocassa[(*ncassa)-1].fine;
                double inizio=tempocassa[(*ncassa)-1].inizio;
                double tmp=fine-inizio;
                tempocassa[(*ncassa)-1].periodo=tempocassa[(*ncassa)-1].periodo+tmp;
            }
            statocassa[(*ncassa)-1]=0; //chiudo cassa
            pthread_mutex_unlock(&mutex[(*ncassa)-1]);

            #if DEBUGCA
            printf("terminazione thread cassa %d \n", *ncassa);
            #endif

            pthread_exit((void*)0);
        }

        //segnale SIGHUP nel caso in cui la cassa sia chiusa 
        if ( (segnalesh==1) && (statocassa[(*ncassa)-1]==0)) {
            pthread_mutex_unlock(&mutex[(*ncassa)-1]);

            #if DEBUGCA
            printf("terminazione thread cassa %d \n", *ncassa);
            #endif

            pthread_exit((void*)0);
        }

        //il cassiere serve il cliente (lo rimuove dalla coda)
        item cliente=rimuovi(coda, K, *ncassa, statocassa, codaclienti);

        //se la cassa è vuota ed è stato mandato un segnale SIGHUP chiudi cassa
        //( quando la cassa è vuota viene restituito un oggetto cliente con value=-1 (settaggi di default) )
        if ( (cliente.value==-1) && (segnalesh==1) ) {
           //aggiorna periodo apertura cassa (il periodo viene aggiornato se la cassa è aperta)
           if (statocassa[(*ncassa)-1]==1) { 
                tempocassa[(*ncassa)-1].fine=current_time_millisecond();
                double fine=tempocassa[(*ncassa)-1].fine;
                double inizio=tempocassa[(*ncassa)-1].inizio;
                double tmp=fine-inizio;
                tempocassa[(*ncassa)-1].periodo=tempocassa[(*ncassa)-1].periodo+tmp;
            }
            statocassa[(*ncassa)-1]=0; //chiudo cassa
            pthread_mutex_unlock(&mutex[(*ncassa)-1]);

            #if DEBUGCA
            printf("terminazione thread cassa %d \n", *ncassa);
            #endif

            pthread_exit((void*)0);
        }

        prodottielaborati[(*ncassa)-1]=prodottielaborati[(*ncassa)-1]+cliente.prodotti;
        double prodotticliente=cliente.prodotti; //numero prodotti acquistati dal cliente
        double tprocess=tempofisso+prodotticliente*tprodotto; //tempo processamento cliente col cassiere
        /* stampa info cliente su FILE */
        double tiniziocoda=cliente.timeiniziocoda;
        double tfinecoda=cliente.timefinecoda;
        double tpermanenzasupermercato=cliente.tempoacquisti+tprocess+(tfinecoda-tiniziocoda); //millisecondi
        double tattesacoda=tfinecoda-tiniziocoda;
        pthread_mutex_lock(&filemtx);
        fprintf(file,"id_cliente:%ld prodottiacquistati:%d tempototalenelsupermercato:%.3f tempototalespesoincoda:%.3f n.codevisitate:%d tempocolcassiere(temposerviziodiogniclienteservito):%.3f \n",cliente.value,cliente.prodotti,tpermanenzasupermercato*(0.001),tattesacoda*(0.001),cliente.changecoda,tprocess*(0.001));
        pthread_mutex_unlock(&filemtx);
        nclientiserviti[(*ncassa)-1]++; 

        #if DEBUGCA
        printf("#cassiere %d serve cliente %ld con %d prodotti \n",*ncassa,cliente.value,cliente.prodotti);
        #endif

        pthread_mutex_unlock(&mutex[(*ncassa)-1]);
        ms_sleep(tprocess); //il cassiere processa il cliente
        dec(&nclienti); //decremento variabile che conta il numero di clienti attivi
    }

    /*FUORI DAL WHILE: significa che ho ricevuto un segnale SIGQUIT*/
    //aggiorna periodo apertura cassa (il periodo viene aggiornato se la cassa è aperta)
     if (statocassa[(*ncassa)-1]==1) { 
        tempocassa[(*ncassa)-1].fine=current_time_millisecond();
        double fine=tempocassa[(*ncassa)-1].fine;
        double inizio=tempocassa[(*ncassa)-1].inizio;
        double tmp=fine-inizio;
        tempocassa[(*ncassa)-1].periodo=tempocassa[(*ncassa)-1].periodo+tmp;
    }
    statocassa[(*ncassa)-1]=0; //chiudo cassa

    #if DEBUGCA
    printf("terminazione thread cassa %d \n", *ncassa);
    #endif

    pthread_exit((void*)0);
}



static void* direttore (void* arg) {

    #if DEBUGD
    printf("inizializza direttore \n");
    #endif

    int cont=0; //numero di casse con al più 1 cliente in coda.
    int cont2=0; //numero di casse con S2 clienti in coda
    int ris=0; //variabile di appoggio
    int cassa; //variabile di appoggio
    int temp=0; //variabile di appoggio
    int* numeroclienti=malloc(sizeof(int)*K); //array di appoggio per il numero di clienti ad ogni cassa
    int* statecassa=malloc(sizeof(int)*K); //array di appoggio per lo stato di ogni cassa
    for (int i=0;i<K;i++) {
        numeroclienti[i]=0;
        statecassa[i]=0;
    }

    //finchè non ricevo un segnale SIGQUIT o SIGHUP il thread direttore rimane attivo
    while (segnalesq==0 && segnalesh==0) {
        //reset variabili
        cont=0; cont2=0; ris=0; temp=0;

        //dai il permesso di uscire ai clienti con 0 prodotti 
        pthread_mutex_lock(&lockuscita);
        permesso=1;
        pthread_cond_broadcast(&condcliente);
        pthread_mutex_unlock(&lockuscita);

        //gestione notifica cassieri
        pthread_mutex_lock(&locknotify);
        while(getnotifica(notifica)==0) {
            pthread_cond_wait(&attendinotify,&locknotify);
        }
        //leggi array
        for (int i=0;i<K;i++) {
            numeroclienti[i]=clientinotify[i];
            statecassa[i]=cassenotify[i];
        }
        
        setnotifica(notifica);
        pthread_mutex_unlock(&locknotify);

        #if DEBUGD
        printf("numeroclienti: ");
        for (int i=0;i<K;i++) {
            printf("[%d] ",numeroclienti[i]);
        }
        printf("\nstatecassa: ");
        for (int i=0;i<K;i++) {
            printf("[%d] ",statecassa[i]);
        }
        printf("\n");
        stampalista(lista);
        printf("\n");
        #endif

        //CASO 1 CHIUSURA CASSE: se ci sono almeno S1 casse con al più 1 cliente in coda
        int trovata=0; int cassachiusa=0;
        for (int i=0;i<K;i++) {
            if (statecassa[i]==1) { //cassa aperta
                if (numeroclienti[i]==0 || numeroclienti[i]==1) { //cassa con al più un cliente in coda
                    cont++;
                }
            }
        }
        if (cont>=S1) {
            //cerca cassa aperta da chiudere
            while (trovata==0) {
                cassa=generacassa(lista);
                pthread_mutex_lock(&mutex[cassa]);
                if (statocassa[cassa]==0) { //cassa chiusa
                    pthread_mutex_unlock(&mutex[cassa]);
                }
                else { //cassa aperta
                    cassachiusa=closecassa(coda,cassa,K,statocassa,tempocassa,nchiusure, lista);
                    pthread_mutex_unlock(&mutex[cassa]);
                    trovata=1;
                }
            }

            if (cassachiusa == 1) {
                temp=1;
                #if DEBUGD
                printf("il direttore ha chiuso la cassa %d \n",ris+1);
                #endif
            }
        }

            //CASO 2 APERTURA CASSA: se c'è almeno una cassa con S2 clienti in coda
            if (temp==0) { //l'apertura di una cassa la posso fare solo se immediatamente prima non ho chiuso casse
                trovata=0; cassachiusa=0;
                for (int i=0;i<K;i++) {
                    if (statecassa[i]==1) { //cassa aperta
                        if (numeroclienti[i]>=S2) { //cassa con almeno S2 clienti in coda
                            cont2++;
                        }
                    }
                }
                if (cont2>=1) {
                    //cerca cassa chiusa da aprire
                    for (int i=0;(i<K) && (trovata==0);i++) {
                        pthread_mutex_lock(&mutex[i]);
                        if (statocassa[i]==1) { //cassa aperta
                            pthread_mutex_unlock(&mutex[i]);
                        }
                        else { //cassa chiusa
                            cassachiusa=opencassa(coda,i,K,statocassa, tempocassa, lista);
                            ris=i;
                            pthread_mutex_unlock(&mutex[i]);
                            trovata=1;     
                        }
                    }

                    if (cassachiusa == 1) {
                        #if DEBUGD
                        printf("il direttore ha aperto la cassa %d \n",ris+1);
                        #endif
                    }
                }
            } //chiudo if CASO 2

    }

    /* FUORI DAL WHILE: significa che ho ricevuto un segnale di SIGQUIT o SIGHUP  */
    //segnale SIGHUP: il direttore da il via libera a tutti i clienti con 0 prodotti acquistati di uscire 
    if (segnalesh==1) {
        pthread_mutex_lock(&lockuscita);
        permesso=1;
        pthread_cond_broadcast(&condcliente);
        pthread_mutex_unlock(&lockuscita);    
    }


    free(numeroclienti);
    free(statecassa);

    #if DEBUGD
    printf("terminazione thread direttore \n");
    #endif

    pthread_exit((void*)0);
}



int main() 
{

    /* VARIABILI PER SALVARE IL TEMPO TOTALE DI APERTURA DEL SUPERMERCATO */
    double tempoaperturasupermercato,tempochiusurasupermercato;


    /* CONFIGURAZIONE VARIABILI - lettura file di configurazione iniziale */
    parsingfile(&K,&C,&E,&T,&P,&S1,&S2,&KLIM,filelog,&TP,&TIM);
    #if DEBUGM
    printf("K:%d C:%d E:%d T:%d P:%d S1:%d S2:%d KLIM:%d filelog:%s TP:%d TIM:%d \n", K, C, E, T, P, S1, S2, KLIM, filelog, TP, TIM);
    #endif
    nclienti=0; //set nclienti attivi
    permesso=0; //set flag di via libera per i clienti con 0 prodotti
    long client_id=0; //set id del cliente


    /* OPEN FILELOG */
    file=fopen(filelog,"w");
    if (filelog==NULL) {
        fprintf(stderr,"errore apertura filelog \n");
        exit(EXIT_FAILURE);
    }
    //dati iniziali di config.txt (scrivo nel file config.txt)
    fprintf(file,"K:%d C:%d E:%d T:%d P:%d S1:%d S2:%d KLIM:%d filelog:%s TP:%d TIM:%d \n", K, C, E, T, P, S1, S2, KLIM, filelog, TP, TIM);


    /* INSTALLA SEGNALE SIGQUIT */
    struct sigaction sq;
    memset(&sq,0,sizeof(sq));
    sq.sa_handler=gestoreSIGQUIT;
    sigaction(SIGQUIT,&sq,NULL);
    segnalesq=0;


    /* INSTALLA SEGNALE SIGHUP */
    struct sigaction sh;
    memset(&sh,0,sizeof(sh));
    sh.sa_handler=gestoreSIGHUP;
    sigaction(SIGHUP,&sh,NULL);
    segnalesh=0;


    /* INIZIALIZZA MUTEX E VARIABILI DI CONDIZIONE */
    mutex=malloc(sizeof(pthread_mutex_t)*K);
    cond=malloc(sizeof(pthread_cond_t)*K);
    for (int i=0;i<K;i++) {
        if (pthread_mutex_init(&mutex[i], NULL) != 0) {
            fprintf(stderr,"errore mutex %d \n",i);
            exit(EXIT_FAILURE);
         }
    }
    for (int i=0;i<K;i++) {
        if (pthread_cond_init(&cond[i], NULL) != 0) {
            fprintf(stderr,"errore var.condizione %d \n",i);
            exit(EXIT_FAILURE);
         }
    }


    /* INIZIALIZZA STRUTTURE DATI e CASSE */
    coda=NULL;
    codaclienti=malloc(sizeof(int)*K); //array che contiene il numero di clienti ad ogni cassa, ogni posizione corrisponde a una cassa.
    cassenotify=malloc(sizeof(int)*K);
    clientinotify=malloc(sizeof(int)*K);
    statocassa=malloc(sizeof(int)*K); //array che indica se la cassa è aperta o chiusa. 0->chiusa, 1->aperta
    tempocassa=malloc(sizeof(timecassa)*K); //conta periodo apertura di ogni cassa
    nclientiserviti=malloc(sizeof(int)*K); //numero di clienti serviti ad ogni cassa
    notifica=malloc(sizeof(int)*K); //flag di notifica da parte dei cassieri
    prodottielaborati=malloc(sizeof(int)*K);
    nchiusure=malloc(sizeof(int)*K); //numero di chiusure di ogni cassa (quante volte ogni cassa è stata chiusa)
    for (int i=0;i<K;i++) {
        nclientiserviti[i]=0;
        nchiusure[i]=0;
        notifica[i]=0;
        prodottielaborati[i]=0;
    }
    coda=crea(K,coda,statocassa,KLIM,codaclienti,tempocassa, lista); //creo K di cui KLIM aperte


    /* APERTURA SUPERMERCATO */
    tempoaperturasupermercato=current_time_millisecond();


    /* ATTIVA CASSE */
    pthread_t* tidcassa=malloc(sizeof(pthread_t)*K);
    int err, status;
    long* id_cassa=malloc(sizeof(long)*K); //inizializza id_cassa
    for (int i=0;i<K;i++)
        id_cassa[i]=i+1;

     for (int i=0;i<K;i++) {
        if ( (err=pthread_create(&tidcassa[i], NULL, cassiere, &id_cassa[i])) !=0 ) {
            fprintf(stderr,"errore nella creazione del thread cassiere \n");
            return EXIT_FAILURE;
        }   
    }   


    /* ATTIVA DIRETTORE */
    pthread_t tidirettore;

   if ( (err=pthread_create(&tidirettore, NULL, direttore, NULL)) !=0 ) {
        fprintf(stderr,"errore nella creazione del thread direttore \n");
        return EXIT_FAILURE;
    }   


    /* ATTIVA CLIENTI */
    pthread_t tidcliente; pthread_attr_t atth;
    
    if (pthread_attr_init(&atth) != 0){
        fprintf(stderr, "errore pthread_attr_init\n");
        exit(EXIT_FAILURE);
    }
    
    if (pthread_attr_setdetachstate(&atth, PTHREAD_CREATE_DETACHED) != 0){
        fprintf(stderr,"errore pthread_attr_setdetachstate\n");
        pthread_attr_destroy(&atth);
        exit(EXIT_FAILURE);
    }

    //attivo C clienti
    for (int i=0;i<C;i++) {
        client_id++;
        if (pthread_create(&tidcliente,&atth,cliente, (void*)client_id) != 0){
            fprintf(stderr,"errore pthread_create\n");
            exit(EXIT_FAILURE);
        }
    }
    
    while (segnalesq==0 && segnalesh==0)  {
        //attendo che il numero dei clienti sia effettivamente minore di C-E
        while( (segnalesq==0) && get(&nclienti)>(C-E) );

        //creo E clienti
        if (segnalesq==0) {
            for (int i=0;(segnalesq==0) && (segnalesh==0) && (i<E);i++) {
                client_id++;
                if (pthread_create(&tidcliente,&atth,cliente, (void*)client_id) != 0){
                    fprintf(stderr,"errore pthread_create\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }


    /* ATTENDI TERMINAZIONE CASSE */
    for (int i=0;i<K;i++) {
        pthread_join(tidcassa[i],(void*) &status); 

        if (status !=0 ) {
            fprintf(stderr,"errore nella join del thread cassiere \n");
            exit(EXIT_FAILURE);
        }
    }


    /* ATTENDI TERMINAZIONE DIRETTORE */
    pthread_join(tidirettore,(void* )&status);
    
    if (status!=0) {
        fprintf(stderr,"errore nella join del thread direttore \n");
        exit(EXIT_FAILURE);
    }


    /* FACCIO USCIRE I CLIENTI RIMASTI BLOCCATE ALLE CASSE A CAUSA DEL SEGNALE SIGQUIT*/
    if (segnalesq==1) {
        fprintf(file,"LISTA CLIENTI RIMASTI BLOCCATI IN CODA \n");

        item* pun=NULL;
        for(int i=0;i<K;i++) {
            pun=coda[i];
            if (codaclienti[i]==0) {
                fprintf(file,"cassa %d ha %d clienti in coda: cassa chiusa \n", i+1, codaclienti[i]);
            }
            else {
            fprintf(file,"cassa %d ha %d clienti in coda: \n", i+1, codaclienti[i]);
            }
            //cassa aperta
                while(pun!=NULL) {
                    double iniziocoda=pun->timeiniziocoda;
                    double finecoda=current_time_millisecond();
                    double tempopermanenzasupermercato=pun->tempoacquisti+(finecoda-iniziocoda);
                    double tempoattesacoda=finecoda-iniziocoda;
                    fprintf(file,"[id_cliente:%ld prodottiacquistati:%d tempototalenelsupermercato:%.3f tempototalespesoincoda:%.3f n.codevisitate:%d tempocolcassiere(temposerviziodiogniclienteservito):%.3f \n",pun->value,0,tempopermanenzasupermercato*(0.001),tempoattesacoda*(0.001),pun->changecoda,0.0);
                    pun=pun->next;
                }
        }
    }
    


    /* CHIUSURA SUPERMERCATO */
    tempochiusurasupermercato=current_time_millisecond();


    /* GESTISCO STATISTICHE SUPERMERCATO (scrivo nel filelog) */

    for (int i=0;i<K;i++) {
        fprintf(file,"cassa %d: ",i+1);
         //numero di prodotti elaborati da ogni cassiere
        fprintf(file,"il numero di prodotti elaborati è:%d ",prodottielaborati[i]);
        //numero dei clienti serviti ad ogni cassa
        fprintf(file,"numero clienti serviti:%d ",nclientiserviti[i]);
        //tempo in millisecondi (e secondi) di ogni periodo di apertura della cassa;
        fprintf(file,"periodo apertura totale cassa (s):%.3f ", tempocassa[i].periodo*(0.001));
        //tempo medio di servizio (quanto tempo impiega in media il cassiere a servire il cliente)
        if (tempocassa[i].periodo==0 || prodottielaborati[i]==0) //cassa mai aperta
            fprintf(file,"tempo medio di servizio della cassa:%.3f ", 0.000); 
        else {
            double tmedioservizio=tempocassa[i].periodo/prodottielaborati[i]; //ms
            tmedioservizio=tmedioservizio*(0.001); //s
            fprintf(file,"tempo medio di servizio della cassa:%.3f ", tmedioservizio);
        }
        //il numero di chiusure di ogni cassa
        fprintf(file,"numero di volte in cui la cassa è stata chiusa:%d \n",nchiusure[i]);
    }



    /* CLOSE FILELOG */
    fclose(file);


    /* DEALLOCAZIONI GENERALI, MUTEX E VAR.COND. */
    dealloca(coda,K,statocassa, codaclienti);

    for (int i=0;i<K;i++) {
        pthread_mutex_destroy(&mutex[i]);
        pthread_cond_destroy(&cond[i]);
    }

    pthread_mutex_destroy(&lockclienti);
    pthread_cond_destroy(&condcliente);
    pthread_mutex_destroy(&locknotify);
    pthread_attr_destroy(&atth);
    pthread_mutex_destroy(&lockuscita);
    pthread_cond_destroy(&attendinotify);

    free(tempocassa);
    free(prodottielaborati);
    free(nclientiserviti);
    free(nchiusure);
    free(mutex);
    free(cond);
    free(id_cassa);
    free(tidcassa);
    free(notifica);
    free(cassenotify);
    free(clientinotify);
    deletelista();

    #if DEBUGM
    printf("programma terminato con successo \n");
    #endif

    return 0;
}