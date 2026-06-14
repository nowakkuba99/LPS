#ifndef FILE_OPERATIONS_HPP
#define FILE_OPERATIONS_HPP

#include <thread>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <memory>
#include <optional>
#include <queue>



class ThreadSafeQueue {
public:
    void push(const float value) {
        std::unique_lock<std::mutex> lk(_m);
        _q.push(value);
        _cv.notify_one();
    }

    std::optional<float> pop() {
        std::unique_lock<std::mutex> lk(_m);
        const auto pop = _cv.wait_for(lk, std::chrono::seconds(1),[this](){
            return !_q.empty();
        });
        if(pop) {
            auto item = _q.front();
            _q.pop();
            return item;
        } else {
            return {};
        }
    }
private:
    std::queue<float> _q;
    
    std::mutex _m;
    std::condition_variable _cv;
};


/*
    File Writer that uses thread safe queue to perform
    file writes in seprate thread. Now only supports single
    file and therad.
 */
class FileWriter {
public:
    FileWriter(const std::string fileName) {
        _pQueue = std::make_shared<ThreadSafeQueue>();
        _file.open(fileName, std::ios_base::out);
        
        _t = std::thread([this](){
            while(1) {
                auto item = _pQueue->pop();
                if(item.has_value()) {
                    _file<<item.value()<<',';
                } else {
                    _file.close();
                    std::cout<<"Finished writing to file!\n";
                    break;
                }
            }
        });
    }
    
    ~FileWriter() {
        _t.join();
        if(_file.is_open()) {
            _file.close();
        }
    }
    
    void writeFloat(const float value) {
        _pQueue->push(value);
    }
    
private:
    std::shared_ptr<ThreadSafeQueue>    _pQueue;
    std::ofstream                       _file;
    std::thread                         _t;
    
};

#endif //FILE_OPERATIONS
