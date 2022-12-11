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
		float pressure;
	};

	World(const glm::ivec2& size);

	void SetWater(const glm::ivec2& position, const WaterCell& water);
	const std::vector<std::vector<WaterCell>>& GetWaterCells() const { return m_WaterCells; }

	void SetBoundary(const glm::ivec2& position, bool boundary);
	const std::vector<std::vector<bool>>& GetBoundaries() const { return m_Boundaries; }

	void Update();

private:
	std::vector<std::vector<WaterCell>> m_WaterCells;
	std::vector<std::vector<bool>> m_Boundaries;
	glm::ivec2 m_Size;

	const float m_Gravity = -0.1f;
	const float m_PressureVelocity = 0.5f;
	const float m_Drag = 0.1f;
	const float m_MaxPressure = 1.1f;
	const float m_IDK = 1.f;
};
