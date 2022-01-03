#include <iostream>
#include <memory>

#include "log.h"

namespace Tempo {
	DebugLogger::DebugLogger(const std::string& out, bool print_std) : out_file_(out), print_std_(print_std) {
		event_listener_.callback = [=](Event_ptr& event) {
			if (print_std_) {
				auto log = LOGEVENT_PTRCAST(event.get());
				std::cout << log->getMessage() << std::endl;
			}
		};
		event_listener_.filter = "log/debug";

		EventQueue::getInstance().subscribe(&event_listener_);
	}

	DebugLogger::~DebugLogger() {
		EventQueue::getInstance().unsubscribe(&event_listener_);
	}

	void debug_event(const std::string& file, const std::string& func, const std::string& str) {
		file;
		EventQueue::getInstance().post(Event_ptr(new LogEvent("debug", "[" + func + "] " + str)));
	}
}