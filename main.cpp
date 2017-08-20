#include <ucontext.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "Logging.h"
#include "Coroutine.h"

void thread1( Scheduler* s){ 
    for (int i=0;i<10;++i){
        [=](){
        LOG_DEBUG << "Thread i:" << i;
        }();
        s->yield();
    }
}

void thread2( Scheduler* s ){
    for (int i=0;i<10;++i){
        [=](){
        LOG_DEBUG << "Thread 2:" << i;
        }();
        s->yield();
    }
}

int main (){
    Scheduler s;
    auto co1 = s.createCoroutine(thread1);
    auto co2 = s.createCoroutine(thread2);
    while (!co1->dead()  && !co2->dead()){
        co1->resume();
        co2->resume();
    }
}
