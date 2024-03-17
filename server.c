#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <sys/time.h>
#include <semaphore.h>
#include <pthread.h>
#define MAX_LENGTH 256
#define DATABASE_I "intrebari.db"
#define DATABASE_U "users.db"

pthread_mutex_t nr_clienti_mutex = PTHREAD_MUTEX_INITIALIZER;  // Mutex pentru a proteja accesul la nr_clienti

/* portul folosit */
#define PORT 2909

/* codul de eroare returnat de anumite apeluri */
extern int errno;

int id_i=1, nr_clienti, terminat, trimis;
char mesaj_intrare[100];
char mesaj_sql_nou[MAX_LENGTH]={};
char mesaj_sql[MAX_LENGTH]={}, mesaj_sql_2[MAX_LENGTH];
char raspuns[MAX_LENGTH]=" Castigatorii sunt: \n", destinatie[MAX_LENGTH][MAX_LENGTH];

void raspunde(void *, int);

typedef struct thData{
	int idThread; //id-ul thread-ului tinut in evidenta de acest program
	int cl; //descriptorul intors de accept
}thData;

static void *treat(void *); /* functia executata de fiecare thread ce realizeaza comunicarea cu clientii */

static int callback(void* data, int argc, char** argv, char** azColName)
{
    for (int i = 0; i < argc; i++)
    {
        snprintf(mesaj_sql_nou + strlen(mesaj_sql_nou), MAX_LENGTH - strlen(mesaj_sql_nou), "%s : %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    strcpy(mesaj_sql, mesaj_sql_nou);
    strcpy(mesaj_sql_nou, " ");
    return 0;
}

static int callback_c(void* data, int argc, char** argv, char** azColName)
{
    int i;
  strcpy(mesaj_sql_2, "");

    for (i = 0; i < argc; i++)
    {
        snprintf(mesaj_sql_2 + strlen(mesaj_sql_2), MAX_LENGTH - strlen(mesaj_sql_2), "%s\n", argv[i] ? argv[i] : "NULL");
        strcpy(destinatie[i], mesaj_sql_2);
        strcat(raspuns, destinatie[i]);
    } 
    return 0;
}

void executare_castigatori()
{
    sqlite3* DB;
    int exit = 0;
    
    exit = sqlite3_open(DATABASE_U, &DB);

    char data[] = "CALLBACK FUNCTION";
    
    if (exit != SQLITE_OK)
    {
        fprintf(stderr, "Error open DB %s\n", sqlite3_errmsg(DB));
    }
    else
        printf("Opened Database Successfully!\n");

    char comanda_sql[500] = "SELECT USERNAME FROM USERS WHERE PUNCTAJ=(SELECT MAX(PUNCTAJ) FROM USERS);";

    int rc = sqlite3_exec(DB, comanda_sql, callback_c, (void*)data, NULL);

    if (rc != SQLITE_OK)
        fprintf(stderr, "Error SELECT\n");
    else
        printf("Operation OK!\n");
    printf("\n%s\n", raspuns);
    sqlite3_close(DB);
}

void executare_sql(char* comanda_sql)
{
  sqlite3* DB;
    int exit = 0;
    //char* mesaj_sql[MAX_LENGTH]={ };
    exit = sqlite3_open(DATABASE_I, &DB);

    char data[] = "CALLBACK FUNCTION";
    
    if (exit != SQLITE_OK)
    {
        fprintf(stderr, "Error open DB %s\n", sqlite3_errmsg(DB));
        //return ("ERROR");
    }
    else
        printf("Opened Database Successfully!\n");

     int rc= sqlite3_exec(DB, comanda_sql, callback, (void*)data, NULL);

    if (rc != SQLITE_OK)
        fprintf(stderr, "Error SELECT\n");
    else
        printf("Operation OK!\n");
    sqlite3_close(DB);
}

void adaugare_users(char * comanda_sql)
{
    sqlite3* DB;
    int exit = 0;
    char* mesajError;
    exit = sqlite3_open(DATABASE_U, &DB);
    
    if (exit != SQLITE_OK)
    {
        fprintf(stderr, "Error open DB %s\n", sqlite3_errmsg(DB));
    }
    else
        printf("Opened Database Successfully!\n");

     int rc= sqlite3_exec(DB, comanda_sql, NULL, 0 , &mesajError);

    if (rc != SQLITE_OK)
        {
          fprintf(stderr, "Error SQL\n");
          sqlite3_free(mesajError);
        }
    else
        printf("Operation OK!\n");
    sqlite3_close(DB);
}

void citire_sql(int id)
{ 
    char citire[500]="SELECT INTREBARE, A, B, C FROM INTREBARI WHERE ID=";
    char nr[10]={ };
    sprintf(nr, "%d", id);
    strcat(citire, nr);
    strcat(citire, ";");
    executare_sql(citire);
}

void citire_raspuns_sql(int id)
{ 
    char citire[500]="SELECT raspuns FROM INTREBARI WHERE ID=";
    char nr[10]={ };
    sprintf(nr, "%d", id);
    strcat(citire, nr);
    strcat(citire, ";");
    executare_sql(citire);
}

void adaugare_sql(int id, char* username)
{
  char adaugare[500]="INSERT INTO USERS VALUES (";
  char nr[10]={ };
  sprintf(nr, "%d", id);
  strcat(adaugare, "'");
  strcat(adaugare, nr);
  strcat(adaugare, "'");
  strcat(adaugare, ", ");
  strcat(adaugare, "'");
  strcat(adaugare, username);
  strcat(adaugare, "'");
  strcat(adaugare, ", ");
  strcat(adaugare, "'");
  strcat(adaugare, "-1");
  strcat(adaugare, "'");
  strcat(adaugare, ");" );
  adaugare_users(adaugare);
}

void update_sql(int id, int punctaj)
{
  char adaugare[500]="UPDATE USERS SET PUNCTAJ=";
  char nr[10]={ }, user_id[10]={ };
  sprintf(nr, "%d", punctaj);
  strcat(adaugare, nr);
  strcat(adaugare, " WHERE ID=");
  sprintf(user_id, "%d", id);
  strcat(adaugare, user_id);
  strcat(adaugare, ";");
  adaugare_users(adaugare);
}

int comparare_raspuns(int id, int scor)
{
  citire_raspuns_sql(id);
  if (strncmp(mesaj_intrare, mesaj_sql, strlen(mesaj_intrare)) == 0 ) //comparam daca e corect
    scor++;//daca da creste scorul
  printf("scor: %d\n", scor);
  return scor;
}

void primire_mesaj(void *arg)
{
  printf("am intrat in primire_mesaj()\n");
	struct thData tdL; 
  bzero(mesaj_intrare,100);
	tdL= *((struct thData*)arg);
	if (read (tdL.cl, &mesaj_intrare,sizeof(mesaj_intrare)) <= 0)
			{
			  printf("[Thread %d]\n",tdL.idThread);
			  perror ("Eroare la read() de la client.\n");
			
			}
      mesaj_intrare[strlen(mesaj_intrare)-1]='\0';
	printf ("[Thread %d]Mesajul receptionat este...%s...\n",tdL.idThread, mesaj_intrare);
}

void raspunde(void *arg, int id)
{
  char intrebare[500]={ };
  citire_sql(id);
  if(strncmp(mesaj_intrare,"EXIT", strlen(mesaj_intrare))==0)
      strcpy(intrebare,"Te-ai deconectat");
  else strcpy(intrebare, mesaj_sql);
  
  strcpy(mesaj_intrare, " ");

	struct thData tdL; 
	tdL= *((struct thData*)arg);
		      /*pregatim mesajul de raspuns */
		      id++;   
	printf("[Thread %d]Trimitem prima intrebare...%s\n",tdL.idThread, intrebare);
		      
		      
		      /* returnam mesajul clientului */
	 if (write (tdL.cl, &intrebare, sizeof(intrebare)) <= 0)
		{
		 printf("[Thread %d] ",tdL.idThread);
		 perror ("[Thread]Eroare la write() catre client.\n");
		}
	else
		printf ("[Thread %d]Mesajul a fost trasmis cu succes.\n",tdL.idThread);	

}

void golim(int id)
{
  char adaugare[MAX_LENGTH], nr[10]={ };
  sprintf(nr, "%d", id);
  strcpy(adaugare, " DELETE FROM USERS WHERE ID=");
  strcat(adaugare, nr);
  strcat(adaugare, ";");
  adaugare_users(adaugare);
}

void golim_tot()
{
  char adaugare[MAX_LENGTH];
  strcpy(adaugare, " DELETE FROM USERS;");
  adaugare_users(adaugare);
}

int am_terminat(int nr)
{
  if(nr==nr_clienti)
    return 0;
}

int castigator(void *arg)
{
  if (strncmp(mesaj_intrare, "EXIT", strlen("EXIT")) == 0) 
  {
    strcpy(raspuns,"Te-ai deconectat!");
    printf("%s\n", raspuns);
  }
  else
  {
    strcpy(raspuns, " Castigatorii sunt: \n");
    if(am_terminat(terminat)==0)
      executare_castigatori();
    else return 1;
  }
  struct thData tdL; 
	tdL= *((struct thData*)arg);
		      /*pregatim mesajul de raspuns */
		       
	printf("[Thread %d]Trimitem Castigatorul\n %s\n",tdL.idThread, raspuns);
		      
		      
		      /* returnam mesajul clientului */
	 if (write (tdL.cl, &raspuns, sizeof(raspuns)) <= 0)
		{
		 printf("[Thread %d] ",tdL.idThread);
		 perror ("[Thread]Eroare la write() catre client.\n");
		}
	else
		printf ("[Thread %d]Mesajul a fost trasmis cu succes.\n",tdL.idThread);	
    fflush (stdout);
    trimis++;
    return 0;
}

int main ()
{
  struct sockaddr_in server;	// structura folosita de server
  struct sockaddr_in from;	
  int nr;		//mesajul primit de trimis la client 
  int sd;		//descriptorul de socket 
  int pid;
  pthread_t th[100];    //Identificatorii thread-urilor care se vor crea
	int i=0;
  int punctaj[100]={-1};
  nr_clienti=0; terminat =0, trimis=0;

  /* crearea unui socket */
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("[server]Eroare la socket().\n");
      return errno;
    }
  /* utilizarea optiunii SO_REUSEADDR */
  int on=1;
  setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
  
  /* pregatirea structurilor de date */
  bzero (&server, sizeof (server));
  bzero (&from, sizeof (from));
  
  /* umplem structura folosita de server */
  /* stabilirea familiei de socket-uri */
    server.sin_family = AF_INET;	
  /* acceptam orice adresa */
    server.sin_addr.s_addr = htonl (INADDR_ANY);
  /* utilizam un port utilizator */
    server.sin_port = htons (PORT);
  
  /* atasam socketul */
  if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
    {
      perror ("[server]Eroare la bind().\n");
      return errno;
    }

  /* punem serverul sa asculte daca vin clienti sa se conecteze */
  if (listen (sd, 2) == -1)
    {
      perror ("[server]Eroare la listen().\n");
      return errno;
    }

  /* servim in mod concurent clientii...folosind thread-uri */
  while (1)
  {
      int client, id_cl;
      thData * td; //parametru functia executata de thread     
      int length = sizeof (from);

      printf ("[server]Asteptam la portul %d...\n",PORT);
      fflush (stdout);

      /* acceptam un client (stare blocanta pina la realizarea conexiunii) */
      if ( (client = accept (sd, (struct sockaddr *) &from, &length)) < 0)
      {
        perror ("[server]Eroare la accept().\n");
        continue;
      }
	
    nr_clienti++;
    int idThread; //id-ul threadului
    td=(struct thData*)malloc(sizeof(struct thData));	
    td->idThread=i++;
    //printf("idThread= %d \n", idThread);
    td->cl=client;
    pthread_create(&th[i], NULL, &treat, td);	   

	}//while    
};				

static void *treat(void * arg)
{		
    int nr=0, max=3;
    struct thData tdL; 
    tdL= *((struct thData*)arg);
    printf ("[Thread %d ] Asteptam mesajul...\n", tdL.idThread);
    fflush (stdout);		 
    pthread_detach(pthread_self());	
    
    //Primirea mesajului si adaugarea lui in tabela users
    primire_mesaj((struct thData*)arg); //mesaj primit
    adaugare_sql(tdL.idThread, mesaj_intrare);
    strcpy(mesaj_intrare, " ");
    //pornirea trimiterii de intrebari
    for(int i=id_i; i<=max; i++)
    {
      raspunde((struct thData*)arg, i);//raspundem
      fflush(stdout);
      primire_mesaj((struct thData*)arg);
      printf("\n%s\n", mesaj_intrare);

      //verificare daca se doreste parasirea jocului
      if (strncmp(mesaj_intrare, "EXIT", 4) == 0) 
      {
        //stergem user-ul din tabela users
        i=max+1;
        nr_clienti--;
        golim(tdL.idThread);
      } 
      else //continuam
      {
        char lala[500]={ };
        strcpy(lala, " raspuns : ");
        strcat(lala, mesaj_intrare);
        strcpy(mesaj_intrare, lala);
        nr = comparare_raspuns(i, nr);
        printf("thread:%d, punctaj:%d", tdL.idThread, nr);
      }
      id_i++;
    }
      if (strncmp(mesaj_intrare, "EXIT", 4) != 0) 
      {
        update_sql(tdL.idThread, nr);
      } 
    terminat++;

    while(1)
    {
      if(castigator((struct thData*)arg))
        sleep(2);
      else break;
    }
    if(strncmp(mesaj_intrare, "EXIT", 4) != 0)
    while(1)
    {
      if(trimis==nr_clienti)
      {
        golim_tot();
        trimis--; nr_clienti--;
        break;
      }
      else
      sleep(1);
    }

    printf ("[server]S-a terminat jocul. Ai luat %d din %d\n", nr, id_i);
    id_i=1;

		/* am terminat cu acest client, inchidem conexiunea */
		close ((intptr_t)arg);
    terminat--;
		return(NULL);	
};
