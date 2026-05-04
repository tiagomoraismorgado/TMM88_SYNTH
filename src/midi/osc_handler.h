#ifndef OSC_HANDLER_H
#define OSC_HANDLER_H

#include <string>
#include <vector>
#include <functional>
#include <map>

class OscHandler {
public:
    using OscCallback = std::function<void(float)>;

    OscHandler() = default;

    // Register a callback for a specific OSC address (e.g., "/filter/cutoff")
    void registerAddress(const std::string& address, OscCallback callback);

    // Simulated dispatch for the purpose of the framework logic
    // In a real scenario, this would parse a UDP packet
    void dispatch(const std::string& address, float value);

    // Helper to match patterns (basic string match for now)
    bool match(const std::string& pattern, const std::string& address) {
        return pattern == address;
    }

private:
    std::map<std::string, OscCallback> callbacks_;
};

#endif // OSC_HANDLER_H