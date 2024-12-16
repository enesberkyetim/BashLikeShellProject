#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */

/* The setup function below will not return any value, but it will just: read
in the next command line; separate it into distinct arguments (using blanks as
delimiters), and set the args array entries to point to the beginning of what
will become null-terminated, C-style strings. */

int background_process_counter = 0;
pid_t background_pids[MAX_LINE/2 + 1];
char * historyCount[10][MAX_LINE/2 + 1];
int historyCountLine =0;
pid_t currently_running_foreground;

void add_history(char *args[]) {
	int i = 0;
	if (!(strcmp(args[0],"history"))) {

	}
	else {
		int count =0;
		for (i=0;i<MAX_LINE/2+1;i++) {
			if (args[i] != NULL) {
				count++;
			}
			else {
				break;
			}

		}

		if (historyCountLine == 0) {
			for (i=0;i<count;i++) {
				char *arg = malloc(sizeof(char)*(strlen(args[i])));
				strcpy(arg,args[i]);
				historyCount[0][i] = arg;
				historyCount[0][count]= NULL;
			}
			historyCountLine++;

		}
		else {
			for (int j=historyCountLine;j>=0;j--) {
				for (i=0;i<MAX_LINE/2 + 1;i++){
					if (historyCount[j][i] != NULL) {
						char *arg = malloc(sizeof(char)*(strlen(historyCount[j][i])));
						strcpy(arg,historyCount[j][i]);
						historyCount[j + 1][i] = arg;
					}
					else {
						historyCount[j + 1][i] = NULL;
					}
				}
			}

			for (i=0;i<count;i++){
				char *arg = malloc(sizeof(char)*(strlen(args[i])));
				strcpy(arg,args[i]);
				historyCount[0][i] = arg;
				historyCount[0][count]= NULL;
			}

			if (historyCountLine == 8) {
				historyCountLine = 8;
			}
			else {
				historyCountLine++;
			}
		}
	}
}

void setup(char inputBuffer[], char *args[],int *background)
{
    int length, /* # of characters in the command line */
        i,      /* loop index for accessing inputBuffer array */
        start,  /* index where beginning of next command parameter is */
        ct;     /* index of where to place the next parameter into args[] */

    ct = 0;

    /* read what the user enters on the command line */

    length = read(STDIN_FILENO,inputBuffer,MAX_LINE);

    /* 0 is the system predefined file descriptor for stdin (standard input),
       which is the user's screen in this case. inputBuffer by itself is the
       same as &inputBuffer[0], i.e. the starting address of where to store
       the command that is read, and length holds the number of characters
       read in. inputBuffer is not a null terminated C-string. */

    start = -1;
    if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */

/* the signal interrupted the read system call */
/* if the process is in the read() system call, read returns -1
  However, if this occurs, errno is set to EINTR. We can check this  value
  and disregard the -1 value */
    if ( (length < 0) && (errno != EINTR) ) {
        perror("error reading the command");
	exit(-1);           /* terminate with error code of -1 */
    }

	//printf(">>%s<<",inputBuffer);
    for (i=0;i<length;i++){ /* examine every character in the inputBuffer */

        switch (inputBuffer[i]){
	    case ' ':
	    case '\t' :               /* argument separators */
		if(start != -1){
                    args[ct] = &inputBuffer[start];    /* set up pointer */
		    ct++;
		}
                inputBuffer[i] = '\0'; /* add a null char; make a C string */
		start = -1;
		break;

            case '\n':                 /* should be the final char examined */
		if (start != -1){
                    args[ct] = &inputBuffer[start];
		    ct++;
		}
                inputBuffer[i] = '\0';
                args[ct] = NULL; /* no more arguments to this command */
		break;

	    default :             /* some other character */
		if (start == -1)
		    start = i;
                if (inputBuffer[i] == '&'){
		    *background  = 1;
                    inputBuffer[i-1] = '\0';
		}
	} /* end of switch */
     }    /* end of for */
     args[ct] = NULL;

	/* just in case the input line was > 80 */
	char **arg = malloc(sizeof(args));
	memcpy(arg,args,sizeof(args));
	add_history(args);


}



 /* end of setup routine */

void background_to_foreground(pid_t pid) {
	waitpid(pid, NULL, 0);
	return;
}

void ctrl_z(int signal_number) {
	char message[] = "\nCTRL-Z pressed : Foreground process terminated\n";


	// this loop will be useful for exit command (when we will implement it)
	// user will have to carry bg processes to fg and kill them before exiting
	// so we check that is the current fg process that will be killed were a bg process before
	// and erase its pid from out background_pids list
	for (int i = 0; i < MAX_LINE/2 + 1; i++) {
		if (background_pids[i] == currently_running_foreground) {
			background_pids[i] = 0;
		}
	}

	// inform the user then kill the fg process
	write(STDOUT_FILENO,message,strlen(message));
	kill(currently_running_foreground,SIGKILL);
}

