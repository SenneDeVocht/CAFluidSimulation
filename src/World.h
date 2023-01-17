#pragma once
#include <vector>
#include <glm/glm.hpp>

class World
{
public:
	World() = default;
	virtual ~World() = default;
	World(const World& other) = delete;
	World(World&& other) = delete;
	World& operator=(const World& other) = delete;
	World& operator=(World&& other) = delete;


	[[nodiscard]] virtual glm::ivec2 GetSize() const = 0;

	virtual void SetWater(const glm::ivec2& position, bool water) = 0;
	[[nodiscard]] virtual std::vector<std::vector<float>> GetWaterPressures() const = 0;

	virtual void SetBoundary(const glm::ivec2& position, bool boundary) = 0;
	[[nodiscard]] virtual std::vector<std::vector<bool>> GetBoundaries() const = 0;

	virtual void Update() = 0;
};
