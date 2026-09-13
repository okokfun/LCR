#include "Sweep.hpp"
#include "gui.hpp"
#include "HardwareLimits.hpp"
#include "log.h"
#include "cast.hpp"

#define Log_Sweep (LevelDebug|LevelInfo|LevelWarn|LevelError|LevelCrit)

constexpr Sweep::Config Sweep::defaultConfig;
static constexpr char* variableNames[] = { "Disabled", "|Z|", "Phase",
                                           "Resistance", "Capacitance", "Inductance", "ESR", "Q-Factor",
                                           nullptr,
                                         };
static constexpr char* scaleTypeNames[] = { "Linear", "Log", nullptr };


Sweep::Sweep(coords_t size, Menu& menu, Config c) {
    this->size = size;
    config = c;
    initialSweep = true;
    pointCnt = 0;
    marker = 0;
    // 创建菜单项
    mConfig = new Menu("Sweep", menu.getSize());
    // X轴菜单
    auto mX = new Menu("频率\n设置", menu.getSize());
    auto mXmin = new MenuValue<uint32_t>("Min.Freq",
                                         &config.X.f_min, Unit::Frequency,
                                         pmf_cast<void (*)(void*, Widget* w), Sweep, &Sweep::MayorSettingChanged>::cfn,
                                         this,
                                         HardwareLimits::MinFrequency, HardwareLimits::MaxFrequency);
    auto mXmax = new MenuValue<uint32_t>("Max.Freq",
                                         &config.X.f_max, Unit::Frequency,
                                         pmf_cast<void (*)(void*, Widget* w), Sweep, &Sweep::MayorSettingChanged>::cfn,
                                         this,
                                         HardwareLimits::MinFrequency, HardwareLimits::MaxFrequency);
    auto mPoints = new MenuValue<uint16_t>("Points",
                                           &config.X.points, Unit::None,
                                           pmf_cast<void (*)(void*, Widget* w), Sweep, &Sweep::MayorSettingChanged>::cfn,
                                           this, 2, MaxDataPoints);
    auto mXScale = new MenuChooser("Scale", scaleTypeNames,
                                   (uint8_t*) &config.X.type,
                                   pmf_cast<void (*)(void*, Widget* w), Sweep, &Sweep::MayorSettingChanged>::cfn,
                                   this, false);
    mX->AddEntry(mXmin);
    mX->AddEntry(mXmax);
    mX->AddEntry(mPoints);
    mX->AddEntry(mXScale);
    mX->AddEntry(new MenuBack());
    // 主、次 Y 轴菜单
    Menu* mAxis[2];
    for (uint8_t i = 0; i < 2; i++) {
        auto mVar = new MenuChooser("变量", variableNames,
                                    (uint8_t*) &config.axis[i].var,
                                    pmf_cast<void (*)(void*, Widget* w), Sweep, &Sweep::MayorSettingChanged>::cfn,
                                    this);
        auto mMin = new MenuValue<float>("Y 最小",
                                         &config.axis[i].min, Unit::None,
                                         pmf_cast<void (*)(void*, Widget* w), Sweep, &Sweep::MinorSettingChanged>::cfn,
                                         this);
        auto mMax = new MenuValue<float>("Y 最大",
                                         &config.axis[i].max, Unit::None,
                                         pmf_cast<void (*)(void*, Widget* w), Sweep, &Sweep::MinorSettingChanged>::cfn,
                                         this);
        auto mScale = new MenuChooser("Scale", scaleTypeNames,
                                      (uint8_t*) &config.axis[i].type,
                                      pmf_cast<void (*)(void*, Widget* w), Sweep, &Sweep::MinorSettingChanged>::cfn,
                                      this, false);
        constexpr char* MenuNames[] = { "主\nY-轴", "次\nY-轴" };
        mAxis[i] = new Menu(MenuNames[i], menu.getSize());
        mAxis[i]->AddEntry(mVar);
        mAxis[i]->AddEntry(mMin);
        mAxis[i]->AddEntry(mMax);
        mAxis[i]->AddEntry(mScale);
        mAxis[i]->AddEntry(new MenuBack());
    }
    // 采集菜单
    auto mAcq = new Menu("采集\n设置",
                         menu.getSize());
    auto mAvg = new MenuValue<uint16_t>("平均值",
                                        &config.averages, Unit::None,
                                        pmf_cast<void (*)(void*, Widget* w), Sweep, &Sweep::MinorSettingChanged>::cfn,
                                        this, 1, 1000);
    auto mExc = new MenuValue<uint32_t>("激发",
                                        &config.excitationVoltage, Unit::Voltage,
                                        pmf_cast<void (*)(void*, Widget* w), Sweep, &Sweep::MinorSettingChanged>::cfn,
                                        this,
                                        HardwareLimits::MinExcitationVoltage,
                                        HardwareLimits::MaxExcitationVoltage);
    auto mBias = new MenuValue<uint32_t>("偏置",
                                         &config.biasVoltage, Unit::Voltage,
                                         pmf_cast<void (*)(void*, Widget* w), Sweep, &Sweep::MinorSettingChanged>::cfn,
                                         this,
                                         HardwareLimits::MinBiasVoltage,
                                         HardwareLimits::MaxBiasVoltage);
    mAcq->AddEntry(mAvg);
    mAcq->AddEntry(mExc);
    mAcq->AddEntry(mBias);
    mAcq->AddEntry(new MenuBack());
    // 向主配置菜单添加子菜单
    mConfig->AddEntry(mX);
    mConfig->AddEntry(mAxis[0]);
    mConfig->AddEntry(mAxis[1]);
    mConfig->AddEntry(mAcq);
    mConfig->AddEntry(new MenuBack);
    // 在主菜单中添加配置菜单
    menu.AddEntry(mConfig);
}

