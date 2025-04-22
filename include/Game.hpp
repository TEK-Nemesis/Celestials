#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <GL/glew.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include "World.hpp"
#include "Renderer.hpp"
#include "DataManager.hpp"
#include "InputManager.hpp" // Moved after class definition

// Forward declaration to break circular dependency
class InputManager;

class Game {
public:
    Game();
    ~Game();

    bool initialize();
    void run();
    void setRunning(bool value) { running = value; } // Setter for running flag

private:
    SDL_Window* window;
    SDL_GLContext context;
    bool running;

    std::unique_ptr<World> world;
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<InputManager> inputManager;
    std::unique_ptr<DataManager> dataManager;

    int numPlayers;

    void initializeImGui();
    void shutdownImGui();
};

