#include "Game.hpp"
#include "DataManager.hpp"
#include <Constants.hpp>

Game::Game() : window(nullptr), context(nullptr), running(false), numPlayers(INITIAL_PLAYER_COUNT) {}

Game::~Game() {
    shutdownImGui();
    if (context) SDL_GL_DestroyContext(context);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

bool Game::initialize() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        DataManager::LogError("Game", "initialize", "SDL_Init failed: " + std::string(SDL_GetError()));
        return false;
    }

    if (!TTF_Init()) {
        DataManager::LogError("Game", "initialize", "TTF_Init failed: " + std::string(SDL_GetError()));
        return false;
    }

    window = SDL_CreateWindow("Celestials",
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);
    if (!window) {
        DataManager::LogError("Game", "initialize", "SDL_CreateWindow failed: " + std::string(SDL_GetError()));
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    context = SDL_GL_CreateContext(window);
    if (!context) {
        DataManager::LogError("Game", "initialize", "SDL_GL_CreateContext failed: " + std::string(SDL_GetError()));
        return false;
    }

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        DataManager::LogError("Game", "initialize", "GLEW initialization failed");
        return false;
    }

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    // Initialize systems
    dataManager = std::make_unique<DataManager>();
    world = std::make_unique<World>();
    renderer = std::make_unique<Renderer>();
    inputManager = std::make_unique<InputManager>(this, renderer.get(), world.get());

    if (!world->initialize()) {
        DataManager::LogError("Game", "initialize", "World initialization failed");
        return false;
    }

    if (!renderer->initialize(world.get())) {
        DataManager::LogError("Game", "initialize", "Renderer initialization failed");
        return false;
    }

    initializeImGui();

    running = true;
    return true;
}

void Game::run() {
    Uint64 lastTime = SDL_GetTicks();
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            inputManager->handleInput(event);
        }

        // Calculate delta time
        Uint64 currentTime = SDL_GetTicks();
        float dt = (currentTime - lastTime) / 1000.0f; // Convert to seconds
        lastTime = currentTime;

        // Cap the delta time to avoid large jumps
        if (dt > 0.1f) dt = 0.1f;

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // Update and render
        world->update(dt);
        renderer->render(dt);

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);

        // Cap the frame rate to ~60 FPS
        Uint64 frameTime = SDL_GetTicks() - currentTime;
        const Uint64 targetFrameTime = 1000 / 60; // 16.67 ms for 60 FPS
        if (frameTime < targetFrameTime) {
            SDL_Delay(static_cast<Uint32>(targetFrameTime - frameTime));
        }
    }
}

void Game::initializeImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable keyboard navigation

    if (!ImGui_ImplSDL3_InitForOpenGL(window, context)) {
        DataManager::LogError("Game", "initializeImGui", "Failed to initialize ImGui for SDL3 and OpenGL");
    }
    if (!ImGui_ImplOpenGL3_Init("#version 330")) {
        DataManager::LogError("Game", "initializeImGui", "Failed to initialize ImGui for OpenGL3");
    }
}

void Game::shutdownImGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}