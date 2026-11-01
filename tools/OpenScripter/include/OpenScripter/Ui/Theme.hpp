/**
 * @file Theme.hpp
 * @author Naikekongwo
 * @brief 项目级界面主题常量（启动页深色）
 * @version 0.1
 * @date 2026-10-28
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include "Core/Math/OpenCore_Color.hpp"

/**
 * @brief 视觉尺寸与配色基准
 *
 * 圆角半径与描边宽度按「相对逻辑画面高度」交给引擎：1/720 即 1280×720
 * 视觉空间里的 1 像素。字号走 setFontSize() 的 1920 基准换算。
 */
namespace theme
{

// ---- 启动页深色基底
inline const Color PageBackground =
    Color(18.f / 255.f, 20.f / 255.f, 26.f / 255.f, 1.f);

// ---- 文字
inline const Color TextPrimary =
    Color(245.f / 255.f, 247.f / 255.f, 250.f / 255.f, 1.f);
inline const Color TextButton =
    Color(229.f / 255.f, 231.f / 255.f, 235.f / 255.f, 1.f);
inline const Color TextInverse = Color(1.f, 1.f, 1.f, 1.f);

// ---- 主按钮 / BETA 标记
inline const Color Primary = Color(37.f / 255.f, 99.f / 255.f, 235.f / 255.f, 1.f);
inline const Color PrimaryHover =
    Color(29.f / 255.f, 78.f / 255.f, 216.f / 255.f, 1.f);
inline const Color PrimaryPressed =
    Color(30.f / 255.f, 64.f / 255.f, 175.f / 255.f, 1.f);
inline const Color PrimaryBorder = Color(96.f / 255.f, 165.f / 255.f, 250.f / 255.f, 1.f);

// ---- 次按钮：透明底，只描边，悬停时浮起一层深灰
inline const Color SecondaryFill    = None;
inline const Color SecondaryHover   = Color(30.f / 255.f, 34.f / 255.f, 43.f / 255.f, 1.f);
inline const Color SecondaryPressed = Color(23.f / 255.f, 26.f / 255.f, 33.f / 255.f, 1.f);
inline const Color SecondaryBorder  = Color(75.f / 255.f, 85.f / 255.f, 99.f / 255.f, 1.f);

// ---- 圆角（相对逻辑画面高度；9/720 即视觉 9 像素）
constexpr float kControlRadius = 9.0f / 720.0f;
constexpr float kPillRadius    = 20.0f / 720.0f;

// ---- 描边（相对逻辑画面高度；1/720 即视觉 1 像素）
constexpr float kBorderWidth = 1.0f / 720.0f;

// ---- 字号（走 setFontSize 的 1920 基准换算）
//
// 换算关系：逻辑字号 = 设计字号 × 逻辑宽 / 1920，逻辑宽 = 3840 时即 2 倍；
// 逻辑画面再被 LETTERBOX 缩到 1280 宽窗口（÷3），窗口里的视觉字号 = 设计字号 × 2/3。
constexpr short kFontLogo   = 96; // 视觉 64
constexpr short kFontBadge  = 22; // 视觉 15
constexpr short kFontButton = 24; // 视觉 16

} // namespace theme
