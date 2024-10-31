 class IPCPipeTestDecorator : public IPCPipe {
 public:
     IPCPipeTestDecorator(std::unique_ptr<IPCPipe> pipe) : pipe_(std::move(pipe)) {}

     void setFailureMode(FailureMode mode) { failureMode_ = mode; }

     std::string read() override {
         if (shouldFail(FailureType::Read)) {
             throw IPCException("Simulated read failure");
         }
         return pipe_->read();
     }

     void write(const std::string& message) override {
         if (shouldFail(FailureType::Write)) {
             throw IPCException("Simulated write failure");
         }
         pipe_->write(message);
     }

 private:
     std::unique_ptr<IPCPipe> pipe_;
     FailureMode failureMode_ = FailureMode::None;

     bool shouldFail(FailureType type) {
         // Implement logic to determine if this operation should fail
         // based on failureMode_ and type
     }
 };

 TEST_CASE("IPCQueue handles pipe read failures") {
     auto mockPipe = std::make_unique<MockIPCPipe>();
     auto testPipe = std::make_unique<IPCPipeTestDecorator>(std::move(mockPipe));
     testPipe->setFailureMode(FailureMode::ReadFail);

     IPCQueue queue(std::move(testPipe));

     REQUIRE_THROWS_AS(queue.receiveMessage(), IPCException);
     REQUIRE(queue.getState() == IPCQueue::State::Recovering);
 }
 3 Factory for Test Scenarios: You could create a factory that produces different configurations of the test decorator:


 class IPCPipeTestFactory {
 public:
     static std::unique_ptr<IPCPipe> createIntermittentFailurePipe() {
         auto pipe = std::make_unique<IPCPipeTestDecorator>(std::make_unique<RealIPCPipe>());
         pipe->setFailureMode(FailureMode::IntermittentFail);
         return pipe;
     }

     static std::unique_ptr<IPCPipe> createDelayedResponsePipe() {
         auto pipe = std::make_unique<IPCPipeTestDecorator>(std::make_unique<RealIPCPipe>());
         pipe->setFailureMode(FailureMode::DelayedResponse);
         return pipe;
     }

     // Other factory methods for different test scenarios...
 };


 4 Dependency Injection: Ensure your IPCQueue and other components accept IPCPipe interfaces, allowing you to inject these test decorators:


 class IPCQueue {
 public:
     IPCQueue(std::unique_ptr<IPCPipe> requestPipe, std::unique_ptr<IPCPipe> responsePipe)
         : requestPipe_(std::move(requestPipe)), responsePipe_(std::move(responsePipe)) {}

     // ... rest of the implementation
 };

