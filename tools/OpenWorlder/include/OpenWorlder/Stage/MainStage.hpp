/**
 * @file MainStage.hpp
 * @author Naikekongwo
 * @brief OpenWorlder 的主场景（占位，仅显示标题）
 * @version 0.1
 * @date 2026-11-01
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include "World/Stage/BaseStage.hpp"

class MainStage : public BaseStage
{
  public:
    MainStage()           = default;
    ~MainStage() override = default;

    bool parseEvents(Event *event) override;

    void onEnter() override;
    void initializeComponents() override;

    void onUpdate() override;
    void onRender() override;

    void onExit() override;
    void onDestroy() override;
};
