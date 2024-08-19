#include "SharedMemory.h"
#include <iostream>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

const char *SHM_NAME = "/my_shared_memory";
const size_t SHM_SIZE = 1024;

void create_shared_memory(const std::string &message) {
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        std::cerr << "Failed to create shared memory." << std::endl;
        return;
    }

    // Set the size of the shared memory
    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        std::cerr << "Failed to set the size of shared memory." << std::endl;
        close(shm_fd);
        return;
    }

    // Map the shared memory into the address space of the process
    void *shm_ptr = mmap(0, SHM_SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        std::cerr << "Failed to map shared memory." << std::endl;
        close(shm_fd);
        return;
    }

    // Write the provided message to shared memory
    memcpy(shm_ptr, message.c_str(), message.size() + 1);

    std::cout << "Shared memory created and initialized with message: " << message << std::endl;

    // Unmap and close the shared memory (leave it for Python to access)
    munmap(shm_ptr, SHM_SIZE);
    close(shm_fd);
}

std::string read_shared_memory() {
    int shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (shm_fd == -1) {
        std::cerr << "Failed to open shared memory." << std::endl;
        return "";
    }

    // Map the shared memory into the address space of the process
    void *shm_ptr = mmap(0, SHM_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        std::cerr << "Failed to map shared memory." << std::endl;
        close(shm_fd);
        return "";
    }

    // Read the message from shared memory
    std::string message(static_cast<char*>(shm_ptr));

    // Unmap and close the shared memory
    munmap(shm_ptr, SHM_SIZE);
    close(shm_fd);

    return message;
}

void cleanup_shared_memory() {
    if (shm_unlink(SHM_NAME) == -1) {
        std::cerr << "Failed to unlink shared memory." << std::endl;
    } else {
        std::cout << "Shared memory unlinked successfully." << std::endl;
    }
}