Frontend::settings Sweep::GetAcquisitionSettings() {
    Frontend::settings s;
    s.biasVoltage = config.biasVoltage;
    s.excitationVoltage = config.excitationVoltage;
    s.averages = config.averages;
    s.range = config.range;
    s.frequency = PointToFrequency(pointCnt >= config.X.points ?
                                   0 : pointCnt);
    return s;
}

uint32_t Sweep::PointToFrequency(uint16_t point) {
    switch (config.X.type) {
        case ScaleType::Linear:
            return util_Map(point, 0, config.X.points - 1,
                            config.X.f_min, config.X.f_max);
        case ScaleType::Log:
            float b = log(config.X.f_max / config.X.f_min) /
                      (config.X.points - 1);
            return config.X.f_min * exp(b * point);
    }
}

bool Sweep::AddResult(LCR::Result r) {
    if (pointCnt >= config.X.points) {
        // 循环至开头
        pointCnt = 0;
        initialSweep = false;
    }
    if (!points) {
        LOG(Log_Sweep, LevelWarn, "无法添加点位，内存不足");
        return false;
    }
    // 提取正确的变量
    for (uint8_t i = 0; i < 2; i++) {
        float var;
        switch (config.axis[i].var) {
            case Variable::Magnitude:
                var = abs(r.frontend.Z);
                break;
            case Variable::Phase:
                var = 180.0f / M_PI * arg(r.frontend.Z);
                break;
            case Variable::Resistance:
                var = real(r.Z);
                break;
            case Variable::Capacitance:
                var = r.C.capacitance;
                break;
            case Variable::Inductance:
                var = r.L.inductance;
                break;
            case Variable::ESR:
                var = real(r.frontend.Z);
                break;
            case Variable::Quality:
                var = r.qualityFactor;
                break;
            default:
                var = 0.0f;
        }
        points[pointCnt].y[i] = var;
    }
    pointCnt++;
    LOG(Log_Sweep, LevelDebug, "添加数据点 %d", pointCnt);
    return true;
}

