/**
 * @file client.c
 * @author Maximilian Kleinegger <e12041500@student.tuwien.ac.at>
 * @date 2022-12-31
 *
 * @brief This program servers as a http server (version 1.1)
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

/**
 * @brief size of the buffer
 *
 */
#define BUF_SIZE 1024

static char *PROG_NAME; /** <name of the programm */

/**
 * @brief Flag which decides if the process should stop
 *
 */
static volatile sig_atomic_t quit = 0;

/**
 * @brief Function which handels the SIGINT and SIGTERM signals and sets variable quit to 1
 * @details global variables: quit
 *
 * @param signal Signal-number (which will be not used)
 */
static void handle_signal(int signal) { quit = 1; }

/**
 * @brief Prints the usage message and exits with EXIT_FAILURE
 *
 * @details global variables: PROG_NAME
 */
static void printUsageInfoAndExit(void)
{
    fprintf(stdout, "Usage: %s [-p PORT] [-i INDEX] DOC_ROOT]\n", PROG_NAME);
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
 * @brief This function parses the arguments
 * @details The function parses the options and arguments and saves them in the provided pointers.
 * If the user provides to little or to many arguments or wrong arguments the programm terminates
 * with return-value EXIT_FAILURE and prints the usage-message, or error-message if the format is
 * not matched.
 *
 * @param argumentCount number of arguments provided
 * @param arguments values of arguments provided
 * @param port pointer, where the port should be saved
 * @param index pointer, where the index-file should be saved
 * @param docRoot pointer, where the root-directory is saved
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
            if (!checkIfValidPort(*port))
                printErrorAndExit("invalid port");
            else if (pFlag)
                printErrorAndExit("invalid options");

            pFlag = true;
            break;
        case 'i':
            if (iFlag)
                printErrorAndExit("invalid options");

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

/**
 * @brief Creates a server Socket object
 * @details If any method to create the socket fails, the server terminates
 * with EXIT_FAILURE and prints a error-message to stderr.
 *
 * @param port the specified port, this server should be running on
 * @return the file-descriptor for the newly created socket
 */
static int createServerSocket(char *port)
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
        freeaddrinfo(ai);
        printErrorAndExit("socket() failed");
    }

    // option to reuse port immediatley
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

    if (bind(sockfd, ai->ai_addr, ai->ai_addrlen) < 0)
    {
        freeaddrinfo(ai);
        close(sockfd);
        printErrorAndExit("bind() failed");
    }

    if (listen(sockfd, 10) < 0)
    {
        freeaddrinfo(ai);
        close(sockfd);
        printErrorAndExit("listen() failed");
    }

    freeaddrinfo(ai);
    return sockfd;
}

/**
 * @brief accepts new client connections
 * @details if an error occures -1 is returned and an error-message is printed to
 * stderr, only if a signal interrupts 0 is returned and no error-message is displayed.
 * (error-messages are for logging)
 *
 * @param sockfd file-descriptor for the server-socket
 * @param connectedClient pointer to the file to read and write from connection
 * @return the file-descriptor for the client-connections
 */
static int acceptClient(int sockfd, FILE **connectedClient)
{
    // open connection
    int connfd = accept(sockfd, NULL, NULL);
    if (connfd < 0)
    {
        if (errno == EINTR)
            return 0;

        printError("accept() failed");
        return -1;
    }

    *connectedClient = fdopen(connfd, "r+");
    if (*connectedClient == NULL)
    {
        close(connfd);
        printError("fdopen() for new connection failed");
        return -1;
    }

    return connfd;
}

/**
 * @brief reads the client header
 * @details if the first-line does not contain the method, the version and a requested
 * path or the http-version is invalid 404 is returned, if the method != GET 501 is returned and
 * an error-message is printed to stderr (error-messages are for logging)
 *
 * @param connectedClient pointer to the I/O from the client
 * @param requestPath pointer to save the requested path from the client inquiry
 * @return returns the statuscode according to parsing the first header-line
 */
static int readClientHeaders(FILE *connectedClient, char **requestPath)
{
    char *buffer = NULL;
    size_t bufferSize = 0;
    int statusCode = 200;

    if (getline(&buffer, &bufferSize, connectedClient) == EOF)
    {
        printError("getline() failed");
        statusCode = 400;
    }

    // parse first line
    char *method = strtok(buffer, " ");
    char *requestedPath = strtok(NULL, " ");
    char *version = strtok(NULL, "\r\n");

    if ((method == NULL || requestedPath == NULL || version == NULL) && statusCode == 200)
    {
        printError("method, version or requestedPath is missing");
        statusCode = 400;
    }

    if (statusCode == 200 && strcmp(version, "HTTP/1.1") != 0)
    {
        printError("invalid version");
        statusCode = 400;
    }

    if (statusCode == 200 && strcmp(method, "GET") != 0)
    {
        printError("invalid method");
        statusCode = 501;
    }

    if (statusCode == 200)
    {
        *requestPath = realloc(*requestPath, (strlen(*requestPath) + strlen(requestedPath) + 1) * (sizeof(char)));
        strcat(*requestPath, requestedPath);
    }

    // skip extra header-lines
    while (getline(&buffer, &bufferSize, connectedClient) != EOF)
    {
        if (strncmp(buffer, "\r\n", strlen("\r\n")) == 0)
            break;
    }

    free(buffer);
    return statusCode;
}

/**
 * @brief writes the header in case of an error (statuscode != 200) to the client
 *
 * @param connectedClient pointer to the I/O from the client
 * @param statusCode status-code for the header
 */
static void writeErrorHeader(FILE *connectedClient, int statusCode)
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
    case 500:
        statusMessage = "Internal Server Error";
        break;
    case 501:
        statusMessage = "Not Implemented";
        break;
    }

    fprintf(connectedClient, "HTTP/1.1 %d (%s)\r\nConnection: close\r\n\r\n", statusCode, statusMessage);
    fflush(connectedClient);
}

