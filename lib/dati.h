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


/* STRUTTURA PERIODO APERTURA CASSA: utilizzata per contare il periodo di apertura di ogni cassa */
typedef struct _periodoaperturacassa {
    double inizio; //momento in cui la cassa è stata aperta
    double fine;   //momento in cui la cassa è stata chiusa
    double periodo; //periodo di apertura cassa
} timecassa;


/* STRUTTURA CLIENTE: utilizzata dalla struttura dati "coda" */
typedef struct _node {
    struct _node* next;
    long value; //id_cliente
    int prodotti; //numero prodotti acquistati
    int changecoda; //se il cliente ha cambiato coda vale >0 (numero di volte in cui l'ha cambiata) altrimenti 0
    double timeiniziocoda; //secondo in cui il cliente entra in coda 
    double timefinecoda; //secondo in cui il cliente esce dalla coda
    double tempoacquisti; //tempo in cui il cliente passa a fare acquisti
} item;


/* STRUTTURA CASSA: utilizzata dalla struttura dati "lista" */
typedef struct node {
    struct node* next;
    int indexcassa; //indice di cassa [0,K-1]
} elemcassa;


/* VARIABILI STRUTTURA DATI "coda" */
item** coda;
int* codaclienti; //array di K posizioni in cui salvo il numero di clienti in coda. ogni posizione dell'array corrisponde a una cassa
int* statocassa; //array che indica se la cassa è aperta o chiusa. 0->chiusa 1->aperta
int* prodottielaborati; //totale prodotti elaborati da ogni cassa
int* nchiusure; //quante volte ogni cassa è stata chiusa
int* nclientiserviti; //numero di clienti serviti in totale da ogni cassa
timecassa* tempocassa; //tempo di ogni periodo di apertura cassa (somma di tutti i periodi di ogni cassa)

//cassiere
pthread_mutex_t* mutex; //mutex[i]: variabile di mutua esclusione della cassa i
pthread_cond_t* cond; //cond[i]: v.condizione dove si ferma il cassiere quando non ci sono clienti in coda

/* VARIABILI STRUTTURA DATI "lista" */
elemcassa* lista;
static pthread_mutex_t locklista=PTHREAD_MUTEX_INITIALIZER; 
int _K;


/* interfaccia "coda" */

/* 
//RICORDA: all'apertura del supermercato le casse non possono essere aperte tutte ma di un numero <K
@overview: crea array di K posizioni dove ogni posizione rappresenta una lista (coda della cassa)
@argument: 
"statocassa[i]" serve per capire se la cassa i è aperta (1) o chiusa (0).
"casseaperte" indica quante casse devono essere aperte all'apertura del supermercato (compreso tra [1,K-1])
"codaclienti[i]" indica quanti clienti sono in coda alla cassa i 
"tempocassa[i]" contiene il periodo di tempo di apertura totale della cassa i
@return: ritorna un puntatore alla struttura dati coda */
item** crea(int K,item** coda,int* statocassa, int casseaperte, int* codaclienti, timecassa* tempocassa, elemcassa* lista);


/*
@overview: inserisce un cliente in coda ad una cassa specificata
@argument:
"K": numero casse 
"cassa": numero cassa in cui voglio inserire valore [1,K]
"value": valore da inserire in coda
"prodotti": numero prodotti acquistati dal cliente
"tempoperacquisti": tempo che il cliente passa a fare acquisti prima di andare a pagare alla cassa.
"arrivoincoda": il momento in cui il cliente si è messo in coda alla cassa
@return: ritorna il puntatore alla struttura dati coda */
item** inserisci(item** coda, int K, int cassa, long value, int prodotti, int tempoperacquisti,int* statocassa,int* codaclienti, double arrivoincoda);


/* 
@overview: copia il cliente in una coda diversa (quella di "cassa")
@argument:
"K": numero casse
"cassa": numero cassa in cui voglio inserire cliente ([1,K])
"cliente": cliente che voglio inserire nella coda "cassa"
@return: ritorna il puntatore alla struttura dati coda */
item** copiaincodadiversa(item** coda, int K, int cassa, item* cliente,int* statocassa, int* codaclienti);


/* 
@overview: rimuove il primo elemento inserito nella cassa "cassa" (variabile "cassa" compreso tra [1,K])
@argument:
"K" rappresenta il numero di casse totali
@return: if cassa vuota return default-client else return cliente rimosso
*/
item rimuovi(item** coda, int K, int cassa, int* statocassa, int* codaclienti);


/*
@overview: apre la cassa "cassa" passata come argomento ("cassa" compreso tra [0,K-1])
@return: return 1 se la cassa è stata aperta con successo else 0  */
int opencassa(item** coda,int cassa ,int K,int* statocassa, timecassa* tempocassa, elemcassa* lista);


/*
//RICORDA: almeno una cassa aperta deve esistere
@overview: chiude la cassa "cassa" passata come argomento
@argument: "cassa" compresa tra [0,K-1]
@return: ritorna 1 se ho chiuso la cassa "cassa", 0 altrimenti (cassa già chiusa oppure non posso chiuderla perchè le chiuderei tutte) */
int closecassa(item** coda,int cassa,int K, int* statocassa, timecassa* tempocassa, int* nchiusure, elemcassa* lista);


void dealloca(item** coda, int K, int* statocassa, int* codaclienti);


void stampa(item** coda, int K, int* statocassa);


/*
@overview: restituisce il numero dei clienti in coda alla cassa "cassa" ("cassa" compreso tra [1,K])
@return: se la cassa risulta chiusa viene restituito -1 altrimenti valore >=0 */
int incoda(item** coda, int K, int cassa, int* statocassa);


/*
@overview: controlla se la cassa "cassa" è aperta
@argument:
"cassa": indice di cassa (compreso tra [0,K-1])
@return: restituisce -1 se la cassa è chiusa, 1 se la cassa è aperta */
int infocassa(int K, int* statocassa, int cassa);


/*
@overview: stampa per ogni cassa, i relativi clienti in coda e le proprie informazioni */
void stampacliente(item cliente);


/* interfaccia "lista" */


/*
@overview: crea lista che può contenere al massimo K casse */
void initlista(int K);


/*
@overview: inserisci cassa aperta in lista (inserimento in coda)
@argument: "indexcassa" compreso tra 0 e K-1 */
void inseriscicassa(int indexcassa);


/*
@overview: rimuove la cassa "cassa" passata come argomento 
@argument: "indexcassa" compreso tra 0 e K-1 */
void rimuovicassa(int indexcassa);


/*
RICORDA: uso esclusivo di generacassa (non protetto da mutua esclusione)
@overview: restituisce la dimensione della lista */
int sizelista();


/*
@overview: genera un valore che corrisponde all'indice di una delle casse aperte (quelle presenti nella lista) 
@return: restituisce valore compreso tra [0,K-1]*/
int generacassa();


void stampalista();


void deletelista();
