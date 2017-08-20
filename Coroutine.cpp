#include "Coroutine.h"
#include <sys/types.h>
#include <vector>
#include <algorithm>
#include <assert.h>
#include <string.h>
#include "Logging.h"
using namespace std;
struct Dummy{
    char dummy;
};

Coroutine::Coroutine(const CoroutineFunc & func, Scheduler* s)
    :func_(func),s_(s)
{
    status_ = READY;
    idx_ = -1;
}

Coroutine::~Coroutine(){
}

void Coroutine::threadFunc(uint32_t lo, uint32_t hi){
    Dummy d;
    [&](){
    LOG_TRACE << "Coroutine::threadFunc(), sp="<<&d;
    }();
    uint64_t u64 = hi ;
    u64 = (u64 << 32 )+ lo;
    Coroutine* c = reinterpret_cast<Coroutine*>(u64);
    auto * s = c->s_;
    c->func_(s);
    c->status_ = DEAD;
    s->remove(c);
}

void Coroutine::deleter(Coroutine* c){
    if (c->idx_ < 0){
        assert(c->dead());
    }
    else {
        assert(!c->dead()); //TODO: stronger sanity check?
        c->s_->remove(c);
    }
    delete c;
}

void Coroutine::saveStack(const void* src, size_t sz){
    if (stack_.size() < sz){
        stack_.resize(sz);
    }
    uintptr_t  dst_ =reinterpret_cast<uintptr_t>( stack_.data()+ stack_.size() - sz);
    void * dst = reinterpret_cast<void*>(dst_);
    ctx_.uc_stack.ss_sp = stack_.data();
    ctx_.uc_stack.ss_size = stack_.size();
    LOG_DEBUG << "Coroutine::saveStack memcpy("<<dst<< ","<<src<<","<< sz<<")";
    ::memcpy(dst,src,sz);
}

void Coroutine::resume()
{
    LOG_TRACE << "Coroutine::resume(), this=" << this;
    assert(s_->running_ == nullptr && !dead());
    s_->setRunning(this);
    ucontext_t* main_ = &s_->main_;
    if (!stack_.empty()){
        //restore stack
        LOG_DEBUG << "stack is non empty, try to restore stack";
        assert (status_ == SUSPEND);
        union {
            char * top;
            uint64_t u64;
        };
        top = s_->stack_ + s_->STACK_SIZE;
        u64-=stack_.size();
        void* dst = top, *src=stack_.data();
        LOG_VERBOSE << "memcpy("<<dst<<"," << src <<","<<stack_.size()<<")";
        ::memcpy(dst, src ,stack_.size());
    }
    else {
        assert(status_ == READY);
        LOG_DEBUG << "Coroutine hasn't started yet, create a new one";
        union {
            uint64_t u64;
            uint32_t u32[2];
        };
        u64 = reinterpret_cast<uint64_t>(this);
        LOG_DEBUG << "getcontext()";
        getcontext(&ctx_);
        ctx_.uc_stack.ss_sp = s_->stack_;
        ctx_.uc_stack.ss_size = Scheduler::STACK_SIZE;
        LOG_DEBUG << "makecontext()";
        ::makecontext(&ctx_, reinterpret_cast<void(*)()>(&Coroutine::threadFunc), 2 ,u32[0] ,u32[1]);
    }
    status_ = RUNNING;
    swapcontext(main_, &ctx_);
}

Scheduler::Scheduler()
{
    running_ = nullptr;
    char * sp = stack_ + STACK_SIZE;
    LOG_DEBUG << "Scheduler::ctor(), stack_=" << static_cast<void*>(stack_)
        <<", sp="<<static_cast<void*>(sp);
}

void Scheduler::yield()
{
    Dummy d;
    [&](){
    LOG_TRACE<< "Scheduler::yield()";
    LOG_DEBUG<< "switch from coroutine " << running_ << " to main"
            <<", sp="<<&d;
    }();
    auto *co = running_;
    uintptr_t top =reinterpret_cast<uintptr_t>( stack_) + sizeof (stack_);
    uintptr_t sp =reinterpret_cast<uintptr_t>( &d);
    assert(top > sp);
    auto sz = top-sp;
    assert (running_ );
    assert( running_->status_ == Coroutine::RUNNING);
    running_->saveStack(reinterpret_cast<void*>(sp),sz);
    running_->status_ = Coroutine::SUSPEND;
    setRunning(nullptr);
    ::swapcontext( &co->ctx_ , &main_);
}

void Scheduler::remove(Coroutine *c){
    LOG_TRACE<< "Scheduler::remove, Coroutine=" << c ;
    assert(c->idx_>=0 && static_cast<size_t>(c->idx_) < coroutines_.size());
    auto & p = coroutines_[c->idx_];
    auto & back =coroutines_.back();
    assert(p==c);
    back->idx_ = c->idx_;
    p->idx_ = -1;
    std::swap(p,back);
    coroutines_.pop_back();
}

Scheduler::CoroutinePtr Scheduler::createCoroutine(const CoroutineFunc& func)
{
    LOG_TRACE<< "Scheduler::createCoroutine()";
    auto * c=new Coroutine(func,this);
    c->idx_ = static_cast<int>(coroutines_.size());
    LOG_DEBUG << "index="<< c->idx_;
    coroutines_.emplace_back(c);
    return CoroutinePtr{c,&Coroutine::deleter};
}

void Scheduler::setRunning(Coroutine* c)
{
    LOG_DEBUG << "Scheduler::setRunning() : c=" << c;
    running_ = c;
}
