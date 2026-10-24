/**
 * @file execute.c
 *
 * @brief Implements interface functions between Quash and the environment and
 * functions that interpret an execute commands.
 *
 * @note As you add things to this file you may want to change the method signature
 */

#include "execute.h"

#include <stdio.h>
#include <string.h>

#include <sys/wait.h>
#include <sys/types.h>

#include "quash.h"

#include "deque.h"

// Remove this and all expansion calls to it
/**
 * @brief Note calls to any function that requires implementation
 */
#define IMPLEMENT_ME()                                                  \
  fprintf(stderr, "IMPLEMENT ME: %s(line %d): %s()\n", __FILE__, __LINE__, __FUNCTION__)



#define READ 0
#define WRITE 1



/* Job Queue */

#define MAX_NUMBER_OF_PIDS 10 
// if there are more than 10, error out
// this can be solved with dynamic array. 
// don't feel like it tbh lol

typedef struct Job{
  int job_id;
  pid_t pid;
  char* cmd_str;
  pid_t all_pids[MAX_NUMBER_OF_PIDS];
  int total_processes;
  
} Job;





IMPLEMENT_DEQUE_STRUCT(JobQueue, Job);
IMPLEMENT_DEQUE(JobQueue, Job);
PROTOTYPE_DEQUE(JobQueue, Job);

bool is_job_queue_initialized = false;
JobQueue job_queue;

int max_job_id = 1;

void assign_job_id(Job* job){
  job->job_id = max_job_id;
  max_job_id++;
}

/* Global Redirect Struct */
typedef struct Redirect{
  char* in;
  char* out;
  bool append;
} Redirect;

Redirect global_redirect;

/***************************************************************************
 * Interface Functions
 ***************************************************************************/

// Return a string containing the current working directory.
char* get_current_directory(bool* should_free) {
  // TODO: Get the current working directory. This will fix the prompt path.
  // HINT: This should be pretty simple
  
  char* cwd_str; //static to save it when it leaves scope
  cwd_str = (char*)getcwd(NULL, 0);  //this is a malloc

  if (cwd_str == NULL){
    perror("ERROR: Unable to print CWD");
    exit(EXIT_FAILURE);
    // either that or return NULL
    return NULL;
  }

  // Change this to true if necessary
  *should_free = true;

  return cwd_str;
}

// Returns the value of an environment variable env_var
const char* lookup_env(const char* env_var) {
  // TODO: Lookup environment variables. This is required for parser to be able
  // to interpret variables from the command line and display the prompt
  // correctly
  // HINT: This should be pretty simple

  return getenv(env_var);
}

void write_env(const char* env_var, const char* val){

  // this value when non-zero will overwrite the current env_var if already set
  const bool overwrite = true; 
  if (setenv(env_var, val, overwrite) == -1){
    perror("ERROR: Unable to export");
  }  

}

// Check the status of background jobs
void check_jobs_bg_status() {
  // TODO: Check on the statuses of all processes belonging to all background
  // jobs. This function should remove jobs from the jobs queue once all
  // processes belonging to a job have completed.
  // IMPLEMENT_ME();

  // TODO: Once jobs are implemented, uncomment and fill the following line
  // print_job_bg_complete(job_id, pid, cmd);
}

// Prints the job id number, the process id of the first process belonging to
// the Job, and the command string associated with this job
void print_job(int job_id, pid_t pid, const char* cmd) {
  printf("[%d]\t%8d\t%s\n", job_id, pid, cmd);
  fflush(stdout);
}

// Prints a start up message for background processes
void print_job_bg_start(int job_id, pid_t pid, const char* cmd) {
  printf("Background job started: ");
  print_job(job_id, pid, cmd);
}

// Prints a completion message followed by the print job
void print_job_bg_complete(int job_id, pid_t pid, const char* cmd) {
  printf("Completed: \t");
  print_job(job_id, pid, cmd);
}

/***************************************************************************
 * Functions to process commands
 ***************************************************************************/
// Run a program reachable by the path environment variable, relative path, or
// absolute path
void run_generic(GenericCommand cmd) {
  // Execute a program with a list of arguments. The `args` array is a NULL
  // terminated (last string is always NULL) list of strings. The first element
  // in the array is the executable
  char* exec = cmd.args[0];
  char** args = cmd.args;
/*   char* s;

  size_t count = 0;
  fprintf(stdout, "exec = %s\n",exec);

  for (s = *args; s != NULL; s=*++args){
    fprintf(stdout, "%s, ", s);
    count += 1;
  }
  fprintf(stdout, "\n");


  for (; count != 0; count--){
    args--;
  }
 */



  if (execvp(exec, args) == -1){
    perror("ERROR: Failed to execute program");
    exit(EXIT_FAILURE);
  }
  
}