char ** retrieve_path_env(char **paths) {
	char *path_of_enviroment = getenv("PATH");

	for (int i = 0; i < 100; i++) {
		char *empty = malloc(sizeof(char) * 100);
		for (int j = 0; j < 100; j++) {
			empty[j] = '\0';
		}
		paths[i] = empty;
	}



	for (int i = 0, j = 0, k = 0; path_of_enviroment[i] != '\0'; i++) {
		if (path_of_enviroment[i] == ':') {
			paths[j][k] = '/';
			paths[j][k + 1] = '\0';
			k = 0;
			j++;
		}
		else if (path_of_enviroment[i + 1] == '\0') {
			paths[j][k] = '/';
			paths[j][k + 1] = '\0';
			k = 0;
			j++;
			break;
		}
		else {
			paths[j][k] = path_of_enviroment[i];
			k++;
		}
	}
}

void background_execution(char *command, char *command_args[]) {
	char *paths[100];
	retrieve_path_env(paths);

	pid_t child_pid;
	int path_length = 9 + strlen(command);

	child_pid = fork();

	if (child_pid == -1) {
		perror("Failed to fork (Foreground)");
	}

	if (child_pid == 0) {
		char path[path_length];
		for (int m = 0; m < path_length; m++) {
			path[m] = '\0';
		}

		for (int i = 0; i < 100; i++) {
			if (paths[i] == NULL) {
				break;
			}
			else {
				strcat(paths[i], command);

				struct stat buffer;
				if (!stat(paths[i], &buffer)) {
					execv(paths[i], command_args);
				}
				else {
					continue;
				}
			}
		}
	}
	else {
		background_pids[background_process_counter] = child_pid;
		background_process_counter++;
	}
}



void foreground_execution(char *command, char *command_args[]) {
	char *paths[100];
	retrieve_path_env(paths);

	pid_t child_pid;
	int path_length = 9 + strlen(command);

	child_pid = fork();

	if (child_pid == -1) {
		perror("Failed to fork (Foreground)");
	}

	if (child_pid == 0) {
		char path[path_length];
		for (int m = 0; m < path_length; m++) {
			path[m] = '\0';
		}

		for (int i = 0; i < 100; i++) {
			if (paths[i] == NULL) {
				break;
			}
			else {
				strcat(paths[i], command);

				struct stat buffer;
				if (!stat(paths[i], &buffer)) {
					execv(paths[i], command_args);
				}
				else {
					continue;
				}
			}
		}
	}
	else {
		currently_running_foreground = child_pid;

		// setting up the signal handling for the ctrl z

		struct sigaction action;
		int status;

		action.sa_handler = ctrl_z;
		action.sa_flags = 0;
		status = sigemptyset(&action.sa_mask);

		if (status == -1) {
			perror("Failed to initialize the signal set");
			exit(-1);
		}

		status = sigaction(SIGTSTP, &action, NULL);

		if (status == -1) {
			perror("Failed to install SIGTSTP signal handler");
			exit(-1);
		}

		wait(NULL);
	}
}

void io_redirection_execution(char *command, char *command_args[], char *redirection_type, char *input_file_name, char *output_file_name) {
	// aşağıdaki variavle'ı debug için koydum. burası io_redirection yapılacaksa çağırılıyor
	int a = 0;
}



