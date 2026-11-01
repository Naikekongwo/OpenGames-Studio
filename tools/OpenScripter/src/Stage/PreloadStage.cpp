/**
 * @file PreloadStage.cpp
 * @author Naikekongwo
 * @brief 预加载场景的实现
 * @version 0.1
 * @date 2026-06-14
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "OpenScripter/Stage/PreloadStage.hpp"
#include "OpenScripter/Stage/StartupStage.hpp"

#include "OpenCore.hpp"
#include "Runtime/Graphics/Manager/ElementManager.hpp"
#include "World/Stage/StageManager.hpp"

#include <memory>

void PreloadStage::onEnter()
{
    LOG("预加载开始");
    initializeComponents();
}

void PreloadStage::onExit() { Elements->onDestroy(); }

void PreloadStage::onDestroy() { LOG("预加载场景已销毁"); }

bool PreloadStage::parseEvents(Event *event)
{
    Elements->parseEvents(event, timer->getTotalTime());
    return true;
}

void PreloadStage::onUpdate()
{
    Elements->onUpdate(timer->getTotalTime());
    pipeline.onUpdate(timer->getTotalTime());
}

void PreloadStage::onRender() { Elements->onRender(); }

void PreloadStage::initializeComponents()
{
    // 本工具暂时没有需要预先载入的资源：字体在第一次取用时由 PackageManager 加载。
    // 这里只负责把舞台交给启动页并移除覆盖层。
    //
    // 推迟到下一帧再切：引擎在首帧渲染时才按 3840 宽刷新窗口属性，
    // 提前建启动页会让 setFontSize() 按 1920 基准换算，字只有设计尺寸的一半。
    pipeline.next(
        [this]() -> bool
        {
            sController->changeStage(std::make_unique<StartupStage>());
            sController->removeStage(StageType::overlayStage);
            return true;
        });
}
