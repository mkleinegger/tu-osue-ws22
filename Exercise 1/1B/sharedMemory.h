/**
 * @file sharedMemory.h
 * @author Maximilian Kleinegger <e12041500@student.tuwien.ac.at>
 * @date 2022-11-02
 *
 * @brief Defines open/close for the shared memory.
 *
 * This module defines the structure for the shared memory and the
 * functions concerning the shared memory. The functionality includes
 * openening/closing the shared memory.
 */

#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H

#include <stdbool.h>

/**
 * @brief Length of the buffer in bytes
 */
#define BUFFER_LENGTH 2048

/**
 * @brief Name of the shared memory
 */
#define SHM_NAME "12041500_OSUE_SHAREDMEM"

/**
 * Data structure to implement the sharedMemory
 * @brief It contains a char[] buffer, a read-/write-position
 * and a flag, that indicates if the shared-memory is activ or not.
 * @details The buffer is a char array and not an array of pointers
 * to edges, because it not possible to write pointer into shared-
 * memory
 */
struct sharedMemory
{
    int writePos;
    int readPos;
    bool isAlive;
    char buffer[BUFFER_LENGTH];
};

/**
 * @brief This function creates/opens(connects to) the shared memory depending if its a server or client
 * @details This functions needs to be called first by a server process, so that the
 * shared memory segment gets created. After that all clients can connect to this
 * shared memory segment. If any error occurs or something doesn't work the functions returns
 * NULL and therefore the filedescriptor *shmfd should not be used. Also it shouldn't be
 * used to write directly to the shared memory, it is only saved for closeSharedMemory.
 * @param shmfd A pointer to a filedescriptor. This function will overwrite the value with a filedescriptor to the shared memory.
 * @param isServer Flag that indicates if the caller is a server (true) or client (false)
 * @return Returns a pointer to a struct sharedMemory, which is connected to the shared memory
 */
struct sharedMemory *openSharedMemory(int *shmfd, bool isServer);

/**
 * @brief This function closes/disconnects from the shared memory depending if its a server or client
 * @details After this call the *sharedMemory will be not valid and therefore it should not be used
 * to write or read data from it.
 * @param sharedMemory A pointer to the shared memory struct created by openSharedMemory
 * @param shmfd A pointer to a file descriptor created by openSharedMemory
 * @param isServer Flag that indicates if the caller is a server (true) or client (false)
 * @return Returns an int-value that indicates if an error happened (Error == -1)
 */
int closeSharedMemory(struct sharedMemory *sharedMemory, int *shmfd, bool isServer);

#endif
