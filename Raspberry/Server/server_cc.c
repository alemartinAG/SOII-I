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
#define SIZE 256 //tamaño del buffer

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

#define UPDATE 1
#define TELE 2
#define SCAN 3
#define EXIT 4

#define TAMIMG 1500
#define MTU 1448

bool registro(char[], char[]);
void identificar();
int checkCommand(char[]);
void updateFirmware();
char * leerArchivo(char[]);
void conectarSocket(char[]);
void getTelemetria();
void getImagenSatelital();

unsigned long tamBinario;
int socketFileDescr, newSockFd;

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

            if(write(newSockFd, buffer, strlen(buffer)) < 0){
                perror("\n\nEL CLIENTE SE HA DESCONECTADO");
                conectarSocket(argumento);

                if(write(newSockFd, buffer, strlen(buffer)) < 0){
                    perror("ERROR AL VOLVER A CONECTAR");
                    exit(ERROR);
                }
            }

            //exit: termino la ejecucion del programa
            if(num == EXIT){
                if(close(socketFileDescr) < 0){
                    perror("ERROR CERRANDO SOCKET");
                }

                exit(0);
            }

            //Update Firmware.bin
            if(num == UPDATE){

                updateFirmware();
                //system("./version.sh"); //corro shell script
                //char *bufferUpdate = leerArchivo("update64");
                char *bufferUpdate = leerArchivo("client_cc.c");

                printf("STRLEN U64: %lu\n", strlen(bufferUpdate));
                
                int cantmens = (strlen(bufferUpdate)/MTU)+1;
                
                printf("CANTMES: %d\n", cantmens);

                int beg = 0;
                
                for(int i=0;i<cantmens;i++){

                    char substr[MTU] = {""};
                    strncpy(substr, bufferUpdate+beg, MTU);

                    if(write(newSockFd, substr, MTU) < 0){
                        perror("escritura de socket");
                        //exit(ERROR);
                    }

                    beg = beg+MTU;

                    memset(buffer, 0, sizeof(buffer)); //Limpio el buffer

                    if(read(newSockFd, buffer, sizeof(buffer)-1) < 0){
                        perror("lectura");
                    }

                }

                if(write(newSockFd, "FIN", strlen("FIN")) < 0){
                    perror("escritura de socket");
                }

                free(bufferUpdate);
                close(socketFileDescr);

                usleep(5000);

                conectarSocket(argumento);

            }

            if(num == TELE){
                getTelemetria();
            }

            if(num == SCAN){
                getImagenSatelital();
            }

        }

    }
}

void getImagenSatelital(){

    /* Obtiene todas las partes de la imagen satelital
        en base 64, las concatena y recupera la imagen */

    bool receiving = true;
    char buffer[TAMIMG] = {""};

    //Reloj
    clock_t start, end;
    double cpu_time_used;

    FILE *fp = NULL;
    int i=0;

    printf("Obteniendo Imagen Satelital\n");
    start = clock();

    while(receiving){

        char filename[8] = {""};
        sprintf(filename, "x%06d", i);
        char filename2[20] = {"Recibido/"};
        strcat(filename2, filename);

        memset(buffer, 0, sizeof(buffer));

        if(read(newSockFd, buffer, TAMIMG) < 0){
            perror("lectura de socket");
            exit(ERROR);
        }

        if(!strcmp(buffer, "FIN")){
            end = clock();
            cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
            receiving = false;
            printf("Imagen Obtenida en %f segundos\n", cpu_time_used);
        }
        else{
            
            fp = fopen(filename2, "w");
            
            if(fp != NULL){
                fwrite(buffer, 1, strlen(buffer), fp);
                fclose(fp);
                write(newSockFd, "ok", 2);
            }
            else{
                perror("ERROR AL ABRIR ARCHIVO");
            }

            i++;
        }
    }

    printf("Procesando Imagen...\n");

    system("./generarImagen.sh");

}

void updateFirmware(){

    int versionActual;

    printf("--SENDING UPDATE--\n");

    char * buffer = leerArchivo("client_cc.c");

    char * match;
    char data_found[64] = "";

    /* busco la version en el codigo fuente del cliente */

    char seccion[SIZE] = {"char VERSION[] ="};
    match = strstr(buffer, seccion);
    sscanf(match, "%[^\n]", data_found);
    
    char *token;
    token = strtok(data_found, "\"");

    //genero string para reemplazar la linea de la version
    char replace[SIZE];
    strcpy(replace, token);
    strcat(replace, "\"");

    while(token != NULL) {

        if(strlen(token) <= 2){
            versionActual = atoi(token);
            break;
        }

        token = strtok(NULL, "\"");
    }

    //una vez encontrado, aumento n° de version
    versionActual++;

    char version[SIZE];
    sprintf(version, "%d", versionActual);
    strcat(replace, version);
    strcat(replace, "\"};");

    //reemplazo linea
    strncpy(match, replace, strlen(replace));


    /*abro y sobreescribo el codigo del cliente*/
    FILE *fp;

    fp = fopen("client_cc.c", "w");
    fwrite(buffer, 1, strlen(buffer), fp);
    fclose(fp);

    free(buffer);

}

char * leerArchivo(char file[]){

    /* Lee un archivo de forma completa */

    FILE *fp;
    unsigned long TAM;


    fp = fopen(file, "r"); // modo de lectura

    if (fp == NULL){
        perror("ERROR abriendo archivo\n");
        exit(ERROR);
    }

    TAM = sizeof(char) * 50000; //max de 50Kb

    char * buffer = malloc(TAM);
    size_t bytes_read;


    bytes_read = fread(buffer, 1, TAM-1, fp);
    
    fclose(fp);

    //buffer[bytes_read] = "\0";

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

    int puerto, cl_len;
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
    int puerto = 27815;

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