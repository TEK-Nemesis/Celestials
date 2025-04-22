#pragma once

#include <string>
#include <vector>
#include "Enums.hpp"

class Game;  // Forward declaration

class DataManager {
public:
    DataManager();

    // Static utility functions for logging
    static void LogError(const std::string& className, const std::string& methodName, const std::string& message);
    static void LogDebug(DebugCategory category, const std::string& className, const std::string& methodName, const std::string& message);
    static void LogWarning(const std::string& className, const std::string& methodName, const std::string& message); // New method
};
