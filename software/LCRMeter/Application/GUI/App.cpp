#include "App.hpp"
#include "desktop.hpp"
#include "FreeRTOS.h"
#include "task.h"
#include "log.h"
#include "gui.hpp"

App::App(Info info, Desktop& d) {
    this->info = info;
    state = State::Stopped;
    handle = nullptr;
    topWidget = nullptr;
    this->d = &d;
    d.AddApp(*this);
}

bool App::Start() {
    // attempt to create thread
    state = State::Starting;
    if (xTaskCreate(info.task, info.name, info.StackSize, this,
                    5,
                    &handle) != pdPASS) {
        state = State::Stopped;
        LOG(Log_GUI, LevelError, "创建任务 \"%s\" 失败",
            info.name);
        return false;
    } else {
        LOG(Log_GUI, LevelInfo, "已创建任务 \"%s\"",
            info.name);
        return true;
    }
}

void App::StartComplete(Widget* top) {
    topWidget = top;
    state = State::Running;
    LOG(Log_GUI, LevelInfo, "\"%s\" 启动了", info.name);
    GUIEvent_t ev;
    ev.type = EVENT_APP_STARTED;
    ev.app = this;
    GUI::SendEvent(&ev);
}

void App::Exit() {
    state = State::Stopped;
    LOG(Log_GUI, LevelInfo, "\"%s\" 退出了", info.name);
    GUIEvent_t ev;
    ev.type = EVENT_APP_EXITED;
    ev.app = this;
    GUI::SendEvent(&ev);
}

bool App::Stop() {
    // TODO send stop signal
    state = State::Stopping;
    constexpr uint32_t maxStopDelay = 2000;
    uint32_t start = HAL_GetTick();
    while (state != State::Stopped) {
        vTaskDelay(10);
        if (HAL_GetTick() - start > maxStopDelay) {
            LOG(Log_GUI, LevelWarn, "应用未关闭，正在强制终止进程");
            vTaskDelete(handle);
            state = State::Stopped;
        }
    }
    return true;
}
