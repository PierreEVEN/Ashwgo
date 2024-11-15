#include "jobsys/job_sys.hpp"

#include "profiler.hpp"

#include <iostream>

JobSystem* global_js = nullptr;

JobSystem::JobSystem(size_t num_tasks)
{
    for (size_t i = 0; i < num_tasks; ++i)
        workers.emplace_back(std::make_unique<Worker>(this));
    assert(!global_js);
    global_js = this;
}

JobSystem::~JobSystem()
{
    global_js = nullptr;
    workers.clear();
}

JobSystem& JobSystem::get()
{
    assert(global_js);
    return *global_js;
}

Worker::Worker(JobSystem* job_system) : js(job_system)
{
    thread = std::thread(
        [&]
        {
            while (!b_need_stop)
            {
                std::shared_ptr<IJob> job = nullptr;
                {
                    PROFILER_SCOPE(Worker_WaitForTask);
                    std::unique_lock lk(js->job_add_mutex);
                    js->job_added.wait(lk,
                                       [&]
                                       {
                                           if (b_need_stop)
                                               return true;
                                           js->jobs.try_dequeue(job);
                                           return static_cast<bool>(job);
                                       });
                }
                if (job)
                    job->run();
            }
        });
}

Worker::~Worker()
{
    b_need_stop = true;
    js->job_added.notify_all();
    thread.join();
}

void Worker::stop()
{
    b_need_stop = true;
}