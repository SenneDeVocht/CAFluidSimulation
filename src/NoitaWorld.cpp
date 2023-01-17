#include "NoitaWorld.h"

NoitaWorld::NoitaWorld(const glm::ivec2& size)
	: m_Cells(size.x, std::vector<CellType>(size.y, CellType::Empty))
	, m_Dirs(size.x, std::vector<bool>(size.y, false))
	, m_Size(size)
{}

glm::ivec2 NoitaWorld::GetSize() const
{
	return m_Size;
}

void NoitaWorld::SetWater(const glm::ivec2& position, bool water)
{
	if (!IsPositionInBounds(position))
		return;

	if (water)
	{
		m_Cells[position.x][position.y] = CellType::Water;
		m_Dirs[position.x][position.y] = rand() % 2;
	}
	else
	{
		if (m_Cells[position.x][position.y] == CellType::Water)
		{
			m_Cells[position.x][position.y] = CellType::Empty;
		}
	}
}
std::vector<std::vector<float>> NoitaWorld::GetWaterPressures() const
{
	std::vector<std::vector<float>> pressures(m_Size.x, std::vector<float>(m_Size.y, 0));
	for (int x = 0; x < m_Size.x; ++x)
	{
		for (int y = 0; y < m_Size.y; ++y)
		{
			pressures[x][y] = m_Cells[x][y] == CellType::Water ? 1 : 0;
		}
	}
	return pressures;
}

void NoitaWorld::SetBoundary(const glm::ivec2& position, bool boundary)
{
	if (!IsPositionInBounds(position))
		return;

	if (boundary)
	{
		m_Cells[position.x][position.y] = CellType::Boundary;
	}
	else
	{
		if (m_Cells[position.x][position.y] == CellType::Boundary)
		{
			m_Cells[position.x][position.y] = CellType::Empty;
		}
	}
}
std::vector<std::vector<bool>> NoitaWorld::GetBoundaries() const
{
	std::vector<std::vector<bool>> pressures(m_Size.x, std::vector<bool>(m_Size.y, false));
	for (int x = 0; x < m_Size.x; ++x)
	{
		for (int y = 0; y < m_Size.y; ++y)
		{
			pressures[x][y] = m_Cells[x][y] == CellType::Boundary;
		}
	}
	return pressures;
}

void NoitaWorld::Update()
{
	m_UpdateDir = !m_UpdateDir;

	for (int y = 0; y < m_Size.y; ++y)
	{
		for(int x = m_UpdateDir ? 0 : m_Size.x - 1;
			m_UpdateDir ? x < m_Size.x : x >= 0;
			m_UpdateDir ? x++ : x--)
		{
			if (m_Cells[x][y] != CellType::Water)
				continue;

			if (IsPositionInBounds({x, y - 1}) &&
				m_Cells[x][y - 1] == CellType::Empty)
			{
				m_Cells[x][y] = CellType::Empty;
				m_Cells[x][y - 1] = CellType::Water;
				m_Dirs[x][y - 1] = m_Dirs[x][y];
				continue;
			}

			const int dir = m_Dirs[x][y] ? 1 : -1;
			if (IsPositionInBounds({ x + dir, y - 1 }) &&
				m_Cells[x + dir][y - 1] == CellType::Empty)
			{
				m_Cells[x][y] = CellType::Empty;
				m_Cells[x + dir][y - 1] = CellType::Water;
				m_Dirs[x + dir][y - 1] = m_Dirs[x][y];
				continue;
			}

			if (IsPositionInBounds({ x + dir, y }) &&
				m_Cells[x + dir][y] == CellType::Empty)
			{
				m_Cells[x][y] = CellType::Empty;
				m_Cells[x + dir][y] = CellType::Water;
				m_Dirs[x + dir][y] = m_Dirs[x][y];
				continue;
			}
			else
			{
				m_Dirs[x][y] = !m_Dirs[x][y];
			}
		}
	}
}

bool NoitaWorld::IsPositionInBounds(const glm::ivec2& position) const
{
	return position.x >= 0 && position.x < m_Size.x&&
		position.y >= 0 && position.y < m_Size.y;
}
