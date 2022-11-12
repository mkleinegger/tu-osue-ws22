/**
 * @file supervisor.c
 * @author Maximilian Kleinegger <e12041500@student.tuwien.ac.at>
 * @date 2022-11-01
 *
 * @brief supervisor program for the 3coloring problem
 *
 * Reads and prints solution for the 3coloring problem to stdout, if
 * it is the best so far.
 */
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include "circleBuffer.h"

/**
 * @brief Flag which decides if the process should stop
 *
 */
static volatile sig_atomic_t quit = 0;

/**
 * @brief Function which handels the SIGINT and SIGTERM signals and sets variable quit to 1
 * @details global variables: quit
 * @param signal Signal-number (which will be not used)
 */
static void handle_signal(int signal) { quit = 1; }

/**
 * Supervisors entry point.
 * @brief This function opens and closes the circle-buffer and also does
 * the reading and printing of the new solutions found by the generators.
 * The programm will only terminate if a signal interrupts the process or
 * the graph is 3colorable!
 * @details global variables: quit
 * @param argc the number of arguments provided
 * @param argv the argument-values provided
 * @return Returns EXIT_SUCCESS upon success or EXIT_FAILURE upon failure.
 */
int main(int argc, char *argv[])
{
    // if any arguments are provided the program terminates
    if (argc > 1)
    {
        fprintf(stderr, "Usage: %s\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Setup the singal handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // opening the circle buffer
    struct circleBuffer *circleBuffer = openCircleBuffer(true);
    if (circleBuffer == NULL)
    {
        fprintf(stderr, "[%s] Error: Opening the circle-buffer failed: %s\n", argv[0], strerror(errno));
        return EXIT_FAILURE;
    }

    int min = 0;
    bool hasMin = true;

    while (quit == false)
    {
        // Read a solution from the circular buffer
        char *s = readCircleBuffer(circleBuffer);
        if (s == NULL)
            break;

        // Read the first character, which is the length of this solution and
        // convert it to an integer
        int tmp_min = s[0] - '0';

        // Print the solution, if its the best so far
        if (tmp_min < min || hasMin == true)
        {
            min = tmp_min;
            hasMin = false;

            if (min > 0)
            {
                printf("[%s] Solution with %d edges: %s\n", argv[0], min, &s[2]);
            }
            else
            {
                // Best possible solution is found, we can stop the processes
                printf("[%s] The graph is 3-colorable!\n", argv[0]);
                quit = true;
            }
        }

        free(s);
    }

    // closing the circle buffer
    if (closeCircleBuffer(circleBuffer, true) == -1)
    {
        fprintf(stderr, "[%s] Error: Closing the circle-buffer failed: %s\n", argv[0], strerror(errno));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
