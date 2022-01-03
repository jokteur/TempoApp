#include "jobscheduler.h"

#include <chrono>
#include <condition_variable>
#include <exception>
#include <mutex>

#include "log.h"

namespace Tempo {
    jobResultFct JobScheduler::no_op_fct = [](const std::shared_ptr<JobResult>&) {};

    /*
     * Exceptions related to the JobScheduler class
     */

    class JobSchedulerException : public std::exception {
    private:
        const char* what_;

    public:
        explicit JobSchedulerException(const char* what) : what_(what) {}
        virtual const char* what() const noexcept {
            return what_;
        }
    };

    /*
     * Implementations of JobScheduler
     */

    void JobScheduler::setWorkerPoolSize(int size) {
        if (size < 1) {
            throw JobSchedulerException("Cannot set thread pool size to less than 1");
        }

        // Add thread(s)
        std::lock_guard<std::mutex> guard(kill_mutex_);
        if (size > num_active_workers_) {
            for (int i = 0; i < size - num_active_workers_; i++) {
                workers_.push_back(Worker{});
                Worker& worker = *(--workers_.end());
                worker.id = worker_counter_++;
                std::thread* thread = new std::thread(&JobScheduler::worker_fct, this, std::ref(worker));
                worker.thread = thread;  //Is freed when killed (with the garbage collector)
            }
        }
        // Remove thread(s)
        else if (size < num_active_workers_) {
            kill_x_workers_ += num_active_workers_ - size;
            // Notify the first thread to kill itself
            for (int i = 0; i < num_active_workers_ - size; i++)
                semaphore_.post();
        }

        num_active_workers_ = size;
    }

    void JobScheduler::worker_fct(JobScheduler::Worker& worker) {
        while (true) {
            worker.state = WORKER_STATE_IDLE;
            semaphore_.wait();
            // If any thread must be killed, this thread will commit suicide
            {
                std::lock_guard<std::mutex> guard(kill_mutex_);
                if (kill_x_workers_ > 0) {
                    worker.state = WORKER_STATE_KILLED;
                    --kill_x_workers_;
                    break;
                }
            }

            JobReference job_ref;
            std::shared_ptr<Job> current_job;

            bool execute_job = false;
            // Search for a pending job
            {
                std::lock_guard<std::recursive_mutex> guard(jobs_mutex_);
                // Get the most urgent job from the priority queue
                job_ref = priority_queue_.top();
                current_job = *job_ref.it;
                if (current_job->abort) {
                    current_job->state = Job::JOB_STATE_CANCELED;
                    post_event(current_job);
                }
                else {
                    current_job->state = Job::JOB_STATE_RUNNING;
                    execute_job = true;
                }
                priority_queue_.pop();
            }

            if (execute_job) {
                // Execute job
                try {
                    worker.state = WORKER_STATE_WORKING;
                    auto result = current_job->fct(current_job->progress, current_job->abort);
                    {
                        std::lock_guard<std::recursive_mutex> guard(jobs_mutex_);
                        if (current_job->abort) {
                            current_job->state = Job::JOB_STATE_ABORTED;
                            current_job->success = result->success;
                        }
                        else {
                            current_job->state = Job::JOB_STATE_FINISHED;
                            current_job->success = result->success;
                        }
                        result->id = current_job->id;
                        current_job->result = result;
                        finalize_jobs_list_.push_back(current_job);
                    }
                }
                catch (std::exception& e) {
                    std::lock_guard<std::recursive_mutex> guard(jobs_mutex_);
                    current_job->state = Job::JOB_STATE_ERROR;
                    current_job->exception = e;
                    APP_DEBUG(e.what());
                    current_job->result = std::make_shared<JobResult>();
                    finalize_jobs_list_.push_back(current_job);
                }
                post_event(current_job);
            }
            {
                std::lock_guard<std::recursive_mutex> guard(jobs_mutex_);
                remove_job_from_list(job_ref);
            }
        }
    }

    std::shared_ptr<Job>& JobScheduler::addJob(std::string name, jobFct& function, jobResultFct& result_fct, Job::jobPriority priority) {
        Job job;
        job.name = name;
        job.id = job_counter_++;
        job.fct = function;
        job.priority = priority;
        job.result_fct = result_fct;

        std::lock_guard<std::recursive_mutex> guard(jobs_mutex_);
        jobs_list_.emplace_back(std::make_shared<Job>(job));

        JobReference jobReference;
        jobReference.it = --(jobs_list_.end());

        priority_queue_.push(jobReference);
        semaphore_.post();
        return *--(jobs_list_.end());
    }

    bool JobScheduler::stopJob(jobId jobId) {
        std::lock_guard<std::recursive_mutex> guard(jobs_mutex_);
        for (auto& job : jobs_list_) {
            if (job->id == jobId) {
                job->abort = true;
                return false;
            }
        }
        return true;
    }

    void JobScheduler::clean() {
        for (auto& worker : workers_) {
            if (worker.state == WORKER_STATE_KILLED) {
                worker.thread->join();
                delete worker.thread;
            }
        }
    }

    Job JobScheduler::getJobInfo(jobId id) {
        std::lock_guard<std::recursive_mutex> guard(jobs_mutex_);
        for (auto& job : jobs_list_) {
            if (job->id == id) {
                Job return_job;
                return_job.name = job->name;
                return_job.id = job->id;
                return_job.state = job->state;
                return_job.priority = job->priority;
                return_job.progress = job->progress;
                return_job.exception = job->exception;
                return_job.abort = job->abort;
                return_job.success = job->success;
                return return_job;
            }
        }
        // Did not found any job
        Job job;
        job.name = "";
        job.state = Job::JOB_STATE_NOTEXISTING;
        return job;
    }

    bool JobScheduler::isBusy() {
        std::lock_guard<std::recursive_mutex> guard(jobs_mutex_);
        for (auto& job : jobs_list_) {
            if (job->state == Job::JOB_STATE_PENDING || job->state == Job::JOB_STATE_RUNNING)
                return true;
        }
        return false;
    }

    void JobScheduler::cancelAllPendingJobs() {
        std::lock_guard<std::recursive_mutex> guard(jobs_mutex_);
        for (auto& job : jobs_list_) {
            if (job->state == Job::JOB_STATE_PENDING) {
                job->abort = true;
            }
        }
    }

    void JobScheduler::abortAll() {
        std::lock_guard<std::recursive_mutex> guard(jobs_mutex_);
        for (auto& job : jobs_list_) {
            job->abort = true;
        }
    }

    void JobScheduler::post_event(std::shared_ptr<Job> job) {
        std::string event_name = std::string("jobs/ids/") + std::to_string(job->id);
        std::string event_name2 = std::string("jobs/names/") + job->name;

        event_queue_.post(Event_ptr(new JobEvent(event_name, job)));
        event_queue_.post(Event_ptr(new JobEvent(event_name2, job)));
    }

    void JobScheduler::remove_job_from_list(JobReference& jobReference) {
        jobs_list_.erase(jobReference.it);
    }

    void JobScheduler::finalizeJobs() {
        std::lock_guard<std::recursive_mutex> guard(jobs_mutex_);
        for (auto& job : finalize_jobs_list_) {
            job->result_fct(job->result);
        }
        finalize_jobs_list_.clear();
    }
}