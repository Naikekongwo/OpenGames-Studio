#include "OpenPackager/Stage/PreloadStage.hpp"
#include "OpenCore.hpp"
#include "OpenPackager/OpenPackager.hpp"
#include "OpenPackager/Stage/WorktableStage.hpp"

#include <cstddef>
#include <memory>

PreloadStage::PreloadStage() {}

void PreloadStage::onEnter() { initializeComponents(); }

void PreloadStage::onExit()
{
    Elements->onDestroy();
    LOG("预加载场景退出，已清理元素");
}

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
    // 帧率计数器（同时充当启动画面的计时器）
    auto frameCounter = UI<OpenUI::FrameCounter>("frameCounter", 100, "", 0, 0);
    frameCounter->Configure().Sequence(true);
    frameCounter->setFontName("Font_Eng");
    frameCounter->setFontSize(36);
    frameCounter->Animate().Timer(6.0f).Commit();
    Elements->PushElement(std::move(frameCounter));

    // 引擎 LOGO
    auto logo = UI<OpenUI::ImageBoard>("core_logo", 5, "CORE_LOGO", 1, 1);
    logo->Configure()
        .Parent(nullptr)
        .Anchor(AnchorPoint::Center)
        .PositeR(0.5f, 0.5f)
        .ScaleR(0.5f, 0.25f * widthheight)
        .Alpha(1.0f);
    Elements->PushElement(std::move(logo));

    pipeline.next([]() -> bool { return true; })
        .next(
            []() -> bool
            {
                auto *framecounter = OpenEngine::getInstance()
                                         .getStageController()
                                         ->getOverlayStage()
                                         ->getElementManager()
                                         ->find("frameCounter");
                return framecounter && framecounter->isAnimeFinished();
            })
        .next(
            []() -> bool
            {
                auto *controller = OpenEngine::getInstance().getStageController();
                controller->removeStage(StageType::overlayStage);

                auto workstage = std::make_unique<WorktableStage>();
                controller->changeStage(std::move(workstage));
                return true;
            });
}
