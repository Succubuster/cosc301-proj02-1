#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>



/*
**
** Ideas:
** 		1. LL's DONE [almost: remove and go_to]
**			* For what? History, Forked Processes
**			* Generic Functions Needed:
**				# create, [add, remove (delete-friendly?)] == queue-like, goto, print
**		2. History Condensing: before adding, compare to all existing and overlap
**		3. Parallel Mode: rn nodes holding cmd and child pid, must add poll and other built-in funct
** Issues:
**		1. Working Directory
**			* maybe some type of folder rep
**		2. Absolute/Relative Pathnames DONE
**			* maybe folder iterator
**			* hash map???
*/

typedef struct Node {
	int number; 
	bool status;
	char string[128];
	char extra_space[128];
	struct Node *next;
} Node;

void add(Node **h, Node **t, char *s, int n) {
	Node *new = calloc(1,sizeof(Node)); // setup
	new->number = n;
	strcpy(new->string,s);
	new->next = NULL;
	if (*h == NULL && *t == NULL) {
		*h = new;
		*t = new;
		//printf("creating. check me for lies. %d %d\n", (*h)->number, new->number);
		return;
	}
	//printf("adding %d %s, hopefully...\n", n, new->string);
	(*t)->next = new;
	*t = new;
}
void print_list(Node *h, bool s) {
	for (; h != NULL; h = h->next) {
		if (s) {
			char status[16] = "blank";
			if (h->status) {
				strcpy(status,"RUNNING");
			} else {
				strcpy(status,"PAUSED");
			}
			printf("%5d\t%s\t%s\n",h->number,h->string,status);
		} else {
			printf("%-5d%s\n",h->number,h->string);
		}
	}  
}
Node *remove_(Node **h, Node **t, Node *he, Node *ta, int n) { // must be freed after use
	//print_list(*h);
	if (he == NULL || ta == NULL) {
		return NULL;
	}
	if (he->number == n) {
		Node *pop = *h;
		*h = (*h)->next;
		return pop;
	}
	Node *prev = he;
	Node *curr = he->next;
	while (curr->number != n) {
		prev = prev->next;
		curr = curr->next;
		if (curr == ta) {
			return NULL;
		}
	}
	//curr = go_to(*h,curr->number);
	//prev = go_to(*h,prev->number);
	prev->next = curr->next;
	return curr;
}
Node *go_to(Node *h, int loc) {
	for (; h; h = h->next) { 
		if (h->number == loc) {
			return h;
		}
	} return NULL;
}


char** tokenify(const char *s, const char *delim) { // why const ???
    char* str = strdup(s);
    int h = 0;
    int arrlen = 1; 
    
    while (str[h++]) {
    	if (strchr(delim, str[h]) != NULL) { 
    		arrlen++; 
    	}
    } 
    
    char **tokens = calloc(arrlen,sizeof(char*)); 
    char *token = calloc(32,sizeof(char)); // why 16?
    int numtok = 0;
    
    for (token = strtok(str, delim); token; token = strtok(NULL, delim)) {
    //while (token != NULL) { // NEEDS HELP
    	tokens[numtok++] = token;
	} tokens[numtok] = NULL;
    
    return tokens;
}

int count_big_tokens(char ***tokens) {
	int count = 0;
	for (int i = 0; tokens[i] != NULL; i++) {
		count++;	
	} return count;
}
int count_little_tokens(char **tokens) {
	int count = 0;
	for (int i = 0; tokens[i] != NULL; i++) {
		count++;	
	} return count;
}

void print_tokens(char *tokens[]) {
    int i = 0;
    printf("\n");
    while (tokens[i] != NULL) {
        printf("Token %d: %s\n", i+1, tokens[i]);
        i++;
    }
}

void free_little_tokens(char **tokens) {
    int i = 0;
    while (tokens[i] != NULL) {
        // free properly...
        free(tokens[i++]);
    }
    //free(tokens); // then free the array
}
void free_big_tokens(char ***tokens) {
	int i = 0;
	int size = count_big_tokens(tokens);
	while (i != size) {
		free_little_tokens(tokens[i]);
		free(tokens[i++]);
	} //free(tokens);
}
void free_list(Node *head) {
	while (head) {
		Node *t = head->next;
		free(head);
		head = t;
	}
}
		

