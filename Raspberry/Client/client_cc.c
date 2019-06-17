#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>

#define SIZE 800 //buffSize de buffer
#define ERROR 1 //codigo de error para exit
#define CLEAR "\033[H\033[J" //borrar consola
#define SATID "x01121sat"

void scan();
void telemetria();
void update();
void enviarDato(char[]);
int leerDato(char *, int, bool);
char * getUptime();

//int SAT_ID;
char VERSION[] = {"2"};
char serverIP[16];
int socketFileDescr;

int main(int argc, char *argv[]){

	//Manejo manualmente un error en el pipe
	signal(SIGPIPE, SIG_IGN);


	/* Chequeo los argumentos */
	if(argc < 3){
        printf("Uso: %s <host> <puerto>\n", argv[0]);
        exit(0);
    }

	/* Conexion de Socket de Internet TCP */

	int puerto;
	struct sockaddr_in sv_addr;
	struct hostent *server;

	// Numero de puerto
    puerto = atoi(argv[2]);
    // Ip del server
    strcpy(serverIP, argv[1]);

    if((socketFileDescr = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("creación de socket");
		exit(ERROR);
	}

    server = gethostbyname(serverIP);
	if (server == NULL) {
		perror("Error, no existe el host");
		exit(ERROR);
	}

	memset((char *)&sv_addr, '\0', sizeof(sv_addr));
	sv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&sv_addr.sin_addr.s_addr, server->h_length);
	sv_addr.sin_port = htons(puerto);

	//Intento conectar al socket hasta conseguirlo
	while(1){
		if(connect(socketFileDescr, (struct sockaddr *)&sv_addr, sizeof(sv_addr)) >= 0){
			printf(CLEAR);
			printf("\n-- Conectado --\n\n");
			break;
		}
	}

	
	/* Recibo comandos */

	char buffer[SIZE];
 	
	while(1){

        memset(buffer, '\0', SIZE);

        leerDato(buffer, SIZE, false);

  		switch(atoi(buffer)){
  			
  			case 0 :
  			exit(0);

  			case 1 :
  			update();
  			break;

  			case 2 :
  			telemetria();
  			break;

  			case 3 :
  			scan();
  			break;

  			case 4 :
  			exit(0);

  			default :
  			break;
  		}
	}

	return 0;
}

void scan(){

	/* Envia todos los archivos en los que
		fue dividida la imagen luego de ser
		codificada en base 64 */

	int TAM = 1448;
	int i = 0;
	char * buffer;
	buffer = (char *) malloc(TAM);
	
	FILE *fp;

	/* Leo y envío el tamaño total de la imagen codificada */ 
	system("stat -c \"%s\" Image/ImgB64 > count.txt");
	fp = fopen("count.txt", "r");
	fread(buffer, sizeof(char), TAM, fp);
	fclose(fp);
	enviarDato(buffer);

	//Espero confirmacion del servidor
	leerDato(buffer, TAM, false);

	/* Comienza envio del archivo */
	while(1){
		
		//el nombre de los archivos incrementa numericamente
		char *filename;
		filename = (char *) calloc(sizeof(char),20);
		strcpy(filename, "A");
		snprintf(filename, 19, "Image/x%06d", i);

		fp = fopen (filename, "r");
		free(filename);  		

  		if (fp!=NULL)
  		{
  			memset(buffer, 0, TAM);
		    fread(buffer, 1, TAM, fp);
			
			fclose(fp);
		
		    enviarDato(buffer);
  		}
  		else{
  			//Si el archivo no se encuentra, termina
  			printf("FIN DE ENVIO\n");
  			break;
  		}

  		i++;
	}
	
	free(buffer);

}

