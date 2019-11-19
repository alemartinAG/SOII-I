#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h> 
#include <stdbool.h>
#include <string.h>
#include <zconf.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>


#define QUEUE 1	//tamaño maximo de la cola de conexiones pendientes
#define PORTN 27815

#define PASS "123" //contraseña
#define USER "admin" //nombre de usuario
#define TRIES 3 //cantidad de intentos de identicacion

#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define CRESET  "\x1b[0m"
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"

#define CLEAR "\033[H\033[J" //borrar consola

#define ERROR 1 //codigo de error

/* numero de comando */
#define UPDATE 1
#define TELE 2
#define SCAN 3
#define EXIT 4

#define SIZE 256 //tamaño del buffer
#define MTU 1448

bool registro(char[], char[]);
void identificar();
int checkCommand(char[]);
void updateFirmware();
char * leerArchivo(char[]);
void conectarSocket(char[]);
void getTelemetria();
void getImagenSatelital();

bool enviarDato(char[], bool);
bool leerDato(char *, int, bool);

int socketFileDescr, newSockFd;
size_t fileReadSz;

const char *commands[4] = { "update firmware.bin", 
                            "obtener telemetria", 
                            "start scanning",
                            "exit"
                        };

int main(int argc, char *argv[]){

    //Manejo manualmente un error en el pipe
    signal(SIGPIPE, SIG_IGN);

    //Chequeo los argumentos
    if(argc != 2) {
        printf("Uso: %s <puerto>\n", argv[0]);
        exit(ERROR);
    }

    //Chequeo ingreso de usuario y contraseña
    identificar();

    char argumento[SIZE];
    strcpy(argumento, argv[1]);

    //Creo el socket y espero conexión
    conectarSocket(argumento);


    char buffer[SIZE];

    /* Loop principal */

    while(1){

        memset(buffer, 0, sizeof(buffer)); //Limpio el buffer

        //prompt espera el ingreso de un comando
        printf(MAGENTA"%s"CYAN"@EstacionTerrena:"CRESET"~$ ", USER);
        fgets(buffer, SIZE-1, stdin);

        buffer[strlen(buffer)-1] = '\0';

        //verifico que el comando ingresado exista
        int num = checkCommand(buffer);

        memset(buffer, 0, sizeof(buffer)); //Limpio el buffer
        sprintf(buffer, "%d", num); //parseo el numero de comando

        //Si el comando existe lo envio
        if(num >= 0){

            if(!enviarDato(buffer, false)){
                perror("EL CLIENTE SE HA DESCONECTADO");
                close(newSockFd);
                close(socketFileDescr);
                conectarSocket(argumento);
            }

            // Exit: termino la ejecucion del programa
            if(num == EXIT){
                close(newSockFd);
                close(socketFileDescr);
                exit(0);
            }

            // Update Firmware.bin
            if(num == UPDATE){
                updateFirmware();
                usleep(5000);
                conectarSocket(argumento);
            }

            // Obtener Telemetria
            if(num == TELE){
                getTelemetria();
            }

            // Start Scanning
            if(num == SCAN){
                getImagenSatelital();
            }

        }

    }
}

// Si no es el primer comando lee basura del buffer?

