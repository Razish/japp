#pragma once

#include <vector>
#include <memory>
#include <future>
/*

new Task([ function ], [ callback ( nullptr) ], argN...)


*/

class TaskInterface {
  public:
    virtual bool isReady(void) = 0;
    virtual void runCallback(void *arg) = 0;
    virtual void *getResult(void) = 0;
    virtual ~TaskInterface() = 0;
};

extern std::vector<TaskInterface *> tasks;

template <class R> class Task : public TaskInterface, public std::future<R> {
  public:
    typedef std::function<void(R)> Callback;

    Callback callback_function;
    bool valid;

    Task(std::future<R> instance, Callback clbck) : std::future<R>(std::move(instance)), callback_function(clbck), valid(true) { tasks.push_back(this); };
    ~Task(){};

    void runCallback(void *arg) {
        std::unique_ptr<R> *ptr = (std::unique_ptr<R> *)arg;
        callback_function(*ptr->get());
        ptr->release();
    }

    void *getResult(void) {
        valid = false;
        return &std::make_unique<R>(new R(this->get()));
    }

    bool isReady(void) {
        if (valid && this->wait_for(std::chrono::microseconds(1)) == std::future_status::ready) {
            return true;
        }
        return false;
    }
};

void CheckTasks(void);
