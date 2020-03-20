//gcc serverspiaggia.c -pthread -o server

#include<stdio.h>
#include<string.h>   
#include<stdlib.h>    
#include<sys/socket.h>
#include<arpa/inet.h> 
#include<unistd.h>   
#include<pthread.h>
#include<time.h>


#define M 10
#define N 5
#define MAX_SIZE 2000

typedef struct Ombrellone {
    int numero;
    int giugno [30]; //0->libero 1->occupato 2->temporanemante prenotato
    int luglio [31]; //0->libero 1->occupato 2->temporanemante prenotato
    int agosto [31]; //0->libero 1->occupato 2->temporanemante prenotato
}Ombrellone;

typedef struct Prenotazione{
  char dataInizio [11];
  char dataFine [11];
  int numeroOmbrellone;
  int stato;
} Prenotazione;

typedef struct Nodo{
    struct Prenotazione p;
    struct Nodo* next;
} Nodo;

typedef struct Lista{
    Nodo *testa;
} Lista;

int timeout (int);
void *connection_handler(void *);
void inizializzazioneSpiaggia (Ombrellone spiaggia [M][N]);
void letturaPrenotazioni(Ombrellone spiaggia [M][N]);
int disponibilitaOmbrelloniRange(Ombrellone spiaggia [M][N], int *ombrelloniDisponibili, int giornoInizio, int meseInizio, int giornoFine, int meseFine);
void aggiungiPrenotazione(Ombrellone spiaggia [M][N], int i, int j, int giornoInizio, int meseInizio, int giornoFine, int meseFine);
void cambioStatoOmbrellone(Ombrellone spiaggia [M][N], int stato, int i, int j, int giornoInizio, int meseInizio, int giornoFine, int meseFine);
int eliminazionePrenotazione(int numeroOmbrellone, char* dataInizio, char* dataFine);


//variabili condivise
Ombrellone spiaggia [M][N];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int main(void){
    int socket_desc , client_sock , c , *new_sock;
    struct sockaddr_in server , client;
    
    system("clear");

    //inizializzazione matrice
    inizializzazioneSpiaggia(spiaggia);

    //caricamento prenotazioni
    letturaPrenotazioni(spiaggia);

    //Creazione socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Impossibile creare il socket");
    }
    puts("Socket creato");

    //Preparazione della struttura sockaddr_in
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 3000 );

    //Bind del socket
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("Bind fallita.");
        close(socket_desc);
        return 1;
    }
    puts("Bind eseguita con successo");

    //Listen, server in ascolto
    listen(socket_desc , 3);
    
    //Accept e inizio comunicazione con client
    puts("In attesa di una connessione...");
    c = sizeof(struct sockaddr_in);

    while(client_sock=accept(socket_desc,(struct sockaddr*)&client,(socklen_t*)&c)){
        puts("Connessione accettata");

        pthread_t sniffer_thread;
        new_sock = malloc(1);
        *new_sock = client_sock;

        if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0)
        {
            perror("Impossibile creare il thread");
            return 1;
        }

        puts("Handler assegnato");
    }

    if (client_sock < 0)
    {
        perror("Accept fallita");
        return 1;
    }
    
}
//////////////////////////////////////////////////////////////////////

