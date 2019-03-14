#include <sys/types.h>
#include <sys/socket.h>

#define PORTN 6020

int main(){
	
	int socketFileDescr;

	socketFileDescr = socket(AF_INET, SOCK_STREAM, 0) //TODO: Protocol?

	if(socketFileDescr < 0){
		perror("ERROR abriendo el socket");
		exit(1);
	}

	

}
