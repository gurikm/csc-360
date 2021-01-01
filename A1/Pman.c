

#include <stdbool.h>
#include <unistd.h>           
#include <string.h>            
#include <stdio.h>             
#include <stdlib.h>           
#include <sys/types.h>         
#include <sys/wait.h>          
#include <signal.h>            
#include <readline/readline.h> 
#include <readline/history.h>
#include <errno.h>      



typedef struct node {
	pid_t pid;
	int active;
	char* process;
	struct node* next;
} node;



node* head = NULL; //Global Variable.

/*
 *adds a node named pid to the list.
 */
void add_Node(pid_t pid, char* process) {
	node* newNode = (node*)malloc(sizeof(node));
	newNode->pid = pid;
	newNode->process = process;
	newNode->active = true;
	newNode->next = NULL;

	if (head == NULL) {
		head = newNode;
	} else {
		node* curr = head;
		while (curr->next != NULL) {
			curr = curr->next;
		}
		curr->next = newNode;
	}
	return;
}

/*
 *Checks to see if the node you are looking for 
 *is in the list.
 */
	int check_node(pid_t pid) {
	node* curr = head;
	while (curr != NULL) {
		if (curr->pid == pid) {
			return true;
		}
		curr = curr->next;
	}
	return false;
}


/*
 *removes a node pid from the list.
 */
void remove_node(pid_t pid) {
	if (check_node(pid) == false) {
		return;
	}
	node* curr1 = head;
	node* curr2 = NULL;
	while (curr1 != NULL) {
		if (curr1->pid == pid) {
			if (curr1 == head) {
				head = head->next;
			} else {
				curr2->next = curr1->next;
			}
			free(curr1);
			return;
		}
		curr2 = curr1;
		curr1 = curr1->next;
	}
}

/*
 *returns a node with a given pid
 */
	node* Node(pid_t pid) {
		node* curr = head;
		while (curr != NULL) {
			if (curr->pid == pid) {
				return curr;
			}
			curr = curr->next;
		}
		return NULL;
	}


/*
 * determines if you are able to begin the
 * background process and outputs if it failed
 * to run or if it was a success. Also starts the
 * process.
 */
void bg_entry(char **argv) {
	pid_t pid = fork();
	if (pid == 0) {  
		if(execvp(argv[1],&argv[1]) < 0){
		perror("Failed to run user input");
	}
		exit(1);
	} else if (pid > 0) {	
		printf("Started background process %d\n", pid);
		add_Node(pid, argv[1]);
		sleep(1);
	} else {
		printf("fork failed\n");
	}
}

/*
 *send the CONT signal to the job pid to re-start
 *that job (which has been previously stopped).
 */
void bgstart(pid_t pid) {
	if (check_node(pid) == false) {
		printf("Invalid pid\n");
		return;
	}
	if(!kill(pid, SIGCONT)){
		sleep(1);
	}
	 else {
		printf("Failed to execute bgstart\n");
	}
}

/*
 * send the STOP signal to the job pid to stop (temporarily) that job.
 */
void bgstop(pid_t pid) {
	if (check_node(pid) == false) {
		printf("Invalid pid\n");
		return;
	}
	if(!kill(pid, SIGSTOP)){
		sleep(1);
	} else {
		printf("Failed to execute bgstop\n");
	}
}

/*
 *send the TERM signal to the job with process ID pid to terminate that job
 */
void bgkill(pid_t pid) {
	if (check_node(pid) == false) {
		printf("Invalid pid\n");
		return;
	}
	if(!kill(pid, SIGTERM)){
		sleep(1);
	} else {
		printf("Failed to execute bgkill\n");
	}
}

/*
 *will have PMan display a list of all the programs currently executing in the background.
 */
void bglist_entry() {
	int count = 0;
	node* curr = head;
	while (curr != NULL) {
		printf("%d: %s", curr->pid, curr->process);
		if (curr->active == false) {
		printf(" stopped");
		count++;
		}
		curr = curr->next;
		printf("\n");
	}
	printf("Total background jobs: %d\n", count);
}

/*
 *to list the following information related to process pid,where pid is the Process ID.
 */
