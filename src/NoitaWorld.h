#pragma once
#include "World.h"

class NoitaWorld : public World
{
public:
	NoitaWorld(const glm::ivec2& size);
	[[nodiscard]] glm::ivec2 GetSize() const override;

	void SetWater(const glm::ivec2& position, bool water) override;
	[[nodiscard]] std::vector<std::vector<float>> GetWaterPressures() const override;

	void SetBoundary(const glm::ivec2& position, bool boundary) override;
	[[nodiscard]] std::vector<std::vector<bool>> GetBoundaries() const override;

	void Update() override;

private:
	enum class CellType { Empty, Water, Boundary };
	std::vector<std::vector<CellType>> m_Cells;
	std::vector<std::vector<bool>> m_Dirs;
	glm::ivec2 m_Size;
	bool m_UpdateDir = false;

	bool IsPositionInBounds(const glm::ivec2& position) const;
};