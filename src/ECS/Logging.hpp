// Logging.hpp

#ifndef ECS_LOGGING_HPP
#define ECS_LOGGING_HPP

#include <cstdlib>
#include <iostream>

#ifdef ECS_DEBUG
    #define ECS_LOG_ASSERT(condition) \
        do { \
            if (!(condition)) { \
                std::cerr << "[ECS_LOG_ASSERT] [" << __FILE__ << ":" << __LINE__ << "] " << #condition << std::endl; \
                std::abort(); \
            } \
        } while (0)

    #define ECS_LOG_INFO(str) \
        do { \
            std::cout << "[ECS_LOG_INFO] [" << __FILE__ << ":" << __LINE__ << "] "; \
            std::printf(str); \
        } while (0)

    #define ECS_LOG_INFOF(fmt, ...) \
        do { \
            std::cout << "[ECS_LOG_INFO] [" << __FILE__ << ":" << __LINE__ << "] "; \
            std::printf(fmt, ##__VA_ARGS__); \
        } while (0)
#else
    #define ECS_LOG_ASSERT(condition) ((void) 0)
    #define ECS_LOG_INFO(str) ((void) 0)
    #define ECS_LOG_INFOF(fmt, ...) ((void) 0)
#endif

#endif
