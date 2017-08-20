# coroutine

Inspired by CloudWu's C implementation of stackful coroutine.
I use std::unique_ptr<Coroutine, Deleter> to handle cleanup (unregister).

By default each coroutine has a maximum stack of 1 megabytes, 
yet the actual memory consumption can be quite small (< 1kb).

The LogStream class uses a char[65536] on-stack buffer,
this design results in an waste of memory for coroutines, 
one possible solution is to wrap the logging statement with the following lambda:

```
void coroutineFunc(Scheduler * s){
    for (int i=0;i<10;++i){
        //LOG_DEBUG << "i=" << i; //Don't do this
        
        [i](){LOG_DEBUG << "i=" << i;}(); //Use this workaround
        s->yield();
    }
}
```
