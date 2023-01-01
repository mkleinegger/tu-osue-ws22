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
#include "sys/types.h"
#include "sys/socket.h"
#include "string.h"
#include "netinet/in.h"
#include "netdb.h"
#include "stdbool.h"
#include "getopt.h"
#include "errno.h"
#include "unistd.h"

#define BUF_SIZE 1024

static char *PROG_NAME;

/**
 * @brief
 *
 */
static void printUsageInfoAndExit(void)
{
    fprintf(stdout, "Usage: %s [-p PORT] [-o FILE | -d DIR] URL\n", PROG_NAME);
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
 * @param host
 * @param requestPath
 * @param output
 */
static void parseArguments(int argumentCount, char **arguments, char **port, char **host, char **requestPath, char **output)
{
    bool oFlag = false;
    bool dFlag = false;
    bool pFlag = false;
    // parse Arguments
    int opt;
    while ((opt = getopt(argumentCount, arguments, "p:o:d:")) != -1)
    {
        switch (opt)
        {
        case 'p':
            *port = optarg;
            // check if valid port
            if (!checkIfNumber(*port) || pFlag)
            {
                free(*output);
                printUsageInfoAndExit();
            }

            pFlag = true;
            break;
        case 'o':
            if (oFlag)
            {
                free(*output);
                printUsageInfoAndExit();
            }

            *output = strdup(optarg);
            oFlag = true;
            break;
        case 'd':
            if (dFlag)
            {
                free(*output);
                printUsageInfoAndExit();
            }

            *output = strdup(optarg);
            dFlag = true;
            break;
        case '?':
            free(*output);
            printUsageInfoAndExit();
            break;
        default:
            free(*output);
            printUsageInfoAndExit();
        }
    }

    // check options for output-file
    if (oFlag && dFlag)
    {
        free(*output);
        printUsageInfoAndExit();
    }

    // adapt output file
    if (dFlag && (*output)[strlen(*output) - 1] == '/')
        strcat(*output, "index.html");

    // check if the one url is specified
    if (optind + 1 != argumentCount)
    {
        free(*output);
        printUsageInfoAndExit();
    }

    // parse the url
    *host = arguments[optind];

    // Check if url starts with http://
    if (strncmp(*host, "http://", strlen("http://")) != 0)
    {
        free(*output);

        printUsageInfoAndExit();
    }

    *host = *host + strlen("http://");

    // Get file path
    char *requestFile = strpbrk(*host, ";/?:@=&");
    if (requestFile != NULL)
    {
        *requestPath = strdup(requestFile);
        strcpy(requestFile, "");
    }
    else
    {
        *requestPath = malloc(sizeof(char *));
        strcpy(*requestPath, "/");
    }
}

/**
 * @brief
 *
 * @param outputFile
 * @param fileOutput
 * @return int
 */
static int openFiles(char *outputFile, FILE **fileOutput)
{

    // open File
    if (outputFile != NULL)
    {
        if ((*fileOutput = fopen(outputFile, "w")) == NULL)
        {
            return -1;
        }
    }
    else
    {
        *fileOutput = stdout;
    }

    return 0;
}

/**
 * @brief Create a Socket object
 *
 * @param host
 * @param port
 * @return int
 */
static int createSocket(char *host, char *port)
{
    struct addrinfo hints, *ai;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int res = getaddrinfo(host, port, &hints, &ai);
    if (res != 0)
    {
        fprintf(stderr, "[%s] ERROR: getaddrinfo() failed: %s\n", PROG_NAME, gai_strerror(res));
        exit(EXIT_FAILURE);
    }

    int sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (sockfd < 0)
        printErrorAndExit("socket() failed");

    if (connect(sockfd, ai->ai_addr, ai->ai_addrlen) < 0)
        printErrorAndExit("connect() failed");

    freeaddrinfo(ai);
    return sockfd;
}

/**
 * @brief
 *
 * @param sockfile
 * @param host
 * @param requestPath
 */
static void sendRequest(FILE *sockfile, char *host, char *requestPath)
{
    fprintf(sockfile, "GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: osue-http-client/1.0\r\nConnection: close\r\n\r\n", requestPath, host);
    fflush(sockfile);
}

/**
 * @brief
 *
 * @param sockfile
 * @return int
 */
static int readHeaders(FILE *sockfile)
{
    char *buffer = NULL;
    size_t bufferSize = 0;

    if (getline(&buffer, &bufferSize, sockfile) == EOF)
    {
        free(buffer);
        fprintf(stderr, "[%s] Error: Protocol error\n", PROG_NAME);
        return 2;
    }

    // parse first line
    char *protocol = strtok(buffer, " ");
    char *statuscode = strtok(NULL, " ");
    char *statusmessage = strtok(NULL, "\r");

    if (strncmp(protocol, "HTTP/1.1", strlen("HTTP/1.1")) != 0 || !checkIfNumber(statuscode))
    {
        free(buffer);
        fprintf(stderr, "[%s] Error: Protocol error!\n", PROG_NAME);
        return 2;
    }

    if (strncmp(statuscode, "200", strlen(statuscode)) != 0)
    {
        free(buffer);
        fprintf(stderr, "[%s] Error %s: %s\n", PROG_NAME, statuscode, statusmessage);
        return 3;
    }

    // skip header-lines
    while (getline(&buffer, &bufferSize, sockfile) != EOF)
    {
        if (strncmp(buffer, "\r\n", strlen("\r\n")) == 0)
            break;
    }

    free(buffer);
    return 0;
}

/**
 * @brief
 *
 * @param sockfile
 * @param fileOutput
 */
static void readContent(FILE *sockfile, FILE *fileOutput)
{
    size_t read = 0;
    char buffer[BUF_SIZE];
    memset(&buffer, 0, sizeof(buffer));

    while ((read = fread(buffer, sizeof(char), BUF_SIZE, sockfile)) != 0)
        fwrite(buffer, sizeof(char), read, fileOutput);

    fflush(fileOutput);
}

/**
 * @brief
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char **argv)
{
    PROG_NAME = argv[0];

    char *port = "80", *host = NULL, *requestPath = NULL, *output = NULL;
    FILE *fileOutput = NULL;

    // parse Arguments
    parseArguments(argc, argv, &port, &host, &requestPath, &output);
    if (openFiles(output, &fileOutput) == -1)
    {
        free(requestPath);
        free(output);
        printErrorAndExit("Openening or creating output-file failed");
    }

    // open socket
    int sockfd = createSocket(host, port);
    FILE *sockfile = fdopen(sockfd, "r+");
    if (sockfile == NULL)
    {
        free(requestPath);
        free(output);
        close(sockfd);
        fclose(fileOutput);
        printErrorAndExit("socket couldn't be opened");
    }

    // send request
    sendRequest(sockfile, host, requestPath);

    // read content
    int exitStatus = readHeaders(sockfile);
    if (exitStatus == 0)
    {
        readContent(sockfile, fileOutput);
        exitStatus = EXIT_SUCCESS;
    }

    // free resources
    fclose(sockfile);
    fclose(fileOutput);
    close(sockfd);
    free(output);
    free(requestPath);

    exit(exitStatus);
}
