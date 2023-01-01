/**
 * @file client.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2022-12-31
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "stdio.h"
#include "stdlib.h"
#include <sys/stat.h>
#include "sys/types.h"
#include "sys/socket.h"
#include "string.h"
#include "netinet/in.h"
#include "netdb.h"
#include "stdbool.h"
#include "getopt.h"
#include "errno.h"
#include "unistd.h"
#include "signal.h"
#include "time.h"

#define BUF_SIZE 1024

static char *PROG_NAME;

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
 * @brief
 *
 */
static void printUsageInfoAndExit(void)
{
    fprintf(stdout, "Usage: %s [-p PORT] [-i INDEX] DOC_ROOT]\n", PROG_NAME);
    exit(EXIT_FAILURE);
}

/**
 * @brief
 *
 * @param errorMessage
 */
static void printErrorAndExit(char *errorMessage)
{
    if (errno == 0)
        fprintf(stderr, "[%s] ERROR: %s\n", PROG_NAME, errorMessage);
    else
        fprintf(stderr, "[%s] ERROR: %s: %s\n", PROG_NAME, errorMessage, strerror(errno));

    exit(EXIT_FAILURE);
}

/**
 * @brief
 *
 * @param string
 * @return true
 * @return false
 */
static bool checkIfNumber(char *string)
{
    char *endptr = NULL;
    strtol(string, &endptr, 10);

    return *endptr == '\0';
}

/**
 * @brief
 *
 * @param argumentCount
 * @param arguments
 * @param port
 * @param index
 * @param docRoot
 */
static void parseArguments(int argumentCount, char **arguments, char **port, char **index, char **docRoot)
{
    bool pFlag = false;
    bool iFlag = false;

    // parse Arguments
    int opt;
    while ((opt = getopt(argumentCount, arguments, "p:i:")) != -1)
    {
        switch (opt)
        {
        case 'p':
            *port = optarg;
            // check if valid port
            if (!checkIfNumber(*port) || pFlag)
                printUsageInfoAndExit();

            pFlag = true;
            break;
        case 'i':
            if (iFlag)
                printUsageInfoAndExit();

            *index = optarg;
            iFlag = true;
            break;
        case '?':
            printUsageInfoAndExit();
            break;
        default:
            printUsageInfoAndExit();
        }
    }

    // check if the doc-root is specified
    if (optind + 1 != argumentCount)
        printUsageInfoAndExit();

    // parse the doc-root
    *docRoot = arguments[optind];
}

static int createSocket(char *port)
{
    struct addrinfo hints, *ai;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int res = getaddrinfo(NULL, port, &hints, &ai);
    if (res != 0)
    {
        fprintf(stderr, "[%s] ERROR: getaddrinfo() failed: %s\n", PROG_NAME, gai_strerror(res));
        exit(EXIT_FAILURE);
    }

    int sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (sockfd < 0)
    {
        printErrorAndExit("socket() failed");
    }

    // option to reuse port immediatley
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

    if (bind(sockfd, ai->ai_addr, ai->ai_addrlen) < 0)
        printErrorAndExit("bind() failed");

    if (listen(sockfd, 10) < 0)
        printErrorAndExit("listen() failed");

    freeaddrinfo(ai);
    return sockfd;
}

static int readHeaders(FILE *connectFile, char **requestPath)
{
    char *buffer = NULL;
    size_t bufferSize = 0;

    if (getline(&buffer, &bufferSize, connectFile) == EOF)
    {
        free(buffer);
        // fprintf(stderr, "[%s] Error: Protocol error\n", PROG_NAME);
        return 400;
    }

    // parse first line
    char *method = strtok(buffer, " ");
    char *requestedPath = strtok(NULL, " ");
    char *version = strtok(NULL, "\r\n");

    if (method == NULL || requestedPath == NULL || version == NULL)
    {
        free(buffer);
        return 400;
    }

    if (strcmp(version, "HTTP/1.1") != 0)
    {
        free(buffer);
        // fprintf(stderr, "[%s] Error: Protocol error!\n", PROG_NAME);
        return 400;
    }

    if (strcmp(method, "GET") != 0)
    {
        free(buffer);
        // fprintf(stderr, "[%s] Error %s: %s\n", PROG_NAME, statuscode, statusmessage);
        return 501;
    }

    strcat(*requestPath, requestedPath);

    // skip header-lines
    while (getline(&buffer, &bufferSize, connectFile) != EOF)
    {
        if (strncmp(buffer, "\r\n", strlen("\r\n")) == 0)
            break;
    }

    free(buffer);
    return 200;
}

