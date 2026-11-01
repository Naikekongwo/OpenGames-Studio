/**
 * @file OpenPackager.cpp
 * @author Naikekongwo
 * @brief OpenPackager 的启动流程（注册游戏信息 → 初始化引擎 → 注册资源 → 主循环）
 * @version 0.1
 * @date 2026-11-01
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "OpenPackager/OpenPackager.hpp"
#include "OpenCore.hpp"
#include "OpenPackager/Stage/PreloadStage.hpp"

#include <exception>
#include <memory>

void OpenPackager::StartUp()
{
    OpenEngine &engine = OpenEngine::getInstance();

    // 逻辑分辨率 4K，布局统一用相对坐标，窗口等比缩放
    GraphicsInfo gInfo;
    gInfo.nearestScaling   = false;
    gInfo.resolutionWidth  = 3840;
    gInfo.resolutionHeight = 2160;
    gInfo.targetFrameRate  = 144;
    gInfo.keepRatio        = true;
    gInfo.resizable        = false;

    ResourceInfo rInfo;
    rInfo.packageOnly                = false;
    rInfo.keepStructureWhenPackaging = true;
    rInfo.textureResampleFactor      = 1.0f;

    auto appInfo           = std::make_unique<GameInfo>();
    appInfo->_graphicsInfo = gInfo;
    appInfo->_resourceInfo = rInfo;
    appInfo->gameName      = "OpenPackager 打包工具";
    appInfo->gameCode      = "OPENPACKAGER";
    appInfo->beta          = true;
    appInfo->version_major = 0;
    appInfo->version_minor = 1;
    appInfo->entranceStage = std::make_unique<PreloadStage>();

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

    // 资源注册必须在 Initialize 之后（此时 PackageManager 才存在）
    auto *package = engine.getPackageManager();

    package->registerResources({
        // 界面字体：OpenCoreFont 只有 ASCII 字形，中文必须使用思源黑体；
        // Font_Eng 保留给纯数字/英文的控件（如帧率计数器）
        {RscFont, "Font_UI", "assets/ui/font/SourceHanSansHeavy.otf"},
        {RscFont, "Font_Eng", "assets/ui/OpenCoreFont.ttf"},
        {RscTexture, "CORE_LOGO", "assets/ui/opencore_logo.png"},
        {RscTexture, "ICON_TEXTURE", "assets/ui/category/texture.PNG"},
        {RscTexture, "ICON_AUDIO", "assets/ui/category/audio.PNG"},
        {RscTexture, "ICON_FONT", "assets/ui/category/font.PNG"},
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
