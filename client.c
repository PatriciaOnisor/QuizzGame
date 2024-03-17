#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <arpa/inet.h>
/* codul de eroare returnat de anumite apeluri */
extern int errno;

/* portul de conectare la server*/
int port;
char nume[100];
char buf[100];

void semnal()
{
  printf("\n daca vrei sa iesi scrie <EXIT>\n");
};

int timp() {
    fd_set readfds;
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    int result = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);

    if (result == -1) {
        perror("Eroare la select");
        return -1;
    } else if (result > 0) {
        read (0, buf, sizeof(buf));
    } else {
        strcpy(buf, "pzskv");
        printf("Nu ai introdus nimic in timp util.\n");
    }
    return 0;
}

int main (int argc, char *argv[])
{
  signal(SIGINT, semnal);
  signal(SIGQUIT,semnal);
  signal(SIGTERM, semnal);
  signal(SIGTSTP, semnal);

  int sd;			// descriptorul de socket
  struct sockaddr_in server;	// structura folosita pentru conectare 
  		// mesajul trimis
  

  /* exista toate argumentele in linia de comanda? */
  if (argc != 3)
    {
      printf ("Sintaxa: %s <0> <2909>\n", argv[0]);
      return -1;
    }

  /* stabilim portul */
  port = atoi (argv[2]);

  /* cream socketul */
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("Eroare la socket().\n");
      return errno;
    }

  /* umplem structura folosita pentru realizarea conexiunii cu serverul */
  /* familia socket-ului */
  server.sin_family = AF_INET;
  /* adresa IP a serverului */
  server.sin_addr.s_addr = inet_addr(argv[1]);
  /* portul de conectare */
  server.sin_port = htons (port);
  
  /* ne conectam la server */
  if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    {
      perror ("[client]Eroare la connect().\n");
      return errno;
    }

/* citirea mesajului */
    printf ("[client]Introduceti un nume: ");
    fflush (stdout);
    read (0, nume, sizeof(nume));

    /* trimiterea mesajului la server */
    if (write (sd,nume,sizeof(nume)) <= 0)
      {
        perror ("[client]Eroare la write() spre server.\n");
        return errno;
      }
      fflush (stdout);

  while(1)
  {
      char intrebare[500];
    /* citirea raspunsului dat de server (apel blocant pana cand serverul raspunde) */
    if (read (sd, &intrebare, 500) < 0)
      {
        perror ("[client]Eroare la read() de la server.\n");
        return errno;
      }
      if(strncmp(intrebare, " Castigatorii sunt:", strlen(" Castigatorii sunt:")) == 0) 
      {
        printf("Joc terminat!%s\n", intrebare);
        break;
      }
      else if (strncmp(intrebare, "Te-ai deconectat!", strlen("Te-ai deconectat!")) == 0) 
      {
        printf("Ai decis sa te deconectezi.\n");
        break;
      }
      else // afisam mesajul primit 
      {  printf ("%s\n", intrebare);

        /* citirea mesajului de la tastatura*/
        printf ("Introduceti raspunsul cu litera MARE: ");
        fflush (stdout);
        timp();
        printf (" \n");


        /* trimiterea mesajului la server */
        if (write (sd,&buf,sizeof(buf)) <= 0)
        {
          perror ("[client]Eroare la write() spre server.\n");
          return errno;
        }
      }
  }
  
  /* inchidem conexiunea, am terminat */
  close (sd);
  return EXIT_SUCCESS;
}

