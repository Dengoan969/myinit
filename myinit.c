#include<stdio.h>

#include<stdlib.h>

#include<errno.h>

#include<unistd.h>

#include<signal.h>

#include<fcntl.h>

#include<string.h>

#include<sys/wait.h>

#define LOG_FILENAME "/tmp/myinit.txt"
#define BLOCK 10

FILE * logFile;
FILE * configuration;
int isRestart = 0;
int isFinish = 0;

struct Task {
  pid_t pid;
  char * input;
  char * output;
  int argsLen;
  char ** arguments;
};

struct TaskArray {
  int len;
  int capacity;
  struct Task ** taskArray;
};

struct TaskArray * taskArray = NULL;

void addTaskToArray(struct TaskArray * taskArray, struct Task * task) {
  taskArray -> taskArray = realloc(taskArray -> taskArray, sizeof(struct Task * ) * (taskArray -> len + 1));
  if (taskArray -> taskArray == NULL) {
    fprintf(stderr, "Not enough memory\n");
    exit(1);
  }
  taskArray -> taskArray[taskArray -> len++] = task;
  taskArray -> capacity = taskArray -> len;
}

struct Task * createTask() {
  struct Task * task = malloc(sizeof(struct Task));
  if (task == NULL) {
    fprintf(stderr, "Not enough memory\n");
    exit(1);
  }
  task -> input = NULL;
  task -> output = NULL;
  task -> arguments = NULL;
  task -> argsLen = 0;
  task -> pid = 0;
  return task;
}

void freeTask(struct Task * task) {
  free(task -> input);
  free(task -> output);
  for (int i = 0; i < task -> argsLen; i++) {
    free(task -> arguments[i]);
  }
  free(task -> arguments);
  free(task);
}

struct TaskArray * createTaskArray() {
  struct TaskArray * taskArray = malloc(sizeof(struct TaskArray));
  if (taskArray == NULL) {
    fprintf(stderr, "Not enough memory\n");
    exit(1);
  }
  taskArray -> taskArray = NULL;
  taskArray -> len = 0;
  taskArray -> capacity = 0;
  return taskArray;
}

void freeTaskArray(struct TaskArray * taskArray) {
  for (int i = 0; i < taskArray -> len; i++) {
    freeTask(taskArray -> taskArray[i]);
  }
  free(taskArray -> taskArray);
  free(taskArray);
}

int isPathAbsolute(char * path) {
  if (strlen(path) == 0 || path[0] != '/')
    return 0;
  return 1;
}

void help() {
  printf("help:\n");
  printf("./myinit <config>\n");
}

char * duplicateStr(char * s) {
  size_t len = strlen(s);
  char * result = (char * ) malloc(len + 1);

  if (result == NULL) {
    fprintf(stderr, "Error: not enough memory\n");
    exit(EXIT_FAILURE);
  }

  strcpy(result, s);
  return result;
}

void addToArray(char ** * array, int * len, int * capacity, char * p) {
  if ( * len == * capacity) {
    char ** newArray =
      (char ** ) realloc( * array,
        sizeof(char * ) * (( * capacity) + BLOCK));

    if (newArray == NULL) {
      fprintf(stderr, "Not enough memory\n");
      exit(1);
    }

    * array = newArray;
    * capacity += BLOCK;
  }

  if (p == NULL)
    ( * array)[( * len) ++] = NULL;
  else
    ( * array)[( * len) ++] = duplicateStr(p);
}

void getLineArgs(char * str, char ** * array, int * len) {
  * array = NULL;
  * len = 0;
  int capacity = 0;
  char * p = strtok(str, " ");

  while (p != NULL) {
    addToArray(array, len, & capacity, p);
    p = strtok(NULL, " ");
  }

  addToArray(array, len, & capacity, NULL);
}

