/**
 * @file forksort.c
 * @author Maximilian Kleinegger <e12041500@student.tuwien.ac.at>
 * @date 2022-12-05
 *
 * @brief A program to read in lines until an EOF is encountered, sort them
 * alphabetically (case-sensitive) and then output them again by recursively
 * executing itself.
 * @details This program reads lines from stdin until an EOF is encountered.
 * If only one line is read, this line is printed to stdout immediately. Otherwise
 * the process forks twice and redirects his input to the child-process. This happens
 * recursively until #line <= 1. Afterwards those parent-processes compare the returns
 * from the children, sort those inputs and print them to stdout, until the output reaches
 * the first process, which prints it to the stdout (no process knows if it is printing
 * to stdout or to a parent-process).
 * The program takes no arguments. All lines are read from stdin.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/**
 * @brief Structure to hold necessary information about a childprocess
 * @details Stores the read- and write-pipe and pid from the child
 *
 */
struct childProcess
{
    int read;
    int write;
    pid_t pid;
};

static char *PROG_NAME; /** <name of the programm */

/**
 * @brief Prints the usage message and exits with EXIT_FAILURE
 * @details global variables: PROG_NAME
 */
static void usage(void)
{
    fprintf(stderr, "USAGE: %s \n", PROG_NAME);
    exit(EXIT_FAILURE);
}

/**
 * @brief Prints a error-message and if existing the errno-message and then exits with EXIT_FAILURE
 * @details global variables: PROG_NAME

 * @param msg the custom error-message, which should be printed
 */
static void errorExit(char *msg)
{
    if (errno == 0)
    {
        fprintf(stderr, "%s: %s\n", msg, PROG_NAME);
    }
    else
    {
        fprintf(stderr, "%s: %s %s\n", msg, strerror(errno), PROG_NAME);
    }

    exit(EXIT_FAILURE);
}

/**
 * @brief Creates a child-process and sets up piping to redirect input from parent to child
 * @details Each child-process starts a new instance from .\forksort and redirects input from
 * parent to child and each parent-process saves pid and pipes for writing and reading from child.
 *
 * @param c pointer to a childProcess-struct which should contain all information after returning from call
 */
static void createChildProcess(struct childProcess *c)
{
    // setting up common variables.
    int pipes[2][2];
    if ((pipe(pipes[0]) == -1) || (pipe(pipes[1]) == -1))
    {
        errorExit("Failed to open Pipes!");
    }

    c->pid = fork();
    switch (c->pid)
    {
    // error
    case -1:
        errorExit("Failed to fork!");
        break;
    // child-case
    case 0:
        close(pipes[0][1]);
        close(pipes[1][0]);
        if (((dup2(pipes[0][0], STDIN_FILENO)) == -1) || ((dup2(pipes[1][1], STDOUT_FILENO)) == -1))
        {
            errorExit("Failed to redirect stdin or stdout!");
        }
        close(pipes[0][0]);
        close(pipes[1][1]);
        if (execlp(PROG_NAME, PROG_NAME, NULL) == -1)
        {
            errorExit("Failed to execute program!");
        }
        assert(0);
    // parent-case
    default:
        close(pipes[0][0]);
        close(pipes[1][1]);
        c->write = pipes[0][1];
        c->read = pipes[1][0];
        break;
    }
}

/**
 * @brief Cleans up resources used for a child-process
 *
 * @param buf Pointer to buffer for reading from/writing to child-process
 * @param f Pointer to the input/output
 * @param pipe Pointer to the read/write pipe
 */
static void cleanUp(char **buf, FILE **f, int *pipe)
{
    free(*buf);
    fclose(*f);
    close(*pipe);
}

/**
 * @brief Reads the first two lines and creates two child-process to redirect the
 * following inputs to those childs
 * @details If EOF occurs after max one line this line gets printed and methods
 * exits with EXIT_SUCCESS. Otherwise two child-processes are created and the next
 * input is redirected to their input until EOF is reached.
 *
 * @param c1 Pointer to the first child-process
 * @param c2 Pointer to the second child-process
 */
static void readAndRedirectInput(struct childProcess *c1, struct childProcess *c2)
{
    char *bufChild1 = NULL, *bufChild2 = NULL;
    size_t bufSizeChild1 = 0, bufSizeChild2 = 0;

    if (getline(&bufChild1, &bufSizeChild1, stdin) == EOF)
    {
        // no line is read
        free(bufChild1);
        free(bufChild2);
        exit(EXIT_SUCCESS);
    }

    if (getline(&bufChild2, &bufSizeChild2, stdin) == EOF)
    {
        // only one line is read
        fputs(bufChild1, stdout);
        free(bufChild1);
        free(bufChild2);
        exit(EXIT_SUCCESS);
    }

    // if two lines are read, fork two children
    createChildProcess(c1);
    createChildProcess(c2);

    // open stdin for children
    FILE *stdinChild1 = fdopen(c1->write, "w");
    FILE *stdinChild2 = fdopen(c2->write, "w");
    fputs(bufChild1, stdinChild1);
    fputs(bufChild2, stdinChild2);

    // redirect input to children
    while (getline(&bufChild1, &bufSizeChild1, stdin) != EOF)
    {
        if (fputs(bufChild1, stdinChild1) == EOF)
        {
            cleanUp(&bufChild1, &stdinChild1, &c1->write);
            cleanUp(&bufChild2, &stdinChild2, &c2->write);
            errorExit("Failed to write!");
        }

        if (getline(&bufChild2, &bufSizeChild2, stdin) != EOF)
        {
            if (fputs(bufChild2, stdinChild2) == EOF)
            {
                cleanUp(&bufChild1, &stdinChild1, &c1->write);
                cleanUp(&bufChild2, &stdinChild2, &c2->write);
                errorExit("Failed to write!");
            }
        }
    }
    cleanUp(&bufChild1, &stdinChild1, &c1->write);
    cleanUp(&bufChild2, &stdinChild2, &c2->write);
}