void getImagenSatelital(){

    /* Obtiene todas las partes de la imagen satelital
        en base 64, las concatena y recupera la imagen */
    int TAM = 1500;
    //char buffer[TAM];
    char * buffer;
    buffer = calloc(TAM, sizeof(char));
    FILE *fp = NULL;

    //Reloj
    //clock_t start, end;
    //double cpu_time_used;

    char start[64];
    char end[64];
    //int sh, sm, ss;
    //int eh, em, es;

    system("date +\"%T\" > time.txt");
    fp = fopen("time.txt", "r");
    fread(start, 1, 64, fp);
    fclose(fp);

    /* Recibo y seteo la cantidad de bytes que voy a recibir */
    leerDato(buffer, TAM-1, true);

    int bytesTotales = atoi(buffer);
    //printf("bytesTotales = %d - %s\n", bytesTotales, start);
    printf("%s\n", start);
    int bytesLeidos = 0;

    // Envio confirmacion
    enviarDato("OK", false);

    /* Recibo los fragmentos de la imagen codificada */
    printf("Obteniendo Imagen Satelital\n");
    //start = clock();

    
    fp = fopen("Recibido/imagenB64", "a");

    while(bytesLeidos < bytesTotales){
   
        memset(buffer, 0, TAM-1);

        leerDato(buffer, TAM-1, false);

        bytesLeidos += strlen(buffer);
        fwrite(buffer, 1, strlen(buffer), fp);

    }

    fclose(fp);
    free(buffer);

    //end = clock();
    //cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

    system("date +\"%T\" > time.txt");
    fp = fopen("time.txt", "r");
    fread(end, 1, 64, fp);
    fclose(fp);

    printf("%s\n", end);

    //printf("Imagen Obtenida en %f segundos\n", end);

    printf("Procesando Imagen...\n");

    system("./generarImagen.sh");

}

void updateFirmware(){

    char buffer[SIZE];
    //char *bufferUpdate = leerArchivo("client_cc.c");
    char *bufferUpdate = leerArchivo("update-raspbian");
    
    /* Paso cantidad de bytes a enviar */
    //sprintf(buffer, "%lu", strlen(bufferUpdate));
    sprintf(buffer, "%lu", fileReadSz*sizeof(int));
    printf("buffer tam: %s\n", buffer);
    enviarDato(buffer, false);

    /* Espero ok del cliente */
    leerDato(buffer, SIZE, false);

    /* Envio archivo en secciones de igual tamaño */
    //int cantmens = (strlen(bufferUpdate)/MTU)+1;

    if(write(newSockFd, bufferUpdate, fileReadSz*sizeof(int)) < 0){
        perror("escritura de socket");
        exit(ERROR);
    }

    free(bufferUpdate);
    close(socketFileDescr);

}

char * leerArchivo(char file[]){

    /* Lee un archivo de forma completa */

    FILE *fp;
    unsigned long TAM;


    //fp = fopen(file, "r"); // modo de lectura
    fp = fopen(file, "rb"); // modo de lectura

    if (fp == NULL){
        perror("ERROR abriendo archivo\n");
        exit(ERROR);
    }

    //TAM = sizeof(char) * 50000; //max de 50Kb
    TAM = sizeof(int) * 50000; //max de 50Kb

    char * buffer = malloc(TAM);
    //size_t bytes_read;


    //bytes_read = fread(buffer, 1, TAM-1, fp);
    fileReadSz = fread(buffer, sizeof(int), TAM-1, fp);
    printf("bytes_read: %lu\n", fileReadSz);
    
    fclose(fp);

    return buffer;
}

int checkCommand(char reading[]){

    /* Compara un string con los comandos predefinidos */

    size_t cant = sizeof(commands)/sizeof(commands[0]);

    for(int i=1; i<=cant; i++){
        if(!strcmp(reading, commands[i-1])){
            return i;
        }
    }

    printf("Comando '%s' no encontrado\n", reading);

    return -1;
}

bool registro(char print[], char comp[]){

    /* Compara un ingreso por consola contra comp[] */

    char input[SIZE];

    printf("\n%s: ", print);
    fgets(input, SIZE, stdin);
    input[strlen(input)-1] = '\0';

    if(!strcmp(input, comp)){
        return true;
    }

    return false;
}


void identificar(){

    /* Gestiona el ingreso al servidor (3 intentos max)*/

    int intentos = 0;
    bool identified = false;

    while(!identified){

        bool existing = registro("USER", USER);

        if(registro("PASS", PASS) && existing){
            identified = true;
            printf(GREEN"\n***Autentificado correctamente***\n\n"CRESET);
        }
        else{

            intentos++;

            if(intentos == TRIES){
                perror("\nERROR en la identificacion (Demasiados Intentos)\n");
                exit(ERROR);
            }

            printf(RED"\nNombre de usuario y/o contraseña incorrecto, quedan %d intentos\n"CRESET, 3-intentos);
        }

    }
}