/**
 * @brief Get the file-size of the requested file
 *
 * @param requestPath pointer to the requested file
 * @return the length of the file, or -1 if an error occured
 */
static off_t getFileSize(char *requestPath)
{
    struct stat st;

    if (stat(requestPath, &st) == 0)
        return st.st_size;

    return -1;
}

/**
 * @brief Get the current time in the gmt format
 *
 * @return returns the current time as a string in gmt format
 */
static char *getCurrentTimestamp()
{
    char *buffer = malloc(100 * sizeof(char));
    time_t rawtime = time(NULL);
    struct tm *time_info = gmtime(&rawtime);

    strftime(buffer, 100, "%a, %d %b %y %T %Z", time_info);
    return buffer;
}

/**
 * @brief Gets the content-type of the requested path
 *
 * @param requestPath requested resource pathname as string
 * @return content-type as string, if known, otherwise ""
 */
static char *getContentType(char *requestPath)
{
    char *extension = strrchr(requestPath, '.');
    char *contentType = malloc(50 * sizeof(char));
    strcpy(contentType, "");

    if (extension != NULL)
    {
        extension += 1;
        if (strncmp(extension, "html", strlen("html")) == 0 || strncmp(extension, "htm", strlen("htm")) == 0)
            strcpy(contentType, "Content-Type: text/html\r\n");
        else if (strncmp(extension, "css", strlen("css")) == 0)
            strcpy(contentType, "Content-Type: text/css\r\n");
        else if (strncmp(extension, "js", strlen("js")) == 0)
            strcpy(contentType, "Content-Type: application/javascript\r\n");
    }

    return contentType;
}

/**
 * @brief writes response header with statuscode 200
 * @details calculates time, size of file and contentType for
 * the response header and writes it to the client
 *
 * @param connectedClient pointer to the I/O from the client
 * @param requestPath pointer to the requested file for the content-length
 */
static void writeHeader(FILE *connectedClient, char *requestPath)
{
    off_t fileSize = getFileSize(requestPath);
    if (fileSize == -1)
    {
        writeErrorHeader(connectedClient, 500);
        return;
    }

    char *timeAsText = getCurrentTimestamp();        // find out the time
    char *contentType = getContentType(requestPath); // find out the content-type

    fprintf(connectedClient, "HTTP/1.1 200 OK\r\nDate: %s\r\nContent-Length: %lu\r\nConnection: close\r\n%s\r\n", timeAsText, fileSize, contentType);
    fflush(connectedClient);

    free(timeAsText);
    free(contentType);
}

/**
 * @brief reads the content from fileInput and writes it to the connected client
 * @details reads and writes in binary format
 *
 * @param connectFile pointer to the I/O from the client
 * @param fileInput pointer to the requested resource
 */
static void writeContent(FILE *connectedClient, FILE *fileInput)
{
    size_t read = 0;
    char buffer[BUF_SIZE];
    memset(&buffer, 0, sizeof(buffer));

    while ((read = fread(buffer, sizeof(char), BUF_SIZE, fileInput)) != 0)
        fwrite(buffer, sizeof(char), read, connectedClient);

    fflush(connectedClient);
}

/**
 * @brief The entry point of this programm, where all the different functions are called
 * @details The main-function calls the different functions, to read the arguments, to
 * start the server-socket and to listen to client connections
 *
 * @param argc number of arguments, provided from the user
 * @param argv values of arguments, provided from the user
 * @return Returns EXIT_SUCCESS upon success or EXIT_FAILURE upon failure.
 */
int main(int argc, char **argv)
{
    PROG_NAME = argv[0];

    // Setup the signal handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    char *port = "8080", *index = "index.html", *docRoot = NULL;

    // parse Arguments
    parseArguments(argc, argv, &port, &index, &docRoot);

    // open socket
    int sockfd = createServerSocket(port);

    while (!quit)
    {
        sa.sa_flags = 0;
        sigaction(SIGINT, &sa, NULL);
        sigaction(SIGTERM, &sa, NULL);

        // accept new client connections
        FILE *connectedClient = NULL;
        int connfd = acceptClient(sockfd, &connectedClient);
        if (connfd == 0)
        {
            continue;
        }
        else if (connfd == -1)
        {
            printError("openening connection failed");
            continue;
        }

        sa.sa_flags = SA_RESTART;
        sigaction(SIGINT, &sa, NULL);
        sigaction(SIGTERM, &sa, NULL);

        // read headers
        char *requestPath = strdup(docRoot);
        int statusCode = readClientHeaders(connectedClient, &requestPath);

        // Open the specified file which should be transmitted
        FILE *inputFile = NULL;
        if (statusCode == 200)
        {
            // add specified index-file if no file specified
            if ((requestPath)[strlen(requestPath) - 1] == '/')
            {
                requestPath = (char *)realloc(requestPath, (strlen(requestPath) + strlen(index) + 1) * (sizeof(char)));
                strcat(requestPath, index);
            }

            inputFile = fopen(requestPath, "r");

            if (inputFile == NULL)
                statusCode = 404;
        }

        // Write the normal header and content if the status code is 200. Otherwise write the error header and no content
        if (statusCode == 200)
        {
            writeHeader(connectedClient, requestPath);
            writeContent(connectedClient, inputFile);
        }
        else
        {
            writeErrorHeader(connectedClient, statusCode);
        }

        // Free resources
        if (inputFile != NULL)
            fclose(inputFile);

        // close connection
        fclose(connectedClient);
        close(connfd);

        free(requestPath);
    }

    // free resources
    close(sockfd);

    exit(EXIT_SUCCESS);
}