void *connection_handler(void *socket_desc){
  //Get the socket descriptor
  int sock = *(int*)socket_desc;
  int n, x=0, y=0, flag=0, disponibile;
  char sendbuff[MAX_SIZE], client_message[MAX_SIZE];
  char* dataInizio;
  char* dataFine;
  char* num;
  char* sig;
  int giornoInizio, giornoFine, meseInizio, meseFine;
  char* token;
  char* message;
  char message1[MAX_SIZE];  //Message1 e 2 usati in CANCEL
  char message2[MAX_SIZE];
  int numeroOmbrelloniDisponibili, numeroOmbrellone, i, j;
  int ombrelloniDisponibili[M*N] = {}; //1->ombrellone disponibile 0->ombrellone non disponibile
  char caratt[5];
  
  

  while (1==1){
    //ricevo un messaggio dal client
    bzero(client_message, MAX_SIZE);
    recv(sock, client_message, MAX_SIZE, 0);
    strcpy(message1, client_message);
    if (strncmp(client_message, "CLOSE", 6) == 0){
      fprintf(stderr, "Thread chiuso.\n");
      close(sock);
      pthread_exit(NULL);
      break;
    }
    token = strtok(client_message, " ");
    sig = token;

    //decodifica messaggio
    if(strcmp(sig, "BOOK") == 0){ //BOOK datainizio datafine
    //il client chiede di poter occupare un ombrellone
    //risponde con "OK" se pronto per la prenotazione
    //e con "NOK" se non lo è e termina la comunicazione, se non c'è nessun ombrellone disponibile con "NAVAILABLE"

      //suddivisione stringa in data inizio e data fine
      token = strtok(NULL, " ");
      dataInizio = token;
      token = strtok(NULL, " ");
      dataFine = token;
      //suddivisione in giorno mese inizio e fine
      token = strtok(dataInizio, "/");
      giornoInizio = atoi(token);
      token = strtok(NULL, "/");
      meseInizio = atoi(token);
      token = strtok(dataFine, "/");
      giornoFine = atoi(token);
      token = strtok(NULL, "/");
      meseFine = atoi(token);

      numeroOmbrelloniDisponibili = disponibilitaOmbrelloniRange(spiaggia, ombrelloniDisponibili, giornoInizio, meseInizio, giornoFine, meseFine);
      //se non ci sono ombrelloni disponibili ritorno "NAVAILABLE"
      if(numeroOmbrelloniDisponibili == 0){
        send(sock,"NAVAILABLE",n,0);
      }
      //se ci sono ombrelloni disponibili ritorno "AVAILABLE" contatenato con il vettore di disponibilità 
      //e attendo in risposta il numero dell'ombrellone da prenotare
      if(numeroOmbrelloniDisponibili > 0){
        bzero(sendbuff, MAX_SIZE);
        strcat(sendbuff, "AVAILABLE ");

        for(i = 0; i<M*N; i++){
          sprintf(caratt, "%d",  ombrelloniDisponibili[i]);
          num = caratt;
          strcat(sendbuff, num);
          strcat(sendbuff, "-");
        }
        
        send(sock, sendbuff , strlen(sendbuff), 0);
        
        //attendo la risposta composta da BOOK + numero ombrellone da prenotare

        bzero(sendbuff, MAX_SIZE);
        bzero(client_message, MAX_SIZE);
        
        if (recv(sock, client_message, MAX_SIZE, 0)>0){
          printf("%s\n", client_message);
          token = strtok(client_message, " ");
          message = token;
          token = strtok(NULL, " ");
        }
        
        numeroOmbrellone = atoi(token);

        if (strcmp(message, "BOOK")!=0){
          perror("Non sto facendo book, ERROR: ");
        }
        //torno a verificare la disponibilità
        disponibilitaOmbrelloniRange(spiaggia, ombrelloniDisponibili, giornoInizio, meseInizio, giornoFine, meseFine);
        //se l'ombrellone è disponibile
        if(numeroOmbrellone > 0 && numeroOmbrellone < M*N+1 && ombrelloniDisponibili[numeroOmbrellone-1] != 0){ 
          disponibile = 1;
          for(i=0; i<M; i++){
            for(j=0; j<N; j++){
              if(spiaggia[i][j].numero == numeroOmbrellone){
                //INIZIO SEZIONE CRITICA ##########################
                pthread_mutex_lock(&mutex);
                printf("lock\n");
                cambioStatoOmbrellone(spiaggia, 2, i, j, giornoInizio, meseInizio, giornoFine, meseFine);
                ombrelloniDisponibili[numeroOmbrellone-1] = 0;
                pthread_mutex_unlock(&mutex);
                printf("unlock\n");
                //FINE SEZIONE CRITICA #########################
                x = i;
                y = j;
              }
            } 
          }

          i = x;
          j = y;

          strcat(sendbuff, "AVAILABLE");
          send(sock, sendbuff, strlen(sendbuff), 0 );
        }

        else{ //se l'ombrellone nel frattempo è stato prenotato
          disponibile = 0;
          bzero(sendbuff, MAX_SIZE);
          strcat(sendbuff, "NAVAILABLE");
          send(sock, sendbuff, strlen(sendbuff), 0 );
        }

        if (disponibile == 1){
          bzero(client_message, MAX_SIZE);
          bzero(sendbuff, MAX_SIZE);
          x = recv(sock,client_message, MAX_SIZE, 0);

          //INIZIO SEZIONE CRITICA###############################
          pthread_mutex_lock(&mutex);
          printf("lock\n");
          if (x > 0){
            if (strncmp(client_message, "CONFIRM", 8) == 0) {
              fprintf(stderr, "%s\n", client_message);
              aggiungiPrenotazione(spiaggia, i, j, giornoInizio, meseInizio, giornoFine, meseFine);
              fprintf(stderr, "Prenotazione aggiunta.\n");
              strcat(sendbuff, "CONFIRMED");
              send(sock, sendbuff, strlen(sendbuff), 0);
            } 
            
            if (strncmp(client_message, "CANCEL", 7) == 0){
              fprintf(stderr, "%s\n", client_message);
              fprintf(stderr, "Prenotazione cancellata...\n");
              cambioStatoOmbrellone(spiaggia, 0, i, j, giornoInizio, meseInizio, giornoFine, meseFine);
              ombrelloniDisponibili[numeroOmbrellone-1] = numeroOmbrellone;
              strcat(sendbuff, "CANCELED");
              send(sock, sendbuff, strlen(sendbuff), 0);
            }
          } 
          pthread_mutex_unlock(&mutex);
          printf("unlock\n");
          //FINE SEZIONE CRITICA#########################
        }
      } 
      bzero(client_message, MAX_SIZE);  
  } 
  
  //AVAILABLE
  if(strncmp(sig, "AVAILABLE", 10) == 0){
  //per sapere se ci sono ombrelloni disponibili, quanti e quali sono alla data odierna
  //il server risponde con NAVAILABLE se non ci sono ombrelloni disponibili
  //e con AVAILABLE $numero dove $numero è il numero di ombrelloni disponibili
  //il client deve rispondere con AVAILABLE DATESTART DATEEND
    //suddivisione stringa in data inizio e data fine
    token = strtok(NULL, " ");
    dataInizio = token;
    token = strtok(NULL, " ");
    dataFine = token;
    //suddivisione in giorno mese inizio e fine
    token = strtok(dataInizio, "/");
    giornoInizio = atoi(token);
    token = strtok(NULL, "/");
    meseInizio = atoi(token);
    token = strtok(dataFine, "/");
    giornoFine = atoi(token);
    token = strtok(NULL, "/");
    meseFine = atoi(token);

    numeroOmbrelloniDisponibili = disponibilitaOmbrelloniRange(spiaggia, ombrelloniDisponibili, giornoInizio, meseInizio, giornoFine, meseFine);
    if(numeroOmbrelloniDisponibili == 0)
      send(sock,"NAVAILABLE", strlen(sendbuff),0);

    if(numeroOmbrelloniDisponibili > 0){
      bzero(sendbuff, MAX_SIZE);
      strcat(sendbuff, "AVAILABLE ");

      for(i = 0; i<M*N; i++){
        sprintf(caratt, "%d",  ombrelloniDisponibili[i]);
        num = caratt;
        strcat(sendbuff, num);
        strcat(sendbuff, "-");
      }
      
      send(sock, sendbuff , strlen(sendbuff), 0);
    }
  }

  
    if(strncmp(sig, "CANCEL", 7) == 0){
    //IL server riceve CANCEL DATAEND DATASTART dal client
    //il server guarda se nelle date richieste è presente una prenotazione e la cancella, invia al client CANCEL OK
    //suddivisione stringa in data inizio e data fine
        token = strtok(NULL, " ");
        numeroOmbrellone = atoi(token);
        token = strtok(NULL, " ");
        dataInizio = token;
        token = strtok(NULL, " ");
        dataFine = token;

        //INIZIO SEZIONE CRITICA
        pthread_mutex_lock(&mutex);
        x = eliminazionePrenotazione(numeroOmbrellone, dataInizio, dataFine);
        pthread_mutex_unlock(&mutex);
        //FINE SEZIONE CRITICA

        //suddivisione in giorno mese inizio e fine
        token = strtok(dataInizio, "/");
        giornoInizio = atoi(token);
        token = strtok(NULL, "/");
        meseInizio = atoi(token);
        token = strtok(dataFine, "/");
        giornoFine = atoi(token);
        token = strtok(NULL, "/");
        meseFine = atoi(token);

      //ricerca ombrellone per il cambio stato nella  matrice delle disponibilità
      if (x == 1){
        for(i=0; i<M; i++){
          for(j=0; j<N; j++){
            if(spiaggia[i][j].numero == numeroOmbrellone){
              cambioStatoOmbrellone(spiaggia, 0, i, j, giornoInizio, meseInizio, giornoFine, meseFine);
              ombrelloniDisponibili[numeroOmbrellone-1] = numeroOmbrellone;
            }
          } 
        }
        bzero(sendbuff, MAX_SIZE);
        strcat(sendbuff, "CANCEL OK");
        send(sock, sendbuff, strlen(sendbuff),0);
      } 
        if(x == 0) {
        bzero(sendbuff, MAX_SIZE);
        strcat(sendbuff, "CANCEL NOK");
        send(sock, sendbuff , strlen(sendbuff),0);
      }
    } //fine CANCEL
  } // Chiusura while connection handler
  pthread_exit(NULL);
} // Chiusura connection handler

    
///////////////////////////////////////////////////////////////////////////////

