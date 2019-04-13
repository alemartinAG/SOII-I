#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#define SIZE 800 //buffSize de buffer
#define ERROR 1 //codigo de error para exit
#define CLEAR "\033[H\033[J" //borrar consola

void scan();
void telemetria();
void update();
void enviarDato(char[]);
char * getUptime();

int SAT_ID;
char VERSION[] = {"3"};
int socketFileDescr;

int main(int argc, char *argv[]){

	//Manejo manualmente un error en el pipe
	signal(SIGPIPE, SIG_IGN);


	/* Chequeo los argumentos */
	if(argc < 2){
        fprintf(stderr, "Uso: %s archivo\n", argv[0]);
        exit(0);
    }

	/* genero un numero
	aleatorio para el id
	del satelite */
	srand(time(NULL));
	SAT_ID = rand();


	/* Conexion de Socket UNIX TCP */

	int sv_len;
	struct sockaddr_un sv_addr;

	memset((char *)&sv_addr, '\0', sizeof(sv_addr));
	sv_addr.sun_family = AF_UNIX;
	strcpy(sv_addr.sun_path, argv[1]);
	sv_len = strlen(sv_addr.sun_path) + sizeof(sv_addr.sun_family);

	if((socketFileDescr = socket(AF_UNIX, SOCK_STREAM, 0)) < 0){
		perror("creación de socket");
		exit(ERROR);
	}

	//Intento conectar al socket hasta conseguirlo
	while(1){
		if(connect(socketFileDescr, (struct sockaddr *)&sv_addr, sv_len) >= 0){
    		printf(CLEAR);
			printf("\n-- Conectado --\n\n");
			break;
		}
	}


	/* Recibo comandos */
 	
	char buffer[SIZE];

	while(1){

        memset(buffer, '\0', SIZE);

        if(read(socketFileDescr, buffer, SIZE) < 0){
            perror("lectura de socket");
            exit(ERROR);
        }

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
  			printf("RECIBO: %s\n", buffer);
  		}
	}

	return 0;
}

void scan(){

	/* Envia todos los archivos en los que
	fue dividida la imagen luego de ser
	convertida a base 64 */

	int TAM = 1500;
	FILE * fp;
	int i = 0;
	char buffer[TAM];
	size_t bytes_read;

	while(1){
		
		//el nombre de los archivos incrementa numericamente
		char filename[8] = {""};
		sprintf(filename, "x%06d", i);
		char filename2[20] = {"Image/"};
		strcat(filename2, filename);
		fp = fopen (filename2,"r");
  		
  		if (fp!=NULL)
  		{
  			memset(buffer, 0, TAM);
		    bytes_read = fread(buffer, 1, TAM, fp);
		    enviarDato(buffer);
		    fclose (fp);


		    //Espero confirmacion del servidor
			if(read(socketFileDescr, buffer, TAM) < 0){
            	perror("lectura de socket");
            	return;
        	}
  		}
  		else{
  			printf("FIN DE ENVIO\n");
  			enviarDato("FIN");
  			break;
  		}

  		i++;
	}

}

void update(){

	/* Recibe el archivo binario de la actualizacion,
	reemplaza el propio, y se reinicia */

	printf("-UPDATE-\n");

	int TAM = 15000;
	char buffer[TAM];
	char buffSize[sizeof(unsigned long)];
	unsigned long tamBinario;

	//Recibe el archivo
	if(read(socketFileDescr, buffer, TAM) < 0){
        perror("lectura de socket");
        exit(ERROR);
    }

    //Recibe el tamaño total del archivo
    if(read(socketFileDescr, buffSize, sizeof(unsigned long)) < 0){
    	perror("lectura de socket");
        exit(ERROR);
    }

    tamBinario = atoi(buffSize);

    FILE *fp;

    //guardo el archivo
    fp = fopen("client_u", "wb");
    fwrite(buffer, 1, tamBinario, fp);
    fclose(fp);

	enviarDato("FIN");

	printf("REBOOTING...\n");
	system("./restart.sh");

}

void telemetria(){

	/*---Conexión UDP---*/

	int descriptor_udp, resultado, cantidad, i;
	struct sockaddr_un struct_cliente;
	socklen_t tamano_direccion;
	char buffer[SIZE];

	usleep(1000);

	char nom_sock[SIZE] = {"UDPsocket"};

	/* Creacion de socket */
	if((descriptor_udp = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0){
		perror("socket" );
	}

	/* Inicialización y establecimiento de la estructura del cliente */
	memset(&struct_cliente, 0, sizeof(struct_cliente));
	struct_cliente.sun_family = AF_UNIX;
	strncpy(struct_cliente.sun_path, nom_sock, sizeof(struct_cliente.sun_path));


	/*-- Leo uptime del sistema --*/
	FILE *fp = NULL;
    char buff_uptime[SIZE] = "";
    size_t bytes_read;
    char data_found[64] = "";

    fp = fopen("/proc/uptime", "r");
    bytes_read = fread(buff_uptime, 1, sizeof(buff_uptime), fp);
    fclose(fp);

    if(bytes_read == 0 || bytes_read == sizeof(buff_uptime)){
        printf("buffer problem");
    }

    buff_uptime[bytes_read] = '\0';

    int uptime;
    sscanf(buff_uptime, "%d", &uptime);
    sprintf(buff_uptime, "Uptime: %02dD %02d:%02d:%02d", (uptime / 60 / 60 / 24), (uptime / 60 / 60 % 24), (uptime / 60 % 60),
           (uptime % 60));


    /*Obtengo memmory y cpu ussage*/

    char buffCPU[SIZE] = {""};
    system("./getstats.sh");
    fp = fopen("cpumem.txt", "r");
    bytes_read = fread(buffCPU, 1, sizeof(buffCPU), fp);
    fclose(fp);
    system("rm cpumem.txt");

    /*Id del satelite*/

    char id[sizeof(SAT_ID)];
	memset(id, '\0', sizeof(id));
	sprintf(id, "%d", SAT_ID);
	char catid[SIZE] = {"ID: "};
	strcat(catid, id);

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
