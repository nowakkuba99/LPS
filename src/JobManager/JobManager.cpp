#include "JobManager.hpp"
#include "SimulationEngine/SimulationEngine.hpp"

#include <thread>
#include <chrono>
#include <iostream>

JobManager::JobManager(MTL::Device* device) : _pDevice(device) {}

JobManager::~JobManager() {
    // Signal all running jobs to cancel; their threads will clean up themselves
    std::lock_guard<std::mutex> lk(_mutex);
    for (auto& [id, job] : _jobs)
        job->cancelFlag = true;
}

std::string JobManager::generateId() {
    auto ns = std::chrono::system_clock::now().time_since_epoch().count();
    return "sim_" + std::to_string(ns) + "_" + std::to_string(_nextId++);
}

std::string JobManager::submit(const SimulationConfig& config) {
    auto job       = std::make_shared<SimulationJob>();
    job->config    = config;
    job->createdAt = std::chrono::system_clock::now();
    job->results.reserve(config.totalSteps);

    {
        std::lock_guard<std::mutex> lk(_mutex);
        job->id    = generateId();
        _jobs[job->id] = job;
    }

    // Each job runs in a detached thread; the shared_ptr keeps it alive
    std::thread([this, job]{ runJob(job); }).detach();

    return job->id;
}

void JobManager::runJob(std::shared_ptr<SimulationJob> job) {
    {
        std::lock_guard<std::mutex> lk(job->mutex);
        job->status = JobStatus::Running;
    }

    try {
        SimulationEngine engine(_pDevice, job->config);

        bool completed = engine.run(
            [&](uint32_t step, float readout) {
                job->currentStep.store(step + 1, std::memory_order_relaxed);
                std::lock_guard<std::mutex> lk(job->mutex);
                job->results.push_back(readout);
            },
            job->cancelFlag
        );

        std::lock_guard<std::mutex> lk(job->mutex);
        job->status     = completed ? JobStatus::Completed : JobStatus::Cancelled;
        job->finishedAt = std::chrono::system_clock::now();

        std::cout << "[" << job->id << "] "
                  << (completed ? "completed" : "cancelled") << "\n";

    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> lk(job->mutex);
        job->status     = JobStatus::Failed;
        job->error      = e.what();
        job->finishedAt = std::chrono::system_clock::now();
        std::cerr << "[" << job->id << "] failed: " << e.what() << "\n";
    }
}

std::shared_ptr<SimulationJob> JobManager::getJob(const std::string& id) {
    std::lock_guard<std::mutex> lk(_mutex);
    auto it = _jobs.find(id);
    return it != _jobs.end() ? it->second : nullptr;
}

std::vector<std::shared_ptr<SimulationJob>> JobManager::listJobs() {
    std::lock_guard<std::mutex> lk(_mutex);
    std::vector<std::shared_ptr<SimulationJob>> out;
    out.reserve(_jobs.size());
    for (auto& [id, job] : _jobs)
        out.push_back(job);
    return out;
}

bool JobManager::cancel(const std::string& id) {
    std::lock_guard<std::mutex> lk(_mutex);
    auto it = _jobs.find(id);
    if (it == _jobs.end()) return false;
    it->second->cancelFlag = true;
    return true;
}
