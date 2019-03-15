#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h> 

#define PORTN 6020 //numero de puerto
#define QUEUE 7	//tamaño maximo de la cola de conexiones pendientes
#define SIZE 256 //tamaño del buffer

int main(){
	
	int socketFileDescr, newSockFd;
	char buffer[SIZE];
	int portNumber = PORTN;

	char password[36] = "password";

	struct sockaddr_in sv_addr, cl_addr;

	socketFileDescr = socket(AF_INET, SOCK_STREAM, 0); //TODO: Protocol?

	if(socketFileDescr < 0){
		perror("ERROR abriendo el socket");
		exit(1);
	}

	bzero((char *) &sv_addr, sizeof(sv_addr)); //Limpio la direccion del sv

	sv_addr.sin_family = AF_INET; //TODO: comments about
	sv_addr.sin_addr.s_addr = INADDR_ANY;
	sv_addr.sin_port = htons(portNumber); //convierto a network byte order	

	if(bind(socketFileDescr, (struct sockaddr*)&sv_addr, 
			sizeof(sv_addr)) < 0){

		perror("ERROR en el binding");
		exit(1);
	}

	//acepto conexiones
	listen(socketFileDescr, QUEUE);

	socklen_t cl_len = sizeof(cl_addr);

	bool identified = false;

	while(1){

		newSockFd = accept(socketFileDescr, (struct sockaddr*)&cl_addr, &cl_len);

		if(newSockFd < 0){
			perror("ERROR en accept");
			exit(1);
		}

		if(read(newSockFd, buffer, SIZE) < 0){
			perror("ERROR en la lectura");
			exit(1);
		}

		buffer[strlen(buffer)-1] = '\0'; //limpio el enter

		if(!identified){
			
			if(strcmp(buffer, password) == 0){
				identified = true;
				printf("\nAutentificado correctamente\n");
				//TODO: Msg de confirmacion
			}
			else {
				printf("\nnombre de usuario y/o contraseña incorrecto\n");
			}

		}
		else{
			printf("\nMESSAGE: %s\n", buffer);
		}

		memset(buffer, 0, sizeof(buffer)); //Limpio el buffer
	}

	/*close(newSockFd);
	close(socketFileDescr);*/

	return 0;
}
