#include "threadpool.h"

GpibThreadPool& GpibThreadPool::instance() {
    static GpibThreadPool inst;
    return inst;
}