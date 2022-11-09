/**
 * @file generator.c
 * @author Maximilian Kleinegger (12041500)
 * @date 2022-11-01
 * @brief generator program for the 3coloring problem
 * @details This generator programm calculates possible solution
 * for the 3coloring problem for the given graph. It reports it's
 * solution to the circle-buffer. The process is running until the
 * user stops it with a signal, or it finds out that the graph is
 * 3-colorable.
 *
 */
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include "circleBuffer.h"

/**
 * @brief Definies the max length a solution can have
 *
 */
#define MAX_SOLUTION_LENGTH 8

/**
 * @brief Data structure to store an edge
 * @details Because the graph is undirected, this information is not necessary
 * and therefore it isn't important which vertex is the start or the end.
 *
 */
struct Edge
{
    struct Vertex *vertex1;
    struct Vertex *vertex2;
};

/**
 * @brief Data structure to store a vertex
 * @details Color: Green (0), Blue (1), Red (2), Undefined (-1)
 *
 */
struct Vertex
{
    char *name;
    int color;
};

/**
 * @brief name of the programm
 *
 */
static char *progName;

/**
 * @brief Flag which decides if the process should stop
 *
 */
static volatile sig_atomic_t quit = 0;
/**
 * @brief Function which handels the SIGINT and SIGTERM signals
 * @details If this function is called, then the variable quit is set
 * to 1, therefore the process should be stopped.
 *
 * @param signal Signal-number (which will be not used)
 */
static void handle_signal(int signal) { quit = 1; }

/**
 * @brief This function parses arguments
 * @details The function parses the arguments to vertices and edges. It terminates the
 * programm if a argument does not meet the format for a vertex (number-number). Otherwise
 * it saves the new vertices and edges inside the arrays and increases the total-sizes of
 * the arrays.
 *
 * @param argc Number of arguments provided
 * @param argv Values of arguments provided
 * @param vertices Pointer to the array of vertex-pointers, where the vertices are saved to
 * @param totalSizeVertices Size of the array @param vertices
 * @param edges Pointer to the array of edge-pointers, where the edges are saved to
 * @param totalSizeEdges Size of the array @param edges
 */
static void parseArgumentsToGraph(int argc, char **argv, struct Vertex ***vertices, int *totalSizeVertices, struct Edge ***edges, int *totalSizeEdges);

/**
 * @brief This functions solves the 3coloring problem for the given graph
 * @details First this function colors the edges randomly with colors. Then it removes
 * the edges with same colored vertices. Then it reports this solution to the circle-buffer
 * and therefore to the supervisor. This process repeats itself until the circle-buffer
 * is closed, or the graph is 3colorable by itself.
 *
 * @param circleBuffer Pointer to the circle-buffer
 * @param vertices Pointer to the array of vertex-pointers of the graph
 * @param totalSizeVertices Size of the array @param vertices
 * @param edges Pointer to the array of edge-pointers of the graph
 * @param totalSizeEdges Size of the array @param edges
 */
static void solveProblem(struct circleBuffer *circleBuffer, struct Vertex **vertices, int totalSizeVertices, struct Edge **edges, int totalSizeEdges);

/**
 * @brief Frees all the allocated Memory from the graph
 *
 * @param vertices Pointer to the array of vertex-pointers of the graph
 * @param totalSizeVertices Size of the array @param vertices
 * @param edges Pointer to the array of edge-pointers of the graph
 * @param totalSizeEdges Size of the array @param edges
 */
static void freeGraph(struct Vertex **vertices, int totalSizeVertices, struct Edge **edges, int totalSizeEdges);

/**
 * @brief This functions prints the errorMessage to stderr and exits the programm with code EXIT_FAILURE.
 *
 * @param errorMessage The custom errorMessage which will be printed as the error to stderr
 */
static void printErrorAndExit(char *errorMessage);

/**
 * @brief generators entry point
 * @details This function parses the arguments, opens/closes the circle-buffer
 * and calls the function to solve the 3coloring problem.
 *
 * @param argc the number of arguments provided
 * @param argv the argument-values provided
 * @return Returns EXIT_SUCCESS upon success or EXIT_FAILURE upon failure.
 */
