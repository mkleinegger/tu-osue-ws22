/**
 * @file forksort.c
 * @author Maximilian Kleinegger <e12041500@student.tuwien.ac.at>
 * @date 2022-12-05
 *
 * @brief
 * @details
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define BUF_SIZE 1024

struct child
{
    int read;
    int write;
    pid_t pid;
};

static char *PROG_NAME; /** name of the programm */

static void createChild(struct child *c)
{
    // setting up common variables.
    int pipes[2][2];
    if ((pipe(pipes[0]) == -1) || (pipe(pipes[1]) == -1))
    {
        fprintf(stderr, "Failed to pipe!");
    }

    c->pid = fork();
    switch (c->pid)
    {
    case -1: // error
        fprintf(stderr, "Failed to fork! %s", strerror(errno));
        break;
    case 0: // child
        close(pipes[0][1]);
        close(pipes[1][0]);
        if (((dup2(pipes[0][0], STDIN_FILENO)) == -1) || ((dup2(pipes[1][1], STDOUT_FILENO)) == -1))
        {
            fprintf(stderr, "Failed to redirect stdin or stdout!");
        }
        close(pipes[0][0]);
        close(pipes[1][1]);
        if (execlp(PROG_NAME, PROG_NAME, NULL) == -1)
        {
            fprintf(stderr, "Failed to execute program!");
        }
        assert(0);
    default: // parent
        close(pipes[0][0]);
        close(pipes[1][1]);
        c->write = pipes[0][1];
        c->read = pipes[1][0];
        break;
    }
}

int main(int argc, char *argv[])
{
    // parseArguments
    PROG_NAME = argv[0];

    char *buffer1, *buffer2;
    size_t buf_size1 = 0, buf_size2 = 0;
    struct child c1, c2;

    buffer1 = NULL;
    buffer2 = NULL;
    int size = 0;
    if (getline(&buffer1, &buf_size1, stdin) == -1)
    {
        free(buffer1);
        exit(EXIT_SUCCESS);
    }

    if (getline(&buffer2, &buf_size2, stdin) == -1)
    {
        f(stdout, "%s", buffer1);
        free(buffer1);
        free(buffer2);
        exit(EXIT_SUCCESS);
    }

    size++;
    createChild(&c1);
    FILE *file1 = fdopen(c1.write, "w");
    fputs(buffer1, file1);
    size++;
    createChild(&c2);

    FILE *file2 = fdopen(c2.write, "w");
    fputs(buffer2, file2);

    while (getline(&buffer1, &buf_size1, stdin) != -1)
    {
        fputs(buffer1, file1);
        size++;
        /*if (write_data(buffer1, fileE) == -1)
        {
            error_exit("Failed to write");
        }*/
        if (getline(&buffer2, &buf_size2, stdin) != -1)
        {
            fputs(buffer2, file2);
            size++;
            /*if (write_data(buffer2, fileO) == -1)
            {
                error_exit("Failed to write!");
            }*/
        }
    }
    fclose(file1);
    fclose(file2);

    int status;
    waitpid(c1.pid, &status, 0);
    if (WIFEXITED(status))
    {
        if (WEXITSTATUS(status) == EXIT_FAILURE)
        {
            close(c1.read);
            close(c2.read);
            fprintf(stderr, "Child Process failed!");
        }
    }

    waitpid(c2.pid, &status, 0);
    if (WIFEXITED(status))
    {
        if (WEXITSTATUS(status) == EXIT_FAILURE)
        {
            close(c2.read);
            close(c1.read);
            fprintf(stderr, "Child Process failed!");
        }
    }
    char *buffer[size];
    char *buffer3 = NULL;
    char *buffer4 = NULL;

    size_t buf_size3 = 0, buf_size4 = 0;
    FILE *eIn = fdopen(c1.read, "r");
    FILE *oIn = fdopen(c2.read, "r");

    for (int i = 0; i < size / 2; i++)
    {
        if (getline(&buffer3, &buf_size4, eIn) == -1)
        {
            fclose(eIn);
            fclose(oIn);
        }

        fprintf(stdout, "%s", buffer3);

        if (getline(&buffer3, &buf_size3, oIn) == -1)
        {
            fclose(oIn);
            fclose(eIn);
        }

        fprintf(stdout, "%s", buffer3);

        /*if (strcmp(buffer3, buffer4) <= 0)
        {
            fprintf(stdout, "%s", buffer3);
            fprintf(stdout, "%s", buffer4);
        }
        else
        {
            fprintf(stdout, "%s", buffer4);
            fprintf(stdout, "%s", buffer3);
        }*/
    }
    fclose(eIn);
    fclose(oIn);

    return EXIT_SUCCESS;
}
