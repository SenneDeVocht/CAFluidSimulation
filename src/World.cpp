#include "World.h"
#include <SDL.h>
#include <array>
#include <iostream>

int prevSum = 0;

World::World(const glm::ivec2& size)
	: m_WaterCells(size.x, std::vector<WaterCell>(size.y, { {0, 0}, 0 }))
	, m_Boundaries(size.x, std::vector<bool>(size.y, false))
	, m_Size(size)
{}

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

float randFloat()
{
	return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

void World::TransferPressure(float amount, const glm::ivec2& start, const glm::ivec2& destination)
{
	if (start == destination || amount == 0 ||
		m_Boundaries[start.x][start.y] || m_Boundaries[destination.x][destination.y])
	{
		return;
	}

	const glm::vec2 velocity = (m_NextWaterCells[start.x][start.y].Velocity + glm::vec2(destination) - glm::vec2(start)) / 2.f;

	// Weighted average of velocities
	m_NextWaterCells[destination.x][destination.y].Velocity =
		(m_NextWaterCells[destination.x][destination.y].Velocity * m_NextWaterCells[destination.x][destination.y].pressure
		+ velocity * amount)
		/ (m_NextWaterCells[destination.x][destination.y].pressure + amount);

	m_NextWaterCells[start.x][start.y].pressure -= amount;
	m_NextWaterCells[destination.x][destination.y].pressure += amount;
}

float World::GetStableState(float totalPressure) const
{
	if (totalPressure <= 1)
		return 1;

	if (totalPressure < 2 * m_MaxPressure + m_MaxCompression)
		return (powf(m_MaxPressure, 2) + totalPressure * m_MaxCompression) / (m_MaxPressure + m_MaxCompression);

	return (totalPressure + m_MaxCompression) / 2;
}

bool World::IsPositionInBounds(const glm::ivec2& position) const
{
	return position.x >= 0 && position.x < m_Size.x &&
		position.y >= 0 && position.y < m_Size.y;
}

void World::Update()
{
	// Make everything valid
	for (int y = 0; y < m_Size.y; ++y)
	{
		for (int x = 0; x < m_Size.x; ++x)
		{
			if (m_Boundaries[x][y])
			{
				m_WaterCells[x][y].Velocity = { 0, 0 };
				m_WaterCells[x][y].pressure = 0;
			}

			if (m_WaterCells[x][y].pressure < 0)
				m_WaterCells[x][y].pressure = 0;
		}
	}

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
			if (m_WaterCells[x][y].pressure == 0 || m_WaterCells[x][y].Velocity == glm::vec2{0, 0})
				continue;

			float xSize = abs(m_WaterCells[x][y].Velocity.x);
			const float ySize = abs(m_WaterCells[x][y].Velocity.y);
			const float total = xSize + ySize;
			xSize /= total;

			if(randFloat() <= xSize)
				directions[x][y].x = (randFloat() < xSize * m_VelocityMultiplier) * glm::sign(m_WaterCells[x][y].Velocity.x);
			else
				directions[x][y].y = (randFloat() < ySize * m_VelocityMultiplier) * glm::sign(m_WaterCells[x][y].Velocity.y);
		}
	}

	// Move cells to fill wanted direction
	m_NextWaterCells = m_WaterCells;

	for (int y = 0; y < m_Size.y; ++y)
	{
		for (int x = 0; x < m_Size.x; ++x)
		{
			if (m_WaterCells[x][y].pressure <= m_MinPressure)
			{
				m_WaterCells[x][y].pressure = 0;
				continue;
			}

			const auto dir = directions[x][y];
			if (dir == glm::ivec2{ 0, 0 })
				continue;

			// Custom push-only flow
			float remainingPressure = m_WaterCells[x][y].pressure;

			// Try to flow in wanted direction
			if (IsPositionInBounds(glm::ivec2{ x, y } + dir) &&
				!m_Boundaries[x][y] && !m_Boundaries[x + dir.x][y + dir.y])
			{
				float flow = GetStableState(remainingPressure + m_WaterCells[x + dir.x][y + dir.y].pressure) - m_WaterCells[x + dir.x][y + dir.y].pressure;
				flow = glm::clamp(flow, 0.f, std::min(m_MaxFlow, remainingPressure));

				TransferPressure(flow, { x, y }, { x + dir.x, y + dir.y });
				remainingPressure -= flow;

				if (remainingPressure <= 0)
					continue;
			}

			// Left
			if (IsPositionInBounds(glm::ivec2{ x + dir.y, y - dir.x }) &&
				!m_Boundaries[x][y] && !m_Boundaries[x + dir.y][y - dir.x]) {
				//Equalize the amount of water in this block and it's neighbour
				float flow = (m_WaterCells[x][y].pressure - m_WaterCells[x + dir.y][y - dir.x].pressure) / 4;
				flow = glm::clamp(flow, 0.f, remainingPressure);

				TransferPressure(flow, { x, y }, { x + dir.y, y - dir.x });
				remainingPressure -= flow;

				if (remainingPressure <= 0)
					continue;
			}


			// Right
			if (IsPositionInBounds(glm::ivec2{ x - dir.y, y + dir.x }) &&
				!m_Boundaries[x][y] && !m_Boundaries[x - dir.y][y + dir.x]) {
				//Equalize the amount of water in this block and it's neighbour
				float flow = (m_WaterCells[x][y].pressure - m_WaterCells[x - dir.y][y + dir.x].pressure) / 4;
				flow = glm::clamp(flow, 0.f, remainingPressure);

				TransferPressure(flow, { x, y }, { x - dir.y, y + dir.x });
				remainingPressure -= flow;

				if (remainingPressure <= 0)
					continue;
			}

			//Up. Only compressed water flows upwards.
			if (IsPositionInBounds(glm::ivec2{ x, y + 1 }) && !m_Boundaries[x][y]) {
				float flow = remainingPressure - GetStableState(remainingPressure + m_WaterCells[x][y + 1].pressure);
				flow = glm::clamp(flow, 0.f, std::min(m_MaxFlow, remainingPressure));

				TransferPressure(flow, { x, y }, { x, y + 1 });
				remainingPressure -= flow;
			}
		}
	}

	m_WaterCells = m_NextWaterCells;
}