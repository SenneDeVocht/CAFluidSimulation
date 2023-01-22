#pragma once
#include "World.h"

#include <thread>
#include <condition_variable>

class PressVelWorldThreaded : public World
{
public:
	struct WaterCell
	{
		glm::vec2 Velocity;
		float Pressure;
	};

	PressVelWorldThreaded(const glm::ivec2& size);
	~PressVelWorldThreaded();

	[[nodiscard]] glm::ivec2 GetSize() const override;

	void SetWater(const glm::ivec2& position, bool water) override;
	[[nodiscard]] std::vector<std::vector<float>> GetWaterPressures() const override;

	void SetBoundary(const glm::ivec2& position, bool boundary) override;
	[[nodiscard]] std::vector<std::vector<bool>> GetBoundaries() const override;

	void Update() override;

private:
	void TransferPressure(float amount, const glm::ivec2& start, const glm::ivec2& destination);
	void TransferPressure(float amount, const glm::vec2& velocity, const glm::ivec2& start, const glm::ivec2& destination);

	float GetStableState(float totalPressure) const;

	bool IsPositionInBounds(const glm::ivec2& position) const;

	static float RandFloat();

	std::vector<std::vector<WaterCell>> m_WaterCells;
	std::vector<std::vector<WaterCell>> m_NextWaterCells;
	std::vector<std::vector<bool>> m_Boundaries;
	std::vector<std::vector<glm::ivec2>> m_Directions;
	glm::ivec2 m_Size;

	// Threads
	int m_ThreadCount;
	std::vector<std::thread> m_Threads;
	std::vector<std::mutex> m_ThreadMutexes;
	std::vector<std::vector<std::mutex>> m_CellMutexes;
	std::vector<std::condition_variable> m_CVs;
	std::vector<std::atomic<bool>> m_UpdateVelocities;
	std::vector<std::atomic<bool>> m_UpdateFluids;
	std::atomic<bool> m_StopThreads = false;

	const float m_Gravity = -0.1f;
	const float m_Drag = 0.1f;
	const float m_VelocityMultiplier = 1.f;

	const float m_MaxPressure = 1.0f;
	const float m_MinPressure = 0.001f;
	const float m_MaxCompression = 0.25f;
	const float m_MaxFlow = 1.25f;
	const float m_FlowDueToPressure = 0.05f;
};