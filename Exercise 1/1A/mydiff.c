/**
 * @file mydiff.c
 * @author Maximilian Kleinegger (12041500)
 * @brief compares two files line by line
 * @date 2022-10-06
 * @details This programm compares two files line by line and prints
 * the amount of different characters for each line. This amount is
 * either printed to the console or written to the specified file.
 * If two lines have different length, then the comparison shall
 * stop upon reaching the end of the shorter line. Therefore, the
 * lines abc\n und abcdef\n shall be treated as being identical
 * If no differences were found, a corresponding message will be
 * outputted instead.
 * The programms takes two arguments for files and the following options:
 * [-i] this option makes the comparison of the letter case insensitive
 * [-o outputFile], with this option the user can specify the outputFile,
 * where the result should be written to.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <unistd.h>

#define NUMBER_OF_FILES 2

/**
 * @brief name of the programm
 *
 */
static char *progName; // string to save the programm_name

/**
 * @brief This function parses arguments
 * @details The function parses the options and arguments and saves them in the provided pointers. If the user
 * provides to little or to many arguments or wrong arguments the programm terminates with return-value EXIT_FAILURE
 * and prints the usage-Message
 *
 * @param argumentCount Number of arguments provided
 * @param arguments Values of arguments provided
 * @param fileNameInput1 pointer to the string containing the filename/path for the first inputfile
 * @param fileNameInput2 pointer to the string containing the filename/path for the second inputfile
 * @param fileNameOutput pointer to the string containing the filename/path for the outputfile, if not specified it will be NULL
 * @param isCaseInsensitive pointer to a bool value which decides if the option case insensitive is used or not
 */
static void parseArguments(int argumentCount, char **arguments, char **fileNameInput1, char **fileNameInput2, char **fileNameOutput, bool *isCaseInsensitive);

/**
 * @brief Opens the specified files
 * @details Tries to open the two input-files and prints an error and exits with code EXIT_FAILURE if this
 * fails. In addition it opens or creates the output-file, if specified (also prints an error and exits with
 * code EXIT_FAILURE if this fails) Otherwise it sets the output to stdout.
 *
 * @param fileNameInput1 Pointer to the filename/path for the first input-file
 * @param fileNameInput2 Pointer to the filename/path for the second input-file
 * @param fileNameOutput Pointer to the filename/path for the output-file
 * @param fileInput1 Pointer to a file, which is used to store the content of the first input-file.
 * @param fileInput2 Pointer to a file, which is used to store the content of the second input-file.
 * @param fileOutput Pointer to a file, which is used to store the content of the output
 */
static void openFiles(char *fileNameInput1, char *fileNameInput2, char *fileNameOutput, FILE **fileInput1, FILE **fileInput2, FILE **fileOutput);

/**
 * @brief This functions compares the specified files line by line and either prints it to stdout
 *  or writes it to the specified file.
 * @details This function reads character for character in each file until both files reach a
 * linebreak-symbol. If this happens a counter which counts the differences per line will be
 * evaluated. If one file reaches EOF the evaluation will be terminated and existing differences
 * will be printed. If the files are the same a specified messages will be printed at the end.
 *
 * @param fileInput1 Pointer to the input stream of the first input-file
 * @param fileInput2 Pointer to the input stream of the second input-file
 * @param fileOutput Pointer to the stream specified for the output.
 * @param isCaseInsensitive Flag if the comparison should be case insensitive.
 */
static void compareFiles(FILE *fileInput1, FILE *fileInput2, FILE *fileOutput, bool isCaseInsensitive);

/**
 * @brief This functions closes all files and reports possible errors back to the user without
 * terminating.
 *
 * @param fileInput1 Pointer to the input stream of the first input-file
 * @param fileInput2 Pointer to the input stream of the second input-file
 * @param fileOutput Pointer to the stream specified for the output.
 */
static void closeFiles(FILE *fileInput1, FILE *fileInput2, FILE *fileOutput);

/**
 * @brief The entry point of this programm, where all the different functions are called
 * @details In the main-function the different functions, to read the arguments, to read the files
 * to compare the files and to close those are called.
 *
 * @param argc number of arguments, provided from the user
 * @param argv values of arguments, provided from the user
 * @return Returns EXIT_SUCCESS upon success or EXIT_FAILURE upon failure.
 */

int main(int argc, char *argv[])
{
    bool isCaseInsensitive = false;
    char *fileNameOutput = NULL, *fileNameInput1 = NULL, *fileNameInput2 = NULL;

    // parseArguments
    progName = argv[0];
    parseArguments(argc, argv, &fileNameInput1, &fileNameInput2, &fileNameOutput, &isCaseInsensitive);

    // openFiles
    FILE *fileInput1 = NULL, *fileInput2 = NULL, *fileOutput = NULL;
    openFiles(fileNameInput1, fileNameInput2, fileNameOutput, &fileInput1, &fileInput2, &fileOutput);

    // compareFiles
    compareFiles(fileInput1, fileInput2, fileOutput, isCaseInsensitive);

    // close Files
    closeFiles(fileInput1, fileInput2, fileOutput);

    return EXIT_SUCCESS;
}

