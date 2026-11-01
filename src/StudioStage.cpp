/**
 * @file StudioStage.cpp
 * @author Naikekongwo
 * @brief OpenGames Studio 主场景的实现（当前仅显示标题）
 * @version 0.1
 * @date 2026-11-01
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "OpenGamesStudio/StudioStage.hpp"

#include "OpenCore.hpp"
#include "Runtime/Graphics/IDrawableObject/Text.hpp"
#include "Runtime/Graphics/Manager/ElementManager.hpp"
#include "Runtime/Graphics/UI/Panel.hpp"
#include "Runtime/Graphics/UI/TextArea.hpp"

namespace
{

// 浅色简约风格的配色
const Color kPageBackground =
    Color(246.f / 255.f, 247.f / 255.f, 249.f / 255.f, 1.f);
const Color kTextPrimary =
    Color(28.f / 255.f, 32.f / 255.f, 40.f / 255.f, 1.f);
const Color kTextSecondary =
    Color(108.f / 255.f, 116.f / 255.f, 130.f / 255.f, 1.f);

} // namespace

void StudioStage::onEnter() { initializeComponents(); }

void StudioStage::onExit() { Elements->onDestroy(); }

void StudioStage::onDestroy() { LOG("Studio 主场景已销毁"); }

bool StudioStage::parseEvents(Event *event)
{
    Elements->parseEvents(event, timer->getTotalTime());
    return true;
}

void StudioStage::onUpdate()
{
    Elements->onUpdate(timer->getTotalTime());
    pipeline.onUpdate(timer->getTotalTime());
}

void StudioStage::onRender() { Elements->onRender(); }

void StudioStage::initializeComponents()
{
    // 页面底色
    auto background = std::make_unique<OpenUI::Panel>("background", 0);
    background->setBackgroundColor(kPageBackground);
    background->Configure()
        .Parent(nullptr)
        .Anchor(AnchorPoint::TopLeft)
        .PositeR(0.0f, 0.0f)
        .ScaleR(1.0f, 1.0f);
    Elements->PushElement(std::move(background));

    // 主标题
    TextAttribute titleAttr;
    titleAttr.fontName = "Font_UI";
    titleAttr.option   = RENDER_TEXT;
    titleAttr.color    = kTextPrimary;
    titleAttr.fontSize = 96;

    auto title = std::make_unique<OpenUI::TextArea>("title", 10, "Font_UI");
    title->setAttribute(titleAttr);
    title->setText("OpenGames Studio");
    title->align(AnchorPoint::Center);
    title->Configure()
        .Parent(nullptr)
        .Anchor(AnchorPoint::Center)
        .PositeR(0.5f, 0.46f)
        .ScaleR(0.70f, 0.12f);
    Elements->PushElement(std::move(title));

    // 副标题
    TextAttribute subtitleAttr;
    subtitleAttr.fontName = "Font_UI";
    subtitleAttr.option   = RENDER_TEXT;
    subtitleAttr.color    = kTextSecondary;
    subtitleAttr.fontSize = 44;

    auto subtitle =
        std::make_unique<OpenUI::TextArea>("subtitle", 10, "Font_UI");
    subtitle->setAttribute(subtitleAttr);
    subtitle->setText("中枢面板");
    subtitle->align(AnchorPoint::Center);
    subtitle->Configure()
        .Parent(nullptr)
        .Anchor(AnchorPoint::Center)
        .PositeR(0.5f, 0.56f)
        .ScaleR(0.40f, 0.06f);
    Elements->PushElement(std::move(subtitle));
}