/**
 * @brief Waits for child-process to terminate, and exits with EXIT_FAILURE if an
 * error occurs
 *
 * @param c1 Pointer to the first child-process
 * @param c2 Pointer to the second child-process
 */
static void waitForChildProcess(struct childProcess *c1, struct childProcess *c2)
{
    int status;
    waitpid(c1->pid, &status, 0);
    if (WIFEXITED(status))
    {
        if (WEXITSTATUS(status) == EXIT_FAILURE)
        {
            close(c1->read);
            close(c2->read);
            errorExit("Child Process failed!");
        }
    }
}

/**
 * @brief Sorts all lines receiving from the child-processes alphabetically
 * @details Reads line from both children until one returns EOF, then only from the
 * second one is read until EOF. Those read line are compared and the smaller is
 * written to the parent-output.
 *
 * @param c1 Pointer to the first child-process
 * @param c2 Pointer to the second child-process
 */
static void mergeSort(struct childProcess *c1, struct childProcess *c2)
{
    char *bufChild1 = NULL, *bufChild2 = NULL;
    size_t bufSizeChild1 = 0, bufSizeChild2 = 0;
    ssize_t eofChild1 = 0, eofChild2 = 0;
    FILE *stdoutChild1 = fdopen(c1->read, "r");
    FILE *stdoutChild2 = fdopen(c2->read, "r");

    while (eofChild1 != EOF && eofChild2 != EOF)
    {
        if (bufChild1 == NULL)
            eofChild1 = getline(&bufChild1, &bufSizeChild1, stdoutChild1);

        if (bufChild2 == NULL)
            eofChild2 = getline(&bufChild2, &bufSizeChild2, stdoutChild2);

        if (eofChild1 != EOF && eofChild2 != EOF)
        {
            if (strcmp(bufChild1, bufChild2) < 0)
            {
                if (fputs(bufChild1, stdout) == EOF)
                {
                    cleanUp(&bufChild1, &stdoutChild1, &c1->read);
                    cleanUp(&bufChild2, &stdoutChild2, &c2->read);
                    errorExit("Failed to write!");
                }
                free(bufChild1);
                bufChild1 = NULL;
            }
            else
            {
                if (fputs(bufChild2, stdout) == EOF)
                {
                    cleanUp(&bufChild1, &stdoutChild1, &c1->read);
                    cleanUp(&bufChild2, &stdoutChild2, &c2->read);
                    errorExit("Failed to write!");
                }
                free(bufChild2);
                bufChild2 = NULL;
            }
        }
    }

    if (eofChild1 == EOF)
    {
        if (fputs(bufChild2, stdout) == EOF)
        {
            cleanUp(&bufChild1, &stdoutChild1, &c1->read);
            cleanUp(&bufChild2, &stdoutChild2, &c2->read);
            errorExit("Failed to write!");
        }

        while (getline(&bufChild2, &bufSizeChild2, stdoutChild2) != EOF)
        {
            if (fputs(bufChild2, stdout) == EOF)
            {
                cleanUp(&bufChild1, &stdoutChild1, &c1->read);
                cleanUp(&bufChild2, &stdoutChild2, &c2->read);
                errorExit("Failed to write!");
            }
        }
    }
    else
    {
        if (fputs(bufChild1, stdout) == EOF)
        {
            cleanUp(&bufChild1, &stdoutChild1, &c1->read);
            cleanUp(&bufChild2, &stdoutChild2, &c2->read);
            errorExit("Failed to write!");
        }

        while (getline(&bufChild1, &bufSizeChild1, stdoutChild1) != EOF)
        {
            if (fputs(bufChild1, stdout) == EOF)
            {
                cleanUp(&bufChild1, &stdoutChild1, &c1->read);
                cleanUp(&bufChild2, &stdoutChild2, &c2->read);
                errorExit("Failed to write!");
            }
        }
    }

    cleanUp(&bufChild1, &stdoutChild1, &c1->read);
    cleanUp(&bufChild2, &stdoutChild2, &c2->read);
}

/**
 * @brief The entry point of the programm
 * @details Calls all the functions necessary to sort all lines from stdin
 *
 * @param argc the argument counter
 * @param argv the argument values
 * @return returns EXIT_SUCCESS upon success or EXIT_FAILURE upon failure.
 */
int main(int argc, char *argv[])
{
    // parseArguments
    PROG_NAME = argv[0];

    // no arguments allowed
    if (argc != 1)
        usage();

    struct childProcess c1, c2;

    readAndRedirectInput(&c1, &c2);

    waitForChildProcess(&c1, &c2);
    waitForChildProcess(&c2, &c1);

    mergeSort(&c1, &c2);

    return EXIT_SUCCESS;
}