void conectarSocket(char argv[]){

    /* Gestiona creación y conexión del socket tcp */

    int puerto; 
    unsigned int cl_len;
    struct sockaddr_in sv_addr, cl_addr;

    printf("\n-- Esperando la conexion con el satelite --\n");

    socketFileDescr = socket(AF_INET, SOCK_STREAM, 0);
    int on = 1;
    setsockopt(socketFileDescr, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    if(socketFileDescr < 0){
        perror("\nERROR abriendo el socket");
        exit(ERROR);
    }

    bzero((char *) &sv_addr, sizeof(sv_addr)); //Limpio la direccion del sv

    puerto = atoi(argv);

    sv_addr.sin_family = AF_INET;
    sv_addr.sin_addr.s_addr = INADDR_ANY;
    sv_addr.sin_port = htons(puerto);

    if(bind(socketFileDescr, (struct sockaddr*)&sv_addr, sizeof(sv_addr)) < 0){
        perror("\nERROR en el binding");
        exit(ERROR);
    }

    //acepto conexiones
    listen(socketFileDescr, QUEUE);

    cl_len = sizeof(cl_addr);

    newSockFd = accept(socketFileDescr, (struct sockaddr*)&cl_addr, &cl_len);

    if(newSockFd < 0){
        perror("\nERROR en accept");
        exit(ERROR);
    }

    printf("\n--- Conexion Exitosa ---\n");

    sleep(1);
    printf(CLEAR);

}

void getTelemetria(){

    /* Se encarga de recibir información del 
        satelite a traves de un socket udp */

    int socket_udp, resultado;
    struct sockaddr_in struct_servidor;
    socklen_t tamano_direccion;
    char buffer[SIZE] = {""};

    //numero de puerto que utiliza
    int puerto = PORTN;

    memset(buffer, '\0', SIZE);

    /* Creacion de socket UDP*/
    if((socket_udp = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("ERROR AL CREAR EL SOCKET UDP");
        return;
    }

    /* Inicialización y establecimiento de la estructura del servidor */
    memset(&struct_servidor, 0, sizeof(struct_servidor));
    struct_servidor.sin_family = AF_INET;
    struct_servidor.sin_addr.s_addr = INADDR_ANY;
    struct_servidor.sin_port = htons(puerto);
    memset( &(struct_servidor.sin_zero), '\0', 8 );

    /* Ligadura del socket de servidor a una dirección */
    if((bind(socket_udp, (struct sockaddr *)&struct_servidor, sizeof(struct_servidor))) < 0 ) {
        perror("bind");
    }

    /*Recepción y procesamiento de mensaje*/

    tamano_direccion = sizeof(struct sockaddr);

    resultado = recvfrom (socket_udp, buffer, SIZE, 0, (struct sockaddr *) &struct_servidor, &tamano_direccion);
    if(resultado < 0) {
        perror("recepción");
        return;
    }
    else{

        //recibo toda la info en un solo mensaje con separadores
        char *token;
        token = strtok(buffer, ";");

        while(token != NULL) {
            printf("- %s\n", token);
            token = strtok(NULL, ";");
        }

    }

    if(close(socket_udp) < 0){
        perror("ERROR CERRANDO SOCKET UDP");
    }

}

bool enviarDato(char dato[], bool terminate){
    /* Envia un mensaje a traves del socket */
    if(write(newSockFd, dato, strlen(dato)) < 0){
        perror("escritura de socket");
        
        if(terminate){
            exit(ERROR);
        }

        return false;
    }

    return true;
}

bool leerDato(char * buffer, int size, bool terminate){
    /* Recibe mensaje a traves del socket */
    if(read(newSockFd, buffer, size) < 0){
        perror("lectura de socket");
        
        if(terminate){
            exit(ERROR);
        }

        return false;
    }

    return true;
}
