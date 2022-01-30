#include "events.h"

#include <exception>
#include <iostream>
#include <set>

namespace Tempo {
    /*
     * Exceptions related to the EventQueue class
     */

    class EventQueueException : public std::exception {
    private:
        const char* what_;

    public:
        explicit EventQueueException(const char* what) : what_(what) {}
        const char* what() const noexcept override {
            return what_;
        }
    };

    /*
     * Implementations of EventQueue
     */
    void EventQueue::subscribe(Listener* listener) {
        std::lock_guard<std::recursive_mutex> guard(listeners_mutex_);
        if (listeners_.find(listener) == listeners_.end())
            listeners_.insert(listener);
    }

    void EventQueue::unsubscribe(Listener* listener) {
        bool has_found = false;
        {
            std::lock_guard<std::recursive_mutex> guard(listeners_mutex_);
            for (auto it : listeners_) {
                if (it == listener) {
                    has_found = true;
                    break;
                }
            }
        }

        if (has_found) {
            std::lock_guard<std::mutex> guard(pending_mutex_);
            bool unsubscribe_later = false;
            for (auto& name : pending_acknowledged_events_) {
                if (isListener(listener->filter, name)) {
                    unsubscribe_later = true;
                    break;
                }
            }
            if (unsubscribe_later) {
                to_remove_.insert(listener);
            }
            else {
                listeners_.erase(listener);
            }
        }
    }

    void EventQueue::post(Event_ptr event) {
        {
            std::lock_guard<std::recursive_mutex> guard(event_mutex_);
            event_queue_.push(event);
        }
        if (event.get()->isAcknowledgable()) {
            std::lock_guard<std::mutex> guard(pending_mutex_);
            pending_acknowledged_events_.push_back(event.get()->getName());
        }
        // glfwPostEmptyEvent();
    }

    void EventQueue::pollEvents() {
        {
            std::lock_guard<std::recursive_mutex> event_guard(event_mutex_);
            while (!event_queue_.empty()) {
                std::shared_ptr<Event> event = event_queue_.front();

                std::lock_guard<std::recursive_mutex> listener_guard(listeners_mutex_);
                for (const auto& listener : listeners_) {
                    bool filter_ok = isListener(listener->filter, event->getName());

                    if (filter_ok) {
                        listener->callback(event);
                    }
                }
                event_queue_.pop();
            }
        }
        {
            std::lock_guard<std::recursive_mutex> guard(listeners_mutex_);
            // Check if there are listeners that need to be unsubscribed after a poll
            for (auto listener : to_remove_) {
                listeners_.erase((listener));
            }
        }
        {
            std::lock_guard<std::mutex> guard(pending_mutex_);
            pending_acknowledged_events_.clear();
            to_remove_.clear();
        }
    }

    size_t EventQueue::getNumSubscribers(const std::vector<std::string>& event_names) {
        std::set<Listener*> listener_set;
        std::lock_guard<std::recursive_mutex> listener_guard(listeners_mutex_);
        for (const auto& event_name : event_names) {
            for (const auto listener : listeners_) {
                if (isListener(listener->filter, event_name)) {
                    listener_set.insert(listener);
                }
            }
        }
        return listener_set.size();
    }

    bool EventQueue::isListener(const std::string& filter, const std::string& event_name) {
        bool filter_ok = true;

        size_t i = 0;
        for (; i < filter.size() && i < event_name.size(); i++) {
            if (filter[i] == '*') {
                break;
            }
            else if (event_name[i] != filter[i]) {
                filter_ok = false;
                break;
            }
        }
        if (i + 1 == filter.size()) {
            if (*(filter.end() - 1) != '*') {
                filter_ok = false;
            }
        }
        else if (i + 1 < filter.size()) {
            filter_ok = false;
        }
        return filter_ok;
    }
}