#pragma once
#include <stdint.h>
#include "progressbar.hpp"
#include <complex>

namespace Frontend {

    enum class Range : uint8_t {
        AUTO,
        Lowest,
        Highest,
    };

    enum class ResultType : uint8_t {
        Valid,		// 在正确量程内获得有效测量结果
        Ranging,	// 已读取结果，但自动模式下量程尚未稳定
        Overrange,	// 阻抗过高，无法切换量程（非自动模式，或已处于最高量程）
        Underrange, // 阻抗过低，无法切换量程（非自动模式，或已处于最低量程）
        OpenLeads,	// 未检测到电流与电压 → 测试引线未连接
    };

    using Result = struct result {
        std::complex<float> Z;
        // RMS values of current and voltage
        float RMS_I, RMS_U;
        // percentage of used ADC range according to DFT result (will not be accurate in case of clipping)
        uint8_t usedRangeI, usedRangeU;
        // Indicates whether any sample in the current/voltage measurement exceeded the ADC range
        bool clippedI, clippedU;
        // Min/max measurable impedance in this range
        float LimitLow, LimitHigh;
        ResultType type;
        Range range;
        uint32_t frequency;
    };

    using Settings = struct settings {
        uint32_t biasVoltage;
        uint32_t frequency;
        uint32_t excitationVoltage;
        Range range;
        uint32_t averages;
    };

    using Callback = void(*)(void* ctx, Result);

    bool Init();
    void SetCallback(Callback cb, void* ctx = nullptr);
    void SetAcquisitionProgressBar(ProgressBar* p);
    bool Stop();
    bool Start(Settings s);
    bool Calibrate();
}
