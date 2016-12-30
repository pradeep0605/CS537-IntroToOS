#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>
#include<pthread.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/time.h>
#include<stdint.h>
#include"linked_list.h"

typedef unsigned int bool;

#define HASH_SIZE (1000)
#define STDIN_SIZE (1000)
#define USEC_FROM_TIMEVAL(timeval) \
  ((unsigned long int) 1000000 * timeval.tv_sec + timeval.tv_usec)
#define mysh_printf(format, ...) \
  sprintf(output, format, ##__VA_ARGS__); \
  if (write(STDOUT_FILENO, output, strlen(output)) == -1) \
    perror("Error in writing to STDOUT\n");

#define mysh_perror(format, ...) \
  sprintf(output, format, ##__VA_ARGS__); \
  if (write(STDERR_FILENO, output, strlen(output)) == -1) \
    perror("Error in writing to STDERR\n");

typedef struct job {
  int jid;
  int pid;
  bool bg;
  linked_list_t command;
} job_t;


/* Globals */
char output[STDIN_SIZE] = {0};
char *shell = "mysh> ";
volatile int global_jid = 0;
linked_list_t job_hash[HASH_SIZE + 1] = {{0, 0}};
FILE *infile = NULL;

/* Free all strings and linked lists before exiting */
void exit_gracefully(int exit_val) {
  int i = 0;
  for (; i <= HASH_SIZE; ++i) {
    linked_list_t *list = &job_hash[i];
    node_t *node_itr;

    for_each_node(node_itr, list) {
      job_t *job = (job_t *) node_itr->data;
      node_t *cmd_itr;
      /* If a job is not dead, then wait for it to die */
      if (waitpid(job->pid, NULL, WNOHANG) == 0) { /* child not dead */
        /* wait for it to die */
        waitpid(job->pid, NULL, 0);
      }

      /* Free all the linked list of strings that held the command */
      for_each_node(cmd_itr, &job->command) {
        free(cmd_itr->data);
      }
      /* Free the linked list of command */
      ll_free(&job->command);
      /* Free the job */
      free(job);
    }
    /* Free the linked list of jobs in this hash entery */
    ll_free(list);
  }
  if (infile != NULL) {
    fclose(infile);
  }
  exit(exit_val);
}

void read_line(char *s, FILE *infile) {
  if (fgets(s, STDIN_SIZE, infile) == NULL) {
    /* if Nothing to read (EOF or ctrl+d) */
      exit_gracefully(0);
  }
  /* Remove Newline */
  s[strlen(s) - 1] = '\0';
}

void trim_line(char *s) {
  int index = 0;
  /* Remove unwanted space in the begginning */
  while ((s[index] == ' ' || s[index] == '\t') && s[index] != '\0') {
    index++;
  }
  strcpy(s, &s[index]);
  /* Remove unwanted space in the end */
  index = strlen(s) - 1;
  while (index >= 0 && (s[index] == ' ' || s[index] == '\t')) {
    index--;
  }
  s[++index] = '\0';
}

void prune_line(char *s) {
  trim_line(s);
}

int is_inbuilt_command(char **args, int nargs) {
  if (strcmp(args[0], "j") == 0) {
    if (nargs == 1 || (nargs == 2 && (strcmp(args[1], "&") == 0))) {
      return true;
    } else  {
      return false;
    }
  } else if (strcmp(args[0], "exit") == 0) {
    if (nargs == 1 || (nargs == 2 && (strcmp(args[1], "&") == 0))) {
      return true;
    } else {
      return false;
    }
  } else if (strcmp(args[0], "myw") == 0) {
    if (nargs == 2 || (nargs == 3 && (strcmp(args[2], "&") == 0))) {
      return true;
    } else {
      return false;
    }
  }
  return false;;
}

inline int hash_func(int jid) {
  return jid % (HASH_SIZE + 1);
}

inline void inc_global_jid() {
  global_jid++;
}

inline void dec_global_jid() {
  global_jid--;
}

inline int get_global_jid() {
  return global_jid;
}

/* COMPARE Function start */
bool compare(void *key, void *cmp) {
  job_t *j1;
  int kjid = (uintptr_t)(key);
  j1 = (job_t *) cmp;
  return (j1->jid == kjid);
}
/* Compare Function End */


int execute_inbuilt_command(char **args, int nargs, job_t *job) {
  char *cmd = args[0];
  char *arg = args[1];
  int query_jid = -1;

  if (strcmp(cmd, "j") == 0) {
    int i = 0;
    node_t *itr;
    int N = (get_global_jid() < HASH_SIZE) ? get_global_jid() : HASH_SIZE;
    for (; i <= N; ++i) {
      linked_list_t *ll;
      ll = &job_hash[hash_func(i)];
      for_each_node(itr, ll) {
        job_t *job = (job_t *) itr->data;
        if (job->bg) {
          if (waitpid(job->pid, NULL, WNOHANG) == 0) {
            /* still alive */
            node_t *cmds;
            mysh_printf("%d : ", job->jid);
            for_each_node(cmds, &job->command) {
              /* This if to avoid extra space after last argument to the cmnd */
              if (cmds->next == NULL) {
                mysh_printf("%s", (char *) cmds->data);
              } else {
                mysh_printf("%s ", (char *) cmds->data);
              }
            }
            mysh_printf("\n");
          }
        }
      }
    }
    goto success;
  } else if (strcmp(cmd, "exit") == 0) {
    ll_free_with_data(&job->command);
    free(job);
    exit_gracefully(0);
  } else if (strcmp(cmd, "myw") == 0) {
    if (arg[strlen(args[1]) - 1] == '&') {
      arg[strlen(args[1]) - 1] = '\0';
    }

    query_jid = atoi(arg);

    if (query_jid != 0) {
      node_t *job_node;
      job_t *job;

      job_node = ll_find(&job_hash[hash_func(query_jid)],
          (((void *)0) + query_jid), compare);
      if (job_node == NULL || query_jid > get_global_jid()) {
        mysh_perror("Invalid jid %d\n", query_jid);
        goto success;
      } else {
        job = (job_t *) job_node->data;
        if (waitpid(job->pid, NULL, WNOHANG) == 0) {
          /* Child is still alive */
          struct timeval before, after;
          gettimeofday(&before, NULL);
          waitpid(job->pid, NULL, 0);
          gettimeofday(&after, NULL);
          mysh_printf("%lu : Job %d terminated\n",
              USEC_FROM_TIMEVAL(after) - USEC_FROM_TIMEVAL(before), job->jid);
          goto success;
        } else {
          mysh_printf("0 : Job %d terminated\n", job->jid);
          goto success;
       }
      }

    } else {
      /* unable to properly convert the string to integer */
      goto fail;
    }
  }
success:
  return 0;
fail:
  return -1;
}

void execute_job(job_t *job) {
  bool wait_for_job = true;
  char *last_argument = (char *) job->command.tail->data;
  char **args;
  int nargs, i = 0, pid = -1;
  node_t *itr;

  if (last_argument[strlen(last_argument) - 1] == '&') {
    wait_for_job = false;
    job->bg = true;
    last_argument[strlen(last_argument) - 1] = '\0';
    /* if the & was given as the last argument, then remove it from the command
     * list */
    if (strlen(last_argument) == 0) {
      free(last_argument);
      ll_delete_tail(&job->command);
    }
  }

  nargs = ll_size(&job->command);
  if (nargs == 0) {
    ll_delete_node(&job_hash[hash_func(job->jid)], job);
    free(job);
    return;
  }
  args = (char **) malloc (sizeof(char *) * (nargs + 1));

  for_each_node(itr, &job->command) {
    args[i] = (char *) itr->data;
    i++;
  }
  args[i] = NULL;

  if (is_inbuilt_command(args, nargs)) {
      /* Revert the global_id counter increment and remove the job from the list
       * as this is an inbuild command */
      dec_global_jid();
      ll_delete_node(&job_hash[hash_func(job->jid)], job);
      if (execute_inbuilt_command(args, nargs, job) != 0) {
        /* As this command couldn't run as an inbuilt command, run this as an
         * external command */
         inc_global_jid();
         ll_insert_head(&job_hash[hash_func(job->jid)], job);
      } else {
        ll_free_with_data(&job->command);
        free(job);
        goto end;
      }
  }

  pid = fork();

  if (pid == 0) {
    /* Child Process */
    execvp(args[0], args);
    mysh_perror("%s: Command not found\n", args[0]);
    exit_gracefully(1);
  } else {
    /* Parent Process */
    job->pid = pid;
    if (wait_for_job) {
      if (waitpid(pid, NULL, 0) == -1) {
        mysh_perror("Error waiting for child %d\n", job->pid);
      }
    }
  }
end:
  free(args);
}

void process_line(char *s, bool batch_mode) {
  char *token;
  int hash_idx;
  job_t *new_job;

  /* Process the tokens and add then to the list */
  token = strtok(s, " \t");

  /* Nothing to Process if no commands were entered */
  if (token == NULL) {
    goto end;
  }

  new_job = (job_t *) malloc(sizeof(job_t));

  /* Increment the global job counter */
  inc_global_jid();
  new_job->jid = get_global_jid();
  /* Initialize the comamnd-args list */
  ll_initialize(&new_job->command);

  while (token != NULL) {
    ll_insert_tail(&new_job->command, (void *)strdup(token));
    token = strtok(NULL, " \t");
  }

  /* Add it to the hash table */
  hash_idx = hash_func(new_job->jid);
  ll_initialize(&job_hash[hash_idx]);
  ll_insert_head(&job_hash[hash_idx], new_job);

  execute_job(new_job);
end:
  return;
}

int main(int argc, char *argv[]) {
  char input[STDIN_SIZE] = {0};
  if (argc == 2) {
    /* Work on batch mode */
    infile = fopen(argv[1], "r");
    if (infile == NULL) {
      mysh_perror("Error: Cannot open file %s\n", argv[1]);
      goto fail;
    }

    while (1) {
      /* batch mode = true */
      read_line(input, infile);
      mysh_printf("%s\n", input);
      if (strlen(input) > 512) {
        mysh_perror("An error has occurred\n");
        continue;
      }
      process_line(input, true);
    }
    goto success;
  } else if (argc > 2) {
    mysh_perror("Usage: mysh [batchFile]\n");
    goto fail;
  }

  while (1) {
    mysh_printf("%s", shell);
    read_line(input, stdin);
    if (strlen(input) > 512) {
      mysh_perror("An error has occurred\n");
      continue;
    }
    /* Process the line and execute the job if valid */
    /* batch_mode = false */
    process_line(input, false);
  }

success:
  exit_gracefully(0);
fail:
  exit_gracefully(1);
  return 0;
}
