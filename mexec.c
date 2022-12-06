/*
 * @author  Vincent Johansson
 * @since   2022-10-06
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>

#define MAX_LINE 1024
#define READ 0
#define WRITE 1

/* Struct for keeping track of amount of commands user puts in,
 * how many commands have been entered by user.
 */
typedef struct handler
{
    FILE *input;
    char **saved_input;
    char **args;
    int c_input;
    int n_input;
    int n_cmd;
    int c_cmd;
} handler;

/* ---- Function declaration ---- */
void set_input(int argc, char *argv[], handler *ha);
void read_input(handler *ha);
void *init_struct(int argc, char *argv[]);
void sep_args(handler *ha, int n);
void exec_cmd(handler *ha, int fd[][2], pid_t pid[], int pid_num);
void *safe_calloc(size_t num);
void realloc_buff(char ***buffer, int size);
void safe_dup(int pipe_end, int fd[], handler *ha);
void safe_exit(handler *ha, bool child);

int main(int argc, char *argv[])
{
    /* Print error if more than one argument has been given. */
    if (argc > 2)
    {
        fprintf(stderr, "usage: ./mexec [FILE]\n");
        exit(EXIT_FAILURE);
    }

    /*
     * Allocate memory for handler struct, initialize variables
     * and allocate memory for arrays
     */
    handler *ha = init_struct(argc, argv);

    /* Read from stdin saved in ha struct */
    read_input(ha);

    /* pid and pipe declaration */
    pid_t pid[ha->c_input];
    int fd[ha->c_input][2];

    /* Loop through saved input and execute */
    for (int i = 0; i < ha->c_input; i++)
    {
        sep_args(ha, i);
        exec_cmd(ha, fd, pid, i);

        /* Free memory after commands have been executed */
        for (int i = 0; i < ha->c_cmd; i++)
        {
            free(ha->args[i]);
        }
        ha->c_cmd = 0;
    }

    /* Wait for child processes */
    int status;
    for (int i = 0; i < ha->c_input; i++)
    {
        if (waitpid(pid[i], &status, WCONTINUED | WUNTRACED) < 0)
        {
            perror("waitpid");
            safe_exit(ha, false);
            exit(errno);
        }

        /* If child closes unexpectedly, exit parent with childs exit code. */
        if (WEXITSTATUS(status) != 0)
        {
            safe_exit(ha, false);
            exit(WEXITSTATUS(status));
        }
    }

    /* Free allocated memory */
    safe_exit(ha, false);

    exit(EXIT_SUCCESS);
}

/* - - - - - - - - - - FUNCTIONS - - - - - - - - - - */

/**
 * @brief Used to assign where input will be read from
 *
 * @param argc
 * @param argv
 * @param ha        handler struct
 */
void set_input(int argc, char *argv[], handler *ha)
{
    if (argc == 1)
    {
        ha->input = stdin;
    }
    else if (argc == 2)
    {
        ha->input = fopen(argv[1], "r");
        if (ha->input == NULL)
        {
            perror(argv[1]);
            safe_exit(ha, false);
            exit(errno);
        }
    }
}

/**
 * @brief Reads input from given filestream and stores it in a string matrix
 *
 * @param ha            handler struct
 */
void read_input(handler *ha)
{
    char user_input[MAX_LINE] = {'\0'};

    /*
     * Loop fgets() to get user input.
     * save each line of commands in a buffer.
     */
    while (fgets(user_input, MAX_LINE, ha->input) != NULL)
    {
        if (ha->c_input >= ha->n_input)
        {
            realloc_buff(&ha->saved_input, ha->n_input);
        }
        strcpy(ha->saved_input[ha->c_input], user_input);
        memset(user_input, 0, MAX_LINE);
        ha->c_input++;
    }
}

/**
 * @brief Initializes the handler struct
 *
 * @param argc
 * @param argv
 * @return void*    pointer to allocated memory for the struct
 */
void *init_struct(int argc, char *argv[])
{
    /* Allocate memory for handler struct */
    handler *ha = safe_calloc(sizeof(handler));

    /* Initialize values */
    ha->n_input = 50;
    ha->n_cmd = 50;
    ha->c_cmd = 0;
    ha->c_input = 0;

    /* Allocate memory for buffers */
    ha->saved_input = safe_calloc(sizeof(*ha->saved_input) * ha->n_input);
    ha->args = safe_calloc(sizeof(*ha->args) * MAX_LINE);
    for (int i = 0; i < ha->n_input; i++)
    {
        ha->saved_input[i] = safe_calloc(sizeof(char) * MAX_LINE);
    }

    /* Set ha->input depending on how input is given */
    set_input(argc, argv, ha);

    return ha;
}

/**
 * @brief Separates commands and arguments from the saved input
 *
 * @param ha    handler struct
 * @param n     integer specifying which string of commands and
 *              arguments to split
 */
