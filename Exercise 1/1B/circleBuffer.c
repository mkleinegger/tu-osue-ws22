/**
 * @file circleBuffer.c
 * @author Maximilian Kleinegger (12041500)
 * @date 2022-11-02
 * @brief This file implements the methods to open/close the circle-
 * buffer and to write/read from it.
 */
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include "circleBuffer.h"
#include "sharedMemory.h"

struct circleBuffer *openCircleBuffer(char role)
{
    // check if role is valid
    assert(role == 's' || role == 'c');

    struct circleBuffer *circleBuffer = malloc(sizeof(struct circleBuffer));
    if (circleBuffer == NULL)
    {
        return NULL;
    }

    // open sharedmemory
    circleBuffer->sharedMemory = openSharedMemory(&circleBuffer->shmfd, role);
    if (circleBuffer->sharedMemory == NULL)
    {
        free(circleBuffer);
        return NULL;
    }

    circleBuffer->semFreeMemory = NULL;
    circleBuffer->semUsedMemory = NULL;
    circleBuffer->semIsWriting = NULL;

    circleBuffer->semFreeMemory = (role == 's') ? sem_open(SEM_NAME_FREESPACE, O_CREAT | O_EXCL, 0600, BUFFER_LENGTH) : sem_open(SEM_NAME_FREESPACE, 0);
    if (circleBuffer->semFreeMemory == SEM_FAILED)
    {
        closeSharedMemory(circleBuffer->sharedMemory, &circleBuffer->shmfd, role);
        free(circleBuffer);

        return NULL;
    }

    circleBuffer->semUsedMemory = (role == 's') ? sem_open(SEM_NAME_USEDSPACE, O_CREAT | O_EXCL, 0600, 0) : sem_open(SEM_NAME_USEDSPACE, 0);
    if (circleBuffer->semUsedMemory == SEM_FAILED)
    {
        sem_close(circleBuffer->semFreeMemory);
        if (role == 's')
            sem_unlink(SEM_NAME_FREESPACE);
        closeSharedMemory(circleBuffer->sharedMemory, &circleBuffer->shmfd, role);
        free(circleBuffer);

        return NULL;
    }

    circleBuffer->semIsWriting = (role == 's') ? sem_open(SEM_NAME_WRITE, O_CREAT | O_EXCL, 0600, 1) : sem_open(SEM_NAME_WRITE, 0);
    if (circleBuffer->semIsWriting == SEM_FAILED)
    {
        sem_close(circleBuffer->semFreeMemory);
        sem_close(circleBuffer->semUsedMemory);
        if (role == 's')
        {
            sem_unlink(SEM_NAME_FREESPACE);
            sem_unlink(SEM_NAME_USEDSPACE);
        }
        closeSharedMemory(circleBuffer->sharedMemory, &circleBuffer->shmfd, role);

        free(circleBuffer);

        return NULL;
    }

    return circleBuffer;
}

int closeCircleBuffer(struct circleBuffer *circleBuffer, char role)
{
    // check if role is valid
    assert(role == 's' || role == 'c');

    int returnValue = 0;

    if (role == 's')
    {
        sem_post(circleBuffer->semFreeMemory);
        circleBuffer->sharedMemory->isAlive = false;
    }

    if (closeSharedMemory(circleBuffer->sharedMemory, &circleBuffer->shmfd, role) == -1)
        returnValue = -1;

    if (sem_close(circleBuffer->semFreeMemory) == -1)
        returnValue = -1;

    if (sem_close(circleBuffer->semUsedMemory) == -1)
        returnValue = -1;

    if (sem_close(circleBuffer->semIsWriting) == -1)
        returnValue = -1;

    if (role == 's')
    {
        if (sem_unlink(SEM_NAME_FREESPACE) == -1)
            returnValue = -1;
        if (sem_unlink(SEM_NAME_USEDSPACE) == -1)
            returnValue = -1;
        if (sem_unlink(SEM_NAME_WRITE) == -1)
            returnValue = -1;
    }

    free(circleBuffer);

    return returnValue;
}

char *readCircleBuffer(struct circleBuffer *circleBuffer)
{
    // allocate memory to read a possible new result from shared memory
    int i = 0, length = 32;
    char *result = malloc(sizeof(char) * length);
    if (result == NULL)
        return NULL;

    do
    {
        // check if there is something new to read
        if (sem_wait(circleBuffer->semUsedMemory) == -1)
        {
            free(result);
            return NULL;
        }

        // double the result string if there is not enough memory allocated
        if (i == length)
        {
            length *= 2;
            result = realloc(result, sizeof(char) * length);
        }

        // read from shared memory and share that there is new free space available (through semFreeSpace)
        result[i++] = circleBuffer->sharedMemory->buffer[circleBuffer->sharedMemory->readPos];
        sem_post(circleBuffer->semFreeMemory);
        circleBuffer->sharedMemory->readPos += 1;
        circleBuffer->sharedMemory->readPos %= BUFFER_LENGTH;
    } while (circleBuffer->sharedMemory->isAlive && result[i - 1] != '\0');

    return result;
}

void writeCircleBuffer(struct circleBuffer *circleBuffer, char *content)
{
    // check if a generator is already writing to the shared memory
    if (sem_wait(circleBuffer->semIsWriting) == -1)
        return;

    int i = 0;
    do
    {
        // check if there is free space to write to the shared memory
        if (sem_wait(circleBuffer->semFreeMemory) == -1)
        {
            sem_post(circleBuffer->semIsWriting);
            return;
        }

        // write to the shared memory and increase the used space in memory (through semUsedSpace)
        circleBuffer->sharedMemory->buffer[circleBuffer->sharedMemory->writePos] = content[i++];
        sem_post(circleBuffer->semUsedMemory);
        circleBuffer->sharedMemory->writePos += 1;
        circleBuffer->sharedMemory->writePos %= BUFFER_LENGTH;
    } while (circleBuffer->sharedMemory->isAlive && content[i - 1] != '\0');

    // make writing to the shared memory available for others
    sem_post(circleBuffer->semIsWriting);
}
