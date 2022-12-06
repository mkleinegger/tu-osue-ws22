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

    if (getline(&buffer1, &buf_size1, stdin) == EOF)
    {
        free(buffer1);
        exit(EXIT_SUCCESS);
    }

    if (getline(&buffer2, &buf_size2, stdin) == EOF)
    {
        fputs(buffer1, stdout);
        free(buffer1);
        free(buffer2);
        exit(EXIT_SUCCESS);
    }

    int size = 2;

    createChild(&c1);
    createChild(&c2);

    FILE *file1 = fdopen(c1.write, "w");
    FILE *file2 = fdopen(c2.write, "w");
    fputs(buffer1, file1);
    fputs(buffer2, file2);

    while (getline(&buffer1, &buf_size1, stdin) != EOF)
    {
        fputs(buffer1, file1);
        size++;
        if (getline(&buffer2, &buf_size2, stdin) != EOF)
        {
            fputs(buffer2, file2);
            size++;
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

    char *buffer = NULL;
    char *buffer3 = NULL;
    char *buffer4 = NULL;

    size_t buf_size = 0, buf_size3 = 0, buf_size4 = 0;
    FILE *eIn = fdopen(c1.read, "r");
    FILE *oIn = fdopen(c2.read, "r");

    ssize_t readline1 = 0;
    ssize_t readline2 = 0;

    while (readline1 != EOF && readline2 != EOF)
    {
        if (buffer3 == NULL)
            readline1 = getline(&buffer3, &buf_size3, eIn);
        /*if (readline1 != EOF)
        {
            buffer3 = strdup(buffer);
            // strcpy(buffer3, buffer);
        }*/
        if (buffer4 == NULL)
            readline2 = getline(&buffer4, &buf_size4, oIn);
        /*if (readline2 != EOF)
        {
            buffer4 = strdup(buffer);
            // strcpy(buffer4, buffer);
        }*/
        if (readline1 != EOF && readline2 != EOF)
        {
            if (strcmp(buffer3, buffer4) < 0)
            {
                fputs(buffer3, stdout);
                /*if (strchr(buffer3, '\n') == NULL)
                {
                    fputs("\n", stdout);
                }*/
                free(buffer3);
                buffer3 = NULL;
            }
            else
            {
                fputs(buffer4, stdout);
                /*if (strchr(buffer4, '\n') == NULL)
                {
                    fputs("\n", stdout);
                }*/
                free(buffer4);
                buffer4 = NULL;
            }
        }

        free(buffer);
    }

    if (readline1 == EOF)
    {
        fputs(buffer4, stdout);
        /*if (strchr(buffer4, '\n') == NULL)
        {
            fputs("\n", stdout);
        }*/
        while (getline(&buffer4, &buf_size4, oIn) != EOF)
        {
            fputs(buffer4, stdout);
            /*if (strchr(buffer4, '\n') == NULL)
            {
                fputs("\n", stdout);
            }*/
        }
    }
    else
    {
        fputs(buffer3, stdout);
        /*if (strchr(buffer3, '\n') == NULL)
        {
            fputs("\n", stdout);
        }*/
        while (getline(&buffer3, &buf_size3, eIn) != EOF)
        {
            fputs(buffer3, stdout);
            /*if (strchr(buffer3, '\n') == NULL)
            {
                fputs("\n", stdout);
            }*/
        }
    }

    free(buffer);
    free(buffer3);
    free(buffer4);

    /*for (int i = 0; i < size;)
    {
        if (buffer3 == NULL)
        {
            if (getline(&buffer3, &buf_size3, eIn) == EOF)
            {
                fclose(oIn);
            }
            i++;
        }

        if (buffer4 == NULL)
        {
            if (getline(&buffer4, &buf_size4, oIn) == EOF)
            {
                fclose(oIn);
            }
            i++;
        }

        // fprintf(stdout, "%d: %d<%d ", c1.pid, i, size);

        if (buffer4 == NULL && buffer3 == NULL)
        {
        }
        else if (buffer3 != NULL && buffer4 == NULL)
        {
            fputs(buffer3, stdout);
            if (strchr(buffer3, '\n') == NULL)
            {
                fputs("\n", stdout);
            }
        }
        else if (buffer4 != NULL && buffer3 == NULL)
        {
            fputs(buffer4, stdout);
            if (strchr(buffer4, '\n') == NULL)
            {
                fputs("\n", stdout);
            }
        }
        else
        {
            if (strcmp(buffer3, buffer4) < 0)
            {
                fputs(buffer3, stdout);
                if (strchr(buffer3, '\n') == NULL)
                {
                    fputs("\n", stdout);
                }
                free(buffer3);
                buffer3 = NULL;
            }
            else
            {
                fputs(buffer4, stdout);
                if (strchr(buffer4, '\n') == NULL)
                {
                    fputs("\n", stdout);
                }
                free(buffer4);
                buffer4 = NULL;
            }
        }*/

    fclose(eIn);
    fclose(oIn);

    return EXIT_SUCCESS;
}
