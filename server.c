#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <unistd.h>
#include <sys/syscall.h> 
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1

typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;// C'est pas utile changer le après
typedef struct sockaddr SOCKADDR;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORT 1234

//A temporarily structure
typedef struct syscall_info
{
	int syscall_number;
	int param1;
	char param2[256];
	int param3;
}syscall_info;

int main()
{
	SOCKET sock;
	SOCKADDR_IN sin;
	socklen_t recsize = sizeof(sin);

	SOCKET csock;
	SOCKADDR_IN csin;
    socklen_t crecsize = sizeof(csin);
    
    	char buffer[32]="you are connected\n";

    int sock_err;
    
	sock = socket(AF_INET, SOCK_STREAM, 0);
	
	if(sock == INVALID_SOCKET) printf("deal with error later");
	/*Initialisation*/
	sin.sin_addr.s_addr = htonl(INADDR_ANY); //=inet_addr("127.0.0.1") for spesefic address
	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);
	
	sock_err = bind(sock, (SOCKADDR*)&sin, sizeof(sin));

	sock_err = listen(sock, 5);
	if (sock_err) perror("listen");
	csock = accept(sock, (SOCKADDR*)&csin, &crecsize);
	

	sock_err = send(csock, buffer, 32, 0);

	//receive the syscall parameters in the structure new_info from the client
	syscall_info new_info;
	if(recv(csock, &new_info, sizeof(new_info) , 0) != SOCKET_ERROR)
                printf("Recu %d: \n", new_info.syscall_number);
    //execute the syscall 
    int syscall_return_value;
    syscall_return_value = syscall(new_info.syscall_number,new_info.param1,new_info.param2,new_info.param3);
    //return value of the syscall 
    sprintf(buffer,"%d",syscall_return_value);
    sock_err = send(csock, buffer, 32, 0);

	close(csock);
	close(sock);
	
	return 0;	
}

