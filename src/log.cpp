#include <iostream>
#include <memory>

#include "log.h"

namespace Tempo {
	DebugLogger::DebugLogger(const std::string& out, bool print_std) : m_out_file(out), m_print_std(print_std) {
		m_event_listener.callback = [=](Event_ptr& event) {
			if (m_print_std) {
				auto log = LOGEVENT_PTRCAST(event.get());
				std::cout << log->getMessage() << std::endl;
			}
		};
		m_event_listener.filter = "log/debug";

		EventQueue::getInstance().subscribe(&m_event_listener);
	}

	DebugLogger::~DebugLogger() {
		EventQueue::getInstance().unsubscribe(&m_event_listener);
	}

	void debug_event(const std::string&, const std::string& func, const std::string& str) {
		EventQueue::getInstance().post(Event_ptr(new LogEvent("debug", "[" + func + "] " + str)));
	}
}