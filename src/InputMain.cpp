#include <iostream>
#include <chrono>
#include <SDL.h>
#include <thread>
#undef main

#include "NoitaWorld.h"
#include "PressWorld.h"
#include "PressVelWorld.h"
#include "PressVelWorldThreaded.h"

// Size
constexpr int g_WorldWidth = 50;
constexpr int g_WorldHeight = 50;
constexpr int g_WindowWidth = 500;
constexpr int g_WindowHeight = 500;

// Rendering
SDL_Renderer* g_pRenderer = nullptr;

// Input
bool g_LeftMousePressed = false;
bool g_RightMousePressed = false;
int g_MouseX, g_MouseY;

bool HandleInput()
{
	SDL_Event e;
	while (SDL_PollEvent(&e))
	{
		switch (e.type)
		{
			case SDL_QUIT:
				return true;

			default:
				break;
		}
	}

	uint32_t buttons = SDL_GetMouseState(&g_MouseX, &g_MouseY);
	g_MouseY = g_WindowHeight - g_MouseY;
	g_LeftMousePressed = buttons & SDL_BUTTON_LMASK;
	g_RightMousePressed = buttons & SDL_BUTTON_RMASK;

	return false;
}

// Render
void RenderWorld(const World& world)
{
	const float cellWidth = static_cast<float>(g_WindowWidth) / static_cast<float>(world.GetSize().x);
	const float cellHeight = static_cast<float>(g_WindowHeight) / static_cast<float>(world.GetSize().y);

	const auto& pressures = world.GetWaterPressures();

	for (int x = 0; x < world.GetSize().x; ++x)
	{
		for (int y = 0; y < world.GetSize().y; ++y)
		{
			if (pressures[x][y] < 0.001f)
				continue;

			if (pressures[x][y] <= 1)
			{
				if (y + 1 < g_WorldHeight && pressures[x][y + 1] > 0.001f)
				{
					SDL_FRect rect = {
						x * cellWidth,
						g_WindowHeight - (y + 1) * cellHeight,
						cellWidth,
						cellHeight
					};

					SDL_SetRenderDrawColor(g_pRenderer, 255 / 2, 255 / 2, 255, 255);
					SDL_RenderFillRectF(g_pRenderer, &rect);
				}
				else
				{
					SDL_FRect rect{
						x * cellWidth,
						g_WindowHeight - (y + pressures[x][y]) * cellHeight,
						cellWidth,
						pressures[x][y] * cellHeight
					};

					SDL_SetRenderDrawColor(g_pRenderer, 255 / 2, 255 / 2, 255, 255);
					SDL_RenderFillRectF(g_pRenderer, &rect);
				}
			}
			else
			{
				SDL_FRect rect = {
					x * cellWidth,
					g_WindowHeight - (y + 1) * cellHeight,
					cellWidth,
					cellHeight
				};

				SDL_SetRenderDrawColor(g_pRenderer, 255 / (pressures[x][y] + 1), 255 / (pressures[x][y] + 1), 255, 255);
				SDL_RenderFillRectF(g_pRenderer, &rect);
			}
		}
	}

	const auto& boundaries = world.GetBoundaries();

	for (int x = 0; x < world.GetSize().x; ++x)
	{
		for (int y = 0; y < world.GetSize().y; ++y)
		{
			if (!boundaries[x][y])
				continue;

			SDL_FRect rect{
				x * cellWidth,
				g_WindowHeight - (y + 1) * cellHeight,
				cellWidth,
				cellHeight
			};

			SDL_SetRenderDrawColor(g_pRenderer, 0, 0, 0, 255);
			SDL_RenderFillRectF(g_pRenderer, &rect);
		}
	}
}

int main()
{
    // Init SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
        std::cout << "error initializing SDL:" << SDL_GetError() << std::endl;

    // Create window and renderer
    SDL_Window* pWindow = SDL_CreateWindow(
		"Cellular Automata", 
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
		g_WindowWidth, g_WindowHeight, 
		0);

	g_pRenderer = SDL_CreateRenderer(pWindow, -1, SDL_RENDERER_ACCELERATED);

	// Create world
	//NoitaWorld world({ g_WorldWidth, g_WorldHeight });
	//PressWorld world({ g_WorldWidth, g_WorldHeight });
	//PressVelWorld world({ g_WorldWidth, g_WorldHeight });
	PressVelWorldThreaded world({ g_WorldWidth, g_WorldHeight });

	constexpr float updateInterval = 0.01f;
	float timer = 0;

	// Loop
	while(!HandleInput())
	{
		const auto start = std::chrono::high_resolution_clock::now();

		glm::ivec2 wPos = { (float)g_MouseX / g_WindowWidth * g_WorldWidth, (float)g_MouseY / g_WindowHeight * g_WorldHeight };
		if (g_LeftMousePressed)
		{
			world.SetBoundary(wPos, true);
		}
		if (g_RightMousePressed)
		{
			world.SetWater(wPos, true);
		}

		while (timer > 0)
		{
			world.Update();
			timer -= updateInterval;
		}

		SDL_SetRenderDrawColor(g_pRenderer, 255, 255, 255, 255);
		SDL_RenderClear(g_pRenderer);

		RenderWorld(world);

		SDL_RenderPresent(g_pRenderer);

		const auto end = std::chrono::high_resolution_clock::now();
		timer += (end - start).count() / 1000000000.f;
	}

    return 0;
}