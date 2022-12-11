#include "World.h"
#include <SDL.h>
#include <array>
#include <iostream>

int prevSum = 0;

World::World(const glm::ivec2& size)
	: m_WaterCells(size.x, std::vector<WaterCell>(size.y, { {0, 0}, 0 }))
	, m_Boundaries(size.x, std::vector<bool>(size.y, false))
	, m_Size(size)
{
	//m_Water.Moves = true;
	//m_Water.Density = 0.5;
	//m_Water.Color = { 0, 0, 255, 255 };
	//m_Water.BlockedVelocityCalculation = [&](const std::array<std::array<Cell, 3>, 3>& neighbourhood,
	//	const glm::vec2& currentVelocity, const glm::ivec2& wantedDir)
	//{
	//	int dir = glm::sign(currentVelocity.x);
	//	if (dir == 0)
	//	{
	//		dir = (rand() % 2) * 2 - 1;
	//	}
	//	float velX = 0;

	//	// spread out
	//	if (neighbourhood[dir + 1][1].Type == nullptr)
	//		velX = dir;
	//	else
	//		velX = -dir;

	//	return glm::vec2{ velX, 0 };
	//};
}

void World::SetWater(const glm::ivec2& position, const WaterCell& water)
{
	// Check if in bounds
	if (position.x < 0 || position.x >= m_Size.x ||
		position.y < 0 || position.y >= m_Size.y)
	{
		return;
	}

	// Set State
	m_WaterCells[position.x][position.y] = water;
}

float randFloat()
{
	return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

void World::SetBoundary(const glm::ivec2& position, bool boundary)
{
	// Check if in bounds
	if (position.x < 0 || position.x >= m_Size.x ||
		position.y < 0 || position.y >= m_Size.y)
	{
		return;
	}

	// Set State
	m_Boundaries[position.x][position.y] = boundary;
}

void World::Update()
{
	// Apply drag
	for (int y = 0; y < m_Size.y; ++y)
	{
		for (int x = 0; x < m_Size.x; ++x)
		{
			m_WaterCells[x][y].Velocity = m_WaterCells[x][y].Velocity * (1 - m_Drag);
		}
	}

	// Apply gravity
	for (int y = 0; y < m_Size.y; ++y)
	{
		for (int x = 0; x < m_Size.x; ++x)
		{
			m_WaterCells[x][y].Velocity.y += m_Gravity;
		}
	}

    // Calculate wanted directions
	std::vector<std::vector<glm::ivec2>> directions(m_Size.x, std::vector<glm::ivec2>(m_Size.y, { 0, 0 }));
	for (int y = 0; y < m_Size.y; ++y)
	{
		for (int x = 0; x < m_Size.x; ++x)
		{
			if (m_WaterCells[x][y].pressure == 0)
				continue;

			directions[x][y].x = (randFloat() < abs(m_WaterCells[x][y].Velocity.x)) * glm::sign(m_WaterCells[x][y].Velocity.x);
			directions[x][y].y = (randFloat() < abs(m_WaterCells[x][y].Velocity.y)) * glm::sign(m_WaterCells[x][y].Velocity.y);
		}
	}

	// Move cells to fill wanted direction
	for (int y = 0; y < m_Size.y; ++y)
	{
		for (int x = 0; x < m_Size.x; ++x)
		{
			if (m_WaterCells[x][y].pressure == 0)
				continue;

			const auto moveDir = directions[x][y];
			if (moveDir == glm::ivec2{ 0, 0 })
				continue;

			const bool outOfBounds = x + moveDir.x < 0 || x + moveDir.x >= m_Size.x ||
				y + moveDir.y < 0 || y + moveDir.y >= m_Size.y;

			const bool blocked = outOfBounds || m_Boundaries[x + moveDir.x][y + moveDir.y];

			// if not blocked, occupy this space
			if (!blocked)
			{
				float amountToMove = m_MaxPressure - m_WaterCells[x + moveDir.x][y + moveDir.y].pressure;
				if (amountToMove < 0)
					continue;

				if (amountToMove > m_WaterCells[x][y].pressure)
					amountToMove = m_WaterCells[x][y].pressure;

				// weighted average of velocities
				m_WaterCells[x + moveDir.x][y + moveDir.y].Velocity = 
					(amountToMove * m_WaterCells[x][y].Velocity + m_WaterCells[x + moveDir.x][y + moveDir.y].Velocity * m_WaterCells[x + moveDir.x][y + moveDir.y].pressure)
			        / (amountToMove + m_WaterCells[x + moveDir.x][y + moveDir.y].pressure);

				// move pressure
				m_WaterCells[x + moveDir.x][y + moveDir.y].pressure += amountToMove;
				m_WaterCells[x][y].pressure -= amountToMove;
			}
			else
			{
				m_WaterCells[x][y].Velocity *= -1;
			}
		}
	}

	// Apply pressure
	for (int y = 0; y < m_Size.y; ++y)
	{
		for (int x = 0; x < m_Size.x; ++x)
		{
			std::array<float, 9> weights{ 0, 0, 0, 0, 0, 0, 0, 0, 0 };

			for (int nx = -1; nx <= 1; ++nx)
			{
				for (int ny = -1; ny <= 1; ++ny)
				{
					const bool outOfBounds = x + nx < 0 || x + nx >= m_Size.x ||
						y + ny < 0 || y + ny >= m_Size.y;

					if (outOfBounds || m_Boundaries[x + nx][y + ny])
						continue;

					const float pressureDiff = m_WaterCells[x][y].pressure - m_WaterCells[x + nx][y + ny].pressure;
					if (pressureDiff < 0)
						continue;

					float weight = pressureDiff + glm::dot(m_WaterCells[x][y].Velocity, glm::vec2{ nx, ny }) * m_IDK;
					if (weight < 0)
						weight = 0;

					weights[nx + 1 + (ny + 1) * 3] = weight;
				}
			}

			// normalize weights
			float sum = 0;
			for (float w : weights)
			{
				sum += w;
			}
            if (sum == 0)
				continue;

			for (float& w : weights)
			{
				w /= sum;
			}

			// distribute excess pressure acording to weights
			float excess = m_WaterCells[x][y].pressure * m_PressureVelocity;

			for (int nx = -1; nx <= 1; ++nx)
			{
				for (int ny = -1; ny <= 1; ++ny)
				{
					const bool outOfBounds = x + nx < 0 || x + nx >= m_Size.x ||
						y + ny < 0 || y + ny >= m_Size.y;

					if (outOfBounds || m_Boundaries[x + nx][y + ny])
						continue;

					m_WaterCells[x + nx][y + ny].pressure += excess * weights[nx + 1 + (ny + 1) * 3];
				}
			}

			m_WaterCells[x][y].pressure -= excess;
		}
	}

	
}