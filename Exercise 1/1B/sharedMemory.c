/**
 * @file sharedMemory.c
 * @author Maximilian Kleinegger <e12041500@student.tuwien.ac.at>
 * @date 2022-11-02
 *
 * @brief This is the implementation to open/close the shared-memory-module
 */
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include "sharedMemory.h"

struct sharedMemory *openSharedMemory(int *shmfd, bool isServer)
{
    int oFlag = (isServer == true) ? O_RDWR | O_CREAT | O_EXCL : O_RDWR;

    // create/open the shared memory object:
    *shmfd = shm_open(SHM_NAME, oFlag, 0600);
    if (*shmfd == -1)
        return NULL;

    if (isServer == true)
    {
        // set the size of the shared memory:
        if (ftruncate(*shmfd, sizeof(struct sharedMemory)) < 0)
        {
            close(*shmfd);
            return NULL;
        }
    }

    struct sharedMemory *sharedMemory;
    sharedMemory = mmap(NULL, sizeof(*sharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, *shmfd, 0);

    if (sharedMemory == MAP_FAILED)
    {
        close(*shmfd);
        return NULL;
    }

    // set all attributes if the server creates the shared memory
    if (isServer == true)
    {
        sharedMemory->isAlive = true;
        sharedMemory->readPos = 0;
        sharedMemory->writePos = 0;
        memset(sharedMemory->buffer, 0, BUFFER_LENGTH);
    }

    return sharedMemory;
}

int closeSharedMemory(struct sharedMemory *sharedMemory, int *shmfd, bool isServer)
{
    int returnValue = 0;

    if (close(*shmfd) == -1)
        returnValue = -1;

    // unmap shared memory:
    if (munmap(sharedMemory, sizeof(*sharedMemory)) == -1)
        returnValue = -1;

    if (isServer == true)
    {
        // unlink the shared memory for all processes
        if (shm_unlink(SHM_NAME) == -1)
            returnValue = -1;
    }

    return returnValue;
}
