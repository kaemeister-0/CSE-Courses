#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define HISTORY_SIZE 100
#define HISTORY_FILE ".my_shell_history"
char *history[HISTORY_SIZE];
int history_count = 0;
char *prompt = "sh> ";

void handle_sigint(int sig) {
    write(1, "\nsh> ", 5);
}

void load_history() {
    FILE *file = fopen(HISTORY_FILE, "r");
    if (!file) return;
    char *line = NULL;
    size_t len = 0;
    while (getline(&line, &len, file) != -1 && history_count < HISTORY_SIZE)
        history[history_count++] = strdup(line);
    free(line);
    fclose(file);
}

void add_to_history(const char *cmd) {
    if (cmd[0] == '\n') return;
    if (history_count < HISTORY_SIZE) {
        history[history_count++] = strdup(cmd);
    } else {
        free(history[0]);
        for (int i = 1; i < HISTORY_SIZE; ++i)
            history[i - 1] = history[i];
        history[HISTORY_SIZE - 1] = strdup(cmd);
    }
    FILE *file = fopen(HISTORY_FILE, "a");
    if (file) {
        fputs(cmd, file);
        fclose(file);
    }
}

void show_history() {
    for (int i = 0; i < history_count; ++i)
        printf("%d %s", i + 1, history[i]);
}

// Parse command for redirection and return actual argv
char **parse_redirection(char *cmd, char **infile, char **outfile, int *append) {
    char *args[100];
    int i = 0;
    char *token = strtok(cmd, " \t\n");
    while (token != NULL) {
        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " \t\n");
            *infile = token;
        } else if (strcmp(token, ">>") == 0) {
            token = strtok(NULL, " \t\n");
            *outfile = token;
            *append = 1;
        } else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " \t\n");
            *outfile = token;
            *append = 0;
        } else {
            args[i++] = token;
        }
        token = strtok(NULL, " \t\n");
    }
    args[i] = NULL;
    char **argv = malloc(sizeof(char *) * (i + 1));
    for (int j = 0; j <= i; j++) argv[j] = args[j];
    return argv;
}

int execute_command(char *cmd) {
    char *infile = NULL, *outfile = NULL;
    int append = 0;
    char **argv = parse_redirection(cmd, &infile, &outfile, &append);
    if (argv[0] == NULL) return 1;

    if (strcmp(argv[0], "history") == 0) {
        show_history();
        return 0;
    }

    pid_t pid = fork();
    if (pid == 0) {
        signal(SIGINT, SIG_DFL);
        if (infile) {
            int fd = open(infile, O_RDONLY);
            if (fd < 0) { perror("input"); exit(1); }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        if (outfile) {
            int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
            int fd = open(outfile, flags, 0644);
            if (fd < 0) { perror("output"); exit(1); }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        execvp(argv[0], argv);
        perror("execvp");
        exit(1);
    } else {
        int status;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }
}

int execute_piped(char *line) {
    char *cmds[20];
    int n = 0;
    cmds[n++] = strtok(line, "|");
    while ((cmds[n++] = strtok(NULL, "|")));

    int fd[2], in = 0, status;
    pid_t pid;
    for (int i = 0; i < n - 1; ++i) {
        pipe(fd);
        if ((pid = fork()) == 0) {
            signal(SIGINT, SIG_DFL);
            dup2(in, STDIN_FILENO);
            if (i < n - 2) dup2(fd[1], STDOUT_FILENO);
            close(fd[0]);
            close(fd[1]);

            char *infile = NULL, *outfile = NULL;
            int append = 0;
            char **argv = parse_redirection(cmds[i], &infile, &outfile, &append);
            execvp(argv[0], argv);
            perror("execvp");
            exit(1);
        } else {
            wait(&status);
            close(fd[1]);
            in = fd[0];
        }
    }
    return 1;
}

int execute_line(char *line) {
    // split by &&
    char *cmd = strtok(line, "&&");
    while (cmd) {
        while (*cmd == ' ') cmd++;  // trim
        if (strchr(cmd, '|')) {
            if (!execute_piped(cmd)) return 0;
        } else {
            if (!execute_command(cmd)) return 0;
        }
        cmd = strtok(NULL, "&&");
    }
    return 1;
}

int main() {
    signal(SIGINT, handle_sigint);
    load_history();

    char line[1024];
    while (1) {
        fprintf(stderr, "%s", prompt);
        fflush(stderr);
        if (!fgets(line, sizeof(line), stdin)) break;

        add_to_history(line);
        char *cmd = strtok(line, ";");
        while (cmd) {
            if (!execute_line(cmd)) break;
            cmd = strtok(NULL, ";");
        }
    }

    for (int i = 0; i < history_count; ++i)
        free(history[i]);
    return 0;
}

