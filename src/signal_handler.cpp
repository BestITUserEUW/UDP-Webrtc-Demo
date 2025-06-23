#include "signal_handler.hpp"

#include <print>
#include <cstring>
#include <mutex>

#include <unistd.h>
#include <sys/syscall.h>

using std::println;

namespace st {

SignalHandler::SignalHandler(pid_t tid)
    : tid_(tid) {
    std::memset(&sigset_, 0, sizeof(sigset_t));
    sigemptyset(&sigset_);
    sigaddset(&sigset_, SIGINT);
    sigaddset(&sigset_, SIGTERM);
    sigaddset(&sigset_, SIGHUP);
    sigprocmask(SIG_BLOCK, &sigset_, 0);
}

void SignalHandler::Wait() {
    println("Waiting for signals...");
    sigwait(&sigset_, &received_sig_);
    switch (received_sig_) {
        case SIGINT:
            println("Received: SIGINT");
            break;
        case SIGHUP:
            println("Received: SIGHUP");
            break;
        case SIGTERM:
            println("Received: SIGTERM");
            break;
        default:
            println("Received: {}", received_sig_);
            break;
    };
}

void SignalHandler::RaiseTermination() const {
    int rc = killpg(tid_, SIGTERM);
    if (rc != 0)
        println("Failed to raise termination with rc: {}", rc);
    else
        println("Raised termination");
}

int SignalHandler::received_sig() const { return received_sig_; }

std::shared_ptr<SignalHandler> SignalHandler::Instance() {
    static std::shared_ptr<SignalHandler> handler;
    static std::once_flag flag;

    std::call_once(flag, [&]() {
        pid_t tid = static_cast<pid_t>(syscall(SYS_gettid));
        handler = std::make_shared<SignalHandler>(tid);
    });
    return handler;
}

}  // namespace st