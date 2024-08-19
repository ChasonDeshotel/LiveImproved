#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <string>

class SharedMemory {
    public:
        SharedMemory();
        void create_shared_memory(const std::string &message);

        std::string read_shared_memory();

        void cleanup_shared_memory();
};

#endif
