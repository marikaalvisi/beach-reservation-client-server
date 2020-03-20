//gcc -o client clientspiaggia.c

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define MAX_SIZE 2000
#define M 10
#define N 5

int timeout (int);
int correttezzaData (int giornoInizio, int meseInizio, int giornoFine, int meseFine);

int main (void){
    int sock_desc;
    struct sockaddr_in serv_addr;
    char sbuff[MAX_SIZE],rbuff[MAX_SIZE];

    int go = 1, x = 0, controlMenu = 1, flag = 0;
    char* available;
    int res;
    int numeroFila;
    char numFila[100];
    char result[100];
    char giornoInizioChar[100];
    char giornoFineChar[100]; 
    char meseInizioChar[100];
    char meseFineChar[100];
    char numeroOmbrelloneChar [100];
    char* token, scelta, numero;
    char num [3];
    int giornoInizio, giornoFine, meseInizio, meseFine, numeroOmbrellone, value;
    int ombrelloniDisponibili[M*N];

    if((sock_desc = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    printf("Failed creating socket\n");

    bzero((char *) &serv_addr, sizeof (serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(3000);

    if (connect(sock_desc, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0) {
        printf("Failed to connect to server\n");
        return -1;
    }
    system("clear");
    printf("\nConnessione eseguita con successo!\n\n");

    while (controlMenu == 1){
        printf("Benvenuto nel sistema di prenotazione di ombrelloni.\n");
        printf("\nDigitare il numero corrispondente all'operazione desiderata:\n1.Prenotazione ombrellone\n2.Cancellazione Prenotazione\n3.Mostra disponibilità ombrelloni\n4.Chiudi client\n\n");
        printf("Scelta: ");
        scanf("%s", result);
        res = atoi(result);
        while (res != 1 && res != 2 && res != 3 && res != 4){
            printf("\nIl valore inserito non è valido. Si prega di riprovare.\n");
            bzero(result, 100);
            scanf("%s", result);
            res = atoi(result);
        }

            switch(res){

                case (1): //PRENOTAZIONE OMBRELLONE

                    printf("Giorno di inizio prenotazione: ");
                    scanf("%s", giornoInizioChar);
                    printf("Mese di inizio prenotazione: ");
                    scanf("%s", meseInizioChar);
                    printf("Giorno di fine prenotazione: ");
                    scanf("%s", giornoFineChar);
                    printf("Mese di fine prenotazione: ");
                    scanf("%s", meseFineChar);

                    giornoInizio = atoi (giornoInizioChar);
                    giornoFine = atoi (giornoFineChar);
                    meseInizio = atoi (meseInizioChar);
                    meseFine = atoi (meseFineChar);

                    system("clear");

                    value = correttezzaData (giornoInizio, meseInizio, giornoFine, meseFine);
                    if (value == 0){
                        system("clear");
                        printf("Data inserita non valida\n\n");
                    }

                    if (value == 1){
                        bzero(sbuff, MAX_SIZE);
                        strcat(sbuff, "BOOK ");
                        sprintf(num, "%d", giornoInizio);
                        strcat(sbuff, num);
                        strcat(sbuff, "/");
                        sprintf(num, "%d", meseInizio);
                        strcat(sbuff, num);
                        strcat(sbuff, " ");
                        sprintf(num, "%d", giornoFine);
                        strcat(sbuff, num);
                        strcat(sbuff, "/");
                        sprintf(num, "%d", meseFine);
                        strcat(sbuff, num);
                    
                        system("clear");

                        //passaggio al server di BOOK datainizio datafine
                        send(sock_desc,sbuff,strlen(sbuff),0);
                        
                        //ricevo una risposta dal server
                        bzero(rbuff, MAX_SIZE);
                        recv(sock_desc,rbuff,MAX_SIZE,0);

                        token = strtok(rbuff, " ");
                        //available = token;
                        //se non c'è nessun ombrellone disponibile il server ritorna NAVAILABLE
                        if(strncmp(token, "NAVAILABLE", 11) == 0){
                            printf("Nessun ombrellone disponibile per le date richieste.\n");
                        }

                        //se ci sono ombrelloni disponibili il server ritorna AVAILABLE
                        //il server restituisce anche la matrice delle disponibilita nelle date richieste
                        //il client la stampa a video
                        //il client deve rispondere con il numero dell'ombrellone che desidera prenotare

                        if(strncmp(token, "AVAILABLE", 10) == 0){ 

                            //creazione array ombrelloni disponibili
                            for(int i = 1; (i-1)<M*N; i++){
                                    token = strtok(NULL, "-");
                                    value = atoi(token);
                                    ombrelloniDisponibili[i-1] = value;
                                }

                            bzero(result, 100);
                            printf("\n1.Visualizzazione ombrelloni disponibili in una determinata fila\n2.Visualizzazione di tutti gli ombrelloni disponibili\n");
                            printf("\nScelta: ");
                            scanf("%s", result);
                            res = atoi(result);
                            while (res != 1 && res != 2){
                                bzero(result, 100);
                                printf("Scelta non valida, si prega di riprovare: ");
                                scanf("%s", result);
                                res = atoi(result);
                            }
                            system("clear");
                            
                            if (res == 1){ //se voglio visualizzare la disponibilità per righe
                                printf("Inserire il numero di fila desiderata: ");
                                scanf("%s", numFila);
                                system("clear");
                                numeroFila = atoi(numFila);
                                while(numeroFila < 1 || numeroFila > M){
                                    bzero(numFila, 100);
                                    printf("Numero fila non valido, si prega di riprovare: ");
                                    scanf("%s", numFila);
                                    numeroFila = atoi(numFila);
                                }
                                printf("Ombrelloni prenotabili in fila %d nelle date %d/%d - %d/%d: \n\n", numeroFila, giornoInizio, meseInizio, giornoFine, meseFine);
                                for(int i = numeroFila*N-N; i<numeroFila*N; i++){
                                    value = ombrelloniDisponibili[i];
                                    //fare la stampa della fila
                                    if(value == 0)
                                        printf("| X |\t");
                                    else
                                        printf("| %d |\t", value);
                                }
                                bzero(num, 3);
                                printf("\n\nNumero dell'ombrellone che si desidera prenotare: ");
                                scanf("%s", num);
                                numeroOmbrellone = atoi(num);
                                
                                while (numeroOmbrellone < numeroFila*N-N || numeroOmbrellone > numeroFila*N || ombrelloniDisponibili[numeroOmbrellone-1] == 0) {
                                    printf("Numero ombrellone non valido. Si prega di riprovare: ");
                                    scanf("%s", num);
                                    numeroOmbrellone = atoi(num);
                                }
                            }

                            if (res == 2)  {       //se voglio visualizzare l'intera matrice delle disponibilità                                              
                                printf("Ombrelloni prenotabili nelle date %d/%d - %d/%d: \n\n", giornoInizio, meseInizio, giornoFine, meseFine);
                                //token = strtok(rbuff, " ");
                                //available = token;
                                for(int i = 1; (i-1)<M*N; i++){
                                    value = ombrelloniDisponibili[i-1];
                                    //fare la stampa della matrice
                                    if(value == 0)
                                        printf("| X |\t");
                                    else
                                        printf("| %d |\t", value);
                                    if(i%N == 0)
                                        printf("\n");
                                }
                                //inserimento numero ombrellone
                                bzero(num, 3);
                                printf("\n\nNumero dell'ombrellone che si desidera prenotare: ");
                                scanf("%s", num);
                                numeroOmbrellone = atoi(num);
                                
                                while (numeroOmbrellone < 1 || numeroOmbrellone >M*N || ombrelloniDisponibili[numeroOmbrellone-1] == 0) {
                                    printf("Numero ombrellone non valido. Si prega di riprovare: ");
                                    scanf("%s", num);
                                    numeroOmbrellone = atoi(num);
                                }
                            }

                            //il client invia al server BOOK numeroOmbrellone
                            sprintf(num, "%d", numeroOmbrellone);
                            bzero(sbuff, MAX_SIZE);
                            strcat(sbuff, "BOOK ");
                            strcat(sbuff, num);
                            send(sock_desc,sbuff,strlen(sbuff),0);

                            //attendo la risposta dal server
                            //risponde con NAVAILABLE se l'ombrellone selezionato non è fra quelli disponibili
                            //e con AVAILABLE altrimenti
                            //while((recv(sock_desc, rbuff, MAX_SIZE, 0))==0 && timeout(10) == 0)
                            bzero(rbuff, MAX_SIZE);
                            recv(sock_desc,rbuff,MAX_SIZE,0);
                            
                            if((strncmp(rbuff, "NAVAILABLE", 11) == 0)){
                                printf("Ombrellone non disponibile per le date richieste.\nL'ombrellone potrebbe essere stato appena prenotato.\n");
                            }

                            if(strncmp(rbuff, "AVAILABLE", 10) == 0){
                                system("clear");
                                printf("Ombrellone %d disponibile per le date richieste.\nSi desidera confermare la prenotazione? S/N\n", numeroOmbrellone);
                                printf("Scelta: ");
                                scanf(" %c", &scelta);
                                
                                //se si desidera confermare la prenotazione
                                bzero(sbuff, MAX_SIZE);
                                x = 0;
                                while (x==0){
                                    if((scelta == 'S') || (scelta == 's')){
                                        strcat(sbuff, "CONFIRM");
                                        send(sock_desc,sbuff,strlen(sbuff),0);
                                        x = 1;
                                    } else if ((scelta == 'N') || (scelta == 'n')){
                                        strcat(sbuff, "CANCEL");
                                        send(sock_desc,sbuff,strlen(sbuff),0);
                                        x = 1;
                                    }
                                    else{
                                        printf("Scelta non valida. Si prega di inserire una scelta valida.\n");
                                        scanf(" %c", &scelta);
                                    }
                                }   
                                bzero(rbuff, MAX_SIZE);
                                if (recv(sock_desc, rbuff, MAX_SIZE, 0) > 0){
                                    fprintf(stderr, "\n%s\n", rbuff);
                                    if (strncmp(rbuff, "CONFIRMED", 10) == 0) {
                                        system("clear");
                                        printf("Prenotazione confermata e salvata con successo.\n\n");
                                    }
                                    if (strncmp(rbuff, "CANCELED", 9) == 0){
                                        system("clear");
                                        printf("Prenotazione non eseguita\n\n");
                                    }
                                }
                            } //If della conferma della prenotazione
                        } //If nel caso l'ombrellone scelto sia disponibile (AVAILABLE)

                    bzero(rbuff,MAX_SIZE); //to clean buffer-->IMP otherwise previous word characters also came

                    }  //Else se la data ha passato il controll

                break;

                //////////////////////////////////////////////////////

                case(2): //CANCELLAZIONE PRENOTAZIONE

                    printf("Inserire i dati della prenotazione che si desidera cancellare.\n");

                    printf("Numero ombrellone prenotato: ");
                    scanf("%s", numeroOmbrelloneChar);
                    printf("Giorno di inizio prenotazione: ");
                    scanf("%s", giornoInizioChar);
                    printf("Mese di inizio prenotazione: ");
                    scanf("%s", meseInizioChar);
                    printf("Giorno di fine prenotazione: ");
                    scanf("%s", giornoFineChar);
                    printf("Mese di fine prenotazione: ");
                    scanf("%s", meseFineChar);

                    numeroOmbrellone = atoi (numeroOmbrelloneChar);
                    giornoInizio = atoi (giornoInizioChar);
                    giornoFine = atoi (giornoFineChar);
                    meseInizio = atoi (meseInizioChar);
                    meseFine = atoi (meseFineChar);

                    system("clear");

                    value = correttezzaData (giornoInizio, meseInizio, giornoFine, meseFine);
                    if (value == 0){
                        system("clear");
                        printf("Data inserita non valida\n\n");
                    }

                    if(value == 1){
                        system("clear");
                        bzero(sbuff, MAX_SIZE);
                        strcat(sbuff, "CANCEL ");                      
                        sprintf(num, "%d", numeroOmbrellone);
                        strcat(sbuff, num);
                        strcat(sbuff, " ");
                        sprintf(num, "%d", giornoInizio);
                        strcat(sbuff, num);
                        strcat(sbuff, "/");
                        sprintf(num, "%d", meseInizio);
                        strcat(sbuff, num);
                        strcat(sbuff, " ");
                        sprintf(num, "%d", giornoFine);
                        strcat(sbuff, num);
                        strcat(sbuff, "/");
                        sprintf(num, "%d", meseFine);
                        strcat(sbuff, num);

                        //passaggio al server di CANCEL numeroOmbrellone datainizio datafine
                        send(sock_desc,sbuff,strlen(sbuff),0);

                        //ricevo una risposta dal server
                        bzero(rbuff, MAX_SIZE);
                        if (recv(sock_desc,rbuff,MAX_SIZE,0)>0){

                            //se la prenotazione è stata cancellata il server ritorna CANCEL OK
                            if(strncmp(rbuff, "CANCEL OK", 10) == 0){ 
                                fprintf(stderr, "Cancellazione avvenuta con successo.\n");
                            }
                            if(strncmp(rbuff, "CANCEL NOK", 11) == 0){
                                fprintf(stderr, "Cancellazione fallita. Non esiste alcuna prenotazione coincidente con le date inserite all'ombrellone richiesto.\n\n");
                            }
                            bzero(rbuff, MAX_SIZE);
                    }
                }

                break;



                /////////////////////////////////////////////////////

                case(3): //DISPONIBILITA' OMBRELLONI

                    
                    printf("Giorno di inizio prenotazione: ");
                    scanf("%s", giornoInizioChar);
                    printf("Mese di inizio prenotazione: ");
                    scanf("%s", meseInizioChar);
                    printf("Giorno di fine prenotazione: ");
                    scanf("%s", giornoFineChar);
                    printf("Mese di fine prenotazione: ");
                    scanf("%s", meseFineChar);

                    giornoInizio = atoi (giornoInizioChar);
                    giornoFine = atoi (giornoFineChar);
                    meseInizio = atoi (meseInizioChar);
                    meseFine = atoi (meseFineChar);

                    system("clear");

                    //controlli data
                    value = correttezzaData (giornoInizio, meseInizio, giornoFine, meseFine);
                    if (value == 0){
                        system("clear");
                        printf("Data inserita non valida\n\n");
                    }
                        
                    else { //se la data è corretta
                        bzero(sbuff, MAX_SIZE);
                        strcat(sbuff, "AVAILABLE ");
                        sprintf(num, "%d", giornoInizio);
                        strcat(sbuff, num);
                        strcat(sbuff, "/");
                        sprintf(num, "%d", meseInizio);
                        strcat(sbuff, num);
                        strcat(sbuff, " ");
                        sprintf(num, "%d", giornoFine);
                        strcat(sbuff, num);
                        strcat(sbuff, "/");
                        sprintf(num, "%d", meseFine);
                        strcat(sbuff, num);
                        
                        system("clear");
                        //passaggio al server di AVAILABLE datainizio datafine
                        send(sock_desc,sbuff,strlen(sbuff),0);

                        //ricevo una risposta dal server
                        
                        bzero(rbuff, MAX_SIZE);
                        recv(sock_desc,rbuff,MAX_SIZE,0);

                        token = strtok(rbuff, " ");
                        //available = token;
                        //se non c'è nessun ombrellone disponibile il server ritorna NAVAILABLE
                        if(strncmp(token, "NAVAILABLE", 11) == 0){
                            printf("Nessun ombrellone disponibile per le date richieste.\n");
                        }

                        //se ci sono ombrelloni disponibili il server ritorna AVAILABLE
                        //il server restituisce anche la matrice delle disponibilita nelle date richieste
                        //il client la stampa a video
                        //il client deve rispondere con il numero dell'ombrellone che desidera prenotare

                        if(strncmp(token, "AVAILABLE", 10) == 0){ 
                            //creazione array ombrelloni disponibili
                            for(int i = 1; (i-1)<M*N; i++){
                                    token = strtok(NULL, "-");
                                    value = atoi(token);
                                    ombrelloniDisponibili[i-1] = value;
                                }
                                    
                            printf("Ombrelloni prenotabili nelle date %d/%d - %d/%d: \n\n", giornoInizio, meseInizio, giornoFine, meseFine);

                            for(int i = 1; (i-1)<M*N; i++){
                                value = ombrelloniDisponibili[i-1];
                                //fare la stampa della matrice
                                if(value == 0)
                                    printf("| X |\t");
                                else
                                    printf("| %d |\t", value);
                                if(i%N == 0)
                                    printf("\n");
                            }
                            printf("\n\n");
                        }
                    bzero(rbuff,MAX_SIZE);
                    }

                break;

                //////////////////////////////////////////////////////////////////

                case (4): //CHIUSURA CLIENT
                    //Protocollo di chiusura client
                    system("clear");
                    printf("\nArrivederci!\n\n");
                    bzero(sbuff, MAX_SIZE);
                    strcat(sbuff, "CLOSE");
                    send(sock_desc,sbuff,strlen(sbuff),0);
                    close(sock_desc); // Chiusura socket

                    return 0;
                    break;
                
            } //Switch case menù
    } //while per il menù
    return 0;
} // chiusura main

int correttezzaData (int giornoInizio, int meseInizio, int giornoFine, int meseFine){
    int x; //x=0 se la data non è valida     x=1 se è valida
    if(meseInizio != 8 && meseInizio != 7 && meseInizio != 6)
        return x = 0;
    if(meseFine != 8 && meseFine != 7 && meseFine != 6)
        return x = 0;
    if(meseInizio == 6 && (giornoInizio < 1 || giornoInizio > 30))
        return x = 0;
    if(meseFine == 6 && (giornoFine < 1 || giornoFine > 30))
        return x = 0;
    if(meseInizio == 7 && (giornoInizio < 1 || giornoInizio > 31))
        return x = 0;
    if(meseFine == 7 && (giornoFine < 1 || giornoFine > 31))
        return x = 0;
    if(meseInizio == 8 && (giornoInizio < 1 || giornoInizio > 31))
        return x = 0;
    if(meseFine == 8 && (giornoFine < 1 || giornoFine > 31))
        return x = 0;
    if(meseInizio == meseFine && giornoFine < giornoInizio)
        return x = 0;
    if(meseFine < meseInizio)
        return x = 0;
    else
        return x = 1;
    
}