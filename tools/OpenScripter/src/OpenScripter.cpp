/**
 * @file OpenScripter.cpp
 * @author Naikekongwo
 * @brief OpenScripter 的启动流程（注册游戏信息 -> 初始化引擎 -> 注册资源 ->
 * 主循环）
 * @version 0.1
 * @date 2026-06-14
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "OpenScripter/OpenScripter.hpp"
#include "OpenCore.hpp"
#include "OpenScripter/Stage/PreloadStage.hpp"
#include <exception>
#include <memory>

void OpenScripter::StartUp()
{
    OpenEngine &engine = OpenEngine::getInstance();

    // 逻辑分辨率 4K；布局全部用相对坐标，窗口默认 1280×720（引擎写死）等比缩放
    GraphicsInfo gInfo;
    gInfo.nearestScaling = false;
    gInfo.resolutionWidth = 3840;
    gInfo.resolutionHeight = 2160;
    gInfo.targetFrameRate = 144;
    gInfo.keepRatio = true;

    ResourceInfo rInfo;
    rInfo.packageOnly = false; // 开发期直接从磁盘读资源
    rInfo.keepStructureWhenPackaging = true;
    rInfo.textureResampleFactor = 1.0f;

    auto appInfo = std::make_unique<GameInfo>();
    appInfo->_graphicsInfo = gInfo;
    appInfo->_resourceInfo = rInfo;
    appInfo->gameName = "OpenScripter 脚本设计器";
    appInfo->gameCode = "OPENSCRIPTER";
    appInfo->beta = true;
    appInfo->version_major = 0;
    appInfo->version_minor = 1;
    appInfo->entranceStage = std::make_unique<PreloadStage>();

    // PackageManager 在 Initialize 里构造并取用 entranceStage，所以必须先注册
    // GameInfo
    if (!engine.GameRegistry(std::move(appInfo)))
    {
        LOG("游戏信息注册失败");
        return;
    }

    if (!engine.Initialize())
    {
        LOG("OpenCore 引擎在初始化时出现问题");
        return;
    }

    // 字体注册必须在 Initialize 之后（PackageManager 此时才存在）
    auto *package = engine.getPackageManager();
    package->registerResources({
        {RscFont, "ChineseFont", "assets/ui/font/ChineseFont.otf"},
    });

    try
    {
        engine.MainLoop();
        engine.CleanUp();
    }
    catch (const std::exception &e)
    {
        LOG("引擎异常退出: {}", e.what());
    }
}
