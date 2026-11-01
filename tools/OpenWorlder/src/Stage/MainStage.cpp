/**
 * @file MainStage.cpp
 * @author Naikekongwo
 * @brief OpenWorlder 主场景的实现（占位）
 * @version 0.1
 * @date 2026-11-01
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "OpenWorlder/Stage/MainStage.hpp"

#include "OpenCore.hpp"
#include "Runtime/Graphics/IDrawableObject/Text.hpp"
#include "Runtime/Graphics/Manager/ElementManager.hpp"
#include "Runtime/Graphics/UI/Panel.hpp"
#include "Runtime/Graphics/UI/TextArea.hpp"

void MainStage::onEnter() { initializeComponents(); }

void MainStage::onExit() { Elements->onDestroy(); }

void MainStage::onDestroy() { LOG("OpenWorlder 主场景已销毁"); }

bool MainStage::parseEvents(Event *event)
{
    Elements->parseEvents(event, timer->getTotalTime());
    return true;
}

void MainStage::onUpdate()
{
    Elements->onUpdate(timer->getTotalTime());
    pipeline.onUpdate(timer->getTotalTime());
}

void MainStage::onRender() { Elements->onRender(); }

void MainStage::initializeComponents()
{
    // 页面底色
    auto background = std::make_unique<OpenUI::Panel>("background", 0);
    background->setBackgroundColor(
        Color(246.f / 255.f, 247.f / 255.f, 249.f / 255.f, 1.f));
    background->Configure()
        .Parent(nullptr)
        .Anchor(AnchorPoint::TopLeft)
        .PositeR(0.0f, 0.0f)
        .ScaleR(1.0f, 1.0f);
    Elements->PushElement(std::move(background));

    // 标题
    auto title = std::make_unique<OpenUI::TextArea>("title", 10, "Font_UI");
    TextAttribute attr;
    attr.fontName = "Font_UI";
    attr.option   = RENDER_TEXT;
    attr.color    = Color(28.f / 255.f, 32.f / 255.f, 40.f / 255.f, 1.f);
    attr.fontSize = 84;
    title->setAttribute(attr);
    title->setText("OpenWorlder");
    title->align(AnchorPoint::Center);
    title->Configure()
        .Parent(nullptr)
        .Anchor(AnchorPoint::Center)
        .PositeR(0.5f, 0.5f)
        .ScaleR(0.6f, 0.12f);
    Elements->PushElement(std::move(title));
}
