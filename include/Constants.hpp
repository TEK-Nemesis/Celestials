#pragma once

// For simplicity, these constants are provided but this file will be removed and everything will be moved into a proper class
// For example, any Window settings could be put in its own struct and belong to the Game class (e.g. GameSettings.WindowSize)
// Celestial constants would be put in a Celestial class, etc.
// This is just a temporary solution to get the game running quickly
// The main problem with using a Constants.hpp file is that these values have a purpose for a specific class, not all classes.


#ifndef M_PI  
#define M_PI 3.14159265358979323846  
#endif

const int WINDOW_WIDTH = 1728; // 90% of 1920
const int WINDOW_HEIGHT = 972; // 90% of 1080

const int INITIAL_PLAYER_COUNT = 2; // Initial number of players
const float GRAVITY = 9.81f; // Gravity constant
const float PIXELS_PER_METER = 100.0f; // 100 pixels = 1 meter in Box2D

// Debug Mode Flag
const bool DEBUG_MODE = true; // Enable debug mode for testing purposes
const bool DEBUG_LOAD_DEBUG_TESTER = false;  // immediately transfer to DebugTester after Splash Screen if set to true

// Debug Logging Categories
const bool DEBUG_LOG_TRAJECTORY = false;  // Trajectory path calculations
const bool DEBUG_LOG_INPUT = false;       // Keyboard/mouse input events
const bool DEBUG_LOG_GAME_LOOP = false;   // Game loop events
const bool DEBUG_LOG_AI_DECISION = false; // AI decision-making
const bool DEBUG_LOG_RENDERING = false;   // Rendering-related messages (disabled to reduce log spam)
const bool DEBUG_LOG_BACKGROUND = false;  // Background loading and selection
const bool DEBUG_LOG_SETTINGS = false;    // Settings loading/saving
const bool DEBUG_LOG_GAME_STATE = false;  // General game state changes
const bool DEBUG_LOG_DEBUG_TESTER = false; // DebugTester-specific messages (enabled for DebugTester debugging)

const float CELESTIAL_TEXT_SCALE_PLANETS = 0.65f; // Scale for planet text
const float CELESTIAL_TEXT_SCALE_CONSTELLATIONS = 0.65f; // Scale for constellations text
const float CELESTIAL_TEXT_SCALE_SATELLITES_AND_SPACESHIPS = 0.65f; // Scale for satellite abd spaceship text
