#include "FreeRTOS.h"
#include <cstdio>
#include "log.h"

void* operator new (size_t size) {
    void* ptr = pvPortMalloc(size);
    LOG(Log_System, LevelDebug, "新分配：%d 字节：%p",
        size, ptr);
    return ptr;
}

void* operator new[](size_t size) {
    void* ptr = pvPortMalloc(size);
    LOG(Log_System, LevelDebug, "新分配：%d 字节：%p",
        size, ptr);
    return ptr;
}

void operator delete (void* ptr) {
    LOG(Log_System, LevelDebug, "删除：正在释放指针：%p",
        ptr);
    vPortFree(ptr);
}

void operator delete[](void* ptr) {
    LOG(Log_System, LevelDebug, "删除：正在释放指针：%p",
        ptr);
    vPortFree(ptr);
}

extern "C" void __cxa_pure_virtual() {
    LOG(Log_System, LevelCrit, "纯虚函数");
    while (1);
}

