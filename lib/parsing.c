#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "parsing.h"
#define V 11 //numero delle variabili di "config.txt"

/* leggo dal file config.txt che contiene:
riga1: K (>=1) //numero totale di casse
riga2: C (>0)   //numero totale di clienti
riga3: E (>0)   //parametro
riga4: T (>10 millisecondi) 
riga5: P (>0)   //numero prodotti massimi acquistabili da un cliente
riga6: S1 (>0 e < K)
riga7: S2 (>0 e <=C)
riga8: KLIM (>0 e <K) //numero di casse aperte all'apertura del supermercato
riga9: filelog.txt //file di log dove scrivere le statistiche 
riga 10: TP (>0) //tempo di gestione di un singolo prodotto da parte di un cassiere (millisecondi)
riga 11: TIM (>0) //ampiezza dell'intervallo in cui il cassiere invia al direttore il numero di clienti in coda.
*/

void parsingfile (int* K, int* C, int* E, int* T, int* P, int* S1, int* S2, int* KLIM, char* filelog, int* TP, int* TIM) {

        /* gestione config.txt */
    FILE *file;
    if ((file = fopen("config.txt", "r")) == NULL)
    {
        perror("config.txt inesistente");
        exit(EXIT_FAILURE);
    }

    /* parsing config.txt */
    char filetmp[15];
    char buffer[25];
    char *token = NULL;
    int i=0;

    while ((fgets(buffer, 25, file)) != NULL)
    {
        token = strtok(buffer, "=");

        switch (i)
        {
        case 0: // prima riga - gestisco K
            if ((token != NULL) && (strcmp("K", token) == 0))
            {
                if ((token = strtok(NULL, "=")) != NULL)
                {
                    *K = atoi(token);
                    if (*K < 1)
                    {
                        printf("errore: K<1 \n");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            else
            {
                printf("config.txt non corretto (K) \n");
                exit(EXIT_FAILURE);
            }
            i++;
            break;

        case 1: //seconda riga - gestisco C
            if ((token != NULL) && (strcmp("C", token) == 0))
            {
                if ((token = strtok(NULL, "=")) != NULL)
                {
                    *C = atoi(token);
                    if (*C <= 0)
                    {
                        printf("errore: C<=0 \n");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            else
            {
                printf("config.txt non corretto (C) \n");
                exit(EXIT_FAILURE);
            }
            i++;
            break;

        case 2: //terza riga - gestisco E
            if ((token != NULL) && (strcmp("E", token) == 0))
            {
                if ((token = strtok(NULL, "=")) != NULL)
                {
                    *E = atoi(token);
                    if (*E <= 0)
                    {
                        printf("errore: E<=0 \n");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            else
            {
                printf("config.txt non corretto (E) \n");
                exit(EXIT_FAILURE);
            }
            i++;
            break;

        case 3: //quarta riga - gestisco T
            if ((token != NULL) && (strcmp("T", token) == 0))
            {
                if ((token = strtok(NULL, "=")) != NULL)
                {
                    *T = atoi(token);
                    if (*T <= 10)
                    {
                        printf("errore: T<=10 \n");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            else
            {
                printf("config.txt non corretto (T)\n");
                exit(EXIT_FAILURE);
            }
            i++;
            break;

        case 4: //quinta riga - gestisco P
            if ((token != NULL) && (strcmp("P", token) == 0))
            {
                if ((token = strtok(NULL, "=")) != NULL)
                {
                    *P = atoi(token);
                    if (*P <= 0)
                    {
                        printf("errore: P<=0 \n");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            else
            {
                printf("config.txt non corretto (P) \n");
                exit(EXIT_FAILURE);
            }
            i++;
            break;

        case 5: //sesta riga - gestisco S1
            if ((token != NULL) && (strcmp("S1", token) == 0))
            {
                if ((token = strtok(NULL, "=")) != NULL)
                {
                    *S1 = atoi(token);
                    if (*S1 <= 0 || *S1 >= *K)
                    {
                        printf("errore: S1 \n");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            else
            {
                printf("config.txt non corretto (S1)\n");
                exit(EXIT_FAILURE);
            }
            i++;
            break;

        case 6: //settima riga - gestisco S2
            if ((token != NULL) && (strcmp("S2", token) == 0))
            {
                if ((token = strtok(NULL, "=")) != NULL)
                {
                    *S2 = atoi(token);
                    if (*S2 <= 0 || *S2 >= *C)
                    {
                        printf("errore: S2 \n");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            else
            {
                printf("config.txt non corretto (S2) \n");
                exit(EXIT_FAILURE);
            }
            i++;
            break;

        case 7: //ottava riga - gestisco KLIM
            if ((token != NULL) && (strcmp("KLIM", token) == 0))
            {
                if ((token = strtok(NULL, "=")) != NULL)
                {
                    *KLIM = atoi(token);
                    if (*KLIM <= 0 || *KLIM >= *K)
                    {
                        printf("errore: KLIM \n");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            else
            {
                printf("config.txt non corretto (KLIM) \n");
                exit(EXIT_FAILURE);
            }
            i++;
            break;
        
        case 8: //nona riga - filelog.txt
            if ((token != NULL) && (strcmp("filelog", token) == 0)) {
                if ((token = strtok(NULL, "=")) != NULL)
                {
                    /* purifica stringa: togli carattere speciale eof */
                    int h=0;
                    for(h=0;h<strlen(token)-1;h++) {
                        filetmp[h]=token[h];
                    }
                    filetmp[h]='\0';
                }
            }
            else
            {
                printf("config.txt non corretto. chiamalo filelog.txt \n");
                exit(EXIT_FAILURE);
            }
            strcpy(filelog,filetmp);
            i++;
            break;
        
        case 9: //decima riga - TP
            if ((token != NULL) && (strcmp("TP", token) == 0))
            {
                if ((token = strtok(NULL, "=")) != NULL)
                {
                    *TP = atoi(token);
                    if (*TP <= 0)
                    {
                        printf("errore: TP<=0 \n");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            else
            {
                printf("config.txt non corretto (TP) \n");
                exit(EXIT_FAILURE);
            }
            i++;
            break;
        
        case 10: //undicesima riga - TIM
            if ((token != NULL) && (strcmp("TIM", token) == 0))
            {
                if ((token = strtok(NULL, "=")) != NULL)
                {
                    *TIM = atoi(token);
                    if (*TIM <= 0)
                    {
                        printf("errore: TP<=0 \n");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            else
            {
                printf("config.txt non corretto (TIM) \n");
                exit(EXIT_FAILURE);
            }
            i++;
            break;

        default:
            if (i>=V) {
                printf("config.txt accettato anche se con errori \n");
                break;
            }
            printf("config.txt non corretto (default)\n");
            exit(EXIT_FAILURE);
            break;
        }
    }

    if (i != V)
    {
        printf("config.txt non corretto \n");
        exit(EXIT_FAILURE);
    }

    fclose(file);
    return;
}

