/**
 * @file main.cpp
 * @author Naikekongwo
 * @brief OpenGames Studio 的程序入口
 * @version 0.1
 * @date 2026-11-01
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "OpenGamesStudio/StudioStage.hpp"
#include "OpenCore.hpp"

#include <cstdlib>
#include <exception>
#include <memory>

int main(int argc, char **args)
{
    OpenEngine &engine = OpenEngine::getInstance();

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
    appInfo->gameName      = "OpenGames Studio";
    appInfo->gameCode      = "OPENGAMESSTUDIO";
    appInfo->beta          = true;
    appInfo->version_major = 0;
    appInfo->version_minor = 1;
    appInfo->entranceStage = std::make_unique<StudioStage>();

    if (!engine.GameRegistry(std::move(appInfo)))
    {
        LOG("游戏信息注册失败");
        return EXIT_FAILURE;
    }

    if (!engine.Initialize())
    {
        LOG("OpenCore 引擎初始化失败");
        return EXIT_FAILURE;
    }

    auto *package = engine.getPackageManager();
    package->registerResources({
        {RscFont, "Font_UI", "assets/ui/font/SourceHanSansHeavy.otf"},
        {RscFont, "Font_Eng", "assets/ui/OpenCoreFont.ttf"},
    });

    try
    {
        engine.MainLoop();
        engine.CleanUp();
    }
    catch (const std::exception &e)
    {
        LOG("引擎异常退出: {}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
