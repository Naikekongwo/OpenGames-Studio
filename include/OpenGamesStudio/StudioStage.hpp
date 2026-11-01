/**
 * @file StudioStage.hpp
 * @author Naikekongwo
 * @brief OpenGames Studio 的主场景（当前仅显示标题）
 * @version 0.1
 * @date 2026-11-01
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include "World/Stage/BaseStage.hpp"

class StudioStage : public BaseStage
{
  public:
    StudioStage()         = default;
    ~StudioStage() override = default;

    bool parseEvents(Event *event) override;

    void onEnter() override;
    void initializeComponents() override;

    void onUpdate() override;
    void onRender() override;

    void onExit() override;
    void onDestroy() override;
};
