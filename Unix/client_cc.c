/* Cliente en el dominio Unix - orientado a corrientes */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

#define SIZE 80 //tamaño de buffer
#define ERROR 1 //codigo de error para exit

void scan();
void telemetria();
void update();
void enviarDato(char[]);
char * getUptime();

int SAT_ID;
char VERSION[] = {"1"};
int socketFileDescr;

int main(int argc, char *argv[]){

	/* genero un numero 
	aleatorio para el id 
	del satelite */
	srand(time(NULL));
	SAT_ID = rand();
	printf("ID %d\n", SAT_ID);

	//int socketFileDescr, sv_len;
	int sv_len;

	struct sockaddr_un sv_addr;

	char buffer[SIZE];

	int terminar = 0;

    if(argc < 2){
        fprintf(stderr, "Uso: %s archivo\n", argv[0]);
        exit(0);
    }

	memset((char *)&sv_addr, '\0', sizeof(sv_addr));
	sv_addr.sun_family = AF_UNIX;
	strcpy(sv_addr.sun_path, argv[1]);
	sv_len = strlen(sv_addr.sun_path) + sizeof(sv_addr.sun_family);

	if((socketFileDescr = socket(AF_UNIX, SOCK_STREAM, 0)) < 0){
		perror("creación de socket");
		exit(ERROR);
	}

	//Intento conectar al socket constantemente
	while(1){
		if(connect(socketFileDescr, (struct sockaddr *)&sv_addr, sv_len) >= 0){
			printf("\n-- Conectado --\n\n");
			break;
		}
	}
 	

	while(1){

        memset(buffer, '\0', SIZE);

        if(read(socketFileDescr, buffer, SIZE) < 0){
            perror("lectura de socket");
            exit(ERROR);
        }

  		switch(atoi(buffer)){
  			
  			case 0 :
  			update();
  			break;

  			case 1 :
  			telemetria();

  			case 2 :
  			scan();

  			default : 
  			break;
  		}
	}

	return 0;
}

void scan(){

}

void update(){

	enviarDato("FIN");

}

void telemetria(){

	char id[sizeof(SAT_ID)];
	memset(id, '\0', sizeof(id));
	sprintf(id, "%d", SAT_ID);

	char catid[SIZE] = "ID: ";
	strcat(catid, id);
	enviarDato(catid); //envio el id del satelite

	char catver[SIZE] = "SOFTWARE VERSION: ";
	strcat(catver, VERSION);
	enviarDato(catver);

	enviarDato("FIN");



}

void enviarDato(char dato[]){

	if(write(socketFileDescr, dato, strlen(dato)) < 0){
        perror("escritura de socket");
        exit(ERROR);
    }

    //TODO: Usar separadores de mensajes
    usleep(1000);

}
