#ifndef __LOGGER_H__
#define __LOGGER_H__

enum {
    INFO,
    WARN,
    ERROR,
    DEBUG
};

#ifndef LOG_LEVEL
    #define LOG_LEVEL DEBUG
#endif

#define LOG(level, fmt, ...) do { \
    if (level <= LOG_LEVEL) \
        fprintf(stderr, "[%s] %s:%d: " fmt "\n", \
                #level, __FILE__, __LINE__, ##__VA_ARGS__); \
} while(0);

#endif
