#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORTN 6020 //numero de puerto
#define QUEUE 7	//tamaño maximo de la cola de conexiones pendientes

int main(){
	
	int socketFileDescr;

	struct sockaddr_in sv_address;

	if(socketFileDescr < 0){
		perror("ERROR abriendo el socket");
		exit(1);
	}

	sv_address.sin_family = AF_INET; //TODO: comments about
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	sv_address.sin_port = htons(PORTN); //convierto a network byte order

	socketFileDescr = socket(AF_INET, SOCK_STREAM, 0) //TODO: Protocol?

	if(bind(socketFileDescr, (struct sockaddr *)/*&*/sv_address, 
			sizeof(sv_address)) < 0){

		perror("ERROR en el binding");
		exit(1);
	}

	//acepto conexiones
	listen(socketFileDescr, QUEUE);



}
