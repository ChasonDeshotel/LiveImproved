 #include "RequestIDGenerator.h"
 #include <atomic>

 class RequestIDGeneratorImpl {
 public:
     uint64_t getNextID() {
         return nextID_++;
     }

 private:
     std::atomic<uint64_t> nextID_{0};
 };

 RequestIDGenerator& RequestIDGenerator::getInstance() {
     static RequestIDGenerator instance;
     return instance;
 }

 uint64_t RequestIDGenerator::getNextID() {
     static RequestIDGeneratorImpl impl;
     return impl.getNextID();
 }
