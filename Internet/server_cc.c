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
#define CRESET   "\x1b[0m"
#define CLEAR "\033[H\033[J" //borrar consola
#define ERROR 1 //codigo de error

#define UPDATE 1
#define TELE 2
#define SCAN 3
#define EXIT 4

#define TAMIMG 1500

bool registro(char[], char[]);
void identificar();
int checkCommand(char[]);
void updateFirmware();
char * leerArchivo(char[], bool);
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
    
    char buffer[SIZE];

    if(argc != 2) {
        printf("Uso: %s <puerto>\n", argv[0]);
        exit(ERROR);
    }

    identificar();

    char argumento[SIZE];
    strcpy(argumento, argv[1]);

    conectarSocket(argumento);

    //printf(CLEAR);

    while(1){

        bool receiving = true;
        memset(buffer, 0, sizeof(buffer)); //Limpio el buffer

        printf(MAGENTA"%s"CYAN"@EstacionTerrena:"CRESET"~$ ", USER);
        fgets(buffer, SIZE-1, stdin);

        buffer[strlen(buffer)-1] = '\0';

        int num = checkCommand(buffer);

        memset(buffer, 0, sizeof(buffer)); //Limpio el buffer
        sprintf(buffer, "%d", num);

        //Si el comando existe lo envio
        if(num >= 0){

            if(num == EXIT){
                if(close(socketFileDescr) < 0){
                    perror("ERROR CERRANDO SOCKET");
                }

                exit(0);
            }

            if(write(newSockFd, buffer, strlen(buffer)) < 0){
                perror("\n\nEL CLIENTE SE HA DESCONECTADO");
                conectarSocket(argumento);

                if(write(newSockFd, buffer, strlen(buffer)) < 0){
                    perror("ERROR AL VOLVER A CONECTAR");
                    exit(ERROR);
                }
                //exit(ERROR);
            }

            //Update Firmware.bin
            if(num == UPDATE){
                updateFirmware();
                system("./version.sh"); //corro shell script
                //TODO: borrar update despues de pasarlo
                char *bufferUpdate = leerArchivo("update", true);

                printf("strlen bufferUpdate: %lu\n", strlen(bufferUpdate));
                printf("sizeof bufferUpdate: %lu\n", sizeof(bufferUpdate));
                printf("sizeof * bufferUpdate: %lu\n", sizeof( * bufferUpdate));

                if(write(newSockFd, bufferUpdate, tamBinario) < 0){
                    perror("escritura de socket");
                    exit(ERROR);
                }

                free(bufferUpdate);

                usleep(1000);

                char buffSize[sizeof(unsigned long)];
                sprintf(buffSize, "%lu", tamBinario);

                if(write(newSockFd, buffSize, sizeof(unsigned long)) < 0){
                    perror("escritura de socket");
                    exit(ERROR);
                }

                usleep(1000);
                close(socketFileDescr);
                
                system("./removeUpdate.sh");

                conectarSocket(argumento);
                //receiving = false;

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
        //char buffer[TAMIMG] = {""};

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

    printf("-UPDATE-\n");

    //char buffer[8000];

    //char archivo[SIZE] = {"client_cc.c"};
    //strcpy(buffer, leerArchivo("client_cc.c"));
    char * buffer = leerArchivo("client_cc.c", false);

    //printf("BUFFER:\n\n%s\n", buffer);

    char * match;
    char data_found[64] = "";

    char seccion[SIZE] = {"char VERSION[] ="};
    match = strstr(buffer, seccion);
    sscanf(match, "%[^\n]", data_found);
    
    char *token;

    /* get the first token */
    token = strtok(data_found, "\"");

    char replace[SIZE];
    strcpy(replace, token);
    strcat(replace, "\"");

    /* walk through other tokens */
    while(token != NULL) {

        if(strlen(token) <= 2){
            versionActual = atoi(token);
            break;
        }

        token = strtok(NULL, "\"");
    }

    versionActual++;

    char version[SIZE];
    sprintf(version, "%d", versionActual);
    strcat(replace, version);
    strcat(replace, "\"};");

    strncpy(match, replace, strlen(replace));

    FILE *fp;

    fp = fopen("client_cc.c", "w");
    fwrite(buffer, 1, strlen(buffer), fp);
    fclose(fp);

    free(buffer);

}

char * leerArchivo(char file[], bool binary){

    FILE *fp;
    unsigned long TAM;

    //abro el archivo el modo lectura
    if(binary){
        fp = fopen(file, "rb"); // modo de lectura binario
    }
    else{ 
        fp = fopen(file, "r"); // modo de lectura
    }
    if (fp == NULL){
        perror("ERROR abriendo archivo\n");
        exit(ERROR);
    }

    if(binary){
        TAM = sizeof(int) * 15000;
    }
    else{
        TAM = sizeof(char) * 8000;
    }

    char * buffer = malloc(TAM);
    size_t bytes_read;

    if(binary){
        bytes_read = fread(buffer, sizeof(int), TAM, fp);
        tamBinario = bytes_read * sizeof(int);
    }
    else{
        bytes_read = fread(buffer, 1, TAM, fp);
    }
    
    if(!binary){
        buffer[bytes_read] = '\0';
    }
    
    fclose(fp);

    return buffer;
}

int checkCommand(char reading[]){

    size_t cant = sizeof(commands)/sizeof(commands[0]);

    for(int i=1; i<=cant; i++){
        if(!strcmp(reading, commands[i-1])){
            return i;
        }
    }

    printf("Comando '%s' no encontrado\n", reading);

    return -1;

}

//Ingreso y lectura para usuario y contraseña
bool registro(char print[], char comp[]){

    char input[SIZE];

    //leemos la consola
    printf("\n%s: ", print);
    fgets(input, SIZE, stdin);
    input[strlen(input)-1] = '\0';

    if(!strcmp(input, comp)){
        return true;
    }

    return false;

}

//Identificarse en el servidor
void identificar(){

    //Para proceso de identificacion
    int intentos = 0;
    bool identified = false;

    while(!identified){

        if(registro("USER", USER) && registro("PASS", PASS)){
            identified = true;
            printf("\n***Autentificado correctamente***\n\n");
        }
        else{

            intentos++;

            if(intentos == TRIES){
                perror("\nERROR en la identificacion (Demasiados Intentos)\n");
                exit(ERROR);
            }

            printf("\nNombre de usuario y/o contraseña incorrecto, quedan %d intentos\n", 3-intentos);
        }

    }
}

void conectarSocket(char argv[]){

    int pid, sv_len, puerto, cl_len;
    struct sockaddr_in sv_addr, cl_addr;

    printf("\n-- Esperando la conexion con el satelite --\n");

    socketFileDescr = socket(AF_INET, SOCK_STREAM, 0);
    int on = 1;
    setsockopt(socketFileDescr, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    if(socketFileDescr < 0){
        perror("\nERROR abriendo el socket");
        exit(ERROR);
    }

    //unlink(argv); //Remuevo el nombre de archivo si existe

    bzero((char *) &sv_addr, sizeof(sv_addr)); //Limpio la direccion del sv

    puerto = atoi(argv);

    sv_addr.sin_family = AF_INET;
    sv_addr.sin_addr.s_addr = INADDR_ANY;
    sv_addr.sin_port = htons(puerto);
    
    //strcpy(sv_addr.sun_path, argv);
    //sv_len = strlen(sv_addr.sun_path) + sizeof(sv_addr.sun_family);

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

    int socket_udp, resultado;
    struct sockaddr_in struct_servidor;
    socklen_t tamano_direccion;
    char buffer[SIZE] = {""};

    int puerto = 27815;

    memset(buffer, '\0', SIZE);

    char nom_sock[SIZE] = {"UDPsocket"};

    /* Creacion de socket UDP*/
    if((socket_udp = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("socket");
        //exit(ERROR);
    }

    /* Inicialización y establecimiento de la estructura del servidor */
    memset(&struct_servidor, 0, sizeof(struct_servidor));
    struct_servidor.sin_family = AF_INET;
    struct_servidor.sin_addr.s_addr = INADDR_ANY;
    struct_servidor.sin_port = htons(puerto);
    memset( &(struct_servidor.sin_zero), '\0', 8 );
    //strncpy(struct_servidor.sun_path, nom_sock, sizeof(struct_servidor.sun_path));

    /* Ligadura del socket de servidor a una dirección */
    if((bind(socket_udp, (struct sockaddr *)&struct_servidor, sizeof(struct_servidor))) < 0 ) {
        perror("bind");
        //exit(ERROR);
    }

    tamano_direccion = sizeof(struct sockaddr);


    /*Recepción y procesamiento de mensaje*/

    resultado = recvfrom (socket_udp, buffer, SIZE, 0, (struct sockaddr *) &struct_servidor, &tamano_direccion);
    if(resultado < 0) {
        perror("recepción");
        return;
        //exit(ERROR);
    }
    else{

        char *token;
        token = strtok(buffer, ";");

        /* walk through other tokens */
        while(token != NULL) {
            printf("- %s\n", token);
            token = strtok(NULL, ";");
        }

    }

    if(close(socket_udp) < 0){
        //perror("ERROR CERRANDO SOCKET UDP");
    }

}