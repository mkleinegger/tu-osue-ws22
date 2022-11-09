/**
 * @file sharedMemory.h
 * @author Maximilian Kleinegger (12041500)
 * @date 2022-11-02
 * @brief The header-file defines open/close for the shared memory.
 * @details This header-file defines the structure for the
 * shared memory and the functions concerning the shared memory.
 * The functionality includes openening/closing the shared memory.
 */

#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H

#include <stdbool.h>

/**
 * @brief Length of the buffer in bytes
 *
 */
#define BUFFER_LENGTH 2048

/**
 * @brief Name of the shared memory
 *
 */
#define SHM_NAME "12041500_OSUE_SHAREDMEM"

/**
 * @brief Data structure to implement the sharedMemory
 * @details It contains a char[] buffer, a read-/write-
 * position and a flag, that indicates if the shared-memory
 * is activ or not.
 *
 */
struct sharedMemory
{
    int writePos;
    int readPos;
    bool isAlive;
    char buffer[BUFFER_LENGTH];
};

/**
 * @brief This function creates/opens(connects to) the shared memory depending on the role
 * @details This functions needs to be called first by a server process ('s'), so that the
 * shared memory segment gets created. After that all clients ('c') can connect to this
 * shared memory segment. If any error occurs or something doesn't work the functions returns
 * NULL and therefore the filedescriptor @param shmfd should not be used. Also it shouldn't be
 * used to write directly to the shared memory, it is only saved for closeSharedMemory.
 * @param shmfd A pointer to a filedescriptor. This function will overwrite
 * the value with a filedescriptor to the shared memory.
 * @param role Flag that indicates if the caller is a server ('s') or a client ('c')
 * @return Returns a pointer to a struct sharedMemory, which is connected to the shared memory
 */
struct sharedMemory *openSharedMemory(int *shmfd, char role);

/**
 * @brief This function closes/disconnects from the shared memory depending on the role
 * @details This functions closes the filedescriptor, unmaps (closes) the shared memory
 * segment and unlinks all connected clients if its called by a server ('s'). If it
 * called by a client ('c'), this clients just disconnects from the shared memory.
 * @param sharedMemory A pointer to the shared memory struct created by openSharedMemory
 * @param shmfd A pointer to a file descriptor created by openSharedMemory
 * @param role Flag that indicates if the caller is a server ('s') or a client ('c')
 * @return Returns an int-value that indicates if an error happened (Error == -1)
 */
int closeSharedMemory(struct sharedMemory *sharedMemory, int *shmfd, char role);

#endif
