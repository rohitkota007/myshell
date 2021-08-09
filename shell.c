#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <set>
#include <pwd.h>
#include <sys/prctl.h>
#include <iostream>
#include <vector>
using namespace std;

#define MAXLINE 1000
#define DEBUG 0

/* Function declarations and globals */
int donotclose = 1;
int parent_pid ;
char ** tokenize(char*) ;
int execute_command(char** tokens) ;
char** tokens;
static char *home;              //home directory
set<int> background_process;
void newhandler(int signal){ // free tokens to avoid memory leak
	int i;
	for(i=0;tokens[i]!=NULL;i++){
		free(tokens[i]);
	}
	free(tokens);
	return;
}
/*void backgroundhandler(int sig,siginfo_t *siginfo,void *context){
	
	
	if(background_process.find(siginfo->si_pid) != background_process.end()){
		printf("%s", "Process exited:");
		printf("%d\n",siginfo->si_pid);
		printf("%s","Status:");
		printf("%d\n",siginfo->si_status);
		fflush(stdout);
	}
	return;
	
}*/
void backgroundhandler(int signal) {
	int status;
	int pid = waitpid(-1,&status,WNOHANG);             // makes the process not wait here
	int exitstatus = WIFEXITED(status);
	if(background_process.find(pid) != background_process.end()){
		printf("%s", "Process exited:");
		printf("%d\n",pid);
		printf("%s","Status:");
		printf("%d\n",exitstatus);
		fflush(stdout);
		background_process.erase(pid);
	}
	return;
}	

void siginthandler(int signal) {   
	return ;
}

void getuserdetails(char username[],char pc_name[]) {   //access the details of the username and hostname
       struct passwd *details;
       uid_t user_id;
       char comp_name[100];
       user_id = geteuid();
       details = getpwuid(user_id);
       gethostname(comp_name,100);
       strcpy(username, details->pw_name);
       strcpy(pc_name, comp_name);
}
  
int substr(char *actpath,char *curpath) {  //finds if home is substring of current working directory
	int i;
  	if(strlen(actpath) > strlen(curpath))
    		return 0;
  	for(i = 0;i < strlen(actpath);i++)
          if(actpath[i] != curpath[i])
               return 0;
       return 1;
}  
  
void curworkdir(char buf[]) {  //returns the current working directory
      getcwd(buf,100); 
}     

void sethomdir() {                     // sets the current directory as a home directory
      home = (char *)malloc(sizeof(char)*100);
      getcwd(home,100);
}

char *gethomdir() {  //return ~ home directory
      return home;  
}    

int child_dir(char *actpath,char *curpath)  {      //finds the pattern of home directory in current working directory
      int i;
      for(i = 0;i < strlen(actpath);i++)
         if(actpath[i] != curpath[i])
      break;
  return i;
}
   