void Sweep::draw(coords_t offset) {
    size = getSize();
    auto pos = offset;
    coords_t graphTopLeft = pos + COORDS(Font_Medium.height + 2,
                                         0);
    coords_t graphBottomRight = pos + size - COORDS(
            Font_Medium.height + 2, 2 * Font_Medium.height + 2);
    uint16_t markerX = util_Map(marker, 0, config.X.points - 1,
                                graphTopLeft.x + 1, graphBottomRight.x - 1);
    auto GetPointCoordinate = [this, graphTopLeft,
          graphBottomRight](uint8_t axis, uint16_t point) -> coords_t {
        coords_t p;
        float val = points[point].y[axis];
        // 添加数据点
        if (val < config.axis[axis].min)
            val = config.axis[axis].min;
        if (val > config.axis[axis].max)
            val = config.axis[axis].max;
        p.x = util_Map(point, 0, config.X.points - 1, graphTopLeft.x + 1, graphBottomRight.x - 1);
        if (config.axis[axis].type == ScaleType::Linear) {
            p.y = util_MapF(val, config.axis[axis].min,
                            config.axis[axis].max, graphBottomRight.y - 1,
                            graphTopLeft.y + 1);
        } else {
            float b = log(config.axis[axis].max / config.axis[axis].min)
            / (graphTopLeft.y + 1 - graphBottomRight.y - 1);
            float a = config.axis[axis].max / exp(b * (graphTopLeft.y +
                                                       1));
            p.y = log(val / a) / b;
        }
        return p;
    };
    if (redrawClear) {
        // 填充背景
        display_SetForeground(ColorBackground);
        display_SetBackground(ColorBackground);
        display_RectangleFull(pos.x, pos.y, pos.x + size.x - 1,
                              pos.y + size.y - 1);
        // 绘制 X 轴
        display_SetForeground(ColorAxis);
        display_HorizontalLine(graphTopLeft.x, graphBottomRight.y,
                               graphBottomRight.x - graphTopLeft.x + 1);
        // X 轴极值刻度
        char tick[6];
        Unit::SIStringFromFloat(tick, 5, config.X.f_min);
        display_SetFont(Font_Medium);
        display_String(pos.x,
                       pos.y + size.y - 2 * Font_Medium.height, tick);
        Unit::SIStringFromFloat(tick, 5, config.X.f_max);
        display_String(pos.x + size.x - strlen(tick) *
                       Font_Medium.width, pos.y + size.y - 2 * Font_Medium.height,
                       tick);
        const char* xlabel =
            config.X.type == ScaleType::Linear ? "频率(线性)" : "频率(对数)";
        display_String((pos.x + size.x - strlen(xlabel) *
                        Font_Medium.width) / 2,
                       pos.y + size.y - 2 * Font_Medium.height,
                       xlabel);
        // 主Y轴的极值刻度与标签
        if (config.axis[0].var != Variable::None) {
            display_SetForeground(ColorPrimary);
            display_VerticalLine(graphTopLeft.x, graphTopLeft.y,
                                 graphBottomRight.y - graphTopLeft.y);
            Unit::SIStringFromFloat(tick, 5, config.axis[0].min);
            display_StringRotated(pos.x + 1, graphBottomRight.y, tick);
            Unit::SIStringFromFloat(tick, 5, config.axis[0].max);
            display_StringRotated(pos.x + 1,
                                  pos.y + strlen(tick) * Font_Medium.width, tick);
            // 标签
            char label[50];
            strcpy(label, variableNames[(int) config.axis[0].var]);
            strcat(label, config.axis[0].type == ScaleType::Linear ?
                   " (线性的)" : " (log)");
            display_StringRotated(pos.x + 1,
                                  (pos.y + graphBottomRight.y + strlen(label) *
                                   Font_Medium.width) / 2, label);
        }
        // 次坐标轴的极值刻度与标签
        if (config.axis[1].var != Variable::None) {
            display_SetForeground(ColorSecondary);
            display_VerticalLine(graphBottomRight.x, graphTopLeft.y,
                                 graphBottomRight.y - graphTopLeft.y);
            Unit::SIStringFromFloat(tick, 5, config.axis[1].min);
            display_StringRotated(pos.x + size.x - Font_Medium.height,
                                  graphBottomRight.y, tick);
            Unit::SIStringFromFloat(tick, 5, config.axis[1].max);
            display_StringRotated(pos.x + size.x - Font_Medium.height,
                                  pos.y + strlen(tick) * Font_Medium.width, tick);
            // 标签
            char label[50];
            strcpy(label, variableNames[(int) config.axis[1].var]);
            strcat(label, config.axis[1].type == ScaleType::Linear ?
                   " (线性的)" : " (log)");
            display_StringRotated(pos.x + size.x - Font_Medium.height,
                                  (pos.y + graphBottomRight.y + strlen(label) *
                                   Font_Medium.width) / 2, label);
        }
        // 显示标记
        display_SetForeground(ColorMarker);
        display_VerticalLine(markerX, graphTopLeft.y,
                             graphBottomRight.y - graphTopLeft.y);
        display_SetForeground(COLOR_BLACK);
        display_String(2, pos.y + size.y - Font_Medium.height,
                       "标记:");
        char freq[10];
        Unit::StringFromValue(freq, 8, PointToFrequency(marker),
                              Unit::Frequency);
        display_SetForeground(ColorAxis);
        display_String(50, pos.y + size.y - Font_Medium.height,
                       freq);
        // 显示数据点
        for (uint8_t axis = 0; axis < 2; axis++) {
            if (config.axis[axis].var == Variable::None) {
                // 此轴未激活
                continue;
            }
            if (axis == 0)
                display_SetForeground(ColorPrimary);
            else
                display_SetForeground(ColorSecondary);
            uint16_t highestPoint = initialSweep ? pointCnt :
                                    config.X.points;
            for (uint16_t i = 1; i < highestPoint; i++) {
                coords_t from = GetPointCoordinate(axis, i - 1);
                coords_t to = GetPointCoordinate(axis, i);
                display_Line(from.x, from.y, to.x, to.y);
            }
        }
    } else {
        // 仅更新最新数据点
        if (pointCnt > 1) {
            bool cleared = false;
            for (uint8_t axis = 0; axis < 2; axis++) {
                if (config.axis[axis].var == Variable::None) {
                    // 此轴未激活
                    continue;
                }
                coords_t from = GetPointCoordinate(axis, pointCnt - 2);
                coords_t to = GetPointCoordinate(axis, pointCnt - 1);
                if (!cleared) {
                    display_SetForeground(ColorBackground);
                    uint16_t x1 = GetPointCoordinate(axis, pointCnt).x;
                    if (x1 - from.x < 5)
                        x1 = from.x + 5;
                    if (x1 >= graphBottomRight.x)
                        x1 = graphBottomRight.x - 1;
                    display_RectangleFull(from.x + 1, graphTopLeft.y + 1, x1,
                                          graphBottomRight.y - 1);
                    if (markerX >= from.x + 1 && markerX <= x1) {
                        // 标记已清除，重新绘制
                        display_SetForeground(ColorMarker);
                        display_VerticalLine(markerX, graphTopLeft.y,
                                             graphBottomRight.y - graphTopLeft.y);
                    }
                    cleared = true;
                }
                if (axis == 0)
                    display_SetForeground(ColorPrimary);
                else
                    display_SetForeground(ColorSecondary);
                display_Line(from.x, from.y, to.x, to.y);
            }
        }
    }
    // 始终更新标记变量
    display_SetFont(Font_Medium);
    for (uint8_t i = 0; i < 2; i++) {
        if (i == 0)
            display_SetForeground(ColorPrimary);
        else
            display_SetForeground(ColorSecondary);
        char buf[10];
        if (pointCnt <= marker && initialSweep) {
            // 标记位置暂无可用数据
            strcpy(buf, "?.???");
        } else
            Unit::SIStringFromFloat(buf, 7, points[marker].y[i]);
        display_String(120 + i * 70,
                       pos.y + size.y - Font_Medium.height, buf);
    }
}

void Sweep::MayorSettingChanged(Widget* w) {
    initialSweep = true;
    pointCnt = 0;
    if (marker >= config.X.points)
        marker = config.X.points - 1;
    // 待办事项：检查设置
    requestRedrawFull();
}

void Sweep::MinorSettingChanged(Widget* w) {
    // 待办事项：检查设置
    requestRedrawFull();
}

Sweep::~Sweep() {
    if (mConfig)
        delete mConfig;
}

void Sweep::input(GUIEvent_t* ev) {
    switch (ev->type) {
        case EVENT_TOUCH_DRAGGED:
            ev->pos = ev->dragged;
        /* 不中断 */
        case EVENT_TOUCH_PRESSED: {
                // 计算新标记位置
                uint16_t xLeft = Font_Medium.height + 3;
                uint16_t xRight = size.x - (Font_Medium.height + 3);
                int16_t marker_new = util_Map(ev->pos.x, xLeft, xRight, 0,
                                              config.X.points - 1);
                if (marker_new < 0)
                    marker_new = 0;
                else if (marker_new >= config.X.points)
                    marker_new = config.X.points - 1;
                if (marker_new != marker) {
                    marker = marker_new;
                    requestRedrawFull();
                }
            }
            break;
        default:
            break;
    }
}