bool isBuiltin(char** c, bool* res, Node *hh, Node *ph) { // res should be an array of bool
	if (strcmp(c[0],"exit") == 0) {
		if (!res[0] && ph != NULL) {
			return true;
		} res[1] = false;
		return true; 
	} else if (strcmp(c[0],"history") == 0) {
		print_list(hh,false);
		return true;
	} else if (strcmp(c[0],"jobs") == 0) {
		print_list(ph, true);
		return true;
	} else if (strcmp(c[0],"pause") == 0) {
		kill((int)c[1],SIGSTOP);
		printf("Process %d paused.", (int)c[1]);
		return true;
	} else if (strcmp(c[0],"resume") == 0) {
		kill((int)c[1],SIGCONT);
		printf("Process %d resumed.", (int)c[1]);
		return true;
	} else if (strcmp(c[0],"mode") == 0) {
		int len = count_little_tokens(c);
		if (len != 1) {
			if (strcmp(c[1],"sequential") == 0 || strcmp(c[1],"s") == 0) {
				if (res[0]) {
					printf("Sequential --> Sequential\n");
				} else {
					printf("Parallel --> Sequential\n");
				} res[0] = true;
				res[2] = true;
			} else if (strcmp(c[1],"parallel") == 0 || strcmp(c[1],"p") == 0) {
				if (res[0]) {
					printf("Sequential --> Parallel\n");
				} else {
					printf("Parallel --> Parallel\n");
				} res[0] = false;
				res[2] = false;
			} else {
				if (res[0]) {
					printf("Current Mode: Sequential\n");
				} else {
					printf("Current Mode: Parallel\n");
				}
			}
		} else {
			if (res[0]) {
				printf("Current Mode: Sequential\n");
			} else {
				printf("Current Mode: Parallel\n");
			}
		} return true;	
	} return false; // maybe add history cmd	
}

void execute(char** c) {
	//call execv using command info
	if (execv(c[0], c) < 0) {
		printf("The command: ");
		int i = 0;
		while (c[i] != NULL) {
			printf("%s ",c[i]);
		} printf("\n");
	}
}

void print_prompt() {
	// print working directory
	//getcwd()
	char buf[128];
	getcwd(buf, 128);
	//printf("%s:~$ ", buf);
	printf("Testing---> ");
}

void little_to_command(char **l, char **comm) {
	int count = 0;
	while (count != count_little_tokens(l)) {
		strcat(*comm,l[count++]);
		strcat(*comm," ");
	} 
}

void load_paths(Node **h, Node **t) {
	FILE *pathpos = fopen("shell-config", "r");
	add(h,t,"",-1);
	while (!feof(pathpos)) {
		char pp[32];
		fgets(pp,32,pathpos);
		for (int i = 0; i < strlen(pp); i++) { // chop
			if (pp[i] == '\n') {
				pp[i] = '\0';
				break;
			}
		} add(h,t,pp,-1);
	} fclose(pathpos);
}
bool test_exist(char* fn) {
	struct stat results;
	return stat(fn,&results) >= 0;
}
bool check_existence(Node *h, char ***ch, char *c) {
	char *builtin[7] = {"mode","exit","history","jobs","kill","resume",NULL};
	for (; h != NULL; h = h->next) {
		char path[128] = "";
		char path_copy[128] = "";
		char *pre_tok;
		char pre_c[128] = "";
		strcpy(pre_c,c);
		strcpy(path,h->string);
		//printf("%s\n",path);
		strcat(path,"/");
		//printf("%s\n",path);
		strcpy(path_copy, path);
		
		pre_tok = strtok(pre_c," \n\t\r");
		//printf("%s\n",pre_tok);
		int i = 0;
		while (builtin[i] != NULL) {
			if (strcmp(pre_tok,builtin[i++]) == 0) {
				*ch = tokenify(c," \n\t\r");
				//printf("A built-in appears.\n");
				return true;
			}
		}
		strcat(path_copy, pre_tok);
		//printf("%s\n",path_copy);
		//printf("%s\n",path);
		if (test_exist(path_copy)) {
			//printf("\n\n");
			//print_tokens(ch);
			char path_c[128] = "";
			//little_to_command(ch,c);
			//printf("%s\n",c);
			strcpy(path_c,path);
			strcat(path_c,c);
			*ch = tokenify(path_c," \n\t\r");
			//print_tokens(*ch);
			//printf("Final path: %s %s\n", ch[0], ch[1]);
			return true;
		}
	} 
	printf("An error occurred: Maybe command %s entered incorrectly.\n", c);
	return false;	
}