int main(int argc, char** argv) {  
	char *cwd, username[100],  pc_name[100];
	cwd = (char *)malloc(sizeof(char)*100);
	getuserdetails(username,pc_name);           //gets user details
	sethomdir();                          //sets the current working directory as home directory
	parent_pid = getpid() ;
	 //kill background process afer killing shell
	/* Set (and define) appropriate signal handlers */
	/* Exact signals and handlers will depend on your implementation */
	// signal(SIGINT, quit);
	// signal(SIGTERM, quit);

	struct sigaction sa;
	sa.sa_handler = SIG_IGN;

	if(sigaction(SIGINT,&sa,NULL) == -1){
		perror("Error : cannot handle SIGINT");
	}
	if(sigaction(SIGQUIT,&sa,NULL) == -1){
		perror("Error : cannot handle SIGINT");
	}
	
	/*struct sigaction sa1;
	sa1.sa_sigaction = &backgroundhandler;
	sa1.sa_flags = SA_SIGINFO;
	if(sigaction(SIGCHLD,&sa1,NULL) == -1){
		perror("Error : cannot handle SIGINT");
	}*/
	signal(SIGCHLD,backgroundhandler);



	char input[MAXLINE];
	
	
	while(1) { 
		curworkdir(cwd);
		if(!strcmp(gethomdir(),cwd))                    //when home is cwd
      		printf("%s@%s:~$ ",username,pc_name);
    		else if(substr(gethomdir(),cwd)) {               // when cwd is child of home
      		       cwd += child_dir(gethomdir(),cwd);
     		       printf("%s@%s:~%s$ ",username,pc_name,cwd);
      		}
    		else                                                // when cwd is not in home branch
     		       printf("%s@%s:~%s$ ",username,pc_name,cwd);
		//printf("$ "); // The old prompt
		fflush(stdin);

		char *in = fgets(input, MAXLINE, stdin); // Taking input one line at a time
		//Checking for EOF
		if (in == NULL){
			if (DEBUG) printf("shell: EOF found. Program will exit.\n");
			break ;
		}

		// Calling the tokenizer function on the input line    
		tokens = tokenize(input);	
		// Executing the command parsed by the tokenizer
		execute_command(tokens) ; 
		
		// Freeing the allocated memory	
		int i ;
		for(i=0;tokens[i]!=NULL;i++){
			free(tokens[i]);
		}
		free(tokens);
	}
	
	/* Kill all Children Processes and Quit Parent Process */
	return 0 ;
}

/*The tokenizer function takes a string of chars and forms tokens out of it*/
char ** tokenize(char* input){
	int i, doubleQuotes = 0, tokenIndex = 0, tokenNo = 0 ;
	char *token = (char *)malloc(MAXLINE*sizeof(char));
	char **tokens;

	tokens = (char **) malloc(MAXLINE*sizeof(char*));

	for(i =0; i < strlen(input); i++){
		char readChar = input[i];
		if (readChar == '"'){
			doubleQuotes = (doubleQuotes + 1) % 2;
			if (doubleQuotes == 0){
				token[tokenIndex] = '\0';
				if (tokenIndex != 0){
					tokens[tokenNo] = (char*)malloc(MAXLINE*sizeof(char));
					strcpy(tokens[tokenNo++], token);
					tokenIndex = 0; 
				}
			}
		} else if (doubleQuotes == 1){
			token[tokenIndex++] = readChar;
		} else if (readChar == ' ' || readChar == '\n' || readChar == '\t'){
			token[tokenIndex] = '\0';
			if (tokenIndex != 0){
				tokens[tokenNo] = (char*)malloc(MAXLINE*sizeof(char));
				strcpy(tokens[tokenNo++], token);
				tokenIndex = 0; 
			}
		} else {
			token[tokenIndex++] = readChar;
		}
	}

	if (doubleQuotes == 1){
		token[tokenIndex] = '\0';
		if (tokenIndex != 0){
			tokens[tokenNo] = (char*)malloc(MAXLINE*sizeof(char));
			strcpy(tokens[tokenNo++], token);
		}
	}

	tokens[tokenNo] = NULL ;
	return tokens;
}


int find_symbol(char** tokens){
	int i,countL = 0, countG = 0, countGG = 0, countP = 0;
	for(i=0;tokens[i]!=NULL;i++){
		if (!strcmp(tokens[i],"<")) countL++;
		if (!strcmp(tokens[i],">")) countG++;
		if (!strcmp(tokens[i],">>")) countGG++;
		if (!strcmp(tokens[i],"|")) countP++;
	}
	if(countL == 1 && countG == 1 && countGG == 0) return 1; // new output file
	if(countL == 1 && countG == 0 && countGG == 1) return 2; // append output file
	if(countL == 1 && countG == 0 && countGG == 0) return 3; // no output file

	if(countL == 0 && countG == 1 && countGG == 0) return 4; // new output file with no input file
	if(countL == 0 && countG == 0 && countGG == 1) return 5; // append output file with no input fileeturn
	if(countL == 0 && countG == 0 && countGG == 0 && countP == 0) return 7;
	if(countP == 1) return 6; // pipe case
	return -1; // error case
}