void execution_controller(char* args[],int background) {
	int i = 0;


	if (!(strcmp(args[0], "history"))){
		//if history has no argument
		if (args[1] == NULL) {
			printf("\n");
			for (int m = 0; m < 10; m++) {
				if (historyCount[m] != NULL) {
					printf("%d ", m);
					for (int n = 0; n < MAX_LINE/2 +1 ; n++) {
						if (historyCount[m][n] == NULL) {
							break;
						}
						printf("%s " ,historyCount[m][n]);
						// a
					}
					printf("\n");
				}
			}
		}
		//if history has arguments
		else {
			int line = atoi(args[2]);
			char **arg = malloc(sizeof(historyCount[line]));
			memcpy(arg,historyCount[line],sizeof(historyCount[line]));
			add_history(arg);
			execution_controller(arg, 0);
		}

	}
	if (!(strcmp(args[0], "fg"))) {
		int pid;
		char pid_str[50];

		for (int i = 1; args[1][i] != '\0'; i++) {
			pid_str[i - 1] = args[1][i];
		}
		pid_str[i - 1] = '\0';
		pid = atoi(pid_str);

		background_to_foreground(pid);
	}
	else if (!(strcmp(args[0], "exit"))) {
		int bg_process_counter = 0;
		for (int i = 0; i < MAX_LINE/2 + 1; i++) {
			if (background_pids[i] == 0) {
				continue;
			}
			else {
				if (bg_process_counter == 0) {
					printf("\nThere are currently running background processes : \n");
				}

				printf("PID : %d\n", background_pids[i]);
				bg_process_counter++;
			}
		}

		if (bg_process_counter == 0) {
			printf("\nTerminating the shell..\n");
			exit(0);
		}
	}
	else if (isalpha(args[0][0]) && background == 0) {
		char *command_args[MAX_LINE/2 + 1];
		for (int j = 0; j < (MAX_LINE/2 + 1); j++) {
				command_args[j] = NULL;
		}

		i++;
		int command_args_index = 0;

		while (args[i] != NULL) {
			if (args[i][0] == '|' || args[i][0] == '<' || args[i][0] == '>' || (args[i][0] == '2' && args[i][1] == '>')) {

				// Here will be the C (i/o redirection) part
				// It will be nice if you implement this inside a function

				i = 1;
				command_args_index = 0;
				char *input_file_name = NULL;
				char *output_file_name = NULL;
				char *redirection_type = NULL;

				for (int j = 0; j < (MAX_LINE/2 + 1); j++) {
					command_args[j] = NULL;
				}

				int double_redirection = 0;

				while (args[i] != NULL) {
					if (args[i][0] == '|' || args[i][0] == '<' || args[i][0] == '>' || (args[i][0] == '2' && args[i][1] == '>')) {
						if (args[i][0] == '<') {
							int j = i + 1;

							while (args[j] != NULL) {
								if (args[j][0] == '>') {
									input_file_name = args[j - 1];
									output_file_name = args[j + 1];
									double_redirection = 1;
									break;
								}
								j++;
							}

							if (double_redirection) {
								redirection_type = "<>";
								break;
							}
							else {
								redirection_type = args[i];
								input_file_name = args[i + 1];
								break;
							}
						}
						else if (args[i][0] == '>') {
							redirection_type = args[i];
							output_file_name = args[i + 1];
							break;
						}
						else if (args[i][0] == '2' && args[i][1] == '>') {
							redirection_type = args[i];
							output_file_name = args[i + 1];
							break;
						}
					}
					else {
						command_args[command_args_index] = args[i];
						command_args_index++;
					}
					i++;
				}
				io_redirection_execution(args[0], command_args, redirection_type, input_file_name, output_file_name);
				return;
			}
			else {
				command_args[command_args_index] = args[i];
				command_args_index++;
				i++;
			}
		}

		if (command_args[0] == NULL) {
			foreground_execution(args[0], NULL);
			return;
		}

		int j = 0;
		while (command_args[j] != NULL) {
			j++;
		}

		char *command_args_sent[j + 2];
		command_args_sent[0] = args[0];
		command_args_sent[j + 1] = NULL;

		for (int k = 1; k < j + 1; k++) {
			command_args_sent[k] = command_args[k - 1];
		}

		foreground_execution(args[0], command_args_sent);
	}
	else if (isalpha(args[0][0]) && background == 1) {
		char *command_args[MAX_LINE/2 + 1];
		for (int j = 0; j < (MAX_LINE/2 + 1); j++) {
			command_args[j] = NULL;
		}

		i++;
		int command_args_index = 0;

		while (args[i] != NULL) {
			if (args[i][0] == '|' || args[i][0] == '<' || args[i][0] == '>' || (args[i][0] == '2' && args[i][1] == '>')) {
				// Here will be the C (i/o redirection) part
				// It will be nice if you implement this inside a function
				return;
			}
			else {
				command_args[command_args_index] = args[i];
				command_args_index++;
				i++;
			}
		}

		if (command_args[0] == NULL) {
			background_execution(args[0], NULL);
			return;
		}

		int j = 0;
		while (command_args[j] != NULL) {
			j++;
		}

		char *command_args_sent[j + 2];
		command_args_sent[0] = args[0];
		command_args_sent[j + 1] = NULL;

		for (int k = 1; k < j + 1; k++) {
			command_args_sent[k] = command_args[k - 1];
		}

		background_execution(args[0], command_args_sent);

	}


}




int main(void)
{

	char inputBuffer[MAX_LINE]; /*buffer to hold command entered */
	int background = 0; /* equals 1 if a command is followed by '&' */
	char *args[MAX_LINE/2 + 1]; /*command line arguments */

	for (int i=0;i<10;i++) {
		for (int j =0;j<MAX_LINE/2+1;j++) {
			historyCount[i][j] = NULL;
		}
	}

	while (1){
		printf("myshell: ");
            	/*setup() calls exit() when Control-D is entered */
		setup(inputBuffer, args, &background);

            	/** the steps are:
                (1) fork a child process using fork()
                (2) the child process will invoke execv()
				(3) if background == 0, the parent will wait,
                otherwise it will invoke the setup() function again. */
		execution_controller(args, background);
	}
}