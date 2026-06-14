#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <chrono>

#include "SimulationEngine/SimulationConfig.hpp"

enum class JobStatus { Queued, Running, Completed, Failed, Cancelled };

struct SimulationJob {
    std::string      id;
    SimulationConfig config;

    // Written only by the job thread after construction; read under mutex by HTTP handlers
    JobStatus   status    = JobStatus::Queued;
    std::string error;
    std::vector<float> results;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point finishedAt;
    std::mutex  mutex;

    // Written atomically by the job thread; read without a lock for progress polling
    std::atomic<uint32_t> currentStep{0};

    // Set to true by the HTTP DELETE handler to request cancellation
    std::atomic<bool> cancelFlag{false};

    SimulationJob()  = default;
    ~SimulationJob() = default;

    // Non-copyable/movable due to mutex and atomics
    SimulationJob(const SimulationJob&)            = delete;
    SimulationJob& operator=(const SimulationJob&) = delete;
};
