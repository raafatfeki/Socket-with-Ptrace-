#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <unistd.h>

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define PORT 1234

typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;// C'est pas utile changer le après
typedef struct sockaddr SOCKADDR;

typedef struct syscall_info
{
	int syscall_number;
	int param1;
	char param2[256];
	int param3;
}syscall_info;

#include <stdio.h>
#include <stdlib.h>

#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/reg.h>
#include <sys/syscall.h>
#include <string.h>
const int long_size = sizeof(long);



int set_syscall_info(pid_t child, long syscall_number, unsigned long long* param_list, syscall_info *info);
void getdata(pid_t child, long addr, char *str, int len);

int main()
{
	/* Deal with the sockets */
	SOCKET sock;
	SOCKADDR_IN sin;
	char buffer[32]="";

	sock = socket(AF_INET, SOCK_STREAM, 0);
	
	sin.sin_addr.s_addr = inet_addr("127.0.0.1");
	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);
	if(connect(sock, (SOCKADDR*)&sin, sizeof(sin)) != SOCKET_ERROR)
            printf("Connexion à %s sur le port %d\n", inet_ntoa(sin.sin_addr), htons(sin.sin_port));
    else
				printf("Impossible de se connecter\n");
				
	if(recv(sock, buffer, 32, 0) != SOCKET_ERROR)
                printf("Recu : %s\n", buffer);
	
	/*---------------------------- Extract the arguments from write syscall------------------------------*/
    pid_t child_process;
	long ptrace_return;
	int status; // to wait for child process
	long orig_rax; //The syscall number called 
	unsigned long long params[6];
	int insyscall = 0;
    struct user_regs_struct regs;
	
	child_process = fork();
		if(child_process == 0) 
			{
				/* Initiate the child to be traced by his parent */
				ptrace(PTRACE_TRACEME, 0, NULL, NULL);
				execl("./helloworld_dynamic", "hello_world_static", NULL);
			}
		else
			{

				while(1) 
					{

						wait(&status); // is equivalent to waitpid(-1, &status, 0) meaning wait for any child process
						if(WIFEXITED(status)) break; //returns true if the child terminated normally
						
						orig_rax = ptrace(PTRACE_PEEKUSER, child_process, 8 * ORIG_RAX, NULL);
						if (orig_rax == SYS_write)
						{
							
						if(insyscall == 0) // Syscall before execution
							{
								insyscall = 1;
								/* Getting the 6 parameters from registers. */
								ptrace(PTRACE_GETREGS, child_process, NULL, &regs);
								
								params[0] = regs.rdi;
								params[1] = regs.rsi;
								params[2] = regs.rdx;
								params[3] = regs.r10;
								params[4] = regs.r8;
								params[5] = regs.r9;
								/* fill the structure with the arguments */

								syscall_info *info = malloc(sizeof(syscall_info));
								info->syscall_number=0;
								info->param1 =0;
								info->param3 =0;

								if(!set_syscall_info(child_process, orig_rax, params, info))
								printf("before1 ===================>orig_rax=%d---regs.orig_rax=%s---rax=%d\n"
									, info->param1, (char*)info->param2, info->param3);
								/* Send the arguments */
								send(sock, info ,sizeof(*info), 0);
								/* receive the syscall return value */
								if(recv(sock, buffer, 32, 0) != SOCKET_ERROR)
               						printf("Recu : %s\n", buffer);

								/* Set the return value of syscall to register rax*/
								regs.rax = atoi(buffer);
								ptrace (PTRACE_SETREGS, child_process, NULL, &regs);
								
							}
							
						else // Syscall After execution
							{ 
								insyscall = 0;
							}
							
						}
						ptrace(PTRACE_SYSCALL, child_process, NULL, NULL);
					}
			}

	

	close(sock);
	return 0;
}




int set_syscall_info(pid_t child, long syscall_number, unsigned long long* param_list, syscall_info *info)
{

			char *str;
			str = (char *)calloc((param_list[2]+1), sizeof(char));
			getdata(child, param_list[1], str, param_list[2]);
			info->syscall_number = (int)syscall_number;
			info->param1 = (int)param_list[0];
			strcpy(info->param2, str);
			info->param3 = (int)param_list[2];
			printf("\n-----%d------%s-------------%d\n", info->param1,info->param2,info->param3);
			return 0;
}

void getdata(pid_t child, long addr, char *str, int len)
{   
	char *laddr;
    int i, j;
    union u {
            long val;
            char chars[long_size];
    }data;
    
    i = 0;
    j = len / long_size;
    laddr = str;
    while(i < j) 
    {
        data.val = ptrace(PTRACE_PEEKDATA,child, addr + i * 8, NULL);
        memcpy(laddr, data.chars, long_size);
        ++i;
        laddr += long_size;
    }
    
    j = len % long_size;
    if(j != 0) 
    {
        data.val = ptrace(PTRACE_PEEKDATA, child, addr + i * 8, NULL);
        memcpy(laddr, data.chars, j);
    }
    str[len] = '\0';
}