void pstat_entry(pid_t pid) {
	if (check_node(pid)) {
		char stat[128];
		char status[128];
		sprintf(stat, "/proc/%d/stat", pid);
		sprintf(status, "/proc/%d/status", pid);

		char* stat_items[128];
		
	FILE *stat_file = fopen(stat, "r");
	char items[1500];
	if (stat_file != NULL) {
		int i = 0;
		while (fgets(items, sizeof(items)-1, stat_file) != NULL) {
			char* token;
			token = strtok(items, " ");
			stat_items[i] = token;
			while (token != NULL) {
				stat_items[i] = token;
				token = strtok(NULL, " ");
				i++;
			}
		}
		fclose(stat_file);
	}else{
		printf("Cant open stat files.");
	}

		char status_items[128][128];
		FILE* status_File = fopen(status, "r");
		if (status_File != NULL) {
			int i = 0;
			while (fgets(status_items[i], 128, status_File) != NULL) {
				i++;
			}
			fclose(status_File);
		} else {
			return;
		}
		char* p;
		long unsigned int utime = strtoul(stat_items[13], &p, 10) / sysconf(_SC_CLK_TCK);
		long unsigned int stime = strtoul(stat_items[14], &p, 10) / sysconf(_SC_CLK_TCK);
		printf("comm:%s\n", stat_items[1]);
		printf("state:%s\n", stat_items[2]);
		printf("utime:%lu\n", utime);
		printf("stime:%lu\n", stime);
		printf("rss:%s\n", stat_items[24]);
		printf("%s\n", status_items[39]);
		printf("%s\n", status_items[40]);
	} else {
		printf("Incorret Pid\n");
	}
}

/*
 *will parse an array input.
 */
void parse(char* input, char** array, char* tok) {
	char* token = strtok(input, tok);
	for(int i = 0; i < 128; i++) {
		array[i] = token;
		token = strtok(NULL, tok);
	}
}

/*
 *updates the list currently running and starts,stops or terminates
 *processes.
 */
void check_zombieProcess() {
	pid_t pid;
	int	p_status;
	while (1) {
		pid = waitpid(-1, &p_status, WCONTINUED | WNOHANG | WUNTRACED);
		if (pid > 0) {
			if (WIFSTOPPED(p_status)) {
				printf("Process stopped by signal %d\n", WSTOPSIG(p_status));
				node* A_node = Node(pid);
				A_node->active = false;
			}
			if (WIFCONTINUED(p_status)) {
				printf("Process %d continues\n", pid);
				node* A_node = Node(pid);
				A_node->active = true;
			}
			if (WIFSIGNALED(p_status)) {
				printf("Process was killed by signal %d\n", WTERMSIG(p_status));
				remove_node(pid);
			}
			if (WIFEXITED(p_status)) {
				printf("status code = %d\n",WEXITSTATUS(p_status));
				remove_node(pid);
			}
		} else {
				break;
		}
	}
}

/*
 *main function compares user input with one of the commands
 *and excutes a comand if the user input is the same as a command
 *producing the desired output.
 */

int main() {
	while (1) {
		char* user[128];
		char* input = readline("Pman:> ");

		if(strcmp(input, "") == 0) continue;
		parse(input, user, " ");

		check_zombieProcess();
		if(strcmp(user[0], "bg") == 0) {
		if(user[1] == NULL) {
			printf("Error: no process name.\n");
			continue;
		}
		bg_entry(user);
	} 
	else if(strcmp(user[0], "bglist") == 0) {
		bglist_entry();
	} 
	else if(strcmp(user[0], "bgkill") == 0) {
			if(user[1] == NULL ) {
			printf("Enter PID\n");
			continue;
		}
		pid_t pid = atoi(user[1]);
		bgkill(pid);
	} 
	else if(strcmp(user[0], "bgstop") == 0) {
		 if(user[1] == NULL ) {
			printf("Enter PID\n");
			continue;
		}
	pid_t pid = atoi(user[1]);
		bgstop(pid);
	} 
	else if(strcmp(user[0], "bgstart") == 0) {
		if(user[1] == NULL) {
			printf("Enter PID\n");
			continue;
		}
		pid_t pid = atoi(user[1]);
		bgstart(pid);
	} 
	else if(strcmp(user[0], "pstat") == 0) {
		if(user[1] == NULL ) {
			printf("Enter PID\n");
			continue;
		}
		pid_t pid = atoi(user[1]);
		pstat_entry(pid);
	} 
	else if(strcmp(user[0], "exit") == 0) {
		node* curr = head;
		while(curr!= NULL) {
			bgkill(curr->pid);
			curr = curr->next;
		}
		return 1;
	} 
	else {
		printf("Pman:> %s: Command not found\n",user[0]);
	}
		check_zombieProcess();
	}
	return 0;
}
