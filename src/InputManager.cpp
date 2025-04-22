#include "InputManager.hpp"
#include "Game.hpp" // Include Game.hpp for full definition

InputManager::InputManager(Game* game, Renderer* renderer, World* world)
	: game(game), renderer(renderer), world(world) {
}

void InputManager::handleInput(SDL_Event event) {
	if (event.type == SDL_EVENT_QUIT) {
		game->setRunning(false);
	} else if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
		game->setRunning(false);
	} else if (event.type == SDL_EVENT_KEY_DOWN) {
		switch (event.key.key) {
			case SDLK_ESCAPE:
				game->setRunning(false);
				break;
			case SDLK_F1: // Toggle constellation names
				renderer->toggleShowConstellationNames();
				break;
			case SDLK_F2: // Toggle planet names
				renderer->toggleShowPlanetNames();
				break;
			case SDLK_F3: // Toggle satellite names
				renderer->toggleShowSatelliteNames();
				break;
			case SDLK_F4: // Dawn
				world->setTimeOfDay(TimeOfDay::DAWN);
				renderer->setCurrentTimeOfDayIndex(0);
				break;
			case SDLK_F5: // Mid Day
				world->setTimeOfDay(TimeOfDay::MID_DAY);
				renderer->setCurrentTimeOfDayIndex(1);
				break;
			case SDLK_F6: // Dusk
				world->setTimeOfDay(TimeOfDay::DUSK);
				renderer->setCurrentTimeOfDayIndex(2);
				break;
			case SDLK_F7: // Night
				world->setTimeOfDay(TimeOfDay::NIGHT);
				renderer->setCurrentTimeOfDayIndex(3);
				break;
			case SDLK_F8: // Fall
				renderer->setScene(Scene::FALL);
				break;
			case SDLK_F9: // Spring
				renderer->setScene(Scene::SPRING);
				break;
			case SDLK_F10: // Summer
				renderer->setScene(Scene::SUMMER);
				break;
			case SDLK_F11: // Winter
				renderer->setScene(Scene::WINTER);
				break;
			case SDLK_F12: // Toggle Klingon/English Names
				renderer->toggleUseKlingonNames();
				break;
			case SDLK_A: // Alien
				renderer->setScene(Scene::ALIEN);
				break;
			case SDLK_B: // Regenerate Bottom Terrain
				world->triggerRegeneration(TerrainGenerationMode::BOTTOM);
				break;
			case SDLK_D: // Regenerate Distant Terrain
				world->triggerRegeneration(TerrainGenerationMode::DISTANT);
				break;
		}
	}
}