int validateTask(struct Task * t, FILE * out) {
  if (t -> argsLen < 3) {
    fprintf(out, "Error: task invalid parameters count\n");
    return 0;
  } else if (!isPathAbsolute(t -> arguments[0])) {
    fprintf(out, "Error: executable path not absolute\n");
    return 0;
  } else if (!isPathAbsolute(t -> arguments[t -> argsLen - 3])) {
    fprintf(out, "Error: input path not absolute\n");
    return 0;
  } else if (!isPathAbsolute(t -> arguments[t -> argsLen - 2])) {
    fprintf(out, "Error: output path not absolute\n");
    return 0;
  }
  return 1;
}

struct TaskArray * readConfig(FILE * out, char * configurationFilename) {
  struct TaskArray * taskArray = createTaskArray();
  configuration = fopen(configurationFilename, "r");

  if (configuration == NULL) {
    fprintf(out, "Error: Cannot open file '%s'\n", configurationFilename);
    free(configurationFilename);
    exit(1);
  }

  char * line = NULL;
  size_t len = 0;
  while (getline( & line, & len, configuration) != -1) {

    fprintf(out, "line from config: '%s'\n", line);
    struct Task * t = createTask();
    getLineArgs(line, & t -> arguments, & t -> argsLen);

    if (!validateTask(t, out)) {
      freeTask(t);
    } else {
      t -> input = duplicateStr(t -> arguments[t -> argsLen - 3]);
      t -> output = duplicateStr(t -> arguments[t -> argsLen - 2]);
      free(t -> arguments[t -> argsLen - 3]);
      t -> arguments[t -> argsLen - 3] = NULL;
      free(t -> arguments[t -> argsLen - 2]);
      t -> arguments[t -> argsLen - 2] = NULL;
      addTaskToArray(taskArray, t);
      fprintf(out, "task will start %d\n", taskArray -> len - 1);
    }

    free(line);
  }

  fclose(configuration);

  return taskArray;
}

int checkFileAccess(char * filename, char * mode) {
  FILE * file = fopen(filename, mode);
  if (file == NULL) {
    fprintf(logFile, "Error: cannot open file '%s' with mode '%s'\n", filename, mode);
    return 0;
  }
  fclose(file);
  return 1;
}

void start(int taskIndex, struct TaskArray * taskArray) {
  if (!checkFileAccess(taskArray -> taskArray[taskIndex] -> input, "r") ||
    !checkFileAccess(taskArray -> taskArray[taskIndex] -> output, "w")) {
    fprintf(logFile, "Error: cannot start task with index %d\n", taskIndex);
    return;
  }
  pid_t pid = fork();

  if (pid == -1) {
    fprintf(logFile, "Error: cannot fork and start task with index %d\n", taskIndex);
    return;
  }

  if (pid != 0) {
    taskArray -> taskArray[taskIndex] -> pid = pid;
    fprintf(logFile, "Started task with index %d\n", taskIndex);
    return;
  }

  char * filename = NULL;
  filename = taskArray -> taskArray[taskIndex] -> input;

  if (freopen(filename, "r", stdin) == NULL) {
    perror("Error: cannot change the file associated with stdin");
    exit(1);
  }

  filename = taskArray -> taskArray[taskIndex] -> output;

  if (freopen(filename, "w", stdout) == NULL) {
    perror("Error: cannot change the file associated with stdout");
    exit(1);
  }

  freopen("/dev/null", "a", stderr);

  char ** arguments = taskArray -> taskArray[taskIndex] -> arguments;

  execv(arguments[0], arguments);

  exit(1);
}

void startAll(struct TaskArray * taskArray) {
  for (int i = 0; i < taskArray -> len; i++) {
    start(i, taskArray);
  }
}

void handleTaskCompletion(int taskIdx, int stat_val) {
  if (WIFEXITED(stat_val)) {
    fprintf(logFile, "task %d finished with code: %d\n", taskIdx, WEXITSTATUS(stat_val));
  } else if (WIFSIGNALED(stat_val)) {
    fprintf(logFile, "task %d terminated by signal: %d\n", taskIdx, WTERMSIG(stat_val));
  }
}

