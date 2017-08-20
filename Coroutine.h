#ifndef __COROUTINE_H
#define __COROUTINE_H
#include <functional>
#include <memory>
#include <vector>
#include <ucontext.h>
class Coroutine;
class Scheduler;

using CoroutineFunc = std::function<void(Scheduler*)>;

class Coroutine{
public:
    friend class Scheduler;
    Coroutine(const CoroutineFunc & func, Scheduler* s);
    ~Coroutine();
    bool dead(){return status_ == DEAD;}
    void resume();
private:
    static void deleter(Coroutine*);
    static void threadFunc(uint32_t, uint32_t);
    void saveStack(const void* src, size_t sz);
    enum { READY, SUSPEND, RUNNING, DEAD} status_;
    std::vector<char> stack_;
    ucontext_t ctx_;
    CoroutineFunc func_;
    Scheduler * s_;
    int idx_; // to be used by Scheduler
};

class Scheduler{
public:
    friend class Coroutine;
    Scheduler();
    using CoroutinePtr = std::unique_ptr<Coroutine, decltype(&Coroutine::deleter)>;
    CoroutinePtr createCoroutine(const CoroutineFunc& );
    void yield();
    
private:
    void setRunning(Coroutine* c);
    void remove(Coroutine*);
    std::vector<Coroutine*> coroutines_;
    Coroutine* running_;
    const static int STACK_SIZE=1024*1024;
    char stack_[STACK_SIZE];
    ucontext_t main_;
};

#endif// __COROUTINE_H