int main(int argc, char *argv[])
{
    // set random
    srand(getpid());

    // Setup the singal handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // set up pointers for vertices and edges
    struct Vertex **vertices = NULL;
    struct Edge **edges = NULL;
    int totalSizeVertices = 0, totalSizeEdges = 0;

    // parse arguments to edges and vertices
    parseArgumentsToGraph(argc, argv, &vertices, &totalSizeVertices, &edges, &totalSizeEdges);

    // open the circle-buffer
    struct circleBuffer *circleBuffer = openCircleBuffer('c');
    if (circleBuffer == NULL)
    {
        // free vertices
        freeGraph(vertices, totalSizeVertices, edges, totalSizeEdges);
        printErrorAndExit("Opening circle-buffer failed");
    }

    // solve 3coloring problem
    solveProblem(circleBuffer, vertices, totalSizeVertices, edges, totalSizeEdges);

    // free vertices
    freeGraph(vertices, totalSizeVertices, edges, totalSizeEdges);

    // close cirle-buffer
    if (closeCircleBuffer(circleBuffer, 'c') == -1)
        printErrorAndExit("Closing circle-buffer failed");

    return EXIT_SUCCESS;
}

static void printErrorAndExit(char *errorMessage)
{
    fprintf(stderr, "[%s] ERROR: %s: %s\n", progName, errorMessage, strerror(errno));
    exit(EXIT_FAILURE);
}

static void freeGraph(struct Vertex **vertices, int totalSizeVertices, struct Edge **edges, int totalSizeEdges)
{
    // free vertices
    for (int i = 0; i < totalSizeVertices; i++)
        free(vertices[i]);

    free(vertices);

    // free edges
    for (int i = 0; i < totalSizeEdges; i++)
        free(edges[i]);

    free(edges);
}

/**
 * @brief Returns the Vertex with the specified name out of the vertices array
 *
 * @param vertices Array of pointers to vertices
 * @param name Name of vertex to search for
 * @param length Length of the array
 * @return Pointer to Vertex with the specified name
 */
static struct Vertex *getVertex(struct Vertex **vertices, char *name, int length)
{
    for (int i = 0; i < length; i++)
    {
        if (strcmp(vertices[i]->name, name) == 0)
            return vertices[i];
    }

    return NULL;
}

/**
 * @brief Returns the Edge with the specified vertices out of the edges array
 * @details Because the graph is undirected, it returns the first edge which contains both
 * vertices, where the ordering is ignored. Therefore 0-1 or 1-0 is returned if those are the
 * vertices specified.
 * @param edges Array of pointers to edges
 * @param vertex1 One vertex of the edge
 * @param vertex2 The other vertex of the edge
 * @param length Length of the array
 * @return Pointer to the edge which contains both vertices
 */
static struct Edge *getEdge(struct Edge **edges, struct Vertex *vertex1, struct Vertex *vertex2, int length)
{
    for (int i = 0; i < length; i++)
    {
        if (strcmp(edges[i]->vertex1->name, vertex1->name) == 0 && strcmp(edges[i]->vertex2->name, vertex2->name) == 0)
            return edges[i];

        if (strcmp(edges[i]->vertex1->name, vertex2->name) == 0 && strcmp(edges[i]->vertex2->name, vertex1->name) == 0)
            return edges[i];
    }

    return NULL;
}

/**
 * @brief This functions tries to add a vertex to the provided array
 * @details This functions checks if the vertex already exits. If yes it just
 * returns it, otherwise it adds it to the array and increases the array (reallocates
 * memory) and the @param totalSizeVertices
 * @param name Name of the new Vertex
 * @param vertices Array of vertex-pointers
 * @param totalSizeVertices Size of the array
 * @return A pointer to the newly created or found vertex
 */
static struct Vertex *addVertex(char *name, struct Vertex ***vertices, int *totalSizeVertices)
{
    struct Vertex *vertex = getVertex(*vertices, name, *totalSizeVertices);

    if ((struct Vertex *)vertex == NULL)
    {
        if (*vertices == NULL)
            *vertices = malloc((*totalSizeVertices + 1) * sizeof(struct Vertex *));
        else
            *vertices = realloc(*vertices, (*totalSizeVertices + 1) * sizeof(struct Vertex *));

        // increase to have the right amount of memory allocated, if successful
        if (*vertices != NULL)
            *totalSizeVertices += 1;

        vertex = malloc(sizeof(struct Vertex));
        // return null and exit if allocation failed
        if (*vertices == NULL || vertex == NULL)
            return NULL;

        vertex->name = name;
        (*vertices)[*totalSizeVertices - 1] = vertex;
    }

    return vertex;
}

