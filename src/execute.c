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

#include <unistd.h>
#include <fcntl.h>

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


IMPLEMENT_DEQUE_STRUCT(JobQueue, Job*);
IMPLEMENT_DEQUE(JobQueue, Job*);
PROTOTYPE_DEQUE(JobQueue, Job*);

bool is_job_queue_initialized = false;
JobQueue job_queue;

int max_job_id = 1;



void assign_background_job_id(Job* job){
  job->job_id = max_job_id;
}

void free_job(Job* job){
  free(job->cmd_str);
  free(job);
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


  size_t len = length_JobQueue(&job_queue);
  Job** JobArray = as_array_JobQueue(&job_queue, &len);

  int completed_processes, result;


  if (len == 0){
    len = 1;
  }
  JobQueue new_job_queue = new_JobQueue(len);

  for (int jq_i = 0; jq_i < len; jq_i++){
    completed_processes = 0;
    result = 0;
    Job* job = JobArray[jq_i];

    for (int i = 0; i < job->total_processes; i++){
      int wstatus;
      result = waitpid(job->all_pids[i], &wstatus, WNOHANG);
      // WNOHANG doesn't wait if child process is not finished

      if (result == -1){
        perror("ERROR: on waitpid for background processes");
      }else if (result == 0){
        completed_processes += 1;
      }else{
        break; //
      }
    }

    if (completed_processes != job->total_processes){  // all processes not finished
      push_back_JobQueue(&new_job_queue, job);
      // add it to the new job queue
    }else{
      // ALL PROCESSES FINISHED!
      print_job_bg_complete(job->job_id, job->pid, job->cmd_str);
      free_job(job);
    }
  }
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

  if (old_dir == NULL){
    fprintf(stderr, "ERROR: unable to get old directory\n");
    exit(EXIT_FAILURE);
  }
  
  if (chdir(dir) == -1){
    perror("ERROR: Unable to change directory");
    free(old_dir);
    exit(EXIT_FAILURE);
  }

  char* new_dir = get_current_directory(&should_free);

  if (new_dir == NULL){
    fprintf(stderr, "ERROR: unable to get old directory\n");
    exit(EXIT_FAILURE);
  }

  write_env("OLD_PWD", old_dir);  
  write_env("PWD", new_dir);

  free(old_dir);
  free(new_dir);

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
  size_t len = length_JobQueue(&job_queue);
  Job** JobArray = as_array_JobQueue(&job_queue, &len);
  for (int i = 0; i < len; i++){
    Job* job = JobArray[i];
    fprintf(stdout, "[%d]\t",job->job_id);
    fprintf(stdout, "%d\t", job->pid);
    fprintf(stdout, "%s\t", job->cmd_str);
    fprintf(stdout, "\n");
  }
  free(JobArray);
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
pid_t create_process(CommandHolder holder, int previous_read, int fds[2]) {
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
  { 
     // fprintf(stdout, "p_in %d, p_out %d, r_in %d, r_out %d, r_app %d\n", p_in, p_out, r_in, r_out, r_app);
  }



  pid_t pid;
  pid = fork();

  if (pid == -1){
    perror("ERROR: Unable to process child process");
    exit(EXIT_FAILURE);
  } 
  else if (pid == 0) // child process
  {

    //  ---- REDIRECT IN ----- 
    if (r_in == true){  
      int rin_fd = open(holder.redirect_in, O_RDONLY);

      if (rin_fd == -1){
        perror("ERROR: unable to open redirect file");
      }

      if (dup2(rin_fd, STDIN_FILENO) == -1){
        perror("ERROR: dup2 redirect stdin");
        close(rin_fd);
        exit(EXIT_FAILURE);
      }
    }

    // ---- REDIRECT OUT ----
    if (r_out == true){
      int flags = O_WRONLY | O_CREAT;
      
      if (r_app == true){ 
        flags |= O_APPEND;  // REDIR APPEND
      }else{
        flags |= O_TRUNC; // truncate erases file when writing
        // need to use these flags because need to create file if it doesn't exist
      }

      int rout_fd = open(holder.redirect_out, flags);
      if (rout_fd == -1){
        perror("ERROR: unable to open redirect write/append file");
        exit(EXIT_FAILURE);
      }

      if (dup2(rout_fd, STDOUT_FILENO) == -1){
        perror("ERROR: dub2 redirect stdout");
        exit(EXIT_FAILURE);
      }
    }

    // ---- PIPE IN ----
    if (p_in && previous_read != -1){ // check if there's a previous in file to map
      if (dup2(previous_read, STDIN_FILENO) == -1){
        perror("ERROR: dub2 fail with STDIN");
        exit(EXIT_FAILURE);
      }

    }
    // ---- PIPE OUT ----
    if (p_out){
      if (dup2(fds[WRITE], STDOUT_FILENO) == -1){
        perror("ERROR: dub2 fail with STDOUT");
        exit(EXIT_FAILURE);
      }
    }
    child_run_command(holder.cmd); // run child command

    // close pipes for child process
    if (previous_read != -1){
      close(previous_read);
    }
    if (fds[WRITE] != -1){
      close(fds[WRITE]);
    }

    if (fds[READ] != -1){
      close(fds[READ]);
    }

    exit(EXIT_SUCCESS); // no more need for child process.

  } else{
    parent_run_command(holder.cmd);    
    return pid; 
  }
  

}

// Run a list of commands
void run_script(CommandHolder* holders) {

  if (is_job_queue_initialized == false){
    job_queue = new_JobQueue(1);
    is_job_queue_initialized = true;
  }


  if (holders == NULL)
    return;

  check_jobs_bg_status();

  if (get_command_holder_type(holders[0]) == EXIT &&
      get_command_holder_type(holders[1]) == EOC) {
    end_main_loop();
    return;
  }

  CommandType type;
  Job* job = malloc(sizeof(Job));

  int previous_read = -1;
  int total_processes = 0;
  pid_t pid;

  // Run all commands in the `holder` array
  for (int i = 0; (type = get_command_holder_type(holders[i])) != EOC; ++i)
  {
    
    int fds[2] = {-1, -1};
  
    if (holders[i].flags & PIPE_OUT){
      if (pipe(fds) == -1){
        perror("ERROR: Unable to create pipe");
        exit(EXIT_FAILURE);
      }
    }else{
      fds[READ] = -1;
      fds[WRITE] = -1;
    }

    pid = create_process(holders[i], previous_read, fds);

    if (total_processes < MAX_NUMBER_OF_PIDS){
      if (total_processes == 0){  // initalized job pid
        job->pid = pid;
      }
      job->all_pids[total_processes++] = pid;

    }
    else{
      fprintf(stderr, "ERROR: Reached max amount of processes allowed: %d\n", MAX_NUMBER_OF_PIDS);
      exit(EXIT_FAILURE);
    }

    // close pipes
    if (previous_read != -1){
      close(previous_read);
    }
    if (fds[WRITE] != -1){
      close(fds[WRITE]);
    }
    previous_read = fds[READ];  // assign previous read file descriptor to the previous_read
  }

  job->job_id = 0;

  // works because fds[READ] would be only thing left open
  if (previous_read != -1){
    close(previous_read);
  }
  
  job->cmd_str = get_command_string();
  job->total_processes = total_processes;
  
  if (!(holders[0].flags & BACKGROUND)) {
    for (int i = 0; i < job->total_processes; i++){
      int wstatus;

      if (waitpid(job->all_pids[i], &wstatus, 0) == -1){
        perror("ERROR: unable to wait for pid");
      }
    }
    free_job(job);
  }
  else {
    // A background job.
    // TODO: Push the new job to the job queue
    assign_background_job_id(job); // only need to assign if its a background job  
    push_back_JobQueue(&job_queue, job);

    
    print_job_bg_start(job->job_id, job->pid, job->cmd_str);
  }
}