void stopAll(struct TaskArray * taskArr) {
  for (int i = 0; i < taskArr -> len; i++) {
    pid_t pid = taskArr -> taskArray[i] -> pid;

    if (pid != 0)
      kill(pid, SIGTERM);
  }

  int activeTasks = 0;

  for (int i = 0; i < taskArr -> len; i++)
    if (taskArr -> taskArray[i] -> pid != 0)
      activeTasks++;

  while (activeTasks > 0) {
    int stat_val = 0;
    pid_t finishedPID = waitpid(-1, & stat_val, 0);

    if (finishedPID != -1) {
      int taskIdx;

      for (taskIdx = 0; taskIdx < taskArr -> len; taskIdx++)
        if (taskArr -> taskArray[taskIdx] -> pid == finishedPID)
          break;

      handleTaskCompletion(taskIdx, stat_val);
      activeTasks--;
    }
  }
}

void workCycle(struct TaskArray * taskArr) {
  while (1) {
    int stat_val = 0;
    pid_t finishedPID = waitpid(-1, & stat_val, 0);

    if (finishedPID != -1) {
      int taskIdx;

      for (taskIdx = 0; taskIdx < taskArr -> len; taskIdx++)
        if (taskArr -> taskArray[taskIdx] -> pid == finishedPID)
          break;

      handleTaskCompletion(taskIdx, stat_val);
      taskArr -> taskArray[taskIdx] -> pid = 0;

      if (isRestart || isFinish)
        break;

      start(taskIdx, taskArr);
    } else if (isRestart || isFinish) {
      break;
    }
  }
}

void finishHandler(int signal) {
  isFinish = 1;
  fprintf(logFile, "myinit demon signal: %d\n", signal);
}

void restartHandler(int signal) {
  isRestart = 1;
  fprintf(logFile, "myinit demon signal: %d\n", signal);
}

void setupSignalHandlers() {
  if (signal(SIGINT, finishHandler) == SIG_ERR || signal(SIGTERM, finishHandler) == SIG_ERR) {
    perror("Error: cannot set handler for SIGINT or SIGTERM\n");
    exit(EXIT_FAILURE);
  }

  if (signal(SIGHUP, restartHandler) == SIG_ERR) {
    perror("Error: cannot set handler for SIGHUP\n");
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char * argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Error: config file is not provided\n");
    help();
    exit(EXIT_FAILURE);
  }

  if (!isPathAbsolute(argv[1])) {
    fprintf(stderr, "Error: path is not absolute\n");
    exit(EXIT_FAILURE);
  }

  char * configurationFilename = NULL;

  configurationFilename = duplicateStr(argv[1]);

  pid_t pid = fork();

  if (pid == -1) {
    fprintf(stderr, "Error: fork for demonize process failed\n");
    exit(EXIT_FAILURE);
  }

  if (pid != 0)
    return 0;

  freopen("/dev/zero", "r", stdin);
  freopen("/dev/null", "a", stdout);
  freopen(LOG_FILENAME, "w+", stderr);

  pid_t sid = setsid();

  if (sid == -1) {
    fprintf(stderr, "Error: setsid for demonize process failed\n");
    exit(EXIT_FAILURE);
  }

  logFile = fopen(LOG_FILENAME, "w");

  if (logFile == NULL) {
    fprintf(stderr, "Error: cannot create log file\n");
    exit(EXIT_FAILURE);
  }

  if (chdir("/") == -1) {
    fprintf(stderr, "Error: cannot change directory\n");
    exit(EXIT_FAILURE);
  }

  setupSignalHandlers();

  fprintf(logFile, "myinit demon started\n");

  while (1) {
    taskArray = readConfig(logFile, configurationFilename);
    startAll(taskArray);
    workCycle(taskArray);

    if (isRestart) {
      stopAll(taskArray);
      isRestart = 0;
    }

    freeTaskArray(taskArray);

    if (isFinish) {
      isFinish = 0;
      break;
    }
  }

  fprintf(logFile, "myinit demon stopped\n");
  fclose(logFile);

  return 0;
}