void inizializzazioneSpiaggia (Ombrellone spiaggia [M][N]){
    int i, j, k, count;
    count = 1;
    for (i = 0; i<M; i++){
      for(j = 0; j<N; j++){
        spiaggia[i][j].numero = count;
        //cambioStatoOmbrellone(spiaggia, 0, 1, 6, 31, 8);
        for (k=0; k<30; k++){ //inizializzazione prenotazioni giugno
          spiaggia[i][j].giugno[k] = 0;
        }
        for (k=0; k<31; k++){ //inizializzazione prenotazioni luglio e agosto
          spiaggia[i][j].luglio[k] = 0;
          spiaggia[i][j].agosto[k] = 0;
        }
        count ++;
      }
    }
  }

///////////////////////////////////////////////////////////////////////////////
  
int disponibilitaOmbrelloniRange (Ombrellone spiaggia [M][N], int *ombrelloniDisponibili, int giornoInizio, int meseInizio, int giornoFine, int meseFine){
    int i, j, k, count, numeroOmbrelloniDisponibili;
    count = 1;
    numeroOmbrelloniDisponibili = M*N;
    //inizializzazione array disponibilita come tutto disponibile
    //se l'ombrellone è disponibile l'array contiene il numero dell'ombrellone, altrimenti contiene 0
    for(i=0; i<M*N; i++){
      ombrelloniDisponibili[i] = count;
      count++;
    }
    count = 0;
    for (i = 0; i<M; i++){
      for(j = 0; j<N; j++){
        //prenotazione con partenza a giugno
        if(meseInizio == 6){
          if(meseFine == 6){ //se la prenotazione inizia e finisce a giugno
            for(k=giornoInizio; k<=giornoFine; k++){
              if(spiaggia[i][j].giugno[k-1] != 0){
                ombrelloniDisponibili[count] = 0;
              }
            }
          }
          if (meseFine == 7){ //se la prenotazione inizia a giugno e finisce a luglio
            for(k=giornoInizio; k<=30; k++){
              if(spiaggia[i][j].giugno[k-1] != 0){
                ombrelloniDisponibili[count] = 0;
              }
            } 
            for(k=1; k<=giornoFine; k++){
              if(spiaggia[i][j].luglio[k-1] != 0){
                ombrelloniDisponibili[count] = 0;
              }
            }
          }
          if (meseFine == 8){ //se la prenotazione inizia a giugno e finisce a agosto
            for(k=giornoInizio; k<=30; k++){
              if(spiaggia[i][j].giugno[k-1] != 0){
                ombrelloniDisponibili[count] = 0;
              }
            }
            for(k=1; k<=31; k++){
              if(spiaggia[i][j].luglio[k-1] != 0){
                ombrelloniDisponibili[count] = 0;
              }
            }
            for(k=1; k<=giornoFine; k++){
              if(spiaggia[i][j].agosto[k-1] != 0){
                ombrelloniDisponibili[count] = 0;
              }
            }
          }
        } //giugno
        //prenotazione con partenza a luglio
        if(meseInizio == 7){
          if(meseFine == 7){ //se la prenotazione inizia e finisce a luglio
            for(k=giornoInizio; k<=giornoFine; k++){
              if(spiaggia[i][j].luglio[k-1] != 0){
                ombrelloniDisponibili[count] = 0;
              }
            }
          }
          if(meseFine == 8){  //se la prenotazione inizia a luglio e finisce a agosto
            for(k=giornoInizio; k<=31; k++){
              if(spiaggia[i][j].luglio[k-1] != 0){
                ombrelloniDisponibili[count] = 0;
              }
            }
            for(k=1; k<=giornoFine; k++){
              if(spiaggia[i][j].agosto[k-1] != 0){
                ombrelloniDisponibili[count] = 0;
              }
            }
          }
        } //luglio
        //prenotazione con partenza e fine a agosto
        if((meseInizio == 8) && (meseFine == 8)){
          for(k=giornoInizio; k<=giornoFine; k++){
            if(spiaggia[i][j].agosto[k-1] != 0){
              ombrelloniDisponibili[count] = 0;
            }
          }
        }//agosto
        count++;
      }
    }
    for(i=0; i<M*N; i++){
      if(ombrelloniDisponibili[i] == 0)
        numeroOmbrelloniDisponibili--;
    }
    return numeroOmbrelloniDisponibili;
  }

