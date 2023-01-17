#pragma once
#include "World.h"

class PressWorld : public World
{
public:
	PressWorld(const glm::ivec2& size);
	[[nodiscard]] glm::ivec2 GetSize() const override;

	void SetWater(const glm::ivec2& position, bool water) override;
	[[nodiscard]] std::vector<std::vector<float>> GetWaterPressures() const override;

	void SetBoundary(const glm::ivec2& position, bool boundary) override;
	[[nodiscard]] std::vector<std::vector<bool>> GetBoundaries() const override;

	void Update() override;

private:
	bool IsPositionInBounds(const glm::ivec2& position) const;
	float GetStableState(float totalPressure) const;

	std::vector<std::vector<float>> m_WaterCells;
	std::vector<std::vector<float>> m_NextWaterCells;
	std::vector<std::vector<bool>> m_Boundaries;
	glm::ivec2 m_Size;

	const float m_MaxPressure = 1.0f;
	const float m_MinPressure = 0.001f;
	const float m_MaxCompression = 0.25f;
	const float m_MinFlow = 0.01f;
	const float m_MaxFlow = 1.25f;
};