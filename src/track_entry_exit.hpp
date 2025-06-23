#pragma once

#include <print>
#include <string_view>
#include <source_location>

namespace st {

class TrackEntryExit {
public:
    constexpr TrackEntryExit(const std::source_location loc = std::source_location::current())
        : name_(loc.function_name()) {
        std::println("Entering {}", name_);
    }
    ~TrackEntryExit() { std::println("Exiting {}", name_); }

private:
    std::string_view name_;
};

}  // namespace st