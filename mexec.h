/**
 * @brief Main application for mexec
 *
 * @author Vincent Johansson <dv14vjn>
 * @since 2022-10-06
 */

/**
 * @brief Used to assign where input will be read from
 *
 * @param argc
 * @param argv
 * @param ha        handler struct
 */
void set_input(int argc, char *argv[], handler *ha);

/**
 * @brief Reads input from given filestream and stores it in a string matrix
 *
 * @param ha            handler struct
 */
void read_input(handler *ha);

/**
 * @brief Initializes the handler struct
 *
 * @param argc
 * @param argv
 * @return void*    pointer to allocated memory for the struct
 */
void *init_struct(int argc, char *argv[]);

/**
 * @brief Separates commands and arguments from the saved input
 *
 * @param ha    handler struct
 * @param n     integer specifying which string of commands and
 *              arguments to split
 */
void sep_args(handler *ha, int n);

/**
 * @brief Function which executes the commands read from input
 *
 * @param ha        handler struct
 * @param fd        array of file descriptors
 * @param pid       process id
 * @param pid_num   integer tracking which process is to be run
 */
void exec_cmd(handler *ha, int fd[][2], pid_t pid[], int pid_num);

/**
 * @brief Safe usage of calloc() function
 *
 * @param size      size of memory to be allocated
 * @return void*    pointer to allocated memory
 */
void *safe_calloc(size_t size);

/**
 * @brief Function used ro reallocate memory
 *
 * @param buffer    pointer to char **buffer
 * @param size      integer with current size
 */
void realloc_buff(char ***buffer, int size);

/**
 * @brief Safe usage of dup2() function
 *
 * @param pipe_end      Define which end of pipe to be redirected
 * @param fd            Pipe to be redirected
 * @param ha            handler
 */
void safe_dup(int pipe_end, int fd[], handler *ha);

/**
 * @brief Frees allocated memory before program exit
 *
 * @param ha            handler struct
 * @param child         bool true if func is called in childprocess
 */
void safe_exit(handler *ha, bool child);
