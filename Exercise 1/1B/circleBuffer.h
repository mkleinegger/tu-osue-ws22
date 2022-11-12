/**
 * @file circleBuffer.h
 * @author Maximilian Kleinegger <e12041500@student.tuwien.ac.at>
 * @date 2022-11-02
 *
 * @brief This module defines open/close/read/write for the circle-buffer.
 *
 * This module defines the structure for the circle-buffer and the functions
 * concerning the circle-buffer. The functionality includes openening/closing the
 * circle-buffer and reading/writing from the circle-buffer. This functionality
 * in combination with the structure allows the circle-buffer to be implemented
 * as a queue with a fixed size buffer (shared memory).
 */

#ifndef CIRCLEBUFFER_H
#define CIRCLEBUFFER_H

#include <semaphore.h>
#include <stdbool.h>
#include "sharedMemory.h"

/**
 * @brief Name of semaphore which controls if there is enough free memory
 */
#define SEM_NAME_FREESPACE "12041500_OSUE_SEM_FREE"

/**
 * @brief Name of semaphore which controls how much memory is used
 */
#define SEM_NAME_USEDSPACE "12041500_OSUE_SEM_USED"

/**
 * @brief Name of semaphore which controls if a process is currently writing to the memory
 */
#define SEM_NAME_WRITE "12041500_OSUE_SEM_WRITER"

/**
 * @brief Datatype of the circle-buffer
 */
struct circleBuffer
{
    sem_t *semFreeMemory;
    sem_t *semUsedMemory;
    sem_t *semIsWriting;
    int shmfd;
    struct sharedMemory *sharedMemory;
};

/**
 * @brief This function opens the circle-buffer, with it's semaphores and shared memory.
 * It creates/opens the circle-buffer depending on the role, which is calling the function.
 * If the role is a server then the circle-buffer, with it's semaphores and shared memory,
 * should be created or NULL is returned. If the role is a client, then a circle-buffer which
 * connects to the already existing shared-memory and semaphores is returned, otherwise NULL.
 * @details To be working properly a server has to open the circle-buffer first, before any
 * client can connect to it.
 *
 * @param isServer Flag that indicates if the caller is a server (true) or client (false)
 * @return Pointer to a circle-buffer, if the creation or opening was successfull, otherwise
 * NULL
 */
struct circleBuffer *openCircleBuffer(bool isServer);

/**
 * @brief This function closes the circle-buffer with it's semaphores and sharedmemory. It
 * closes the circlebuffer, with it's semaphores and shared memory, depending on the role
 * in both cases (server and client) the sharedmemory and the semaphores are getting closed.
 * If the specified role is a server than additionally the semaphore will be unlinked from all clients.
 * @details The circleBuffer should not be used after returning from this call, because
 *
 * @param circleBuffer Pointer to the circle-buffer which should be closed
 * @param isServer Flag that indicates if the caller is a server (true) or client (false)
 * @return Returns an int-value that indicates if an error happened (Error == -1)
 */
int closeCircleBuffer(struct circleBuffer *circleBuffer, bool isServer);

/**
 * @brief This function reads a null-terminated string from the specified circlebuffer. It first
 * checks if there is any new content to read from and second it releases the used memory. This
 * is done via two semaphores which control those piecse of information
 * @details The caller should pass a pointer to a valid circleBuffer and only one server
 * should read from the solution, for correct usage.
 *
 * @param circleBuffer A Pointer to the circle-buffer which should be read from
 * @return A String containing the new result read from the shared memory
 */
char *readCircleBuffer(struct circleBuffer *circleBuffer);

/**
 * @brief This function writes to the specified circlebuffer. First it checks if another process
 * is already writing to it. In this case the functions terminates and just returns. Second, if
 * no process is already writing, it checks if there is memory free to write. If yes it writes
 * the content to the shared memory, otherwise it just terminates and returns. Those checks are
 * done via semaphores, which keep track about those pieces of information.
 * @details The caller should pass a pointer to a valid circleBuffer and content != NULL to the
 * function to work properly.
 *
 * @param circleBuffer A Pointer to the circle-buffer which should be written to
 * @param content The content as a char array (zero terminated) which should be written to the circle-buffer
 */
void writeCircleBuffer(struct circleBuffer *circleBuffer, char *content);

#endif
