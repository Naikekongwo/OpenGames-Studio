/**
 * @file StartupStage.hpp
 * @author Naikekongwo
 * @brief 启动页：产品标题、BETA 标记与工程入口按钮
 * @version 0.1
 * @date 2026-10-28
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include "World/Stage/BaseStage.hpp"

class Event;

class StartupStage : public BaseStage
{
  public:
    StartupStage() = default;
    ~StartupStage() override = default;

    bool parseEvents(Event *event) override;

    void onEnter() override;
    void initializeComponents() override;

    void onUpdate() override;
    void onRender() override;

    void onExit() override;
    void onDestroy() override;
};
