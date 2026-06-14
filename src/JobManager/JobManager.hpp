#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <Metal/Metal.hpp>

#include "SimulationJob.hpp"

class JobManager {
public:
    explicit JobManager(MTL::Device* device);
    ~JobManager();

    std::string                                   submit(const SimulationConfig& config);
    std::shared_ptr<SimulationJob>                getJob(const std::string& id);
    std::vector<std::shared_ptr<SimulationJob>>   listJobs();
    bool                                          cancel(const std::string& id);

private:
    MTL::Device* _pDevice;   // non-owning
    std::map<std::string, std::shared_ptr<SimulationJob>> _jobs;
    std::mutex   _mutex;
    uint64_t     _nextId = 1;

    std::string generateId();
    void        runJob(std::shared_ptr<SimulationJob> job);
};
