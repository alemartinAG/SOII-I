#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdbool.h>
#include <string.h>
#include <zconf.h>
#include <unistd.h>

#define QUEUE 7	//tamaño maximo de la cola de conexiones pendientes
#define SIZE 256 //tamaño del buffer
#define PASS "123" //contraseña
#define USER "admin" //nombre de usuario
#define TRIES 3 //cantidad de intentos de identicacion
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define CRESET   "\x1b[0m"
#define CLEAR "\033[H\033[J"
#define ERROR 1 //codigo de error

bool registro(char[], char[]);
void identificar();
int checkCommand(char[]);
void updateFirmware();
char * leerArchivo(char[]);

const char *commands[3] = { "1", 
                            "obtener telemetria", 
                            "start scanning"
                        };

int main(int argc, char *argv[]){

    int socketFileDescr, newSockFd, pid, sv_len;
    char buffer[SIZE];

    struct sockaddr_un sv_addr, cl_addr;

    identificar();

    if(argc != 2) {
        printf("Uso: %s <nombre_de_socket>\n", argv[0]);
        exit(ERROR);
    }

    socketFileDescr = socket(AF_UNIX, SOCK_STREAM, 0);

    if(socketFileDescr < 0){
        perror("\nERROR abriendo el socket");
        exit(ERROR);
    }

    unlink(argv[1]); //Remuevo el nombre de archivo si existe

    bzero((char *) &sv_addr, sizeof(sv_addr)); //Limpio la direccion del sv

    sv_addr.sun_family = AF_UNIX;
    strcpy(sv_addr.sun_path, argv[1]);
    sv_len = strlen(sv_addr.sun_path) + sizeof(sv_addr.sun_family);

    if(bind(socketFileDescr, (struct sockaddr*)&sv_addr, sv_len) < 0){
        perror("\nERROR en el binding");
        exit(ERROR);
    }

    //acepto conexiones
    listen(socketFileDescr, QUEUE);

    printf(CLEAR);

    socklen_t cl_len = sizeof(cl_addr);

    newSockFd = accept(socketFileDescr, (struct sockaddr*)&cl_addr, &cl_len);

    if(newSockFd < 0){
        perror("\nERROR en accept");
        exit(ERROR);
    }

    while(1){

        memset(buffer, 0, sizeof(buffer)); //Limpio el buffer

        printf(MAGENTA"%s"CYAN"@EstacionTerrena:"CRESET"~$ ", USER);
        fgets(buffer, SIZE-1, stdin);

        buffer[strlen(buffer)-1] = '\0';

        int num = checkCommand(buffer);

        memset(buffer, 0, sizeof(buffer)); //Limpio el buffer
        sprintf(buffer, "%d", num);

        if(num == 0){
            updateFirmware();
            system("./version.sh"); //corro shell script
            //TODO: borrar update despues de pasarlo
            char *bufferUpdate = leerArchivo("update");

            if(write(newSockFd, buffer, strlen(buffer)) < 0){
                perror("escritura de socket");
                exit(ERROR);
            }

            free(buffer);
            system(removeUpdate.sh)

        }

        if(num > 0){

            //Si el comando existe lo envio
            if(write(newSockFd, buffer, strlen(buffer)) < 0){
                perror("escritura de socket");
                exit(ERROR);
            }

            else{

                printf("\n");
                bool receiving = true;

                while(receiving){

                    memset(buffer, 0, sizeof(buffer));

                    if(read(newSockFd, buffer, SIZE) < 0){
                        perror("lectura de socket");
                        exit(ERROR);
                    }

                    if(!strcmp(buffer, "FIN")){
                        receiving = false;
                    }
                    else{
                        printf("- %s\n", buffer);
                    }
                }

            }

        }

    }
}

void updateFirmware(){

    int versionActual;

    printf("-UPDATE-\n");

    //char buffer[8000];

    //char archivo[SIZE] = {"client_cc.c"};
    //strcpy(buffer, leerArchivo("client_cc.c"));
    char * buffer = leerArchivo("client_cc.c");

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

char * leerArchivo(char file[]){

    FILE *fp;

    unsigned long TAM = sizeof(char) * 8000;

    char * buffer = malloc(TAM);
    size_t bytes_read;

    fp = fopen(file, "r"); // modo de lectura
    if (fp == NULL){
        perror("ERROR abriendo archivo\n");
        exit(ERROR);
    }

    bytes_read = fread(buffer, 1, TAM, fp);
    buffer[bytes_read] = '\0';

    fclose(fp);

    return buffer;
}

int checkCommand(char reading[]){

    size_t cant = sizeof(commands)/sizeof(commands[0]);

    for(int i=0; i<cant; i++){
        if(!strcmp(reading, commands[i])){
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
            printf("\nAutentificado correctamente\n");
            printf("\n - Eperando la conexion con el satelite - \n");
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