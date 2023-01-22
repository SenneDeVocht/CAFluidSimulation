#include "PressVelWorld.h"
#include <SDL.h>
#include <iostream>
#include <algorithm>
#include <execution>

PressVelWorld::PressVelWorld(const glm::ivec2& size)
	: m_WaterCells(size.x, std::vector<WaterCell>(size.y, { {0, 0}, 0 }))
	, m_Boundaries(size.x, std::vector<bool>(size.y, false))
	, m_Size(size)
{}

glm::ivec2 PressVelWorld::GetSize() const
{
	return m_Size;
}

void PressVelWorld::SetWater(const glm::ivec2& position, bool water)
{
	// Check if in bounds
	if (!IsPositionInBounds(position))
		return;

	if (water)
	{
		// Remove boundaries
		m_Boundaries[position.x][position.y] = false;
		m_WaterCells[position.x][position.y].Pressure = 1;
	}
	else
	{
		m_WaterCells[position.x][position.y].Pressure = 0;
		m_WaterCells[position.x][position.y].Velocity = { 0, 0 };
	}

}
std::vector<std::vector<float>> PressVelWorld::GetWaterPressures() const
{
	std::vector<std::vector<float>> pressures(m_Size.x, std::vector<float>(m_Size.y, 0));
	for (int x = 0; x < m_Size.x; ++x)
	{
		for (int y = 0; y < m_Size.y; ++y)
		{
			pressures[x][y] = m_WaterCells[x][y].Pressure;
		}
	}
	return pressures;
}

void PressVelWorld::SetBoundary(const glm::ivec2& position, bool boundary)
{
	// Check if in bounds
	if (!IsPositionInBounds(position))
		return;

	// Set State
	m_Boundaries[position.x][position.y] = boundary;
}
std::vector<std::vector<bool>> PressVelWorld::GetBoundaries() const
{
	return m_Boundaries;
}

