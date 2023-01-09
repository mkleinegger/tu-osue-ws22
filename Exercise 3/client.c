/**
 * @file client.c
 * @author Maximilian Kleinegger <e12041500@student.tuwien.ac.at>
 * @date 2022-12-31
 *
 * @brief This program implements as a http (version 1.1) client, with the GET-method
 */
#include "stdio.h"
#include "stdlib.h"
#include "netdb.h"
#include "string.h"
#include "stdbool.h"
#include "getopt.h"
#include "errno.h"
#include "unistd.h"
#include "signal.h"
#include "time.h"
#include "sys/stat.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "netinet/in.h"

#define BUF_SIZE 1024

static char *PROG_NAME;

/**
 * @brief Prints the usage message and exits with EXIT_FAILURE
 *
 * @details global variables: PROG_NAME
 */
static void printUsageInfoAndExit(void)
{
    fprintf(stdout, "Usage: %s [-p PORT] [-o FILE | -d DIR] URL\n", PROG_NAME);
    exit(EXIT_FAILURE);
}

/**
 * @brief Prints a error-message and if existing the errno-message
 * @details global variables: PROG_NAME
 *
 * @param errorMessage the custom error-message, which should be printed
 */
static void printError(char *errorMessage)
{
    if (errno == 0)
        fprintf(stderr, "[%s] ERROR: %s\n", PROG_NAME, errorMessage);
    else
        fprintf(stderr, "[%s] ERROR: %s: %s\n", PROG_NAME, errorMessage, strerror(errno));
}

/**
 * @brief Prints a error-message and if existing the errno-message and then exits with EXIT_FAILURE
 * @details global variables: PROG_NAME
 *
 * @param errorMessage the custom error-message, which should be printed
 */
static void printErrorAndExit(char *errorMessage)
{
    printError(errorMessage);
    exit(EXIT_FAILURE);
}

/**
 * @brief Checks if the specified string is a valid port
 *
 * @param string which should be checked
 * @return true, if the specified string is a number
 * @return false, otherwise
 */
static bool checkIfValidPort(char *string)
{
    char *endptr = NULL;
    long port = strtol(string, &endptr, 10);

    return *endptr == '\0' && port >= 0 && port <= 65535;
}

/**
 * @brief Checks if the specified string is a number
 *
 * @param string which should be checked
 * @return true, if the specified string is a number
 * @return false, otherwise
 */
static bool checkIfNumber(char *string)
{
    char *endptr = NULL;
    strtol(string, &endptr, 10);

    return *endptr == '\0';
}

/**
 * @brief This function parses the arguments
 * @details The function parses the options and arguments and saves them in the provided pointers.
 * Additionally it updates them according to the specification. If the user provides to little or
 * to many arguments or wrong arguments the programm terminates with return-value EXIT_FAILURE and
 * prints the usage-message, or error-message if the format is not matched.
 *
 * @param argumentCount number of arguments provided
 * @param arguments values of arguments provided
 * @param port pointer, where the port should be saved
 * @param host pointer, where the host should be saved
 * @param requestPath pointer, where the requested path should be saved
 * @param output pointer, where the output(-file) should be saved
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
            if (!checkIfValidPort(*port))
            {
                free(*output);
                printErrorAndExit("invalid port");
            }
            else if (pFlag)
            {
                free(*output);
                printErrorAndExit("invalid options");
            }

            pFlag = true;
            break;
        case 'o':
            if (oFlag)
            {
                free(*output);
                printErrorAndExit("invalid options");
            }

            *output = strdup(optarg);
            oFlag = true;
            break;
        case 'd':
            if (dFlag)
            {
                free(*output);
                printErrorAndExit("invalid options");
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
    if (dFlag && (*output)[strlen(*output) - 1] != '/')
        strcat(*output, "/");

    if (dFlag && (*output)[strlen(*output) - 1] == '/')
        strcat(*output, "index.html");
    else if (dFlag)
        strcat(*output, strrchr(*output, '/') + 1);

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
        printErrorAndExit("invalid protocol");
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
 * @brief This function opens the specified file
 * @details If no outputFile is specified, results are printed to the
 * output
 *
 * @param outputFile pointer to the name of the output-file
 * @param fileOutput pointer, where the output-file should be saved
 * @return -1 if error occures, otherwise 0
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
 * @brief Create a client-socket object
 * @details If any method to create the socket fails, the server returns
 * -1 and prints a error-message to stderr.
 *
 * @param host the specified host, this client should be connecting to
 * @param port the specified port, this client should be connecting to
 * @return the file-descriptor to the newly connected host
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
        return -1;
    }

    int sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (sockfd < 0)
    {
        freeaddrinfo(ai);
        printError("socket() failed");
        return -1;
    }

    if (connect(sockfd, ai->ai_addr, ai->ai_addrlen) < 0)
    {
        close(sockfd);
        freeaddrinfo(ai);
        printError("connect() failed");
        return -1;
    }

    freeaddrinfo(ai);
    return sockfd;
}

/**
 * @brief This functions send the request to the host
 *
 * @param sockfile pointer to the I/O from the server
 * @param host the host as a string
 * @param requestPath the requested Path as a string
 */
static void sendRequest(FILE *sockfile, char *host, char *requestPath)
{
    fprintf(sockfile, "GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: osue-http-client/1.0\r\nConnection: close\r\n\r\n", requestPath, host);
    fflush(sockfile);
}

/**
 * @brief This functions reads the headers from the response
 * @details Reads only the first line and skips the rest
 *
 * @param sockfile  pointer to the I/O from the server
 * @return returns an int, which reports the result of reading the first-line
 */
static int readHeaders(FILE *sockfile)
{
    char *buffer = NULL;
    size_t bufferSize = 0;

    if (getline(&buffer, &bufferSize, sockfile) == EOF)
    {
        free(buffer);
        printError("Protocol error!");
        return 2;
    }

    // parse first line
    char *protocol = strtok(buffer, " ");
    char *statuscode = strtok(NULL, " ");
    char *statusmessage = strtok(NULL, "\r");

    if (strcmp(protocol, "HTTP/1.1") != 0 || !checkIfNumber(statuscode))
    {
        free(buffer);
        printError("Protocol error!");
        return 2;
    }

    if (strcmp(statuscode, "200") != 0)
    {
        free(buffer);
        fprintf(stderr, "[%s] Error %s %s\n", PROG_NAME, statuscode, statusmessage);
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
 * @brief Reads the content from sockfile and writes it to the fileOutput
 * @details reads and writes in binary format
 *
 * @param sockfile pointer to the I/O from the server
 * @param fileOutput pointer to the output-file
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
 * @brief The entry point of this programm, where all the different functions are called
 * @details The main-function calls the different functions, to read the arguments, to
 * start the client-socket and to connect to a server
 *
 * @param argc number of arguments, provided from the user
 * @param argv values of arguments, provided from the user
 * @return Returns EXIT_SUCCESS upon success or EXIT_FAILURE, 2 or 3 upon failure.
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
    if (sockfd == -1)
    {
        free(requestPath);
        free(output);
        fclose(fileOutput);
        printErrorAndExit("socket couldn't be opened");
    }

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
