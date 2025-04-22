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