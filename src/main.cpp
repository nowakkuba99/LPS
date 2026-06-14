/* C++ Headers */
#include <iostream>
#include <string>
#include <chrono>
#include <ctime>
#include <memory>
#include <mutex>

/* Metal includes and defines */
#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#include <Metal/Metal.hpp>
#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>

/* Third-party */
#include "httplib.h"
#include "json.hpp"

/* Header files */
#include "MyAppDelegate/MyAppDelegate.hpp"
#include "JobManager/JobManager.hpp"

using json = nlohmann::json;

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::string statusToString(JobStatus s) {
    switch (s) {
        case JobStatus::Queued:    return "queued";
        case JobStatus::Running:   return "running";
        case JobStatus::Completed: return "completed";
        case JobStatus::Failed:    return "failed";
        case JobStatus::Cancelled: return "cancelled";
    }
    return "unknown";
}

static std::string toISO8601(std::chrono::system_clock::time_point tp) {
    std::time_t t  = std::chrono::system_clock::to_time_t(tp);
    std::tm*    gm = std::gmtime(&t);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", gm);
    return buf;
}

static json jobToJson(const std::shared_ptr<SimulationJob>& job, bool includeResults) {
    JobStatus   status;
    std::string error;
    std::chrono::system_clock::time_point finishedAt;
    std::vector<float> resultsCopy;

    {
        std::lock_guard<std::mutex> lk(job->mutex);
        status     = job->status;
        error      = job->error;
        finishedAt = job->finishedAt;
        if (includeResults && status == JobStatus::Completed)
            resultsCopy = job->results;
    }

    uint32_t currentStep = job->currentStep.load(std::memory_order_relaxed);
    uint32_t totalSteps  = job->config.totalSteps;

    json j;
    j["id"]           = job->id;
    j["status"]       = statusToString(status);
    j["current_step"] = currentStep;
    j["total_steps"]  = totalSteps;
    j["progress"]     = totalSteps > 0 ? static_cast<double>(currentStep) / totalSteps : 0.0;
    j["config"]       = {
        {"excitation_type", job->config.excitationType},
        {"total_steps",     totalSteps}
    };
    j["created_at"]   = toISO8601(job->createdAt);

    if (!error.empty())
        j["error"] = error;

    if (status == JobStatus::Completed ||
        status == JobStatus::Cancelled  ||
        status == JobStatus::Failed)
        j["finished_at"] = toISO8601(finishedAt);

    if (!resultsCopy.empty())
        j["results"] = resultsCopy;

    return j;
}

// ── Headless HTTP server ──────────────────────────────────────────────────────

static void runServer(MTL::Device* device) {
    JobManager     jobs(device);
    httplib::Server svr;

    // Allow the React dev server (or any frontend) on any origin
    svr.set_default_headers({
        {"Access-Control-Allow-Origin",  "*"},
        {"Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type"}
    });
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        res.status = 204;
    });

    // POST /api/simulations — create and start a simulation
    svr.Post("/api/simulations", [&](const httplib::Request& req, httplib::Response& res) {
        SimulationConfig cfg;
        try {
            auto body = json::parse(req.body);
            if (body.contains("excitation_type"))
                cfg.excitationType = body["excitation_type"].get<std::string>();
            if (body.contains("total_steps"))
                cfg.totalSteps = body["total_steps"].get<uint32_t>();
        } catch (...) {
            res.status = 400;
            res.set_content(R"({"error":"invalid JSON body"})", "application/json");
            return;
        }

        auto id  = jobs.submit(cfg);
        auto job = jobs.getJob(id);
        res.status = 201;
        res.set_content(jobToJson(job, false).dump(), "application/json");
    });

    // GET /api/simulations — list all simulations
    svr.Get("/api/simulations", [&](const httplib::Request&, httplib::Response& res) {
        auto all  = jobs.listJobs();
        json arr  = json::array();
        for (auto& j : all)
            arr.push_back(jobToJson(j, false));
        res.set_content(arr.dump(), "application/json");
    });

    // GET /api/simulations/:id — status + results when completed
    svr.Get("/api/simulations/:id", [&](const httplib::Request& req, httplib::Response& res) {
        auto job = jobs.getJob(req.path_params.at("id"));
        if (!job) {
            res.status = 404;
            res.set_content(R"({"error":"not found"})", "application/json");
            return;
        }
        JobStatus st;
        { std::lock_guard<std::mutex> lk(job->mutex); st = job->status; }
        res.set_content(jobToJson(job, st == JobStatus::Completed).dump(), "application/json");
    });

    // DELETE /api/simulations/:id — cancel a running simulation
    svr.Delete("/api/simulations/:id", [&](const httplib::Request& req, httplib::Response& res) {
        if (jobs.cancel(req.path_params.at("id")))
            res.status = 204;
        else {
            res.status = 404;
            res.set_content(R"({"error":"not found"})", "application/json");
        }
    });

    std::cout << "LPS server listening on http://0.0.0.0:9000\n";
    svr.listen("0.0.0.0", 9000);
}

// ── Entry point ───────────────────────────────────────────────────────────────

int main(int argc, const char* argv[]) {
    bool gui = false;
    for (int i = 1; i < argc; ++i)
        if (std::string(argv[i]) == "--gui") gui = true;

    MTL::Device* device = MTL::CreateSystemDefaultDevice();
    if (!device) {
        std::cerr << "No Metal device found\n";
        return 1;
    }

    if (gui) {
        NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();
        MyAppDelegate app;
        NS::Application* pApp = NS::Application::sharedApplication();
        pApp->setDelegate(&app);
        pApp->run();
        pool->release();
    } else {
        runServer(device);
    }

    device->release();
    return 0;
}
