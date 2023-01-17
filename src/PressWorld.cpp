#include "PressWorld.h"

#include <algorithm>

PressWorld::PressWorld(const glm::ivec2& size)
	: m_WaterCells(size.x, std::vector<float>(size.y, 0))
	, m_NextWaterCells(size.x, std::vector<float>(size.y, 0))
	, m_Boundaries(size.x, std::vector<bool>(size.y, false))
	, m_Size(size)
{}

glm::ivec2 PressWorld::GetSize() const
{
	return m_Size;
}

void PressWorld::SetWater(const glm::ivec2& position, bool water)
{
	if (!IsPositionInBounds(position))
		return;

	if (water)
	{
		// Remove boundaries
		m_Boundaries[position.x][position.y] = false;
		m_WaterCells[position.x][position.y] = 1;
	}
	else
	{
		m_WaterCells[position.x][position.y] = 0;
	}
}
std::vector<std::vector<float>> PressWorld::GetWaterPressures() const
{
	return m_WaterCells;
}

void PressWorld::SetBoundary(const glm::ivec2& position, bool boundary)
{
	if (!IsPositionInBounds(position))
		return;
	
	m_Boundaries[position.x][position.y] = boundary;
	if (boundary)
		m_WaterCells[position.x][position.y] = 0;
}
std::vector<std::vector<bool>> PressWorld::GetBoundaries() const
{
	return m_Boundaries;
}

void PressWorld::Update()
{
    m_NextWaterCells = m_WaterCells;

    //Calculate and apply flow for each cell
    for (int x = 0; x < m_Size.x; x++)
    {
        for (int y = 0; y < m_Size.y; y++)
        {
            //Skip bounds
            if (m_Boundaries[x][y])
                continue;

            // Skip small amount of water
            if (m_WaterCells[x][y] < m_MinPressure)
                continue;

            //Custom push-only flow
            float remaining = m_WaterCells[x][y];
            if (remaining <= 0)
                continue;

            //The block below this one
            if (IsPositionInBounds({x, y - 1}) && !m_Boundaries[x][y - 1])
			{
                float flow = GetStableState(remaining + m_WaterCells[x][y - 1]) - m_WaterCells[x][y - 1];
                if (flow > m_MinFlow)
                {
                    flow *= 0.5f;
                }
                flow = std::clamp(flow, 0.f, std::min(m_MaxFlow, remaining));

                m_NextWaterCells[x][y] -= flow;
                m_NextWaterCells[x][y - 1] += flow;
                remaining -= flow;
            }

            if (remaining <= 0)
                continue;

            //Left
            if (IsPositionInBounds({ x - 1, y }) && !m_Boundaries[x - 1][y])
            {
                //Equalize the amount of water in this block and it's neighbour
                float flow = (m_WaterCells[x][y] - m_WaterCells[x - 1][y]) / 4;
                if (flow > m_MinFlow)
                {
	                flow *= 0.5f;
                }
                flow = std::clamp(flow, 0.f, remaining);

                m_NextWaterCells[x][y] -= flow;
                m_NextWaterCells[x - 1][y] += flow;
                remaining -= flow;
            }

            if (remaining <= 0)
                continue;

            //Right
            if (IsPositionInBounds({ x + 1, y }) && !m_Boundaries[x + 1][y])
            {
                //Equalize the amount of water in this block and it's neighbour
                float flow = (m_WaterCells[x][y] - m_WaterCells[x + 1][y]) / 4;
                if (flow > m_MinFlow)
                {
	                flow *= 0.5f;
                }
                flow = std::clamp(flow, 0.f, remaining);

                m_NextWaterCells[x][y] -= flow;
                m_NextWaterCells[x + 1][y] += flow;
                remaining -= flow;
            }

            if (remaining <= 0) continue;

            //Up. Only compressed water flows upwards.
            if (IsPositionInBounds({ x, y + 1 }) && !m_Boundaries[x][y + 1])
            {
                float flow = remaining - GetStableState(remaining + m_WaterCells[x][y + 1]);
                if (flow > m_MinFlow)
                {
	                flow *= 0.5f;
                }
                flow = std::clamp(flow, 0.f, std::min(m_MaxFlow, remaining));

                m_NextWaterCells[x][y] -= flow;
                m_NextWaterCells[x][y + 1] += flow;
                remaining -= flow;
            }
        }
    }
    
    m_WaterCells = m_NextWaterCells;
}

bool PressWorld::IsPositionInBounds(const glm::ivec2& position) const
{
	return position.x >= 0 && position.x < m_Size.x&&
		position.y >= 0 && position.y < m_Size.y;
}

float PressWorld::GetStableState(float totalPressure) const
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

