#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace Tempo {
    /**
     * The Event class can be used as such for sending events,
     * but it is also possible to inherit from this class to
     * create custom events
     */
    class Event {
    protected:
        std::string name_;
        std::chrono::system_clock::time_point time_;
        bool acknowledgable_ = false;

    public:
        /**
         * Constructor of the Event
         * @param name the name of the event allows EventQueue to call the
         * listeners that are observing this particular name
         * @param acknowledgable this option is useful when one desires that the listeners
         * have to acknowledge to the poster of the event.
         * Setting this variable to true will indicate the EventQueue that every listener
         * listening this event is not allowed to unsubscribe before every event in the queue
         * has been polled, otherwise there could be concurrency problems.
         */
        explicit Event(std::string name, bool acknowledgable = false) : name_(std::move(name)), time_(std::chrono::system_clock::now()), acknowledgable_(acknowledgable) {}

        /**
         * @return returns the name of the event
         */
        const std::string& getName() const { return name_; }

        /**
         * @return returns true if the event is of type "acknowledgable"
         */
        bool isAcknowledgable() const { return acknowledgable_; }

        /**
         * @return returns the time at which the event was posted
         */
        const std::chrono::system_clock::time_point& getTime() const { return time_; }

        virtual ~Event() = default;
    };
    typedef std::shared_ptr<Event> Event_ptr;

    struct Listener {
        std::string filter;
        std::function<void(Event_ptr&)> callback;
    };

    /**
     * @brief The EventQueue is a thread-safe singleton that manages all events
     * (posting and polling events, alerting the listeners)\n
     *
     *
     * The events naming convention should follow a similar POSIX-folder structure
     * e.g.:
     *  jobs/job_1
     *  jobs/job_2
     *  jobs/error_with_scheduler
     *
     *  When observing events (aka listeners), one can filter as one would do on a bash console :
     *  `jobs*` will select everything that begins with jobs
     *
     *  The wildcard * can only be used at the end of strings
     *
     * Here is an example of code in a single threaded context:
     * @code{.cpp}
     * EventQueue& queue = EventQueue::getInstance();
     * // Create a listener that increments a variable each time an event is created
     * int num_listens = 0;
     * Listener listener1{
     *     .filter = "events*", // Using the wildcard * to listen to multiple events
     *     .callback = [&num_listens] (Event_ptr event) {
     *         num_listens++;
     *     },
     * };
     * queue.subscribe(&listener1);
     *
     * // Event_ptr is in fact a shared_ptr of Even
     * // This is necessary, because there can be multiple listeners
     * // that could consume the posted Event, so the shared pointer
     * // avoids accidentally delete an Event at the wrong moment
     * queue.post(Event_ptr(new Event("events/1")));
     * queue.post(Event_ptr(new Event("events/2")));
     * queue.post(Event_ptr(new Event("event/3"))); // Purposefully post an event with a typo
     * queue.post(Event_ptr(new Event("events/3")));
     *
     * queue.pollEvents();
     *
     * // num_listen should be incremented to 3
     *
     * queue.unsubscribe(&listener1);
     * queue.post(Event("events/4"));
     *
     * // num_listen should not have changed because the listener unsubscribed
     *
     * queue.pollEvents();
     * @endcode
     *
     * @note It is recommended to call the queue.pollEvents() from the main thread.
     * However, if one desires to call pollEvents() from another thread, then it is
     * up to the user to guarantee the thread safety of the lambdas created in the
     * listeners.
     */
    class EventQueue {
    private:
        std::queue<Event_ptr> event_queue_;
        std::recursive_mutex event_mutex_;
        std::set<Listener*> listeners_;
        std::recursive_mutex listeners_mutex_;

        std::set<Listener*> to_remove_;
        std::vector<std::string> pending_acknowledged_events_;
        std::mutex pending_mutex_;

        EventQueue() = default;

    public:
        /**
         * Copy constructors stay empty, because of the Singleton
         */
        EventQueue(EventQueue const&) = delete;
        void operator=(EventQueue const&) = delete;

        /**
         * @return instance of the Singleton of the EventQueue
         */
        static EventQueue& getInstance() {
            static EventQueue instance;
            return instance;
        }

        /**
         * @brief Adds a listener which will observe the event queue
         * It is not possible to add the same listener multiple time
         *
         * It is up to the user to manage the pointer of the listener,
         * but beware that when freeing the memory of the listener,
         * it is important to unsubscribe first
         * @param listener pointer to Listener stucture which contains a filter and a callback
         */
        void subscribe(Listener* listener);

        /**
         * @biref Removes the listener from the event queue
         * Use this function before freeing the pointer of the listener
         *
         * @note Unsubscribing a listener that has never been added does nothing
         * @param listener pointer to Listener stucture
         */
        void unsubscribe(Listener* listener);

        /**
         * Returns the number of listeners currently listening to a list of events
         * @param event_names list of events by names
         * @return number of listeners
         */
        size_t getNumSubscribers(const std::vector<std::string>& event_names);

        /**
         * Returns true if the given filter corresponds to a given event name
         * @param filter
         * @param event_name
         * @return
         */
        static bool isListener(const std::string& filter, const std::string& event_name);

        /**
         * Sends an event in the event queue
         *
         * @param event shared ptr of Event or daughter of Event
         * Posting a shared ptr of Event avoids problems with segfault because
         * there could be multiple listeners that consume the Event
         */
        void post(Event_ptr event);

        /**
         * @brief Polls the posted events
         * This function looks for all current listeners that correspond to the events
         * in the queue and calls the corresponding callbacks
         *
         * @note It is recommended to call the function from the main thread.
         * However, if one desires to call pollEvents() from another thread, then it is
         * up to the user to guarantee the thread safety of the lambdas created in the
         * listeners.
         */
        void pollEvents();
    };
}