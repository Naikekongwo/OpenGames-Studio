/**
 * @file Theme.hpp
 * @author Naikekongwo
 * @brief OpenPackager 的界面主题常量（浅色）
 * @version 0.2
 * @date 2026-11-01
 *
 * @details 浅色简约风格：大面积留白、单一强调色、克制的细边框，
 *          用层次（底色 / 面板 / 卡片）而不是边框来划分区域。
 *
 *          视觉基准按 1280×720 计算；圆角与描边宽度以「相对逻辑画面高度」
 *          交给引擎（1/720 即视觉 1 像素），字号走 setFontSize() 的 1920
 *          基准换算。
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include "Core/Math/OpenCore_Color.hpp"

namespace theme
{

// ---- 底色层次
// 页面底色略微带蓝灰，面板纯白浮起，形成自然的层次感
inline const Color PageBackground =
    Color(246.f / 255.f, 247.f / 255.f, 249.f / 255.f, 1.f);
inline const Color PanelBackground = Color(1.f, 1.f, 1.f, 1.f);
inline const Color PanelBorder =
    Color(228.f / 255.f, 231.f / 255.f, 236.f / 255.f, 1.f);
// ---- 文字
inline const Color TextPrimary =
    Color(28.f / 255.f, 32.f / 255.f, 40.f / 255.f, 1.f);
inline const Color TextBody =
    Color(64.f / 255.f, 72.f / 255.f, 84.f / 255.f, 1.f);
inline const Color TextSecondary =
    Color(108.f / 255.f, 116.f / 255.f, 130.f / 255.f, 1.f);
inline const Color TextHint =
    Color(158.f / 255.f, 166.f / 255.f, 178.f / 255.f, 1.f);
inline const Color TextInverse = Color(1.f, 1.f, 1.f, 1.f);

// ---- 强调色：单一克制的靛蓝，只用在主操作与选中态
inline const Color Primary = Color(59.f / 255.f, 110.f / 255.f, 241.f / 255.f, 1.f);
inline const Color PrimaryHover =
    Color(47.f / 255.f, 94.f / 255.f, 220.f / 255.f, 1.f);
inline const Color PrimaryPressed =
    Color(38.f / 255.f, 78.f / 255.f, 190.f / 255.f, 1.f);

// ---- 主按钮把强调色收进描边，填充保持克制
inline const Color PrimaryBorder = Color(59.f / 255.f, 110.f / 255.f, 241.f / 255.f, 1.f);

// ---- 次按钮：白底 + 浅描边，悬停时微微加深
inline const Color SecondaryFill    = Color(1.f, 1.f, 1.f, 1.f);
inline const Color SecondaryHover   = Color(243.f / 255.f, 245.f / 255.f, 248.f / 255.f, 1.f);
inline const Color SecondaryPressed = Color(235.f / 255.f, 238.f / 255.f, 243.f / 255.f, 1.f);
inline const Color SecondaryBorder  = Color(218.f / 255.f, 222.f / 255.f, 229.f / 255.f, 1.f);

// ---- 列表行：白面板上给行一层极浅的灰底，靠底色而非斑马纹区分
inline const Color RowNormal   = Color(250.f / 255.f, 251.f / 255.f, 253.f / 255.f, 1.f);
inline const Color RowHover    = Color(243.f / 255.f, 246.f / 255.f, 251.f / 255.f, 1.f);
inline const Color RowSelect   = Color(234.f / 255.f, 241.f / 255.f, 254.f / 255.f, 1.f);
inline const Color RowSelectBar = Color(59.f / 255.f, 110.f / 255.f, 241.f / 255.f, 1.f);
inline const Color RowBorder   = Color(240.f / 255.f, 242.f / 255.f, 246.f / 255.f, 1.f);

// ---- 状态色（浅色底上需要更深的色值才看得清）
inline const Color Success = Color(16.f / 255.f, 138.f / 255.f, 86.f / 255.f, 1.f);
inline const Color Warning = Color(180.f / 255.f, 118.f / 255.f, 20.f / 255.f, 1.f);
inline const Color Danger  = Color(206.f / 255.f, 55.f / 255.f, 55.f / 255.f, 1.f);

// ---- 圆角与描边（相对逻辑画面高度）
constexpr float kControlRadius = 8.0f / 720.0f;
constexpr float kPanelRadius   = 12.0f / 720.0f;
constexpr float kRowRadius     = 7.0f / 720.0f;
constexpr float kBorderWidth   = 1.0f / 720.0f;
/// 选中行左侧的强调竖条宽度（相对逻辑画面高度）
constexpr float kSelectBarWidth = 3.0f / 720.0f;

// ---- 字号（走 setFontSize 的 1920 基准换算；数值为设计基准字号）
// 注意：设计字号 × 逻辑宽 / 1920 → 逻辑字号，逻辑画面再缩到 1280 宽窗口（÷3），
// 所以窗口里看到的视觉字号约为设计字号的 2/3。这里按 4K 画布取值，
// 保证 1280×720 窗口下仍有足够可读性。
constexpr short kFontHeading = 46; // 视觉 31，面板标题
constexpr short kFontBody    = 42; // 视觉 28，正文/状态栏
constexpr short kFontSmall   = 38; // 视觉 25，列表行
constexpr short kFontButton  = 42; // 视觉 28

} // namespace theme
