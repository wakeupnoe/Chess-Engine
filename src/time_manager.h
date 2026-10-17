#pragma once
#include <chrono>

struct TimeManager {
    bool time_control_enabled = false;
    std::chrono::time_point<std::chrono::steady_clock> start_time;
    int soft_limit = 0;
    int hard_limit = 0;

    inline bool hard_limit_reached() const {
        if(!time_control_enabled) return false;
        std::chrono::duration<double, std::milli> elapsed = std::chrono::steady_clock::now() - start_time;
        return elapsed.count() >= hard_limit;
    }

    inline bool soft_limit_reached() const {
        if(!time_control_enabled) return false;
        std::chrono::duration<double, std::milli> elapsed = std::chrono::steady_clock::now() - start_time;
        return elapsed.count() >= (soft_limit * 0.8);
    }

    inline double elapsed_seconds() const {
        std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start_time;
        return elapsed.count();
    }

    inline int elapsed_ms() const {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time
        );
        return elapsed.count();
    }
};

extern TimeManager TM;