///////////////////////////////////////////////////////////////////////////////

void letturaPrenotazioni(Ombrellone spiaggia [M][N]){
  FILE *fd;
  char *res;
  char *token;
  char *dataInizio;
  char *dataFine;
  int i, j, k,numero, stato;
  char prenotazione [200];
  int giornoInizio, giornoFine, meseInizio, meseFine;

  fd = fopen("prenotazioni.txt", "r");
  if( fd==NULL ) {
    perror("Errore in apertura del file");
    exit(1);
  }
i=0;
  while(fscanf(fd, "%d %d %d/%d %d/%d\n", &numero, &stato, &giornoInizio, &meseInizio, &giornoFine, &meseFine) > 0) {
    //inserimento dati nella matrice
    for(i=0; i<M; i++){
      for(j=0; j<N; j++){
        if(spiaggia[i][j].numero == numero){
          cambioStatoOmbrellone(spiaggia, stato, i, j, giornoInizio, meseInizio, giornoFine, meseFine); 
          break;
        }
      }
    }
  }
  fclose(fd);
} //fine lettura

///////////////////////////////////////////////////////////////////////////////

void aggiungiPrenotazione(Ombrellone spiaggia [M][N], int i, int j, int giornoInizio, int meseInizio, int giornoFine, int meseFine){
  FILE *fd;
  int stato = 1;
  
  cambioStatoOmbrellone(spiaggia, stato, i, j, giornoInizio, meseInizio, giornoFine, meseFine);
  //scrittura prenotazione su file
  fd = fopen("prenotazioni.txt", "a");
  if( fd==NULL ) {
    perror("Errore in apertura del file");
    exit(1);
  }
  else{
    fprintf(fd, "%d %d %d/%d %d/%d\n", spiaggia[i][j].numero, stato, giornoInizio, meseInizio, giornoFine, meseFine);
  }
  fclose(fd);
}