void chop(char *s, char m) {
	for (int i = 0; i < strlen(s); i++) {
		if (s[i] == m) {
			s[i] = '\0';
			return;
		}
	}
}
		
void get_input(char *s) {
	print_prompt();
	while (fgets(s, 128, stdin) == NULL) {
		printf("\n");
		print_prompt();
	}
	fflush(stdout);
}
		
		





int main(int argc, char **argv) {
	bool built_states[4] = {true, true, true, false}; // [0] = mode state, [1] = run state, [2] = input state, [3] = indexing state
	char cmdLn[128] = ""; // maybe malloc? or char cmdLn[128]
	int chdPID;
	int status;// handleLogin(); // trying login?
	int HIST_START = 1;
	int currCmdNum = HIST_START; 
	Node *hhead = NULL;
	Node *htail = NULL;
	//printf("%d\n",hhead->number);
	//int procs[100]; // will be updated to LL
	int proc_count = 0;
	Node *phead = NULL;
	Node *ptail = NULL;
	Node *Phead = NULL;
	Node *Ptail = NULL;
	load_paths(&Phead,&Ptail);
	
	while (built_states[1]) { // infinite loop, maybe fork later
		//print_prompt();
		if (built_states[3]) {// indexing
			
			built_states[3] = false;
		} else {
			if (built_states[2]) { // seq input
				get_input(cmdLn);
				//printf("%s\n",cmdLn);
			} else { // poll (para input)
				// poll
				struct pollfd pfd[1];
				pfd[0].fd = 0; 
				pfd[0].events = POLLIN;
				pfd[0].revents = 0;
				poll(&pfd[0], 1, 200);
				while ((chdPID = waitpid(-1,&status,WNOHANG)) > 0) {
					//printf("Here.");
					//print_list(phead);
					Node *job = remove_(&phead,&ptail,phead,ptail,chdPID);
					if (job != NULL) {
						printf("Process %d (%s) has completed.\n", chdPID, job->string);
						free(job);
						//print_list(phead);	
					}
				} get_input(cmdLn);   
			}
		}
		// possible location for history condensing
		chop(cmdLn, '\n');
		add(&hhead, &htail, cmdLn, currCmdNum++);
		chop(cmdLn, '#');		
		// tokenify twice and give further
		char** cmds = tokenify(cmdLn, ";");
		char** cmdholder[count_little_tokens(cmds)+1]; // malloc?
		int place = 0;
		for (int i = 0; i != count_little_tokens(cmds); i++) {
			if (check_existence(Phead,&(cmdholder[place]),cmds[i])) {
				place++;
			} 
			//print_tokens(cmdholder[place]);
		} cmdholder[place] = NULL;
		
		if (built_states[0]) {
			// sequential
			for (int i = 0; i != count_big_tokens(cmdholder); i++) {
				if (!isBuiltin(cmdholder[i], built_states,hhead,phead))  {
					chdPID = fork();
					if (chdPID == 0) {
						execute(cmdholder[i]);
					} else if (chdPID == -1) { // error handling
						printf("\nA crash occurred...\n");			
					} else {
						wait(&chdPID);
					}
				}
			}
			free_big_tokens(cmdholder);				
		} else {
			// parallel
			for (int i = 0; i != count_big_tokens(cmdholder); i++) {
				if (!isBuiltin(cmdholder[i], built_states,hhead,phead))  {
					chdPID = fork();
					if (chdPID == 0) {
						execute(cmdholder[i]);
					} else if (chdPID == -1) { // error handling
						printf("\nA crash occurred...\n");			
					} else {
						int j = 0;
						char comm[128] = "";
						while (j != count_little_tokens(cmdholder[i])) {
							strcat(comm,cmdholder[i][j++]);
							strcat(comm," ");
						}
						//printf("%d %s\n", i, comm);
						add(&phead,&ptail,comm,chdPID);
						proc_count++;
					}
				}
			}
			free_big_tokens(cmdholder);
		}
		free_list(hhead);
		free_list(Phead);
		free_list(phead);
	} return 0;
}
