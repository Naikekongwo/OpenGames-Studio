/**
 * @file StartupStage.cpp
 * @author Naikekongwo
 * @brief 启动页的实现
 * @version 0.1
 * @date 2026-10-28
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "OpenScripter/Stage/StartupStage.hpp"

#include "OpenScripter/Ui/Theme.hpp"

#include "Core/Info/GameInfo.hpp"
#include "OpenCore.hpp"
#include "Runtime/Graphics/IDrawableObject/Text.hpp"
#include "Runtime/Graphics/Manager/ElementManager.hpp"
#include "Runtime/Graphics/Manager/GraphicsManager.hpp"
#include "Runtime/Graphics/UI/Panel.hpp"
#include "Runtime/Graphics/UI/TextArea.hpp"
#include "Runtime/Graphics/UI/TextButton.hpp"

#include <memory>

namespace
{

// 版式（相对逻辑分辨率 3840×2160 的倍数；注释里的数值按 1280×720 视觉空间计）
constexpr float kTitleCenterX = 0.50f;
constexpr float kTitleCenterY = 0.40f; // 视觉 288
constexpr float kTitleWidth = 0.50f;   // 视觉 640
constexpr float kTitleHeight = 0.12f;  // 视觉 86

constexpr float kBadgeWidth = 0.075f;     // 视觉 96
constexpr float kBadgeHeight = 0.055556f; // 视觉 40
constexpr float kBadgeGap = 12.0f;        // 视觉：BETA 左边到标题字右边缘
constexpr float kBadgeLift = 10.0f;       // 视觉：BETA 底边抬到标题中心线以上

constexpr float kButtonWidth = 0.1875f;      // 视觉 240
constexpr float kButtonHeight = 0.072222f;   // 视觉 52
constexpr float kButtonGap = 0.022222f;      // 视觉 16
constexpr float kFirstButtonTop = 0.519444f; // 视觉 374

TextAttribute MakeButtonTextAttribute(const Color &color)
{
    TextAttribute attribute;
    attribute.fontName = "ChineseFont";
    attribute.option = RENDER_TEXT;
    attribute.color = color;
    return attribute;
}

// 标题文字的实际宽度（逻辑像素）。BETA 要贴文字右上角，不能贴 640
// 宽的标题框角上。
float MeasureTitleWidth()
{
    TextAttribute attribute;
    attribute.fontName = "ChineseFont";
    attribute.fontSize =
        GraphicsManager::getInstance().designFontSize(theme::kFontLogo);

    int measureWidth = 0, measureHeight = 0;
    if (!Text::Measure("OpenScripter", attribute, measureWidth,
                       measureHeight) ||
        measureHeight <= 0)
    {
        // 量不出时按视觉 430 兜底
        const float logicalHeight = static_cast<float>(
            GraphicsManager::getInstance().getSccissorRect().h);
        return 430.0f / 720.0f * logicalHeight;
    }

    return static_cast<float>(attribute.fontSize) * measureWidth /
           measureHeight;
}

} // namespace

void StartupStage::onEnter()
{
    LOG("启动页已建立");
    initializeComponents();
}

void StartupStage::onExit() { Elements->onDestroy(); }

void StartupStage::onDestroy() { LOG("启动页已销毁"); }

void StartupStage::onUpdate() { Elements->onUpdate(timer->getTotalTime()); }

void StartupStage::onRender() { Elements->onRender(); }

bool StartupStage::parseEvents(Event *event)
{
    Elements->parseEvents(event, timer->getTotalTime());
    return true;
}

void StartupStage::initializeComponents()
{
    const float logicalWidth =
        static_cast<float>(GraphicsManager::getInstance().getSccissorRect().w);

#pragma region 页面底色
    auto background = std::make_unique<OpenUI::Panel>("background", 0);
    background->setBackgroundColor(theme::PageBackground);
    background->Configure()
        .Parent(nullptr)
        .Anchor(AnchorPoint::TopLeft)
        .PositeR(0.0f, 0.0f)
        .ScaleR(1.0f, 1.0f);
    Elements->PushElement(std::move(background));
#pragma endregion

#pragma region 产品标题
    TextAttribute titleAttr;
    titleAttr.fontName = "ChineseFont";
    titleAttr.option = RENDER_TEXT;
    titleAttr.color = theme::TextPrimary;

    auto title = std::make_unique<OpenUI::TextArea>("title", 10, "ChineseFont");
    title->setAttribute(titleAttr);
    title->setText("OpenScripter");
    title->setFontSize(theme::kFontLogo);
    title->align(AnchorPoint::Center);
    title->Configure()
        .Parent(nullptr)
        .Anchor(AnchorPoint::Center)
        .PositeR(kTitleCenterX, kTitleCenterY)
        .ScaleR(kTitleWidth, kTitleHeight);
    Elements->PushElement(std::move(title));
#pragma endregion

#pragma region 新建工程（主按钮）
    auto newButton = std::make_unique<OpenUI::TextButton>("newProjectButton",
                                                          20, "新建工程");
    newButton->setFontSize(theme::kFontButton);
    newButton->setBackgroundColor(theme::Primary);
    newButton->setStateBackgroundColors(theme::Primary, theme::PrimaryHover,
                                        theme::PrimaryPressed);
    newButton->setCornerRadius(theme::kControlRadius);
    newButton->setBorderColor(theme::PrimaryBorder);
    newButton->setBorderWidth(theme::kBorderWidth);
    newButton->setNormalAttribute(MakeButtonTextAttribute(theme::TextInverse));
    newButton->setHoveredAttribute(MakeButtonTextAttribute(theme::TextInverse));
    newButton->setPressedAttribute(MakeButtonTextAttribute(theme::TextInverse));
    newButton->Configure()
        .Parent(nullptr)
        .Anchor(AnchorPoint::TopLeft)
        .PositeR((1.0f - kButtonWidth) * 0.5f, kFirstButtonTop)
        .ScaleR(kButtonWidth, kButtonHeight);
    newButton->setOnClick([]()
                          { LOG("新建工程：入口未接入，等 M1 工程体系"); });
    Elements->PushElement(std::move(newButton));
#pragma endregion

#pragma region 打开工程（次按钮）
    auto openButton = std::make_unique<OpenUI::TextButton>("openProjectButton",
                                                           20, "打开工程");
    openButton->setFontSize(theme::kFontButton);
    openButton->setBackgroundColor(theme::SecondaryFill);
    openButton->setStateBackgroundColors(
        theme::SecondaryFill, theme::SecondaryHover, theme::SecondaryPressed);
    openButton->setCornerRadius(theme::kControlRadius);
    openButton->setBorderColor(theme::SecondaryBorder);
    openButton->setBorderWidth(theme::kBorderWidth);
    openButton->setNormalAttribute(MakeButtonTextAttribute(theme::TextButton));
    openButton->setHoveredAttribute(MakeButtonTextAttribute(theme::TextButton));
    openButton->setPressedAttribute(MakeButtonTextAttribute(theme::TextButton));
    openButton->Configure()
        .Parent(nullptr)
        .Anchor(AnchorPoint::TopLeft)
        .PositeR((1.0f - kButtonWidth) * 0.5f,
                 kFirstButtonTop + kButtonHeight + kButtonGap)
        .ScaleR(kButtonWidth, kButtonHeight);
    openButton->setOnClick([]()
                           { LOG("打开工程：入口未接入，等 M1 工程体系"); });
    Elements->PushElement(std::move(openButton));
#pragma endregion

#pragma region BETA 标记（只在 beta 版本显示）
    const auto *info = OpenEngine::getInstance().getGameInfo();
    if (info != nullptr && info->beta)
    {
        // BETA 贴标题文字的右上角：横向从文字右边缘再让 12
        // 像素，底边抬到字顶附近
        const float badgeLeft = kTitleCenterX +
                                MeasureTitleWidth() * 0.5f / logicalWidth +
                                kBadgeGap / 1280.0f;
        const float badgeBottom = kTitleCenterY - kBadgeLift / 720.0f;

        TextAttribute betaAttr;
        betaAttr.fontName = "ChineseFont";
        betaAttr.option = RENDER_TEXT;
        betaAttr.color = theme::TextInverse;

        auto betaText =
            std::make_unique<OpenUI::TextArea>("betaText", 30, "ChineseFont");
        betaText->setAttribute(betaAttr);
        betaText->setText("BETA");
        betaText->setFontSize(theme::kFontBadge);
        betaText->align(AnchorPoint::Center);
        betaText->setBackgroundColor(theme::Primary);
        betaText->setCornerRadius(theme::kPillRadius);
        betaText->Configure()
            .Parent(nullptr)
            .Anchor(AnchorPoint::BottomLeft)
            .PositeR(badgeLeft, badgeBottom)
            .ScaleR(kBadgeWidth, kBadgeHeight);
        Elements->PushElement(std::move(betaText));
    }
#pragma endregion
}
