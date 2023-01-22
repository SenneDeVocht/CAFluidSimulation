#include <iostream>
#include <chrono>
#include <SDL.h>
#include <thread>
#undef main

#include "NoitaWorld.h"
#include "PressWorld.h"
#include "PressVelWorld.h"
#include "PressVelWorldThreaded.h"

// Benchmark
constexpr int g_NumSteps = 4000;
constexpr int g_MaxSize = 250;

// Size
constexpr int g_WindowWidth = 500;
constexpr int g_WindowHeight = 500;

// Rendering
SDL_Renderer* g_pRenderer = nullptr;
constexpr bool g_Render = true;

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
				if (y + 1 < world.GetSize().y &&
					pressures[x][y+1] >= 0.001f)
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
    if (g_Render)
    {
	    SDL_Window* pWindow = SDL_CreateWindow(
			"Cellular Automata", 
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
			g_WindowWidth, g_WindowHeight, 
			0);

		g_pRenderer = SDL_CreateRenderer(pWindow, -1, SDL_RENDERER_ACCELERATED);
	}

	std::cout << "size,time" << std::endl;
	for (size_t size = 10; size <= g_MaxSize; size += 10)
	{
		// Create world
		PressVelWorldThreaded world({ size, size });

		// Full water
		/*for (int x = 0; x < size; x++)
		{
			for (int y = 0; y < size; y++)
			{
				world.SetWater({ x, y }, true);
			}
		}*/

		// Top water with middle hole
		/*for (int x = 0; x < size; x++)
		{
			for (int y = 0; y < size; y++)
			{
				if (y > size / 2)
				{
					world.SetWater({ x, y }, true);
				}
				else if (y == size / 2)
				{
					if (x < size / 2 - 2 || x > size / 2 + 2)
					{
						world.SetBoundary({ x, y }, true);
					}
				}
			}
		}*/

		// Left water with bottom hole
		for (int x = 0; x < size; x++)
		{
			for (int y = 0; y < size; y++)
			{
				if (x < size / 2)
				{
					world.SetWater({ x, y }, true);
				}
				else if (x == size / 2)
				{
					if (y > 3)
					{
						world.SetBoundary({ x, y }, true);
					}
				}
			}
		}

		// Loop
		long long updateTime = 0;
		for (int i = 0; i < g_NumSteps; i++)
		{
			auto updateStart = std::chrono::high_resolution_clock::now();
			world.Update();
			auto updateEnd = std::chrono::high_resolution_clock::now();
			updateTime += (updateEnd - updateStart).count();

			if (g_Render)
			{
				SDL_SetRenderDrawColor(g_pRenderer, 255, 255, 255, 255);
				SDL_RenderClear(g_pRenderer);

				RenderWorld(world);

				SDL_RenderPresent(g_pRenderer);
			}
		}

		std::cout << size << "," << updateTime << std::endl;
	}

    return 0;
}