/**
 * @brief This functions prints the usage-Message to stderr und exits the programm with code EXIT_FAILURE.
 *
 */
static void printUsageInfoAndExit(void)
{
    fprintf(stderr, "Usage: %s [-i] [-o outfile] file1 file2\n", progName);
    exit(EXIT_FAILURE);
}

/**
 * @brief This functions prints the errorMessage to stderr and exits the programm with code EXIT_FAILURE.
 *
 * @param errorMessage The custom errorMessage which will be printed as the error to stderr
 */
static void printErrorAndExit(char *errorMessage)
{
    fprintf(stderr, "[%s] ERROR: %s: %s\n", progName, errorMessage, strerror(errno));
    exit(EXIT_FAILURE);
}

static void parseArguments(int argumentCount, char **arguments, char **fileNameInput1, char **fileNameInput2, char **fileNameOutput, bool *isCaseInsensitive)
{
    // parse Arguments
    int opt;
    while ((opt = getopt(argumentCount, arguments, "io:")) != -1)
    {
        switch (opt)
        {
        case 'i':
            *isCaseInsensitive = true;
            break;
        case 'o':
            *fileNameOutput = optarg;
            break;
        case '?':
            printUsageInfoAndExit();
            break;
        default:
            printUsageInfoAndExit();
        }
    }

    // check if the two filenames are specified
    if (optind + NUMBER_OF_FILES != argumentCount)
        printUsageInfoAndExit();

    // save the filenames
    *fileNameInput1 = arguments[optind];
    *fileNameInput2 = arguments[optind + 1];
}

static void openFiles(char *fileNameInput1, char *fileNameInput2, char *fileNameOutput, FILE **fileInput1, FILE **fileInput2, FILE **fileOutput)
{
    if ((*fileInput1 = fopen(fileNameInput1, "r")) == NULL)
    {
        printErrorAndExit("Opening input-file 1 failed\n");
    }

    if ((*fileInput2 = fopen(fileNameInput2, "r")) == NULL)
    {
        printErrorAndExit("Opening input-file 2 failed\n");
    }

    if (fileNameOutput != NULL)
    {
        if ((*fileOutput = fopen(fileNameOutput, "w")) == NULL)
        {
            printErrorAndExit("Openening or creating output-file failed\n");
        }
    }
    else
    {
        *fileOutput = stdout;
    }
}

/**
 * @brief A functions which checks if a character is either linebreak or EOF-Symbol
 *
 * @param c The character to check
 * @return true, if the specifed character is a linebreak or EOF-Symbol
 * @return false, if the specified character is neither a linebreak or EOF-Symbol
 */
static bool isEOForLinebreak(char c)
{
    return c == EOF || c == '\n';
}

static void compareFiles(FILE *fileInput1, FILE *fileInput2, FILE *fileOutput, bool isCaseInsensitive)
{
    int (*cmpFunction)(const char *s1, const char *s2, size_t n) = isCaseInsensitive ? &strncasecmp : &strncmp;

    char tempCharFile1 = 0, tempCharFile2 = 0;
    int differencePerLine = 0, differentLines = 0, lineIdx = 1;
    bool endOfLineOrEOFFile1 = false, endOfLineOrEOFFile2 = false;

    while (tempCharFile1 != EOF && tempCharFile2 != EOF)
    {
        if (endOfLineOrEOFFile1 == false)
            tempCharFile1 = fgetc(fileInput1);

        if (endOfLineOrEOFFile2 == false)
            tempCharFile2 = fgetc(fileInput2);

        endOfLineOrEOFFile1 = isEOForLinebreak(tempCharFile1);
        endOfLineOrEOFFile2 = isEOForLinebreak(tempCharFile2);

        if (endOfLineOrEOFFile1 == false && endOfLineOrEOFFile2 == false)
        {
            if (cmpFunction(&tempCharFile1, &tempCharFile2, 1) != 0)
                differencePerLine++;
        }

        if (endOfLineOrEOFFile1 == true && endOfLineOrEOFFile2 == true)
        {
            if (differencePerLine > 0)
            {
                fprintf(fileOutput, "Line: %d, characters: %d\n", lineIdx, differencePerLine);
                differentLines++;
            }

            lineIdx++;
            differencePerLine = 0;
            endOfLineOrEOFFile1 = endOfLineOrEOFFile2 = false;
        }
    }

    if (differentLines == 0)
        fprintf(fileOutput, "No differences found!");
}

static void closeFiles(FILE *fileInput1, FILE *fileInput2, FILE *fileOutput)
{
    // no need to terminate because, if an error occures in fclose the stream gets closed
    // but we want to handle the code and report it back to the user.
    if (fclose(fileInput1) == EOF)
        fprintf(stderr, "[%s] ERROR: Closing input-file 1 failed: %s\n", progName, strerror(errno));

    if (fclose(fileInput2) == EOF)
        fprintf(stderr, "[%s] ERROR: Closing input-file 2 failed: %s\n", progName, strerror(errno));

    if (fclose(fileOutput) == EOF)
        fprintf(stderr, "[%s] ERROR: Closing specified output failed: %s\n", progName, strerror(errno));
}
