#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h> 
#include <errno.h>
#include <pthread.h>
#include <time.h>



/* @overview: funzione che restituisce il tempo corrente in millisecondi */
double current_time_millisecond();


/*
@overview: random del numero di prodotti del cliente
@argument: P>0 
@@return: restituisce un valore che varia tra 0 e P */
int randomprodotti(int P);


/*
@overview: random del tempo per gli acquisti del cliente 
@argument: T>0
@return: varia tra 10 millisecondi e T dove T>10 */
int randomtimeacquisticliente(int T);


/*
@argument: var serve ad aggiungere ulteriore dissonanza tra i valori generati)
@return: genera un tempo compreso tra 20 e 80 millisecondi  */
int generatempofissocassiere(long var);


/*
overview: effettua una nanosleep per tot millisecondi "ms" */
int ms_sleep(unsigned int ms);