///////////////////////////////////////////////////////////////////////////////

void cambioStatoOmbrellone(Ombrellone spiaggia [M][N], int stato, int i, int j, int giornoInizio, int meseInizio, int giornoFine, int meseFine){
  int k;
  //prenotazione con partenza a giugno
  if(meseInizio == 6){
    if(meseFine == 6){ //se la prenotazione inizia e finisce a giugno
      for(k=giornoInizio-1; k<giornoFine-1; k++)
        spiaggia[i][j].giugno[k] = stato;
    }
    if (meseFine == 7){ //se la prenotazione inizia a giugno e finisce a luglio
      for(k=giornoInizio-1; k<30; k++)
        spiaggia[i][j].giugno[k] = stato;
      for(k=0; k<giornoFine-1; k++)
        spiaggia[i][j].luglio[k] = stato;
    }
    if (meseFine == 8){ //se la prenotazione inizia a giugno e finisce a agosto
      for(k=giornoInizio-1; k<30; k++)
        spiaggia[i][j].giugno[k] = stato;
      for(k=0; k<31; k++)
        spiaggia[i][j].luglio[k] = stato;
      for(k=0; k<giornoFine-1; k++)
        spiaggia[i][j].agosto[k] = stato;
    }
  } //giugno
  //prenotazione con partenza a luglio
  if(meseInizio == 7){
    if(meseFine == 7){ //se la prenotazione inizia e finisce a luglio
      for(k=giornoInizio-1; k<giornoFine-1; k++)
        spiaggia[i][j].luglio[k] = stato;
    }
    if(meseFine == 8){  //se la prenotazione inizia a luglio e finisce a agosto
      for(k=giornoInizio-1; k<31; k++)
        spiaggia[i][j].luglio[k] = stato;
    for(k=0; k<giornoFine-1; k++)
      spiaggia[i][j].agosto[k] = stato;
    }
  } //luglio
  //prenotazione con partenza e fine a agosto
  if((meseInizio == 8) && (meseFine == 8)){
    for(k=giornoInizio-1; k<giornoFine-1; k++)
    spiaggia[i][j].agosto[k] = stato;
  }//agosto
} //stato