void sep_args(handler *ha, int n)
{
    char *token;
    const char delim[] = " \n";

    token = strtok(ha->saved_input[n], delim);

    int i = 0;
    while (token != NULL)
    {
        if (i > ha->n_cmd)
        {
            realloc_buff(&ha->args, ha->n_cmd);
        }
        ha->args[i] = calloc(1, sizeof(char) * strlen(token) + 1);
        if (ha->args[i] == NULL)
        {
            perror("calloc()");
            exit(EXIT_FAILURE);
        }
        strncpy(ha->args[i], token, strlen(token) + 1);
        strncat(ha->args[i], "\0", 2);
        token = strtok(NULL, delim);
        i++;
        ha->c_cmd++;
    }
    ha->args[i] = NULL;
}

/**
 * @brief Function which executes the commands read from input
 *
 * @param ha        handler struct
 * @param fd        array of file descriptors
 * @param pid       process id
 * @param pid_num   integer tracking which process is to be run
 */
void exec_cmd(handler *ha, int fd[][2], pid_t pid[], int pid_num)
{
    /* Initiate pipe unless it's the last child where
        a new pipe won't be used */
    if (pid_num != ha->c_input - 1)
    {
        if (pipe(fd[pid_num]) != 0)
        {
            perror("pipe error");
            safe_exit(ha, false);
            exit(errno);
        }
    }

    switch (pid[pid_num] = fork())
    {
    case -1:
        perror("fork error");
        safe_exit(ha, false);
        exit(errno);
    case 0: /* Child process */
        /* If only one command has been given, execvp right away. */
        if (ha->c_input == 1)
        {
            if (execvp(ha->args[0], ha->args) < 0)
            {
                perror(ha->args[0]);
                safe_exit(ha, true);
                exit(errno);
            }
        }
        else
        {
            /* Redirect stdout and stdin to pipe
                If it's the last process keep stdout open
                and close write end of pipe */
            if (pid_num + 1 == ha->c_input)
            {
                safe_dup(READ, fd[pid_num - 1], ha);
            }
            else
            {
                /* First child won't redirect stdin */
                if (pid_num == 0)
                {
                    safe_dup(WRITE, fd[pid_num], ha);
                }
                else
                {
                    safe_dup(WRITE, fd[pid_num], ha);
                    safe_dup(READ, fd[pid_num - 1], ha);
                }
            }

            /* Close all unused pipes */
            for (int i = 0; i <= pid_num - 1; i++)
            {
                close(fd[i][WRITE]);
                close(fd[i][READ]);
            }

            /* Execute command */
            if (execvp(ha->args[0], ha->args) < 0)
            {
                perror(ha->args[0]);
                safe_exit(ha, true);
                exit(errno);
            }
        }
        break;
    default: /* Parent process */
        /*
         * Close unused pipes.
         * Only close READ end after child has redirected it.
         */
        if (pid_num != ha->c_input - 1)
        {
            close(fd[pid_num][WRITE]);
        }

        if (pid_num > 0)
        {
            close(fd[pid_num - 1][READ]);
        }

        break;
    }
}

/**
 * @brief Safe usage of calloc() function
 *
 * @param size      size of memory to be allocated
 * @return void*    pointer to allocated memory
 */
void *safe_calloc(size_t size)
{
    void *mem_block;

    if ((mem_block = calloc(1, size)) == NULL)
    {
        perror(strerror(errno));
        exit(errno);
    }

    return mem_block;
}

/**
 * @brief Function used to reallocate memory
 *
 * @param buffer    pointer to char **buffer
 * @param size      integer with current size
 */
void realloc_buff(char ***buffer, int size)
{
    char **temp;

    size *= 2;

    temp = realloc(*buffer, sizeof(char *) * size);
    if (temp == NULL)
    {
        perror("realloc");
        exit(EXIT_FAILURE);
    }
    *buffer = temp;

    for (int i = 0; i < size; i++)
    {
        char *temp2;
        temp2 = realloc(*buffer[i], sizeof(char) * MAX_LINE);
        if (temp2 == NULL)
        {
            perror("realloc");
            exit(EXIT_FAILURE);
        }
        else
        {
            *buffer[i] = temp2;
        }
    }
}

/**
 * @brief Safe usage of dup2() function
 *
 * @param pipe_end      Define which end of pipe to be redirected
 * @param fd            Pipe to be redirected
 * @param ha            handler
 */
void safe_dup(int pipe_end, int fd[], handler *ha)
{
    if (pipe_end == WRITE)
    {
        if (dup2(fd[WRITE], STDOUT_FILENO) < 0)
        {
            perror("dup2 error");
            safe_exit(ha, true);
            exit(errno);
        }
    }
    else
    {
        if (dup2(fd[READ], STDIN_FILENO) < 0)
        {
            perror("dup2 error");
            safe_exit(ha, true);
        }
    }
}

/**
 * @brief Frees allocated memory before program exit
 *
 * @param ha            handler struct
 * @param child         bool true if func is called in childprocess
 */
void safe_exit(handler *ha, bool child)
{
    /* Free args if child process */
    if (child)
    {
        for (int i = 0; i < ha->c_cmd; i++)
        {
            free(ha->args[i]);
        }
    }

    /* Only try to close if input has been read */
    if (ha->c_input != 0)
    {
        fclose(ha->input);
    }

    for (int i = 0; i < ha->n_input; i++)
    {
        free(ha->saved_input[i]);
    }
    free(ha->saved_input);
    free(ha->args);
    free(ha);
}