int is_background(char** tokens){
	int i;
	int countA =0;
	for(i=0;tokens[i] != NULL;i++){
		if(!strcmp(tokens[i],"&")) countA++; 
	}
	//cout << countA << endl;
	if((countA == 1)) return 1;
	if(countA > 1) return -1;
	return 0;
}

//vector<char**>tokenarray;
int execute_command(char** tokens) {
	/* 
	Takes the tokens from the parser and based on the type of command 
	and proceeds to perform the necessary actions. 
	Returns 0 on success, -1 on failure. 
	*/
	if (tokens == NULL) {
		return -1 ; 				// Null Command
	} else if (tokens[0] == NULL) {
		return 0 ;					// Empty Command
	}
	else{
		char* input_file;
		char* output_file;
		
		if(is_background(tokens) == 1){
			
			int c;
			for(c = 0;tokens[c] != NULL; c++){
				if(!strcmp(tokens[c],"&")){
					tokens[c] = NULL;
					break;
				}
			}
			c =0;
						
			int pid = fork();
			if(pid == 0){
				prctl(PR_SET_PDEATHSIG,SIGHUP);
				donotclose = 0;
				execute_command(tokens);
				exit(0);
			}
			background_process.insert(pid);
		return 0;
		
		}
		else if(is_background(tokens) == -1){
			printf("%s\n","Invalid command");
			fflush(stdout);
			return -1;
		}
		else if (find_symbol(tokens) == 1){
			int found_input = 0;
			int found_output = 0;
			int p =0;
			 int c;
			for(c = 0; tokens[c] != NULL; c++){				// contains < and >
				if(!strcmp(tokens[c],"<")){
					found_input = 1;
					found_output = 0;
					if(p == 0) p = c;
					c = c + 1;
				}
				if(!strcmp(tokens[c],">")){
				    found_output = 1;
				    found_input = 0;
				    if(p == 0) p = c;
				    c = c+ 1;
				}
				if(found_input){
					input_file = tokens[c];
					if((tokens[c+1] != NULL) && (strcmp(tokens[c+1],">") != 0)){
						 printf("%s\n","Invalid number of arguements");
						 return 0;
					 }
					 found_input = 0;
				}
				if(found_output){
					output_file = tokens[c];
					if((tokens[c+1] != NULL) && (strcmp(tokens[c+1],"<") != 0)){
						 printf("%s\n","Invalid number of arguements");
						 return 0;
					 }
					 found_output = 0;
				}
			}
			tokens[p] = NULL;
			FILE* finput;
			FILE* foutput;
		   	finput = fopen(input_file, "r");
			foutput = fopen(output_file,"w");
			if(finput == NULL){
				printf("%s\n","Can't open input file");
				fflush(stdout);
				exit(1);
			}else if(foutput == NULL){
				printf("%s\n","Can't open output file");
				fflush(stdout);
				exit(1);
			}
			else{
				int pid = fork() ;
				if(pid == -1){
					perror("Cannot create child process");
					return -1;
				}
				if (pid == 0) {
					if(donotclose == 1){
						struct sigaction sa1;     //for handling signals SIGINT & SIGQUIT
						sa1.sa_handler = &newhandler;
						if(sigaction(SIGINT,&sa1,NULL) == -1){
							perror("Error : cannot handle SIGINT");
						}
						if(sigaction(SIGQUIT,&sa1,NULL) == -1){
							perror("Error : cannot handle SIGQUIT");
						}
					}
					dup2(fileno(finput), STDIN_FILENO);
					dup2(fileno(foutput),STDOUT_FILENO);
					execute_command(tokens);
					fclose(finput);
					fclose(foutput);
					
					int i2;
					int i3 = 0;
					for(i2=0;tokens[i2]!=NULL || i3 == 1;i2++){
						if(!(tokens[i2]!=NULL)) i3++;
						free(tokens[i2]);
					}
					free(tokens);			
					exit(0);
				}
				else{
					int status,ret_error;
				
					ret_error = wait(&status);
					if(ret_error == -1) {
						printf("%s\n","Not waiting for child");
						fflush(stdout);
						return -1;
					}
					if(WIFSIGNALED(status)){
						fprintf(stderr, "%s\n", "Child exited abnormally" );
						fflush(stdout);
						return -1;
					}
						
					//if(WIFEXITED(status))
					if(WEXITSTATUS(status) != 0){										// Getting the exit status of child process
						fprintf(stderr, "%s\n", "Child exited abnormally" );
						fflush(stdout);
						return -1;
					}
				}	
			 }
		    return 0;	
		}
		else if(find_symbol(tokens) == 2){								// Contains < and >>
			int found_input = 0;
			int found_output = 0;
			int p =0;
			 int c;
			for(c = 0; tokens[c] != NULL; c++){
				if(!strcmp(tokens[c],"<")){
					found_input = 1;
					found_output = 0;
					if(p == 0) p = c;
					c = c + 1;
				}
				if(!strcmp(tokens[c],">>")){
				    found_output = 1;
				    found_input = 0;
				    if(p == 0) p = c;
				    c = c+ 1;
				}
				if(found_input){
					input_file = tokens[c];
					if((tokens[c+1] != NULL) && (strcmp(tokens[c+1],">>") != 0)){
						 printf("%s\n","Invalid number of arguements");
						 return 0;
					 }
					 found_input = 0;
				}
				if(found_output){
					output_file = tokens[c];
					if((tokens[c+1] != NULL) && (strcmp(tokens[c+1],"<") != 0)){
						 printf("%s\n","Invalid number of arguements");
						 return 0;
					 }
					 found_output = 0;
				}
			}
			tokens[p] = NULL;
			
			FILE* finput;
			FILE* foutput;
			finput = fopen(input_file, "r");
			foutput = fopen(output_file,"a");
			if(finput == NULL){
				printf("%s\n","Can't open input file");
				fflush(stdout);
				exit(1);
			}else if(foutput == NULL){
				printf("%s\n","Can't open output file");
				fflush(stdout);
				exit(1);
			}
			else{
				int pid = fork() ;
				if(pid == -1){
					perror("Cannot create child process");
					return -1;
				}
				if (pid == 0) {
					if(donotclose == 1){
						struct sigaction sa1;     //for handling signals SIGINT & SIGQUIT
						sa1.sa_handler = &newhandler;
						if(sigaction(SIGINT,&sa1,NULL) == -1){
							perror("Error : cannot handle SIGINT");
						}
						if(sigaction(SIGQUIT,&sa1,NULL) == -1){
							perror("Error : cannot handle SIGQUIT");
						}
					}
					dup2(fileno(finput), STDIN_FILENO);
					dup2(fileno(foutput),STDOUT_FILENO);
					execute_command(tokens);
					fclose(finput);
					fclose(foutput);
					
					int i2;
					int i3 = 0;
					for(i2=0;tokens[i2]!=NULL || i3 == 1;i2++){
						if(!(tokens[i2]!=NULL)) i3++;
						free(tokens[i2]);
					}
					free(tokens);
					exit(0);
				}
				else{
					int status,ret_error;
				
					ret_error = wait(&status);
					if(ret_error == -1) {
						printf("%s\n","Not waiting for child");
						fflush(stdout);
						return -1;
					}
					if(WIFSIGNALED(status)){
						fprintf(stderr, "%s\n", "Child exited abnormally" );
						fflush(stdout);
						return -1;
					}
						
					//if(WIFEXITED(status))
					if(WEXITSTATUS(status) != 0){										// Getting the exit status of child process
						fprintf(stderr, "%s\n", "Child exited abnormally" );
						fflush(stdout);
						return -1;
					}
				}	
			}
				
		   return 0;
			
		}
		
		else if(find_symbol(tokens) == 3){
			char* input_file;
		    
			int found_input = 0;
			
			int p =0;
			 int c;
			for(c = 0; tokens[c] != NULL; c++){				// contains < 
				if(!strcmp(tokens[c],"<")){
					found_input = 1;
					if(p == 0) p = c;
					c = c + 1;
				}
				if(found_input){
					input_file = tokens[c];
					if((tokens[c+1] != NULL)){
						 printf("%s\n","Invalid number of arguements");
						 return 0;
					 }
					 found_input = 0;
				}
			}
	
			tokens[p] = NULL;
			FILE* finput;
			
		   finput = fopen(input_file, "r");
			if(finput == NULL){
				printf("%s\n","Can't open input file");
				fflush(stdout);
				exit(1);
			}
			else{
				int pid = fork() ;
				if(pid == -1){
					perror("Cannot create child process");
					return -1;
				}
				if (pid == 0) {
					if(donotclose == 1){
						struct sigaction sa1;     //for handling signals SIGINT & SIGQUIT
						sa1.sa_handler = &newhandler;
						if(sigaction(SIGINT,&sa1,NULL) == -1){
							perror("Error : cannot handle SIGINT");
						}
						if(sigaction(SIGQUIT,&sa1,NULL) == -1){
							perror("Error : cannot handle SIGQUIT");
						}
					}
					dup2(fileno(finput), STDIN_FILENO);
					execute_command(tokens);
					fclose(finput);
						
						
					int i2;
					int i3 = 0;
					for(i2=0;tokens[i2]!=NULL || i3 == 1;i2++){
						if(!(tokens[i2]!=NULL)) i3++;
						free(tokens[i2]);
					}
					free(tokens);	
					
					
					exit(0);
				}
				else{
					int status,ret_error;
				
					ret_error = wait(&status);
					if(ret_error == -1) {
						printf("%s\n","Not waiting for child");
						fflush(stdout);
						return -1;
					}
					if(WIFSIGNALED(status)){
						fprintf(stderr, "%s\n", "Child exited abnormally" );
						fflush(stdout);
						return -1;
					}
						
					//if(WIFEXITED(status))
					if(WEXITSTATUS(status) != 0){										// Getting the exit status of child process
						fprintf(stderr, "%s\n", "Child exited abnormally" );
						fflush(stdout);
						return -1;
					}
				}	
			}
		    return 0;
		} 
		else if(find_symbol(tokens) == 4){
			int found_output = 0;
			int p =0;
			int c;
			for(c = 0; tokens[c] != NULL; c++){				// contains < and >
			
				if(!strcmp(tokens[c],">")){
				    found_output = 1;
			
				    if(p == 0) p = c;
				    c = c+ 1;
				}
			
				if(found_output){
					output_file = tokens[c];
					if((tokens[c+1] != NULL)){
						 printf("%s\n","Invalid number of arguements");
						 return 0;
					 }
					 found_output = 0;
				}
			}
			tokens[p] = NULL;
			FILE* foutput;
		  	foutput = fopen(output_file,"w");
			if(foutput == NULL){
				printf("%s\n","Can't open output file");
				fflush(stdout);
				exit(1);
			}
			else{
				int pid = fork() ;
				if(pid == -1){
					perror("Cannot create child process");
					return -1;
				}
				if (pid == 0) {
					if(donotclose == 1){
						struct sigaction sa1;     //for handling signals SIGINT & SIGQUIT
						sa1.sa_handler = &newhandler;
						if(sigaction(SIGINT,&sa1,NULL) == -1){
							perror("Error : cannot handle SIGINT");
						}
						if(sigaction(SIGQUIT,&sa1,NULL) == -1){
							perror("Error : cannot handle SIGQUIT");
						}
					}
					//printf("%s\n","Can't open input file1");
					dup2(fileno(foutput),STDOUT_FILENO);
					execute_command(tokens);
					fclose(foutput);
					int i2;
					int i3 = 0;
					for(i2=0;tokens[i2]!=NULL || i3 == 1;i2++){
						if(!(tokens[i2]!=NULL)) i3++;
						free(tokens[i2]);
					}
					free(tokens);
					exit(0);
				}
				else{
					int status,ret_error;
				
					ret_error = wait(&status);
					if(ret_error == -1) {
						printf("%s\n","Not waiting for child");
						fflush(stdout);
						return -1;
					}
					if(WIFSIGNALED(status)){
						fprintf(stderr, "%s\n", "Child exited abnormally" );
						fflush(stdout);
						return -1;
					}
						
					//if(WIFEXITED(status))
					if(WEXITSTATUS(status) != 0){										// Getting the exit status of child process
						fprintf(stderr, "%s\n", "Child exited abnormally" );
						fflush(stdout);
						return -1;
					}
				}	
			}
			return 0;
		}
		else if(find_symbol(tokens) == 5){
			int found_output = 0;
			int p =0;
			 int c;
			for(c = 0; tokens[c] != NULL; c++){				// contains < and >
			
				if(!strcmp(tokens[c],">>")){
				    found_output = 1;
			
				    if(p == 0) p = c;
				    c = c+ 1;
				}
			
				if(found_output){
					output_file = tokens[c];
					if((tokens[c+1] != NULL)){
						 printf("%s\n","Invalid number of arguements");
						 return 0;
					 }
					 found_output = 0;
				}
			}
			tokens[p] = NULL;
			FILE* foutput;
		   	foutput = fopen(output_file,"a");
			if(foutput == NULL){
				printf("%s\n","Can't open output file");
				fflush(stdout);
				exit(1);
			}
			else{
				int pid = fork() ;
				if(pid == -1){
					perror("Cannot create child process");
					return -1;
				}
				if (pid == 0) {
					if(donotclose == 1){
						struct sigaction sa1;     //for handling signals SIGINT & SIGQUIT
						sa1.sa_handler = &newhandler;
						if(sigaction(SIGINT,&sa1,NULL) == -1){
							perror("Error : cannot handle SIGINT");
						}
						if(sigaction(SIGQUIT,&sa1,NULL) == -1){
							perror("Error : cannot handle SIGQUIT");
						}
					}
					//printf("%s\n","Can't open input file1");
					dup2(fileno(foutput),STDOUT_FILENO);
					execute_command(tokens);
					
					fclose(foutput);
					
					int i2;
					int i3 = 0;
					for(i2=0;tokens[i2]!=NULL || i3 == 1;i2++){
						if(!(tokens[i2]!=NULL)) i3++;
						free(tokens[i2]);
					}
					free(tokens);
					exit(0);
				}
				else{
					int status,ret_error;
				
					ret_error = wait(&status);
					if(ret_error == -1) {
						printf("%s\n","Not waiting for child");
						fflush(stdout);
						return -1;
					}
					if(WIFSIGNALED(status)){
						fprintf(stderr, "%s\n", "Child exited abnormally" );
						fflush(stdout);
						return -1;
					}
						
					//if(WIFEXITED(status))
					if(WEXITSTATUS(status) != 0){										// Getting the exit status of child process
						fprintf(stderr, "%s\n", "Child exited abnormally" );
						fflush(stdout);
						return -1;
					}
				}	
			}
			return 0;
		}
		else if (find_symbol(tokens) == 6){
			//int p =0;
			int c;
			int found1 = 1;
			int found2 = 0;
			char command1[1000];
			char command2[1000];
			strcpy(command1,"");
			strcpy(command2,"");
			for(c = 0;tokens[c] != NULL;c++){
				if(!strcmp(tokens[c],"|")){
					found2 = 1;
					found1 = 0;
					c = c + 1;
				}
				if(found1){
					strcat(command1," ");
					strcat(command1,tokens[c]);
				}else if(found2){
					strcat(command2," ");
					strcat(command2,tokens[c]);
				}
			}
			strcat(command1,"\n");
			strcat(command2,"\n");
			int fds[2];
			pipe(fds);
			int pid1 = fork();
			if(pid1 == 0){
					if(donotclose == 1){
						struct sigaction sa1;     //for handling signals SIGINT & SIGQUIT
						sa1.sa_handler = &newhandler;
						if(sigaction(SIGINT,&sa1,NULL) == -1){
							perror("Error : cannot handle SIGINT");
						}
						if(sigaction(SIGQUIT,&sa1,NULL) == -1){
							perror("Error : cannot handle SIGQUIT");
						}
					}
				close(fds[0]);
				FILE* foutput;
				foutput = fdopen(fds[1],"w");
				dup2(fileno(foutput),STDOUT_FILENO);
				execute_command(tokenize(command1));
				//int i = execvp(tokenize(command1)[0],tokenize(command1) );
				//if(close(fds[0]) == -1) perror("Error in closing file");
				if(close(fds[1]) == -1)perror("Error in closing file");
				//printf("%s\n",command1);
				fflush(stdout);
				exit(0);
			}
			int pid = fork();
			if(pid == 0){
					if(donotclose == 1){
						struct sigaction sa1;     //for handling signals SIGINT & SIGQUIT
						sa1.sa_handler = &newhandler;
						if(sigaction(SIGINT,&sa1,NULL) == -1){
							perror("Error : cannot handle SIGINT");
						}
						if(sigaction(SIGQUIT,&sa1,NULL) == -1){
							perror("Error : cannot handle SIGQUIT");
						}
					}
				
				close(fds[1]);
				FILE* finput;
				finput = fdopen(fds[0],"r");
				dup2(fileno(finput),STDIN_FILENO);
				execute_command(tokenize(command2));
				//int i = execvp(tokenize(command2)[0],tokenize(command2) );								// executing the command not among the given list
					//perror("Exe file not found!!");	
			
				
				if(close(fds[0]) == -1) perror("Error in closing file");
				//if(close(fds[1]) == -1)perror("Error in closing file");
				//printf("%s\n",command2);
				fflush(stdout);
				exit(0);
			}
			close(fds[1]);
			close(fds[0]);
			int status,status1;
			int ret_error = wait(&status);
			int ret_error1 = wait(&status1);
			if((ret_error == -1) || (ret_error1 == -1) ) {
					printf("%s\n","Not waiting for child");
					fflush(stdout);
					return -1;
				}
				if(WIFSIGNALED(status) || WIFSIGNALED(status1)){
					fprintf(stderr, "%s\n", "Child exited abnormally" );
					fflush(stdout);
					return -1;
				}
					
				//if(WIFEXITED(status))
				if((WEXITSTATUS(status) != 0) || (WEXITSTATUS(status1) != 0)){						// Getting the exit status of child process
					fprintf(stderr, "%s\n", "Child exited abnormally" );
					fflush(stdout);
					return -1;
				}
			return 0;
		}
		else if(find_symbol(tokens) == -1) {
			printf("%s\n", "Invalid command");
			fflush(stdout);
			return -1;
		}
				
				
		/*else if(tokenscontains(tokens,  '>>'){
			
		}
		else if(tokenscontains(tokens,  '>'){
			
		}
		else if(tokenscontains(tokens, '|'){
			
		}*/
	
	    
			
	       else if (!strcmp(tokens[0],"exit")) {
			/* Quit the running process */
			set<int>::iterator iter = background_process.begin();
			if(iter != background_process.end()){
				kill(*iter,SIGTERM);
				cout << "Child Process " << *iter << endl;
				iter++;
			}
			cout << " All have closed" << endl;
			newhandler(0);
			exit(0);
			return 0 ;

		} else if (!strcmp(tokens[0],"cd")) {
			/* Change Directory, or print error on failure */
			int i=0;
			while(tokens[i] != NULL){
				i++;
			}
			if(i != 2){
				printf("%s\n", "Invalid number of arguments!!");
				fflush(stdout);
				return -1;
			}
			else{
				char* path = tokens[1];
				int i = chdir(path);
				//printf("%s\n","cd_success");
				fflush(stdout);
				if(i == -1){
					perror("Directory not found!!");	
					fflush(stdout);
					return -1;
				}
			}
			return 0 ;
		} 
		 else {
			/* Either file is to be executed or batch file to be run */
			/* Child process creation (needed for both cases) */
			int pid = fork() ;

			if(pid == -1){
				perror("Cannot create child process");
				return -1;
			}

			if (pid == 0) {
				prctl(PR_SET_PDEATHSIG,SIGHUP);
				if(donotclose == 1){
					struct sigaction sa1;     //for handling signals SIGINT & SIGQUIT
					sa1.sa_handler = &newhandler;
					if(sigaction(SIGINT,&sa1,NULL) == -1){
						perror("Error : cannot handle SIGINT");
					}
					if(sigaction(SIGQUIT,&sa1,NULL) == -1){
						perror("Error : cannot handle SIGQUIT");
					}
				}


				if (!strcmp(tokens[0],"run")) {
					/* Locate file and run commands */
					/* May require recursive call to execute_command */
					/* Exit child with or without error */

					int i=0;
					while(tokens[i] != NULL){
						i++;
					}
					if(i != 2){												// LOOKING FOR NUMBER OF ARGUEMENTS
						printf("%s\n", "Invalid number of argument!!");
						fflush(stdout);
						exit(1);
					}

					FILE *fp;						//file descriptor
					fp = fopen(tokens[1], "r");
					if(fp == NULL){
						printf("%s\n","Can't open input file");
						fflush(stdout);
						exit(1);
					}
					else{
						char* lineInput = NULL;
						size_t len =0;
						ssize_t read;

						while((read = getline(&lineInput, &len, fp)) != -1){
							//printf("%zu :\n", read );
							//printf("%s", lineInput);
							//fflush(stdout);
				
							//printf("%s\n","done");
							execute_command(tokenize(lineInput));
						}
						fclose(fp);
						if(lineInput){
							free(lineInput);
						}
					}
					int i1 ;
					for(i1=0;tokens[i1]!=NULL;i1++){
						free(tokens[i1]);
					}
					free(tokens);
					exit (0);
				}
				else {
					/* File Execution */
					/* Print error on failure, exit with error*/
					fflush(stdout);
					int i = execvp(tokens[0], tokens);								// executing the command not among the given list
					perror("Exe file not found!!");								
					int i2 ;
					for(i2=0;tokens[i2]!=NULL;i2++){
						free(tokens[i2]);
					}
					free(tokens);			
					exit(1);
				}
			}
			else {
				/* Parent Process */
				/* Wait for child process to complete */
				//printf("%d\n", pid);
				//fflush(stdout);
				int status,ret_error;
				//printf("%d\n", status);
				ret_error = wait(&status);
				//printf("%d\n", status);
				fflush(stdout);
				if(ret_error == -1) {
					printf("%s\n","Not waiting for child");
					fflush(stdout);
					return -1;
				}
				if(WIFSIGNALED(status)){
					fprintf(stderr, "%s\n", "Child exited abnormally" );
					fflush(stdout);
					return -1;
				}
					
				//if(WIFEXITED(status))
				if(WEXITSTATUS(status) != 0){										// Getting the exit status of child process
					fprintf(stderr, "%s\n", "Child exited abnormally" );
					fflush(stdout);
					return -1;
				}
				return 0 ;
			}
		}
	}
	
	return 0 ;
}