static void writeHeader(FILE *connectFile, char *requestPath)
{
    off_t fileSize = 0;
    struct stat st;

    if (stat(requestPath, &st) == 0)
        fileSize = st.st_size;

    // Find out the time
    char timeAsText[200];
    time_t t = time(NULL);
    struct tm *tmp;
    tmp = gmtime(&t);
    strftime(timeAsText, sizeof(timeAsText), "%a, %d %b %y %T %Z", tmp);

    fprintf(connectFile, "HTTP/1.1 200 OK\r\nDate: %s\r\nContent-Length: %lu\r\nConnection: close\r\n", timeAsText, fileSize);

    char *extension = strrchr(requestPath, '.');
    if (extension != NULL)
    {
        if (strcmp(extension, "html") || strcmp(extension, "htm"))
            fprintf(connectFile, "Content-Type: text/html\r\n");
        else if (strcmp(extension, "css"))
            fprintf(connectFile, "Content-Type: text/css\r\n");
        else if (strcmp(extension, "js"))
            fprintf(connectFile, "Content-Type: application/javascript\r\n");
    }

    fprintf(connectFile, "\r\n");
}

static void writeErrorHeader(FILE *connectFile, int statusCode)
{
    char *statusMessage = NULL;
    switch (statusCode)
    {
    case 400:
        statusMessage = "Bad Request";
        break;
    case 404:
        statusMessage = "Not Found";
        break;
    case 501:
        statusMessage = "Not implemented";
        break;
    }

    fprintf(connectFile, "HTTP/1.1 %d %s\r\nConnection: close\r\n\r\n", statusCode, statusMessage);
}

static void writeContent(FILE *connectFile, FILE *fileInput)
{
    size_t read = 0;
    char buffer[BUF_SIZE];
    memset(&buffer, 0, sizeof(buffer));

    while ((read = fread(buffer, sizeof(char), BUF_SIZE, fileInput)) != 0)
        fwrite(buffer, sizeof(char), read, connectFile);

    fflush(connectFile);
}

int main(int argc, char **argv)
{
    PROG_NAME = argv[0];

    // Setup the signal handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    char *port = "8080", *index = "index.html", *docRoot = NULL;

    // parse Arguments
    parseArguments(argc, argv, &port, &index, &docRoot);

    // open socket
    int sockfd = createSocket(port);

    while (!quit)
    {
        FILE *connectFile = NULL;
        // open connection
        int connfd = accept(sockfd, NULL, NULL);
        if (connfd < 0)
        {
            if (errno == EINTR)
                continue;

            // error
            printErrorAndExit("accept failed");
        }

        connectFile = fdopen(connfd, "r+");
        if (connectFile == NULL)
        {
            close(connfd);
            continue;
        }

        if (connfd == -1)
        {
            fprintf(stderr, "[%s] Error opening connection\n", PROG_NAME);
            continue;
        }

        // Signal occured. continue -> quit is 1 and exit the loop
        if (connectFile == NULL)
        {
            continue;
        }

        // read headers
        char *requestPath = strdup(docRoot);
        int statusCode = readHeaders(connectFile, &requestPath);

        if ((requestPath)[strlen(requestPath) - 1] == '/')
            strcat(requestPath, index);

        // fprintf(stdout, "%s\n", requestPath);

        // Open the specified file which should be transmitted
        FILE *inputFile = fopen(requestPath, "r+");
        if (statusCode == 200)
        {
            if (inputFile == NULL)
                statusCode = 404;
        }
        // fprintf(stderr, "[%s] Status-code [%d], File-path (%s) \n", prog_name, status_code, full_path);

        // Write the normal header and content if the status code is 200. Otherwise write the error header and no content
        if (statusCode == 200)
        {
            writeHeader(connectFile, requestPath);
            writeContent(connectFile, inputFile);
        }
        else
        {
            writeErrorHeader(connectFile, statusCode);
        }

        // Free resources
        if (inputFile != NULL)
            fclose(inputFile);

        fclose(connectFile);
        close(connfd);
    }

    // free resources
    close(sockfd);

    exit(EXIT_SUCCESS);
}
