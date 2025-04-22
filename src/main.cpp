/*
 * Celestials - A Multi-Platform Game Testbed
 *
 * Summary:
 * Celestials is a test project for experimenting with multi-platform game development
 * using C++17, SDL3, and OpenGL. It features a dynamic 3D environment with changeable
 * time of day, seasons, and alien scenes, including real satellites, sci-fi starships,
 * constellations, and procedurally generated terrains using Perlin noise. The project
 * was created with assistance from Grok, an AI developed by xAI, to test cross-platform
 * compatibility and build systems on Windows and Linux.
 *
 * GitHub Repository:
 * https://github.com/TEK-Nemesis/Celestials
 */

#include "Game.hpp"
#include <fstream>
#include "DataManager.hpp"
#include <iostream>

int main(int argc, char* argv[]) {

    // Redirect std::cerr to a log file at the start of the program
    std::ofstream logFile("error.log");

    if (!logFile.is_open()) {
        DataManager::LogError("main", "main", "Failed to open error.log for std::cerr redirection [Error: FileOpenError], proceeding with console output");
        std::cerr.rdbuf(logFile.rdbuf());
    }

    Game game;
    if (!game.initialize()) {
        return 1;
    }
    game.run();
    return 0;
}