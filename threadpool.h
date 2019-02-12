#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include<condition_variable>
#include<queue>
#include<functional>
using namespace std;

class threadpool {


    condition_variable nEvent;
    mutex nEventMutex;
    bool to_exit = false;
    bool to_exitAbruptly = false;
    queue<function<void()>>task_list;
    unsigned Total_num_threads;
    vector<thread> nThreads;
    unsigned num_busyThreads=0;
    void start(unsigned num_Threads)
    {
        Total_num_threads=num_Threads;
        for(unsigned i=0;i<num_Threads;i++)
        {
            nThreads.emplace_back([=]
            {
                while(true)
                {
                    function<void ()> t;

                    {
                        unique_lock<mutex> lock{nEventMutex};
                        nEvent.wait(lock,[=]{return to_exit || !task_list.empty();});

                        if(!(to_exit && ( to_exitAbruptly || task_list.empty())))
                        {
                            t = move(task_list.front());
                            task_list.pop();
                            num_busyThreads++;
                        }
                        else
                        {
                            break;
                        }
                    }
                    t();
                    {
                        unique_lock<mutex> lock{nEventMutex};
                        num_busyThreads--;
                    }
                }
            });

        }
    }

    void stop(unsigned flag) noexcept
    {
        {
            unique_lock<mutex> lock{nEventMutex};
            if(flag)
            {
                to_exitAbruptly=true;
            }
            to_exit=true;
        }
        nEvent.notify_all();
        for(auto &thread:nThreads)
        {
            if(thread.joinable())
            thread.join();
        }
    }

public:

    threadpool(unsigned num_Threads)
    {
        start(num_Threads);
    }
    ~threadpool()
    {
        stop(1);
    }
    void addToQueue(function<void()> task)
    {
        {
            unique_lock<mutex>lock{nEventMutex};
            task_list.emplace(move(task));
            nEvent.notify_one();
        }
    }

    unsigned get_PendingJobs()
    {
        unsigned jobs=0;
        {
            unique_lock<mutex> lock{nEventMutex};
            jobs = task_list.size();
        }
        return jobs;
    }

    unsigned get_numofThreads()
    {
        return Total_num_threads;
    }
    unsigned get_numofBusyThreads()
    {
        return num_busyThreads;
    }

    unsigned terminate_graceful()
    {
        stop(0);
        return task_list.size();
    }
    unsigned terminate_abrupt()
    {
        stop(1);
        return task_list.size();
    }
};

