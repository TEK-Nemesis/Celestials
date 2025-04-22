#include "DataManager.hpp"
#include "Game.hpp"
#include <fstream>
#include <filesystem>
#include <SDL3/SDL.h>  // For SDL_GetBasePath
#include <chrono>
#include <iomanip>
#include <sstream>  // For std::stringstream
#include "Constants.hpp"
#include "Enums.hpp"
#include <json.hpp>
#include <iostream>

DataManager::DataManager() {
    // Ensure debug.log is opened for writing
    static std::ofstream debugLogFile("debug.log");
    if (!debugLogFile.is_open()) {
        // Write directly to error.log to avoid circular dependency during initialization
        std::ofstream errorLogFile("error.log", std::ios::app);
        if (errorLogFile.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto now_c = std::chrono::system_clock::to_time_t(now);
            std::tm localTime = {};
#ifdef _WIN32
            localtime_s(&localTime, &now_c);
#else
            localtime_r(&now_c, &localTime);
#endif
            std::stringstream timestamp;
            timestamp << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
            errorLogFile << timestamp.str() << " [ERROR] DataManager::DataManager - Failed to open debug.log for debug logging, proceeding without debug log" << std::endl;
            errorLogFile.close();
        }
    }
}

void DataManager::LogError(const std::string& className, const std::string& methodName, const std::string& message) {
    // Get current timestamp
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::tm localTime = {};
#ifdef _WIN32
    localtime_s(&localTime, &now_c);
#else
    localtime_r(&now_c, &localTime);
#endif
    std::stringstream timestamp;
    timestamp << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");

    // Log the error with timestamp, log level, class name, method name, and message
    std::ofstream errorLogFile("error.log", std::ios::app);
    if (errorLogFile.is_open()) {
        errorLogFile << timestamp.str() << " [ERROR] " << className << "::" << methodName << " - " << message << std::endl;
        errorLogFile.close();
    }
    // Also output to stderr for immediate visibility
    std::cerr << timestamp.str() << " [ERROR] " << className << "::" << methodName << " - " << message << std::endl;
}

void DataManager::LogDebug(DebugCategory category, const std::string& className, const std::string& methodName, const std::string& message) {
    // Check if the debug category is enabled
    bool shouldLog = false;
    switch (category) {
        case DebugCategory::TRAJECTORY:
            shouldLog = DEBUG_LOG_TRAJECTORY;
            break;
        case DebugCategory::INPUT:
            shouldLog = DEBUG_LOG_INPUT;
            break;
        case DebugCategory::GAME_LOOP:
            shouldLog = DEBUG_LOG_GAME_LOOP;
            break;
        case DebugCategory::AI_DECISION:
            shouldLog = DEBUG_LOG_AI_DECISION;
            break;
        case DebugCategory::RENDERING:
            shouldLog = DEBUG_LOG_RENDERING;
            break;
        case DebugCategory::BACKGROUND:
            shouldLog = DEBUG_LOG_BACKGROUND;
            break;
        case DebugCategory::SETTINGS:
            shouldLog = DEBUG_LOG_SETTINGS;
            break;
        case DebugCategory::GAME_STATE:
            shouldLog = DEBUG_LOG_GAME_STATE;
            break;
        default:
            return;  // Unknown category, do not log
    }

    if (!shouldLog) {
        return;  // Category is disabled, do not log
    }

    // Get current timestamp
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::tm localTime = {};
#ifdef _WIN32
    localtime_s(&localTime, &now_c);
#else
    localtime_r(&now_c, &localTime);
#endif
    std::stringstream timestamp;
    timestamp << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");

    // Log the debug message with timestamp, category, class name, method name, and message
    std::ofstream debugLogFile("debug.log", std::ios::app);
    if (debugLogFile.is_open()) {
        debugLogFile << timestamp.str() << " [DEBUG] [Category: " << static_cast<int>(category) << "] "
            << className << "::" << methodName << " - " << message << std::endl;
        debugLogFile.close();
    }
}

void DataManager::LogWarning(const std::string& className, const std::string& methodName, const std::string& message) {
    // Get current timestamp
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::tm localTime = {};
#ifdef _WIN32
    localtime_s(&localTime, &now_c);
#else
    localtime_r(&now_c, &localTime);
#endif
    std::stringstream timestamp;
    timestamp << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");

    // Log the warning with timestamp, log level, class name, method name, and message
    std::ofstream logFile("error.log", std::ios::app);
    if (logFile.is_open()) {
        logFile << timestamp.str() << " [WARNING] " << className << "::" << methodName << " - " << message << std::endl;
        logFile.close();
    }
    // Also output to stderr for immediate visibility
    std::cerr << timestamp.str() << " [WARNING] " << className << "::" << methodName << " - " << message << std::endl;
}