float randFloat()
{
	return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

void PressVelWorld::TransferPressure(float amount, const glm::ivec2& start, const glm::ivec2& destination)
{
	const glm::vec2 velocity = m_WaterCells[start.x][start.y].Velocity;
	TransferPressure(amount, velocity, start, destination);
}
void PressVelWorld::TransferPressure(float amount, const glm::vec2& velocity, const glm::ivec2& start, const glm::ivec2& destination)
{
	if (start == destination || amount == 0 ||
		m_Boundaries[start.x][start.y] || m_Boundaries[destination.x][destination.y])
	{
		return;
	}

	// Weighted average of velocities
	m_NextWaterCells[destination.x][destination.y].Velocity =
		(m_NextWaterCells[destination.x][destination.y].Velocity * m_NextWaterCells[destination.x][destination.y].Pressure
			+ velocity * amount)
		/ (m_NextWaterCells[destination.x][destination.y].Pressure + amount);

	m_NextWaterCells[start.x][start.y].Pressure -= amount;
	m_NextWaterCells[destination.x][destination.y].Pressure += amount;
}

float PressVelWorld::GetStableState(float totalPressure) const
{
	if (totalPressure <= 1)
	{
		return 1;
	}

	if (totalPressure < 2 * m_MaxPressure + m_MaxCompression)
	{
		return (powf(m_MaxPressure, 2) + totalPressure * m_MaxCompression) / (m_MaxPressure + m_MaxCompression);
	}

	return (totalPressure + m_MaxCompression) / 2;
}

bool PressVelWorld::IsPositionInBounds(const glm::ivec2& position) const
{
	return position.x >= 0 && position.x < m_Size.x&&
		position.y >= 0 && position.y < m_Size.y;
}

void PressVelWorld::Update()
{
	std::vector directions(m_Size.x, std::vector<glm::ivec2>(m_Size.y, { 0, 0 }));
	
	for (int x = 0; x < m_Size.x; ++x)
	{
		for (int y = 0; y < m_Size.y; ++y)
		{
			// Drag
			m_WaterCells[x][y].Velocity = m_WaterCells[x][y].Velocity * (1 - m_Drag);

			// Gravity
			m_WaterCells[x][y].Velocity.y += m_Gravity;

			// Wind
			//m_WaterCells[x][y].Velocity.x += 0.1f;

			// Pressure Diff
			if (m_Boundaries[x][y])
				continue;

			const float pressureAtPos = m_WaterCells[x][y].Pressure;

			if (IsPositionInBounds({ x, y + 1 }) && !m_Boundaries[x][y + 1])
				m_WaterCells[x][y].Velocity += glm::vec2{ 0, 1 } *(pressureAtPos - m_WaterCells[x][y + 1].Pressure) * m_FlowDueToPressure;

			if (IsPositionInBounds({ x, y - 1 }) && !m_Boundaries[x][y - 1])
				m_WaterCells[x][y].Velocity += glm::vec2{ 0, -1 } *(pressureAtPos - m_WaterCells[x][y - 1].Pressure) * m_FlowDueToPressure;

			if (IsPositionInBounds({ x + 1, y }) && !m_Boundaries[x + 1][y])
				m_WaterCells[x][y].Velocity += glm::vec2{ 1, 0 } *(pressureAtPos - m_WaterCells[x + 1][y].Pressure) * m_FlowDueToPressure;

			if (IsPositionInBounds({ x - 1, y }) && !m_Boundaries[x - 1][y])
				m_WaterCells[x][y].Velocity += glm::vec2{ -1, 0 } *(pressureAtPos - m_WaterCells[x - 1][y].Pressure) * m_FlowDueToPressure;

			// Wanted direction
			if (m_WaterCells[x][y].Pressure == 0 || m_WaterCells[x][y].Velocity == glm::vec2{ 0, 0 })
				continue;

			float xSize = abs(m_WaterCells[x][y].Velocity.x);
			const float ySize = abs(m_WaterCells[x][y].Velocity.y);
			const float total = xSize + ySize;
			xSize /= total;

			if (randFloat() <= xSize)
				directions[x][y].x = (randFloat() < xSize * m_VelocityMultiplier) * glm::sign(m_WaterCells[x][y].Velocity.x);
			else
				directions[x][y].y = (randFloat() < ySize * m_VelocityMultiplier) * glm::sign(m_WaterCells[x][y].Velocity.y);
		}
	}
	
	// Move cells to fill wanted direction
	m_NextWaterCells = m_WaterCells;

	for (int x = 0; x < m_Size.x; ++x)
	{
		for (int y = 0; y < m_Size.y; ++y)
		{
			if (m_WaterCells[x][y].Pressure < m_MinPressure)
				continue;

			const auto dir = directions[x][y];
			if (dir == glm::ivec2{ 0, 0 })
				continue;

			// Custom push-only flow
			float remainingPressure = m_WaterCells[x][y].Pressure;

			// Wanted direction
			if (IsPositionInBounds(glm::ivec2{ x, y } + dir) &&
				!m_Boundaries[x][y] && !m_Boundaries[x + dir.x][y + dir.y])
			{
				float flow;

				if (IsPositionInBounds(glm::ivec2{ x, y } + dir + glm::ivec2{0, 1}) && !m_Boundaries[x + dir.x][y + dir.y + 1])
				{
					flow = GetStableState(m_WaterCells[x + dir.x][y + dir.y].Pressure + m_WaterCells[x + dir.x][y + dir.y + 1].Pressure)
						- m_WaterCells[x + dir.x][y + dir.y].Pressure;
				}
				else
				{
					flow = 1 - m_WaterCells[x + dir.x][y + dir.y].Pressure;
				}
				flow = glm::clamp(flow, 0.f, std::min(m_MaxFlow, remainingPressure));

				TransferPressure(flow, { x, y }, { x + dir.x, y + dir.y });
				remainingPressure -= flow;

				if (remainingPressure <= 0)
					continue;
			}

			// Give velocity to wanteddir cell in proportion to remaining
			if (IsPositionInBounds(glm::ivec2{ x, y } + dir) &&
				!m_Boundaries[x][y] && !m_Boundaries[x + dir.x][y + dir.y])
			{
				m_WaterCells[x + dir.x][y + dir.y].Velocity += m_WaterCells[x][y].Velocity * remainingPressure
					/ m_WaterCells[x + dir.x][y + dir.y].Pressure;
			}

			// Left
			if (IsPositionInBounds(glm::ivec2{ x + dir.y, y - dir.x }) &&
				!m_Boundaries[x][y] && !m_Boundaries[x + dir.y][y - dir.x]) {
				//Equalize the amount of water in this block and it's neighbour
				float flow = (m_WaterCells[x][y].Pressure - m_WaterCells[x + dir.y][y - dir.x].Pressure) / 4;
				flow = glm::clamp(flow, 0.f, remainingPressure);

				glm::vec2 vel = glm::vec2{ dir.y, -dir.x } *0.5f * m_WaterCells[x][y].Velocity;
				TransferPressure(flow, vel, { x, y }, { x + dir.y, y - dir.x });
				remainingPressure -= flow;

				if (remainingPressure <= 0)
					continue;
			}

			// Right
			if (IsPositionInBounds(glm::ivec2{ x - dir.y, y + dir.x }) &&
				!m_Boundaries[x][y] && !m_Boundaries[x - dir.y][y + dir.x]) {
				//Equalize the amount of water in this block and it's neighbour
				float flow = (m_WaterCells[x][y].Pressure - m_WaterCells[x - dir.y][y + dir.x].Pressure) / 4;
				flow = glm::clamp(flow, 0.f, remainingPressure);

				glm::vec2 vel = glm::vec2{ -dir.y, dir.x } *0.5f * m_WaterCells[x][y].Velocity;
				TransferPressure(flow, vel, { x, y }, { x - dir.y, y + dir.x });
				remainingPressure -= flow;

				if (remainingPressure <= 0)
					continue;
			}

			// Up
			if (IsPositionInBounds(glm::ivec2{ x, y + 1 }) &&
				!m_Boundaries[x][y] && !m_Boundaries[x][y + 1]) {
				float flow = remainingPressure - GetStableState(remainingPressure + m_WaterCells[x][y + 1].Pressure);
				flow = glm::clamp(flow, 0.f, std::min(m_MaxFlow, remainingPressure));

				const auto vel = glm::vec2{ m_WaterCells[x][y].Velocity.x, 0.5f };
				TransferPressure(flow, vel, { x, y }, { x, y + 1 });
				remainingPressure -= flow;
			}
		}
	}

	m_WaterCells = m_NextWaterCells;

	// Make everything valid
	for (int y = 0; y < m_Size.y; ++y)
	{
		for (int x = 0; x < m_Size.x; ++x)
		{
			if (m_Boundaries[x][y])
			{
				m_WaterCells[x][y].Velocity = { 0, 0 };
				m_WaterCells[x][y].Pressure = 0;
			}

			if (m_WaterCells[x][y].Pressure < m_MinPressure)
				m_WaterCells[x][y].Velocity = { 0, 0 };
		}
	}
}