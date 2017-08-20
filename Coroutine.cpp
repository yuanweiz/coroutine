#include "Coroutine.h"
#include <sys/types.h>
#include <vector>
#include <assert.h>
#include <string.h>
#include "Logging.h"
using namespace std;

void Coroutine::threadFunc(uint32_t lo, uint32_t hi){
    LOG_TRACE << "Coroutine::threadFunc";
    uint64_t u64 = hi ;
    u64 = (u64 << 32 )+ lo;
    Coroutine* c = reinterpret_cast<Coroutine*>(u64);
    auto * s = c->s_;
    c->func_(s);
    c->status_ = DEAD;
    s->remove(c);
    //TODO: do cleanup here
}
//void Coroutine::yield(){
//    
//}

void Coroutine::saveStack(const void* src, size_t sz){
    if (stack_.size() < sz){
        stack_.resize(sz);
    }
    uintptr_t  dst_ =reinterpret_cast<uintptr_t>( stack_.data()+ stack_.size() - sz);
    void * dst = reinterpret_cast<void*>(dst_);
    ctx_.uc_stack.ss_sp = stack_.data();
    ctx_.uc_stack.ss_size = stack_.size();
    ::memcpy(dst,src,sz);
}

void Coroutine::resume()
{
    LOG_TRACE << "Coroutine::resume(), this=" << this;
    assert(s_->running_ == nullptr);
    //s_->running_ = this;
    s_->setRunning(this);
    ucontext_t* main_ = &s_->main_;
    void * ptr = main_->uc_stack.ss_sp;
    auto sz = main_->uc_stack.ss_size;
    ctx_.uc_stack.ss_sp = ptr;
    ctx_.uc_stack.ss_size = sz;
    if (!stack_.empty()){
        //restore stack
        assert (status_ == SUSPEND);
        auto dst = reinterpret_cast<uintptr_t>(ptr)+sz - stack_.size();
        ::memcpy(reinterpret_cast<void*>(dst), stack_.data(),stack_.size());
    }
    else {
        union {
            uint64_t u64;
            uint32_t u32[2];
        };
        u64 = reinterpret_cast<uint64_t>(this);
        assert(status_ == READY);
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

void Scheduler::yield()
{
    LOG_TRACE<< "Scheduler::yield()";
    char dummy;
    uintptr_t top =reinterpret_cast<uintptr_t>( stack_) + sizeof (stack_);
    uintptr_t sp =reinterpret_cast<uintptr_t>( &dummy);
    assert(top > sp);
    auto sz = top-sp;
    assert (running_ );
    assert( running_->status_ == Coroutine::RUNNING);
    running_->saveStack(reinterpret_cast<void*>(sp),sz);
    running_->status_ = Coroutine::SUSPEND;
    setRunning(nullptr);
    ::swapcontext(&main_, &running_->ctx_);
}

void Scheduler::remove(Coroutine *c){
    LOG_TRACE<< "Scheduler::remove, Coroutine=" << c ;
    auto & p = coroutines_[c->idx_];
    auto & back =coroutines_.back();
    assert(p.get()==c);
    back->idx_ = c->idx_;
    p.swap(back);
    coroutines_.pop_back();
}

Coroutine* Scheduler::createCoroutine(const CoroutineFunc& func)
{
    LOG_TRACE<< "Scheduler::createCoroutine()";
    auto * c=new Coroutine(func,this);
    c->idx_ = coroutines_.size();
    LOG_DEBUG << "index="<< c->idx_;
    coroutines_.emplace_back(c);
    return c;
}

void Scheduler::setRunning(Coroutine* c)
{
    LOG_DEBUG << "Scheduler::setRunning() : c=" << c;
    running_ = c;
}
