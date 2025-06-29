#pragma once

#include <csignal>
#include <memory>

namespace oryx {

class SignalHandler {
public:
    explicit SignalHandler(pid_t tid);
    void Wait();
    void RaiseTermination() const;
    int received_sig() const;
    static auto Instance() -> std::shared_ptr<SignalHandler>;

private:
    int received_sig_{};
    sigset_t sigset_;
    pid_t tid_;
};

}  // namespace oryx