// Print strings
void run_echo(EchoCommand cmd) {
  // Print an array of strings. The args array is a NULL terminated (last
  // string is always NULL) list of strings.
  size_t argc = 0;
  size_t start_pos = 0;

  char **arg_strs = cmd.args;
  bool print_new_line = true;

  // bool allow_options = false;

  // get arg_count
  while(*arg_strs != NULL){
    arg_strs++;
    argc++;
  }

  for (size_t i = 0; i < argc; i++){
    arg_strs--;
  }
  // fprintf(stdout, "argc = %d\n", argc); // for debugging in case you want to see argc

  
  if (arg_strs[0] == NULL)
  {
    ;
  }
  else if (strcmp(arg_strs[0], "--help") == 0 && argc == 1)  // 0 means that the strings match
  {
    fprintf(stdout, "insert man page here");
  }
  else if (strcmp(arg_strs[0], "--version") == 0 && argc == 1)
  {
    fprintf(stdout, "insert version here");
  }
  else if (strcmp(arg_strs[0], "-n") == 0 && argc >= 2)
  {
    start_pos++;  // increment starting position to account for -n
    print_new_line = false;

  }

 
  for (size_t i = start_pos; i < argc; i++){

  
    fprintf(stdout, "%s", arg_strs[i]);
    if ((i + 1) != argc){
      fprintf(stdout, " ");
    }
  }

  if (print_new_line == true){
    fprintf(stdout, "\n");
  }

  // Flush the buffer before returning
  fflush(stdout);
  
}


// Sets an environment variable
void run_export(ExportCommand cmd) {
  // Write an environment variable
  const char* env_var = cmd.env_var;
  // printf("env_var = %s\n", env_var); For DEBUG to see env_var
  const char* val = cmd.val;

  write_env(env_var, val);

}

// Changes the current working directory
void run_cd(CDCommand cmd) {
  // Get the directory name
  const char* dir = cmd.dir;

  // Check if the directory is valid
  if (dir == NULL) {
    perror("ERROR: Failed to resolve path");
    return;
  }
  

  bool should_free = true;
  char* old_dir = get_current_directory(&should_free);
  
  if (chdir(dir) == -1){
    perror("ERROR: Unable to change directory");
    free(old_dir);
  }

  write_env("OLD_PWD", old_dir);  
  write_env("PWD", dir);

  free(old_dir);
}

// Sends a signal to all processes contained in a job
void run_kill(KillCommand cmd) {
  int signal = cmd.sig;
  int job_id = cmd.job;

  printf("signal = %d, job_id = %d\n", signal, job_id);
  // TODO: Remove warning silencers
  (void) signal; // Silence unused variable warning
  (void) job_id; // Silence unused variable warning

  // TODO: Kill all processes associated with a background job
  IMPLEMENT_ME();
}


// Prints the current working directory to stdout
void run_pwd() {
  // TODO: Print the current working directory

  //size_t last_dir_idx, i;
  // char* cwd = get_current_directory()
  bool should_free = false;

  char* cwd = get_current_directory(&should_free);

  fprintf(stdout, "%s\n", cwd);

  if (should_free == true){
    free(cwd);
  }
  // Flush the buffer before returning
  fflush(stdout);
}

// Prints all background jobs currently in the job list to stdout
void run_jobs() {
  // TODO: Print background jobs
  IMPLEMENT_ME();

  // Flush the buffer before returning
  fflush(stdout);
}

/***************************************************************************
 * Functions for command resolution and process setup
 ***************************************************************************/

/**
 * @brief A dispatch function to resolve the correct @a Command variant
 * function for child processes.
 *
 * This version of the function is tailored to commands that should be run in
 * the child process of a fork.
 *
 * @param cmd The Command to try to run
 *
 * @sa Command
 */
void child_run_command(Command cmd) {
  CommandType type = get_command_type(cmd);

  switch (type) {
  case GENERIC:
    run_generic(cmd.generic);
    break;

  case ECHO:
    run_echo(cmd.echo);
    break;

  case PWD:
    run_pwd();
    break;

  case JOBS:
    run_jobs();
    break;

  case EXPORT:
  case CD:
  case KILL:
  case EXIT:
  case EOC:
    break;

  default:
    fprintf(stderr, "Unknown command type: %d\n", type);
    break;
  }
}

/**
 * @brief A dispatch function to resolve the correct @a Command variant
 * function for the quash process.
 *
 * This version of the function is tailored to commands that should be run in
 * the parent process (quash).
 *
 * @param cmd The Command to try to run
 *
 * @sa Command
 */
