#pragma once

#include <mutex>
#include <string>
#include <utility>

#include "events.h"

namespace Tempo {
    /**
     * @brief Custom event class for sending some log info
     */
    class LogEvent : public Event {
    private:
        std::string m_message;

    public:
        explicit LogEvent(const std::string& name, std::string message) : Event(std::string("log/") + name), m_message(std::move(message)) {}

        std::string& getMessage() { return m_message; }
    };
#define LOGEVENT_PTRCAST(job) (reinterpret_cast<LogEvent*>((job)))

    class DebugLogger {
    private:
        std::vector<std::string> m_logs;
        std::string m_out_file;
        bool m_print_std;
        Listener m_event_listener;

    public:
        DebugLogger(const std::string& out, bool print_std = false);
        ~DebugLogger();
    };

    void debug_event(const std::string& file, const std::string& func, const std::string& str);
}
#ifdef LOG_DEBUG
#define APP_DEBUG(str) debug_event(__FILE__, __FUNCSIG__, (str))
#else
#define APP_DEBUG(str) ;
#endif