///////////////////////////////////////////////////////////////////////////////

int eliminazionePrenotazione(int numeroOmbrellone, char* dataInizio, char* dataFine){
  FILE *fd;
  int ombrelloneLetto, statoLetto, flag;
  char dataInizioLetta [11];
  char dataFineLetta [11];

  flag = 0; //flag=0 -> prenotazione da cancellare NON trovata flag=1 -> prenotazione da cancellare trovata
  //creazione lista prenotazioni
  Lista l;
  l.testa = NULL;

  //apertura del file in lettura e salvataggio di tutte le prenotazioni in una lista di prenotazioni
  //tutte le prenotazioni eccetto quella da eliminare verranno poi sovrascritte nel file riaprendolo in modalità w
  fd = fopen("prenotazioni.txt", "r");
  if( fd==NULL ) {
    perror("Errore in apertura del file");
    exit(1);
  }
  else{
    while(!feof(fd)){
        fscanf(fd, "%d %d %s %s\n", &ombrelloneLetto, &statoLetto, dataInizioLetta, dataFineLetta);
        //aggiunta dell'elemento nella lista
        //creazione nodo
         Nodo *nuovonodo=malloc(sizeof(Nodo));
         nuovonodo->p.numeroOmbrellone=ombrelloneLetto;
         nuovonodo->p.stato=statoLetto;
         strcpy(nuovonodo->p.dataInizio, dataInizioLetta);
         strcpy(nuovonodo->p.dataFine, dataFineLetta);
         nuovonodo->next=NULL;
         //aggiunta nodo alla lista
         nuovonodo->next=l.testa;
         l.testa=nuovonodo;
    }
    fclose(fd);
    
    //riapertura file per la sovrascrittura
    fd = fopen("prenotazioni.txt", "w");
    if( fd==NULL ) {
        perror("Errore in apertura del file");
        exit(1);
    }
    else{
        //scorrimento lista per la scrittura su file
        Nodo *tempnodo=l.testa;

        while (tempnodo){
            if ((tempnodo->p.numeroOmbrellone != numeroOmbrellone) || (strcmp(tempnodo->p.dataInizio, dataInizio) != 0) || (strcmp(tempnodo->p.dataFine, dataFine) != 0)){
            //se il nodo che sto guardando in questo momento NON è quello da eliminare lo copio nel file
                fprintf(fd, "%d %d %s %s\n", tempnodo->p.numeroOmbrellone, 1, tempnodo->p.dataInizio, tempnodo->p.dataFine);
                tempnodo=tempnodo->next;
            }
            else{
            //se il nodo E' quello da eliminare lo salto
                tempnodo=tempnodo->next;
                flag = 1;
            }
        }
        fclose(fd);
    } //fine else sovrascrittura
  } //fine else
  return flag;
}