void parent_run_command(Command cmd) {
  CommandType type = get_command_type(cmd);

  switch (type) {
  case EXPORT:
    run_export(cmd.export);
    break;

  case CD:
    run_cd(cmd.cd);
    break;

  case KILL:
    run_kill(cmd.kill);
    break;

  case GENERIC:
  case ECHO:
  case PWD:
  case JOBS:
  case EXIT:
  case EOC:
    break;

  default:
    fprintf(stderr, "Unknown command type: %d\n", type);
    break;
  }
}

/**
 * @brief Creates one new process centered around the @a Command in the @a
 * CommandHolder setting up redirects and pipes where needed
 *
 * @note Processes are not the same as jobs. A single job can have multiple
 * processes running under it. This function creates a process that is part of a
 * larger job.
 *
 * @note Not all commands should be run in the child process. A few need to
 * change the quash process in some way
 *
 * @param holder The CommandHolder to try to run
 *
 * @sa Command CommandHolder
 */
pid_t create_process(CommandHolder holder, int* given_pipe) {
  // Read the flags field from the parser
  bool p_in  = holder.flags & PIPE_IN;
  bool p_out = holder.flags & PIPE_OUT;
  bool r_in  = holder.flags & REDIRECT_IN;
  bool r_out = holder.flags & REDIRECT_OUT;
  bool r_app = holder.flags & REDIRECT_APPEND; // This can only be true if r_out
                                               // is true

  // TODO: Setup pipes, redirects, and new process

  if (r_app == true && r_out == false){
    fprintf(stderr, "ERROR: redirect output is false while redirect append is true\n");
    exit(EXIT_FAILURE);
  }


  if (p_in | p_out | r_in | r_out | r_app) // if we have a special thing we need to do
  { // if we need to fork in the first place
    fprintf(stdout, "p_in %d, p_out %d, r_in %d, r_out %d, r_app %d\n", p_in, p_out, r_in, r_out, r_app);

    


    if (r_in == true){
      ;
    }

    if (r_out == true){
      if (r_app == true){ // need to check if append ONLY if r_out
        ;
      }
    }
  } 
  // else no pipes necessary

  // int fds[2];
  // pipe(fds);

  int wstatus;
  pid_t pid;
  pid = fork();

  if (pid == -1){
    perror("ERROR: Unable to process child process");
    exit(EXIT_FAILURE);
  } else if (pid == 0){

    if (p_in){
      dup2(given_pipe[READ], STDIN_FILENO);

    }else{
      // close(fds[READ]);
    }

    if (p_out){
      dup2(given_pipe[WRITE], STDOUT_FILENO);
    }else{
      // close(fds[WRITE]);
    }

    child_run_command(holder.cmd);

  } else{
    // close(fds[READ]);
    // close(fds[WRITE]);
    waitpid(pid, &wstatus, 0);
    parent_run_command(holder.cmd);
  }
  return pid;

}

// Run a list of commands
void run_script(CommandHolder* holders) {
  if (holders == NULL)
    return;

  check_jobs_bg_status();

  if (get_command_holder_type(holders[0]) == EXIT &&
      get_command_holder_type(holders[1]) == EOC) {
    end_main_loop();
    return;
  }


  CommandType type;
  // Job* job = malloc(sizeof(Job));



  int previous_read = -1;

  int fds[2] = 
  {
    -1,
    -1,
  };

  int total_processes = 0;
  // Run all commands in the `holder` array
  for (int i = 0; (type = get_command_holder_type(holders[i])) != EOC; ++i)
  {
  
    pid_t pid;


    if (pipe(fds) == -1){
      perror("ERROR: Unable to create pipe");
      exit(EXIT_FAILURE);
    }

    pid = create_process(holders[i], fds);

    if (total_processes <= 10)
    ;
      // job->all_pids[total_processes++] = pid;
    else{
      fprintf(stderr, "ERROR: Reached max amount of processes allowed: %d\n", MAX_NUMBER_OF_PIDS);
      exit(EXIT_FAILURE);
    }

    dup2(fds[WRITE], fds[READ]);
    

  }

  // free(job);  // REMOVE ONCE DONE DEBUGGING
    
  

  if (!(holders[0].flags & BACKGROUND)) {
    // Not a background Job
    // TODO: Wait for all processes under the job to complete
    // IMPLEMENT_ME();
  }
  else {
    // A background job.
    // TODO: Push the new job to the job queue

    if (is_job_queue_initialized == false){
      job_queue = new_JobQueue(1);
      is_job_queue_initialized = true;
    }
    
    // push_back_JobQueue(&job_queue, )

    
    // IMPLEMENT_ME();

    // TODO: Once jobs are implemented, uncomment and fill the following line
    // print_job_bg_start(job_id, pid, cmd);
  }
}
