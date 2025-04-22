#pragma once

// For simplicity, these Enum classes are provided but they will be moved into a proper class
// For example, TerrainGenerationMode should belong to the Terrain class, and so on.

enum class Scene {  
   SUMMER,  
   FALL,  
   WINTER,  
   SPRING,  
   ALIEN  
};

enum class TerrainGenerationMode {
    BOTTOM,
    DISTANT
};

enum class TimeOfDay {
    DAWN,
    MID_DAY,
    DUSK,
    NIGHT
};

enum class DebugCategory {
    TRAJECTORY,         // For logging trajectory path calculations
    INPUT,              // For keyboard/mouse input events
    GAME_LOOP,          // For game loop events (update, render, etc.)
    AI_DECISION,        // For AI decision-making processes
    RENDERING,          // For rendering-related debug info (e.g., background rendering)
    BACKGROUND,         // For background loading and selection
    SETTINGS,           // For settings loading/saving
    GAME_STATE,         // For general game state changes (e.g., state transitions)
/*  DEBUG_TESTER,       // For DebugTester-specific debug info */
    COUNT               // To keep track of the number of categories
};
