#pragma once

#include <SDL3/SDL.h>
#include "Renderer.hpp"
#include "World.hpp"

// Forward declaration to break circular dependency
class Game;

class InputManager {
public:
    InputManager(Game* game, Renderer* renderer, World* world);
    void handleInput(SDL_Event event);

private:
    Game* game;
    Renderer* renderer;
    World* world;
};