void update(){

	/* Recibe el archivo binario de la actualizacion,
		reemplaza el propio, y se reinicia */

	printf("-UPDATE-\n");

	int TAM = 1500;
	char buffer[TAM];

	/* Recibe el tamaño del archivo */
	leerDato(buffer, TAM, true);
	int bytesTotales = atoi(buffer);

	enviarDato("OK");

	FILE *fp;
	//fp = fopen("client_update.c", "a");
	fp = fopen("update", "a");

	int index = 1;
	int bytesLeidos = 0;

	while(bytesLeidos != bytesTotales){

		memset(buffer, '\0', TAM);
    	int leido = leerDato(buffer, TAM, true);

    	bytesLeidos += leido;
    	//bytesLeidos += strlen(buffer);
    	printf("buffer: %d / %d\n", bytesLeidos, bytesTotales);

    	//fwrite(buffer, 1, strlen(buffer), fp);
    	fwrite(buffer, 1, leido, fp);

    	index++;

	}

	fclose(fp);

	printf("REBOOTING...\n");
	system("./restart.sh");

}

void telemetria(){

	/*---Conexión UDP---*/

	int descriptor_udp, resultado, cantidad, puerto;
	struct sockaddr_in struct_cliente;
	char buffer[SIZE];
	struct hostent *server;

	server = gethostbyname(serverIP);
	if ( server == NULL ) {
		printf("ERROR, no existe el host\n");
		return;
	}

	usleep(1000);

	puerto = 27815;

	/* Creacion de socket */
	if((descriptor_udp = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		perror("socket" );
		return;
	}

	/* Inicialización y establecimiento de la estructura del cliente */
	struct_cliente.sin_family = AF_INET;
	struct_cliente.sin_port = htons(puerto);
	struct_cliente.sin_addr = *( (struct in_addr *)server->h_addr );
	memset( &(struct_cliente.sin_zero), '\0', 8 );


	/*-- Leo uptime del sistema --*/

	FILE *fp = NULL;
    char buff_uptime[SIZE] = {""};
    size_t bytes_read;

    fp = fopen("/proc/uptime", "r");
    bytes_read = fread(buff_uptime, 1, sizeof(buff_uptime), fp);
    fclose(fp);

    if(bytes_read == 0 || bytes_read == sizeof(buff_uptime)){
        printf("buffer problem");
    }

	long uptime;
	char *ptr;
	uptime = strtol(buff_uptime, &ptr, 10);

	memset(buff_uptime, '\0', SIZE-1);
    sprintf(buff_uptime, "Uptime: %02ldD %02ld:%02ld:%02ld", (uptime / 60 / 60 / 24), (uptime / 60 / 60 % 24), (uptime / 60 % 60), (uptime % 60));


    /*Obtengo memmory y cpu ussage*/

    char buffCPU[SIZE] = {""};
    system("./getstats.sh");
    fp = fopen("cpumem.txt", "r");
    fread(buffCPU, 1, sizeof(buffCPU), fp);
    fclose(fp);
    system("rm cpumem.txt");

	char catid[SIZE] = {"ID: "};
	strcat(catid, SATID);

	/*Version de Firmware*/

	char catver[SIZE] = "SOFTWARE VERSION: ";
	strcat(catver, VERSION);

    
    /*---Message Parsing---*/

	memset(buffer, 0, sizeof(buffer)); //Limpio el buffer
	strcpy(buffer, catid);
	strcat(buffer, ";");
	strcat(buffer, catver);
	strcat(buffer, ";");
	strcat(buffer, buff_uptime);
	strcat(buffer, ";");
	strcat(buffer, buffCPU);

	cantidad = strlen(buffer);

	/* Envío de datagrama al servidor */
	resultado = sendto(descriptor_udp, buffer, cantidad, 0, (struct sockaddr *)&struct_cliente, sizeof(struct_cliente));
	if(resultado < 0) {
 		perror("ERROR EN ENVIO UDP");
 		return;
	}
	
}

void enviarDato(char dato[]){
	/* Envia un mensaje a traves del socket */
	if(write(socketFileDescr, dato, strlen(dato)) < 0){
        perror("escritura de socket");
        exit(ERROR);
    }
}

int leerDato(char * buffer, int size, bool terminate){
	/* Recibe mensaje a traves del socket */
	int leido = 0;
	leido = read(socketFileDescr, buffer, size);
	if(leido < 0){
		perror("lectura de socket");
		
		if(terminate){
			exit(ERROR);
		}
	}

	return leido;
}