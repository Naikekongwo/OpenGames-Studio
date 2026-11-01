/**
 * @file PreloadStage.hpp
 * @author Naikekongwo
 * @brief 预加载场景 - 负责把覆盖层交给启动页
 * @version 0.1
 * @date 2026-06-14
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once
#include "World/Stage/OverlayStage.hpp"

class Event;

class PreloadStage : public OverlayStage
{
  public:
    PreloadStage()           = default;
    ~PreloadStage() override = default;

    bool parseEvents(Event *event) override;

    void onEnter() override;
    void initializeComponents() override;

    void onUpdate() override;

    void onRender() override;

    void onExit() override;

    void onDestroy() override;
};
