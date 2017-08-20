#ifndef __COROUTINE_H
#define __COROUTINE_H
#include <functional>
#include <memory>
#include <vector>
#include <ucontext.h>
#include <string.h>
#include <assert.h>
class Coroutine;
class Scheduler;

using CoroutineFunc = std::function<void(Scheduler*)>;

class Coroutine{
public:
    friend class Scheduler;
    Coroutine(const CoroutineFunc & func, Scheduler* s)
        :func_(func),s_(s)
    {
        status_ = READY;
        idx_ = -1;
    }
    bool dead(){return status_ == DEAD;}
    //void yield();
    void resume();
    //const CoroutineFunc & func(){return func_;}
private:
    static void threadFunc(uint32_t, uint32_t);
    void saveStack(const void* src, size_t sz);
    enum { READY, SUSPEND, RUNNING, DEAD} status_;
    std::vector<char> stack_;
    ucontext_t ctx_;
    CoroutineFunc func_;
    Scheduler * s_;
    size_t idx_; // to be used by Scheduler
    //Coroutine(const CoroutineFunc&);
};

class Scheduler{
public:
    friend class Coroutine;
    Scheduler(){
        running_ = nullptr;
    }
    Coroutine* createCoroutine(const CoroutineFunc& );
    void yield();
    
private:
    void setRunning(Coroutine* c);
    void remove(Coroutine*);
    std::vector<std::unique_ptr<Coroutine>> coroutines_;
    Coroutine* running_;
    const static int STACK_SIZE=1024*1024;
    char stack_[STACK_SIZE];
    ucontext_t main_;
};

#endif// __COROUTINE_H