static void parseArgumentsToGraph(int argc, char **argv, struct Vertex ***vertices, int *totalSizeVertices, struct Edge ***edges, int *totalSizeEdges)
{
    // create regexp for arguments
    regex_t regex;
    if (regcomp(&regex, "^[0-9]+-[0-9]+$", REG_EXTENDED) != 0)
        printErrorAndExit("Compilation of regexp failed");

    // parse vertices and edges
    for (int i = 1; i < argc; i++)
    {
        if (regexec(&regex, argv[i], 0, NULL, 0) == REG_NOMATCH)
        {
            fprintf(stderr, "Usage: %s Edge Edge Edge ... \n", argv[0]);
            fprintf(stderr, "Example: %s 0-1 0-2 1-2 \n", argv[0]);
            exit(EXIT_FAILURE);
        }

        char *vertexName1 = strtok(argv[i], "-");
        char *vertexName2 = strtok(NULL, " ");

        struct Vertex *vertex1 = addVertex(vertexName1, vertices, totalSizeVertices);
        struct Vertex *vertex2 = addVertex(vertexName2, vertices, totalSizeVertices);
        // exit if allocation failed
        if (vertex1 == NULL || vertex2 == NULL)
        {
            regfree(&regex);
            freeGraph(*vertices, *totalSizeVertices, *edges, *totalSizeEdges);
            printErrorAndExit("Allocation of memory for a vertex failed");
        }

        struct Edge *newEdge = getEdge(*edges, vertex1, vertex2, *totalSizeEdges);
        if ((struct Vertex *)newEdge == NULL)
        {
            if (*edges == NULL)
                *edges = malloc((*totalSizeEdges + 1) * sizeof(struct Edge *));
            else
                *edges = realloc(*edges, (*totalSizeEdges + 1) * sizeof(struct Edge *));

            if (*edges != NULL)
                *totalSizeEdges += 1;

            newEdge = malloc(sizeof(struct Edge));

            // exit if allocation failed
            if (*edges == NULL || newEdge == NULL)
            {
                regfree(&regex);
                freeGraph(*vertices, *totalSizeVertices, *edges, *totalSizeEdges);
                printErrorAndExit("Allocation of memory for a edge failed");
            }

            newEdge->vertex1 = vertex1;
            newEdge->vertex2 = vertex2;

            (*edges)[*totalSizeEdges - 1] = newEdge;
        }
    }

    // free regexp
    regfree(&regex);
}

/**
 * @brief Convertes the solution and number of edgesRemoved to a null terminated string
 * @param solution Array of edges, which should be converted
 * @param edgesRemoved number of edges, which are specified in the @param solution array
 * @return null-terminated string which has the format "edgesRemoved Edge Edge Edge ...""
 */
static char *generateOutput(struct Edge **solution, int edgesRemoved)
{
    // calculate the length of the output string
    int len = 2;
    for (int i = 0; i < edgesRemoved; i++)
    {
        len += strlen(solution[i]->vertex1->name);
        len += strlen(solution[i]->vertex2->name);
        len += 2;
    }

    // Creeate the output string and fill it
    char *output = malloc(sizeof(char) * len);
    memset(output, 0, len);

    output[0] = edgesRemoved + '0';
    output[1] = ' ';
    for (int i = 0; i < edgesRemoved; i++)
    {
        strcat(output, solution[i]->vertex1->name);
        strcat(output, "-");
        strcat(output, solution[i]->vertex2->name);
        if (i == edgesRemoved - 1)
            strcat(output, "\0");
        else
            strcat(output, " ");
    }

    return output;
}

static void solveProblem(struct circleBuffer *circleBuffer, struct Vertex **vertices, int totalSizeVertices, struct Edge **edges, int totalSizeEdges)
{
    int smallestSolution = MAX_SOLUTION_LENGTH;

    while (circleBuffer->sharedMemory->isAlive && quit == 0)
    {
        // color vertices randomly
        for (int i = 0; i < totalSizeVertices; i++)
        {
            vertices[i]->color = rand() % 3;
        }

        struct Edge *solution[MAX_SOLUTION_LENGTH];
        int edgesRemoved = 0;

        // remove edges with same colored vertex
        for (int i = 0; i < totalSizeEdges && edgesRemoved < smallestSolution; i++)
        {
            if (edges[i]->vertex1->color == edges[i]->vertex2->color)
            {
                solution[edgesRemoved] = edges[i];
                edgesRemoved++;
            }
        }

        if (edgesRemoved < smallestSolution)
        {
            // write new smallest solution to the buffer
            smallestSolution = edgesRemoved;
            char *output = generateOutput(solution, edgesRemoved);
            writeCircleBuffer(circleBuffer, output);

            free(output);
        }
    }
}
