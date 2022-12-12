#pragma once
#include <vector>
#include <glm/glm.hpp>

struct SDL_Renderer;
struct SDL_Window;

class World
{
public:
	struct WaterCell
	{
		glm::vec2 Velocity;
		float Pressure;
	};

	World(const glm::ivec2& size);

	void AddWater(const glm::ivec2& position, const WaterCell& water);
	const std::vector<std::vector<WaterCell>>& GetWaterCells() const { return m_WaterCells; }

	void SetBoundary(const glm::ivec2& position, bool boundary);
	const std::vector<std::vector<bool>>& GetBoundaries() const { return m_Boundaries; }

	void Update();

private:
	void TransferPressure(float amount, const glm::ivec2& start, const glm::ivec2& destination);
	void TransferPressure(float amount, const glm::vec2& velocity, const glm::ivec2& start, const glm::ivec2& destination);
	
	float GetStableState(float totalPressure) const;

	bool IsPositionInBounds(const glm::ivec2& position) const;

	std::vector<std::vector<WaterCell>> m_WaterCells;
	std::vector<std::vector<WaterCell>> m_NextWaterCells;
	std::vector<std::vector<bool>> m_Boundaries;
	glm::ivec2 m_Size;

	const float m_Gravity = -0.1f;
	const float m_Drag = 0.1f;
	const float m_VelocityMultiplier = 1.f;

	const float m_MaxPressure = 1.0f;
	const float m_MinPressure = 0.001f;
	const float m_MaxCompression = 0.02f;
	const float m_MaxFlow = 1.f;
};
