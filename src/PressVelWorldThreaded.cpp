#include "PressVelWorldThreaded.h"
#include <SDL.h>
#include <algorithm>
#include <execution>
#include <random>

PressVelWorldThreaded::PressVelWorldThreaded(const glm::ivec2& size)
	: m_WaterCells(size.x, std::vector<WaterCell>(size.y, { {0, 0}, 0 }))
	, m_Boundaries(size.x, std::vector<bool>(size.y, false))
	, m_Directions(size.x, std::vector<glm::ivec2>(size.y, { 0, 0 }))
	, m_Size(size)
	, m_ThreadCount(std::min((int)std::thread::hardware_concurrency(), size.x / 3))
{
	m_ThreadMutexes = std::vector<std::mutex>(m_ThreadCount);
	m_CVs = std::vector<std::condition_variable>(m_ThreadCount);
	m_UpdateVelocities = std::vector<std::atomic<bool>>(m_ThreadCount);
	m_UpdateFluids = std::vector<std::atomic<bool>>(m_ThreadCount);

	m_CellMutexes.reserve(size.x);
	for (size_t i = 0; i < size.x; i++)
	{
		m_CellMutexes.emplace_back(std::vector<std::mutex>(size.y));
	}

	for (size_t i = 0; i < m_ThreadCount; i++)
	{
		m_Threads.emplace_back(std::thread([&](int threadIdx)
		{
			const int xStart = threadIdx * (m_Size.x / m_ThreadCount);
			const int xEnd = (threadIdx != m_ThreadCount - 1) ? (threadIdx + 1) * (m_Size.x / m_ThreadCount) : m_Size.x;

			std::unique_lock lk(m_ThreadMutexes[threadIdx]);

			while (true)
			{
				m_CVs[threadIdx].wait(lk, [&]() { return m_UpdateVelocities[threadIdx].load() || m_StopThreads.load(); });

				if (m_StopThreads.load())
					break;

				// CALCULATE VELOCITIES
				for (int x = xStart; x < xEnd; ++x)
				{
					for (int y = 0; y < m_Size.y; ++y)
					{
						// Drag
						m_WaterCells[x][y].Velocity = m_WaterCells[x][y].Velocity * (1 - m_Drag);

						// Gravity
						m_WaterCells[x][y].Velocity.y += m_Gravity;

						// Wind
						//m_WaterCells[x][y].Velocity.x += 0.1f;

						// Pressure diff flow
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


						// Calculate wanted directions
						if (m_WaterCells[x][y].Pressure == 0 || m_WaterCells[x][y].Velocity == glm::vec2{ 0, 0 })
							continue;

						float xSize = abs(m_WaterCells[x][y].Velocity.x);
						const float ySize = abs(m_WaterCells[x][y].Velocity.y);
						const float total = xSize + ySize;
						xSize /= total;

						if (RandFloat() <= xSize)
							m_Directions[x][y].x = (RandFloat() < xSize * m_VelocityMultiplier) * glm::sign(m_WaterCells[x][y].Velocity.x);
						else
							m_Directions[x][y].y = (RandFloat() < ySize * m_VelocityMultiplier) * glm::sign(m_WaterCells[x][y].Velocity.y);
					}
				}

				m_UpdateVelocities[threadIdx].store(false);
				m_CVs[threadIdx].notify_all();

				// MOVE FLUID
				m_CVs[threadIdx].wait(lk, [&]() { return m_UpdateFluids[threadIdx].load(); });

				for (int x = xStart; x < xEnd; ++x)
				{
					for (int y = 0; y < m_Size.y; ++y)
					{
						if (m_WaterCells[x][y].Pressure < m_MinPressure)
							continue;

						const auto dir = m_Directions[x][y];
						if (dir == glm::ivec2{ 0, 0 })
							continue;

						// Custom push-only flow
						float remainingPressure = m_WaterCells[x][y].Pressure;

						// Wanted direction
						if (IsPositionInBounds(glm::ivec2{ x, y } + dir) &&
							!m_Boundaries[x][y] && !m_Boundaries[x + dir.x][y + dir.y])
						{
							float flow;

							if (IsPositionInBounds(glm::ivec2{ x, y } + dir + glm::ivec2{ 0, 1 }) && !m_Boundaries[x + dir.x][y + dir.y + 1])
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

				m_UpdateFluids[threadIdx].store(false);
				m_CVs[threadIdx].notify_all();
			}
		}, i));
	}
}

PressVelWorldThreaded::~PressVelWorldThreaded()
{
	m_StopThreads.store(true);
	for (int i = 0; i < m_ThreadCount; i++)
	{
		m_CVs[i].notify_all();
		m_Threads[i].join();
	}
}

glm::ivec2 PressVelWorldThreaded::GetSize() const
{
	return m_Size;
}

void PressVelWorldThreaded::SetWater(const glm::ivec2& position, bool water)
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
std::vector<std::vector<float>> PressVelWorldThreaded::GetWaterPressures() const
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

void PressVelWorldThreaded::SetBoundary(const glm::ivec2& position, bool boundary)
{
	// Check if in bounds
	if (!IsPositionInBounds(position))
		return;

	// Set State
	m_Boundaries[position.x][position.y] = boundary;
}
std::vector<std::vector<bool>> PressVelWorldThreaded::GetBoundaries() const
{
	return m_Boundaries;
}

void PressVelWorldThreaded::TransferPressure(float amount, const glm::ivec2& start, const glm::ivec2& destination)
{
	const glm::vec2 velocity = m_WaterCells[start.x][start.y].Velocity;
	TransferPressure(amount, velocity, start, destination);
}
void PressVelWorldThreaded::TransferPressure(float amount, const glm::vec2& velocity, const glm::ivec2& start, const glm::ivec2& destination)
{
	if (start == destination || amount == 0 ||
		m_Boundaries[start.x][start.y] || m_Boundaries[destination.x][destination.y])
	{
		return;
	}

	// Weighted average of velocities
	std::unique_lock lk1(m_CellMutexes[destination.x][destination.y]);
		m_NextWaterCells[destination.x][destination.y].Velocity =
			(m_NextWaterCells[destination.x][destination.y].Velocity * m_NextWaterCells[destination.x][destination.y].Pressure
				+ velocity * amount)
			/ (m_NextWaterCells[destination.x][destination.y].Pressure + amount);

		m_NextWaterCells[destination.x][destination.y].Pressure += amount;
	lk1.unlock();

	std::unique_lock lk2(m_CellMutexes[start.x][start.y]);
		m_NextWaterCells[start.x][start.y].Pressure -= amount;
	lk2.unlock();
}

float PressVelWorldThreaded::GetStableState(float totalPressure) const
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

bool PressVelWorldThreaded::IsPositionInBounds(const glm::ivec2& position) const
{
	return position.x >= 0 && position.x < m_Size.x&&
		position.y >= 0 && position.y < m_Size.y;
}

float PressVelWorldThreaded::RandFloat()
{
	static thread_local std::mt19937 generator;
	const std::uniform_real_distribution<float> distribution(0.f, 1.f);
	return distribution(generator);
}

void PressVelWorldThreaded::Update()
{
	// Update velocities
	for (int i = 0; i < m_ThreadCount; i++)
	{
		m_UpdateVelocities[i].store(true);
		m_CVs[i].notify_all();
	}

	for (size_t i = 0; i < m_ThreadCount; i++)
	{
		std::unique_lock lk(m_ThreadMutexes[i]);
		m_CVs[i].wait(lk, [&]() { return !m_UpdateVelocities[i].load(); });
	}

	// Move cells
	m_NextWaterCells = m_WaterCells;

	for (int i = 0; i < m_ThreadCount; i++)
	{
		m_UpdateFluids[i].store(true);
		m_CVs[i].notify_all();
	}

	for (size_t i = 0; i < m_ThreadCount; i++)
	{
		std::unique_lock lk(m_ThreadMutexes[i]);
		m_CVs[i].wait(lk, [&]() { return !m_UpdateFluids[i].load(); });
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
