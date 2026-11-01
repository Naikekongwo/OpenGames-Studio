/**
 * @file WorktableStage.cpp
 * @author Naikekongwo
 * @brief 打包工坊场景的实现。
 * @version 0.1
 * @date 2026-11-01
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "OpenPackager/Stage/WorktableStage.hpp"

#include "Core/Info/GameInfo.hpp"
#include "OpenCore.hpp"
#include "OpenPackager/Ui/Theme.hpp"
#include "Runtime/Graphics/IDrawableObject/Text.hpp"
#include "Runtime/Graphics/Manager/ElementManager.hpp"
#include "Runtime/Graphics/Manager/GraphicsManager.hpp"
#include "Runtime/Graphics/UI/Panel.hpp"
#include "Runtime/Graphics/UI/TextArea.hpp"
#include "Runtime/Graphics/UI/TextButton.hpp"
#include "World/Stage/StageManager.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <format>
#include <memory>
#include <string>

namespace fs = std::filesystem;

using std::string;
using std::string_view;

namespace
{

/// 提取产物的根目录
constexpr char kExtractRoot[] = "extract";
/// 自动扫描的资源包目录
constexpr char kPackageRoot[] = "data";

/// 把路径转成可打印的 UTF-8 字符串
string pathToText(const fs::path &path)
{
    const std::u8string utf8 = path.u8string();
    return string(reinterpret_cast<const char *>(utf8.data()), utf8.size());
}

/// 以 UTF-8 语义构造路径（Windows 下避免中文路径乱码）
fs::path toPath(string_view utf8)
{
    return fs::path(reinterpret_cast<const char8_t *>(string(utf8).c_str()));
}

/// 把字节数格式化成易读文本
string humanBytes(std::size_t bytes)
{
    constexpr double kKiB = 1024.0;
    constexpr double kMiB = kKiB * 1024.0;

    const auto value = static_cast<double>(bytes);
    if (value >= kMiB)
        return std::format("{:.2f} MiB", value / kMiB);
    if (value >= kKiB)
        return std::format("{:.1f} KiB", value / kKiB);
    return std::format("{} B", bytes);
}

/// 统一的文字属性构造
TextAttribute makeTextAttribute(const Color &color, short designFontSize)
{
    TextAttribute attribute;
    attribute.fontName = "Font_UI";
    attribute.option   = RENDER_TEXT;
    attribute.color    = color;
    attribute.fontSize = designFontSize;
    return attribute;
}

} // namespace

WorktableStage::WorktableStage() : BaseStage() {}

void WorktableStage::onEnter() { initializeComponents(); }

void WorktableStage::onExit()
{
    // 有未保存的修改时提示一次，不阻断退出
    if (archive.isDirty())
        LOG("资源包尚有未保存的修改，已丢弃");

    archive.close();

    Elements->onDestroy();
    LOG("打包工坊退出");
}

void WorktableStage::onDestroy() { LOG("打包工坊已销毁"); }

bool WorktableStage::parseEvents(Event *event)
{
    Elements->parseEvents(event, timer->getTotalTime());

    // 键盘翻页与删除
    const SDL_Event &sdlEvent = event->GetSDLEvent();
    if (sdlEvent.type == SDL_EVENT_KEY_DOWN)
    {
        switch (sdlEvent.key.key)
        {
        case SDLK_UP:
            scrollList(-1);
            break;
        case SDLK_DOWN:
            scrollList(1);
            break;
        case SDLK_DELETE:
            removeSelected();
            break;
        default:
            break;
        }
    }

    return true;
}

void WorktableStage::onUpdate()
{
    // 处理文件对话框的结果（回调可能来自其它线程，只在这里落地）
    if (!dialogPending)
    {
        if (dialogCancelled)
        {
            dialogCancelled = false;
            setStatus("已取消选择", theme::TextSecondary);
        }
        else
        {
            std::string chosen;
            {
                std::lock_guard<std::mutex> lock(dialogMutex);
                if (!dialogResult.empty())
                {
                    chosen = dialogResult;
                    dialogResult.clear();
                }
            }

            if (!chosen.empty())
            {
                const auto path = toPath(chosen);
                LOG("打开对话框选择的资源包 {}", pathToText(path));
                openPackage(path);
            }
        }
    }

    // 处理按钮回调留下的延迟请求。
    // 按钮回调里不能直接重建列表，否则会销毁正在执行回调的按钮自身。
    runPendingAction();

    if (pendingRefresh)
    {
        pendingRefresh = false;

        if (pendingReveal)
        {
            pendingReveal = false;
            revealSelection();
        }

        refreshResourceList();
    }

    Elements->onUpdate(timer->getTotalTime());
    pipeline.onUpdate(timer->getTotalTime());
}

void WorktableStage::runPendingAction()
{
    const auto action = pendingAction;
    pendingAction     = Action::None;

    switch (action)
    {
    case Action::None:
        break;
    case Action::ExtractAll:
        extractAll();
        break;
    case Action::ExtractOne:
        extractSelected();
        break;
    case Action::Remove:
        removeSelected();
        break;
    case Action::Save:
        savePackage();
        break;
    case Action::PageUp:
        scrollList(-1);
        break;
    case Action::PageDown:
        scrollList(1);
        break;
    }
}

void WorktableStage::onRender() { Elements->onRender(); }

// ──────────────────────────────────────────────
//  界面搭建
// ──────────────────────────────────────────────
void WorktableStage::initializeComponents()
{
    setStatus("正在扫描 data 目录…", theme::TextSecondary);

    // 页面底色
    auto background = std::make_unique<OpenUI::Panel>("background", 0);
    background->setBackgroundColor(theme::PageBackground);
    background->Configure()
        .Parent(nullptr)
        .Anchor(AnchorPoint::TopLeft)
        .PositeR(0.0f, 0.0f)
        .ScaleR(1.0f, 1.0f);
    Elements->PushElement(std::move(background));

    buildToolbar();
    buildResourceList();
    buildStatusBar();

    // 启动后立即尝试打开一个资源包
    pipeline.next(
        [this]() -> bool
        {
            openAuto();
            return true;
        });
}

void WorktableStage::buildToolbar()
{
    // 顶部工具条：与页面底色区分度很低，只用一条细分隔线与内容区隔开，
    // 不放标题字样，让主操作按钮自然成为视觉起点。
    auto toolbar = std::make_unique<OpenUI::Panel>("toolbar", 5);
    toolbar->setBackgroundColor(theme::PageBackground);
    toolbar->Configure()
        .Parent(nullptr)
        .Anchor(AnchorPoint::TopLeft)
        .PositeR(0.0f, 0.0f)
        .ScaleR(1.0f, kToolbarHeightRatio);
    Elements->PushElement(std::move(toolbar));

    // 工具条底部的细分隔线
    auto separator = std::make_unique<OpenUI::Panel>("toolbarSeparator", 6);
    separator->setBackgroundColor(theme::PanelBorder);
    separator->Configure()
        .Parent(nullptr)
        .Anchor(AnchorPoint::TopLeft)
        .PositeR(0.0f, kToolbarHeightRatio - 0.002f)
        .ScaleR(1.0f, 0.002f);
    Elements->PushElement(std::move(separator));

    // 工具条内的控件以工具条为父容器，尺寸按工具条而非整屏计算
    auto *toolbarPtr = Elements->find("toolbar");

    // 工具按钮：提取全部（主操作） / 保存修改（次操作），自右向左排列
    constexpr float kToolButtonWidth  = 0.118f; // 相对工具条宽度
    constexpr float kToolButtonHeight = 0.56f;  // 相对工具条高度
    constexpr float kToolButtonMargin = 0.020f; // 相对工具条宽度的右边距
    constexpr float kToolButtonGap    = 0.010f; // 按钮间距

    struct ToolButtonSpec
    {
        string_view id;
        string_view text;
        float       offsetX; ///< 相对工具条宽度的左边缘位置
        bool        primary; ///< true 用实心强调色，false 用白底浅描边
    };

    constexpr float saveLeft = 1.0f - kToolButtonMargin - kToolButtonWidth;
    constexpr float allLeft  = saveLeft - kToolButtonGap - kToolButtonWidth;
    constexpr float openLeft = allLeft - kToolButtonGap - kToolButtonWidth;

    const ToolButtonSpec specs[] = {
        {"btnSave", "保存修改", saveLeft, false},
        {"btnExtractAll", "提取全部", allLeft, true},
        {"btnOpen", "打开资源包", openLeft, false},
    };

    for (const auto &spec : specs)
    {
        auto button =
            std::make_unique<OpenUI::TextButton>(spec.id, 20, spec.text);

        button->setFontSize(theme::kFontButton);
        button->setCornerRadius(theme::kControlRadius);

        if (spec.primary)
        {
            // 主操作：实心强调色
            button->setStateBackgroundColors(theme::Primary, theme::PrimaryHover,
                                             theme::PrimaryPressed);
            button->setBorderColor(theme::PrimaryBorder);
            button->setBorderWidth(theme::kBorderWidth);

            button->setNormalAttribute(
                makeTextAttribute(theme::TextInverse, theme::kFontButton));
            button->setHoveredAttribute(
                makeTextAttribute(theme::TextInverse, theme::kFontButton));
            button->setPressedAttribute(
                makeTextAttribute(theme::TextInverse, theme::kFontButton));
        }
        else
        {
            // 次操作：白底浅描边
            button->setStateBackgroundColors(
                theme::SecondaryFill, theme::SecondaryHover,
                theme::SecondaryPressed);
            button->setBorderColor(theme::SecondaryBorder);
            button->setBorderWidth(theme::kBorderWidth);

            button->setNormalAttribute(
                makeTextAttribute(theme::TextBody, theme::kFontButton));
            button->setHoveredAttribute(
                makeTextAttribute(theme::TextPrimary, theme::kFontButton));
            button->setPressedAttribute(
                makeTextAttribute(theme::TextPrimary, theme::kFontButton));
        }

        button->Configure()
            .Parent(toolbarPtr)
            .Anchor(AnchorPoint::MiddleLeft)
            .PositeR(spec.offsetX, 0.5f)
            .ScaleR(kToolButtonWidth, kToolButtonHeight);

        Elements->PushElement(std::move(button));
    }

    // 挂接回调（按 id 找回控件再绑定，避免在构建阶段捕获悬空指针）
    if (auto *saveBtn =
            dynamic_cast<OpenUI::TextButton *>(Elements->find("btnSave")))
    {
        saveBtn->setOnClick([this]() { pendingAction = Action::Save; });
    }
    if (auto *allBtn =
            dynamic_cast<OpenUI::TextButton *>(Elements->find("btnExtractAll")))
    {
        allBtn->setOnClick([this]() { pendingAction = Action::ExtractAll; });
    }
    if (auto *openBtn =
            dynamic_cast<OpenUI::TextButton *>(Elements->find("btnOpen")))
    {
        openBtn->setOnClick([this]() { openViaDialog(); });
    }
}

void WorktableStage::buildResourceList()
{
    // 主内容区的左右边距与面板间距
    constexpr float kSideMargin   = 0.018f; ///< 相对屏幕宽度
    constexpr float kPanelGap     = 0.014f; ///< 相对屏幕宽度
    constexpr float kContentTop   = kToolbarHeightRatio + 0.014f;
    constexpr float kListWidth    = 0.600f; ///< 左侧列表占屏幕宽度
    constexpr float kDetailLeft   = kSideMargin + kListWidth + kPanelGap;
    constexpr float kDetailWidth  = 1.0f - kDetailLeft - kSideMargin;

    // 列表底板
    auto listPanel = std::make_unique<OpenUI::Panel>("listPanel", 5);
    listPanel->setBackgroundColor(theme::PanelBackground);
    listPanel->setCornerRadius(theme::kPanelRadius);
    listPanel->setBorderColor(theme::PanelBorder);
    listPanel->setBorderWidth(theme::kBorderWidth);
    listPanel->Configure()
        .Parent(nullptr)
        .Anchor(AnchorPoint::TopLeft)
        .PositeR(kSideMargin, kContentTop)
        .ScaleR(kListWidth, kListHeightRatio);
    Elements->PushElement(std::move(listPanel));

    // 右侧详情底板
    auto detailPanel = std::make_unique<OpenUI::Panel>("detailPanel", 5);
    detailPanel->setBackgroundColor(theme::PanelBackground);
    detailPanel->setCornerRadius(theme::kPanelRadius);
    detailPanel->setBorderColor(theme::PanelBorder);
    detailPanel->setBorderWidth(theme::kBorderWidth);
    detailPanel->Configure()
        .Parent(nullptr)
        .Anchor(AnchorPoint::TopLeft)
        .PositeR(kDetailLeft, kContentTop)
        .ScaleR(kDetailWidth, kListHeightRatio);
    Elements->PushElement(std::move(detailPanel));

    auto *listPanelPtr   = Elements->find("listPanel");
    auto *detailPanelPtr = Elements->find("detailPanel");

    // 列表标题
    auto listTitle =
        std::make_unique<OpenUI::TextArea>("listTitle", 10, "Font_UI");
    listTitle->setAttribute(
        makeTextAttribute(theme::TextPrimary, theme::kFontHeading));
    listTitle->setText("包内资源");
    listTitle->align(AnchorPoint::MiddleLeft);
    listTitle->Configure()
        .Parent(listPanelPtr)
        .Anchor(AnchorPoint::TopLeft)
        .PositeR(0.025f, 0.020f)
        .ScaleR(0.500f, 0.055f);
    Elements->PushElement(std::move(listTitle));

    // 详情标题
    auto detailTitle =
        std::make_unique<OpenUI::TextArea>("detailTitle", 10, "Font_UI");
    detailTitle->setAttribute(
        makeTextAttribute(theme::TextPrimary, theme::kFontHeading));
    detailTitle->setText("资源详情");
    detailTitle->align(AnchorPoint::MiddleLeft);
    detailTitle->Configure()
        .Parent(detailPanelPtr)
        .Anchor(AnchorPoint::TopLeft)
        .PositeR(0.025f, 0.020f)
        .ScaleR(0.500f, 0.055f);
    Elements->PushElement(std::move(detailTitle));

    // 详情正文：拆成多个单行文本框（引擎 Text 不支持换行）
    constexpr float kDetailRowTop    = 0.100f; ///< 相对详情面板高度
    constexpr float kDetailRowHeight = 0.056f; ///< 相对详情面板高度

    for (std::size_t i = 0; i < kDetailRowCount; ++i)
    {
        auto row = std::make_unique<OpenUI::TextArea>(
            "detailRow" + std::to_string(i), 10, "Font_UI");
        row->setAttribute(makeTextAttribute(theme::TextBody, theme::kFontBody));
        row->setText("");
        row->align(AnchorPoint::MiddleLeft);
        row->Configure()
            .Parent(detailPanelPtr)
            .Anchor(AnchorPoint::TopLeft)
            .PositeR(0.040f,
                     kDetailRowTop + static_cast<float>(i) * kDetailRowHeight)
            .ScaleR(0.920f, kDetailRowHeight * 0.75f);
        Elements->PushElement(std::move(row));
    }

    // 操作按钮：提取选中 / 删除条目（详情面板内），翻页按钮（列表面板内）
    struct ActionSpec
    {
        string_view id;
        string_view text;
        float       offsetX; ///< 相对父面板宽度的左边缘
        float       offsetY; ///< 相对父面板高度的上边缘
        float       width;   ///< 相对父面板宽度
    };

    const ActionSpec detailActions[] = {
        {"btnExtract", "提取选中", 0.040f, 0.880f, 0.440f},
        {"btnRemove", "删除条目", 0.520f, 0.880f, 0.440f},
    };

    const ActionSpec listActions[] = {
        {"btnPageUp", "上一页", 0.040f, 0.900f, 0.230f},
        {"btnPageDown", "下一页", 0.300f, 0.900f, 0.230f},
    };

    auto makeActionButton = [this](const ActionSpec &action,
                                   IDrawableObject *parent) -> void
    {
        auto button =
            std::make_unique<OpenUI::TextButton>(action.id, 20, action.text);

        button->setFontSize(theme::kFontButton);
        button->setStateBackgroundColors(theme::SecondaryFill,
                                        theme::SecondaryHover,
                                        theme::SecondaryPressed);
        button->setCornerRadius(theme::kControlRadius);
        button->setBorderColor(theme::SecondaryBorder);
        button->setBorderWidth(theme::kBorderWidth);
        button->setNormalAttribute(
            makeTextAttribute(theme::TextBody, theme::kFontButton));
        button->setHoveredAttribute(
            makeTextAttribute(theme::TextPrimary, theme::kFontButton));
        button->setPressedAttribute(
            makeTextAttribute(theme::TextPrimary, theme::kFontButton));

        button->Configure()
            .Parent(parent)
            .Anchor(AnchorPoint::TopLeft)
            .PositeR(action.offsetX, action.offsetY)
            .ScaleR(action.width, 0.072f);

        Elements->PushElement(std::move(button));
    };

    for (const auto &action : detailActions)
        makeActionButton(action, detailPanelPtr);
    for (const auto &action : listActions)
        makeActionButton(action, listPanelPtr);

    if (auto *extractBtn =
            dynamic_cast<OpenUI::TextButton *>(Elements->find("btnExtract")))
    {
        extractBtn->setOnClick([this]()
                               { pendingAction = Action::ExtractOne; });
    }
    if (auto *removeBtn =
            dynamic_cast<OpenUI::TextButton *>(Elements->find("btnRemove")))
    {
        removeBtn->setOnClick([this]() { pendingAction = Action::Remove; });
    }
    if (auto *pageUp =
            dynamic_cast<OpenUI::TextButton *>(Elements->find("btnPageUp")))
    {
        pageUp->setOnClick([this]() { pendingAction = Action::PageUp; });
    }
    if (auto *pageDown =
            dynamic_cast<OpenUI::TextButton *>(Elements->find("btnPageDown")))
    {
        pageDown->setOnClick([this]() { pendingAction = Action::PageDown; });
    }
}

void WorktableStage::buildStatusBar()
{
    auto status =
        std::make_unique<OpenUI::TextArea>("statusBar", 30, "Font_UI");
    status->setAttribute(
        makeTextAttribute(theme::TextSecondary, theme::kFontBody));
    status->setText(statusText);
    status->align(AnchorPoint::MiddleLeft);
    status->Configure()
        .Parent(nullptr)
        .Anchor(AnchorPoint::TopLeft)
        .PositeR(0.020f, 0.945f)
        .ScaleR(0.960f, 0.035f);
    Elements->PushElement(std::move(status));
}

// ──────────────────────────────────────────────
//  资源列表
// ──────────────────────────────────────────────
string_view WorktableStage::iconForType(ArchiveResourceType type)
{
    switch (type)
    {
    case ArchiveResourceType::Texture:
        return "ICON_TEXTURE";
    case ArchiveResourceType::Audio:
        return "ICON_AUDIO";
    case ArchiveResourceType::Font:
        return "ICON_FONT";
    case ArchiveResourceType::Asset:
        return "ICON_TEXTURE";
    }
    return "ICON_TEXTURE";
}

void WorktableStage::refreshResourceList()
{
    // 清掉上一轮生成的行
    for (const auto &id : rowIdCache)
        Elements->removeElement(id);
    rowIdCache.clear();

    for (const auto &id : rowHitIdCache)
        Elements->removeElement(id);
    rowHitIdCache.clear();

    for (const auto &id : rowBarIdCache)
        Elements->removeElement(id);
    rowBarIdCache.clear();

    for (const auto &id : rowPanelIdCache)
        Elements->removeElement(id);
    rowPanelIdCache.clear();

    const auto &entries = archive.getEntries();
    const auto  rows    = visibleRows();
    const auto  total   = rows.size();

    // 偏移越界时回拉
    if (total <= listPageRows)
        listOffset = 0;
    else if (listOffset + listPageRows > total)
        listOffset = total - listPageRows;

    // 行以列表面板为父容器：面板内从标题下方开始，留出翻页按钮的高度
    auto *listPanelPtr = Elements->find("listPanel");

    constexpr float kRowTop    = 0.075f; ///< 第一行相对面板高度的上边缘
    constexpr float kRowHeight = 0.088f; ///< 单行高度（相对面板高度）

    for (std::size_t row = 0; row < listPageRows; ++row)
    {
        const auto visibleIndex = listOffset + row;
        const bool exists       = visibleIndex < total;

        const string rowId = "row_" + std::to_string(row);
        rowIdCache.push_back(rowId);

        const float rowTop = kRowTop + static_cast<float>(row) * kRowHeight;

        if (!exists)
            continue;

        const auto nodeIndex = rows[visibleIndex];
        const auto &node     = tree.allNodes()[nodeIndex];
        const auto  depth    = tree.depthOf(nodeIndex);
        const bool  isDir    = node.isDirectory();

        // 文件节点才对应 archive 条目
        const ArchiveEntry *entry =
            isDir ? nullptr : &entries[static_cast<std::size_t>(node.entryIndex)];

        const bool isSelected =
            entry != nullptr && node.entryIndex == selectedIndex;

        // 行底板：浅灰底 + 极浅描边，在白面板上形成可辨识的行块
        auto rowPanel = std::make_unique<OpenUI::Panel>(
            "panel_" + std::to_string(row), 8);
        rowPanel->setCornerRadius(theme::kRowRadius);
        rowPanel->setBackgroundColor(isSelected ? theme::RowSelect
                                                : theme::RowNormal);
        rowPanel->setBorderColor(theme::RowBorder);
        rowPanel->setBorderWidth(theme::kBorderWidth);
        rowPanel->Configure()
            .Parent(listPanelPtr)
            .Anchor(AnchorPoint::TopLeft)
            .PositeR(0.020f, rowTop)
            .ScaleR(0.960f, kRowHeight - 0.012f);
        Elements->PushElement(std::move(rowPanel));
        rowPanelIdCache.push_back("panel_" + std::to_string(row));

        // 选中行左侧的强调竖条
        if (isSelected)
        {
            auto selectBar = std::make_unique<OpenUI::Panel>(
                "selBar_" + std::to_string(row), 10);
            selectBar->setBackgroundColor(theme::RowSelectBar);
            selectBar->Configure()
                .Parent(listPanelPtr)
                .Anchor(AnchorPoint::TopLeft)
                .PositeR(0.020f, rowTop)
                .ScaleR(0.006f, kRowHeight - 0.012f);
            Elements->PushElement(std::move(selectBar));
            rowBarIdCache.push_back("selBar_" + std::to_string(row));
        }

        // 层级缩进：每级向右让出一段
        constexpr float kIndentStep = 0.030f;
        const float     indentX = 0.028f + static_cast<float>(depth) * kIndentStep;

        // 目录显示展开状态与子项数，文件显示类型与大小
        // 注意：思源黑体 Heavy 不含 ▾/▸ 等三角字形，这里用基础符号表示
        string label;
        if (isDir)
        {
            label = node.expanded ? "v  " : ">  ";
            label += node.name;
            label += "    · ";
            label += std::to_string(node.children.size());
            label += " 项";
        }
        else
        {
            label = std::format("{}    · {} · {}", entry->name,
                                ArchiveTypeToString(entry->type),
                                humanBytes(static_cast<std::size_t>(
                                    std::max(entry->size(), 0))));
        }

        auto rowText =
            std::make_unique<OpenUI::TextArea>(rowId, 12, "Font_UI");
        rowText->setAttribute(makeTextAttribute(
            isDir ? theme::TextSecondary : theme::TextPrimary,
            theme::kFontSmall));
        rowText->setText(label);
        rowText->align(AnchorPoint::MiddleLeft);
        rowText->Configure()
            .Parent(listPanelPtr)
            .Anchor(AnchorPoint::TopLeft)
            .PositeR(indentX, rowTop + 0.012f)
            .ScaleR(0.925f - static_cast<float>(depth) * kIndentStep,
                    kRowHeight - 0.032f);
        Elements->PushElement(std::move(rowText));

        // 行点击热区：透明文字按钮铺满整行，仅用于接收点击
        const string hitId = "hit_" + std::to_string(row);
        auto         hit   = std::make_unique<OpenUI::TextButton>(hitId, 14, "");
        hit->Configure()
            .Parent(listPanelPtr)
            .Anchor(AnchorPoint::TopLeft)
            .PositeR(0.020f, rowTop)
            .ScaleR(0.960f, kRowHeight - 0.012f);

        const auto capturedNode = nodeIndex;
        hit->setOnClick([this, capturedNode]() { activateNode(capturedNode); });

        Elements->PushElement(std::move(hit));
        rowHitIdCache.push_back(hitId);
    }

    // 详情刷新
    // 注意：引擎的 Text 用 TTF_RenderText 一次性渲染整段，不支持 \n 换行，
    // 换行符会被画成缺字形方框。因此详情拆成若干固定数量的单行文本框。
    refreshDetailRows();
}

std::vector<std::size_t> WorktableStage::visibleRows() const
{
    return tree.flattenVisible();
}

int WorktableStage::visibleRowOfEntry(int entryIndex) const
{
    if (entryIndex < 0)
        return -1;

    const auto rows = visibleRows();
    for (std::size_t i = 0; i < rows.size(); ++i)
    {
        if (tree.allNodes()[rows[i]].entryIndex == entryIndex)
            return static_cast<int>(i);
    }
    return -1;
}

void WorktableStage::activateNode(std::size_t nodeIndex)
{
    const auto &nodes = tree.allNodes();
    if (nodeIndex >= nodes.size())
        return;

    // 注意：这里不直接刷新列表。
    // 行点击热区会在自己的回调里调用本函数，若此时重建列表，
    // 会销毁正在执行回调的那个按钮对象，导致栈上对象被析构而崩溃。
    // 因此只记录请求，由下一帧 onUpdate 安全地执行。

    LOG("激活节点 {}（{}）", nodes[nodeIndex].name,
        nodes[nodeIndex].isDirectory() ? "目录" : "文件");

    if (nodes[nodeIndex].isDirectory())
    {
        tree.toggleExpanded(nodeIndex);
        pendingRefresh = true;
        return;
    }

    selectedIndex  = nodes[nodeIndex].entryIndex;
    pendingRefresh = true;
    pendingReveal  = true;
}

void WorktableStage::revealSelection()
{
    if (selectedIndex < 0)
        return;

    // 选中项的父目录可能被收起，先展开到它
    for (std::size_t i = 0; i < tree.size(); ++i)
    {
        if (tree.allNodes()[i].entryIndex == selectedIndex)
        {
            tree.expandPathTo(i);
            break;
        }
    }

    // 让选中行落入可视范围
    const auto row = visibleRowOfEntry(selectedIndex);
    if (row < 0)
        return;

    const auto rowPos = static_cast<std::size_t>(row);
    if (rowPos < listOffset)
        listOffset = rowPos;
    else if (rowPos >= listOffset + listPageRows)
        listOffset = rowPos - listPageRows + 1;
}

void WorktableStage::rebuildTree()
{
    tree.rebuild(archive.getEntries());
}

void WorktableStage::refreshDetailRows()
{
    const auto &entries = archive.getEntries();

    // 组装每一行的文字内容
    std::vector<string> lines(kDetailRowCount, "");

    if (selectedIndex < 0 ||
        static_cast<std::size_t>(selectedIndex) >= entries.size())
    {
        lines[0] = "未选中任何资源";
        lines[2] = "用方向键或点击列表选中条目";
    }
    else
    {
        const auto &entry = entries[static_cast<std::size_t>(selectedIndex)];
        lines[0] = "名称：" + entry.name;
        lines[1] = "类型：" + string(ArchiveTypeToString(entry.type));
        lines[2] = "大小：" + humanBytes(static_cast<std::size_t>(
                               std::max(entry.size(), 0)));
        lines[3] = "偏移：[";
        lines[3] += std::format("{}, {})", entry.startIndex, entry.endIndex);
        lines[4] = "路径：";
        lines[4] += entry.filePath.empty() ? "（未保留）" : entry.filePath;
    }

    for (std::size_t i = 0; i < kDetailRowCount; ++i)
    {
        auto *row = dynamic_cast<OpenUI::TextArea *>(
            Elements->find("detailRow" + std::to_string(i)));
        if (!row)
            continue;

        // 第一行用主色突出资源名，其余用正文色
        row->setAttribute(makeTextAttribute(i == 0 ? theme::TextPrimary
                                                   : theme::TextBody,
                                            theme::kFontBody));
        row->setText(lines[i]);
    }
}

// ──────────────────────────────────────────────
//  打开
// ──────────────────────────────────────────────
void WorktableStage::openAuto()
{
    std::error_code ec;
    const auto      root = toPath(kPackageRoot);

    if (!fs::exists(root, ec) || !fs::is_directory(root, ec))
    {
        setStatus("未找到 data 目录，请把要处理的 .ocdata 放在 data 下",
                  theme::Warning);
        return;
    }

    // 扫描 data 下的第一个 .ocdata
    std::vector<fs::path> candidates;
    for (const auto &item : fs::directory_iterator(root, ec))
    {
        if (!item.is_regular_file(ec))
            continue;
        if (item.path().extension() == toPath(".ocdata"))
            candidates.push_back(item.path());
    }

    std::sort(candidates.begin(), candidates.end());

    if (candidates.empty())
    {
        setStatus("data 目录下没有 .ocdata 资源包", theme::Warning);
        return;
    }

    LOG("发现 {} 个资源包，打开第一个", static_cast<int>(candidates.size()));
    openPackage(candidates.front());
}

void WorktableStage::openViaDialog()
{
    // SDL 的文件对话框是异步的：函数立即返回，结果通过回调送回。
    // 回调可能在其它线程执行，因此只写原子标志与加锁的路径串，
    // 真正的打开动作交给主线程的 onUpdate。
    {
        std::lock_guard<std::mutex> lock(dialogMutex);
        dialogResult.clear();
    }
    dialogCancelled = false;
    dialogPending   = true;

    SDL_DialogFileFilter filters[] = {
        {"OpenCore 资源包", "ocdata"},
        {"所有文件", "*"},
    };

    SDL_ShowOpenFileDialog(
        [](void *userdata, const char *const *filelist, int /*filter*/)
        {
            auto *self = static_cast<WorktableStage *>(userdata);
            if (!self)
                return;

            if (!filelist || !filelist[0])
            {
                self->dialogCancelled = true;
                self->dialogPending   = false;
                return;
            }

            {
                std::lock_guard<std::mutex> lock(self->dialogMutex);
                self->dialogResult = filelist[0];
            }
            self->dialogPending = false;
        },
        this, OpenCoreManagers::GFXManager.getWindow(), filters, 2, nullptr,
        false);

    setStatus("请在弹出的对话框中选择 .ocdata 文件…", theme::TextSecondary);
}

bool WorktableStage::openPackage(const fs::path &ocdataPath)
{
    selectedIndex = -1;
    listOffset    = 0;

    if (!archive.open(ocdataPath))
    {
        setStatus("打开失败：" +
                      (archive.getWarnings().empty()
                           ? string("未知原因")
                           : archive.getWarnings().front()),
                  theme::Danger);
        state = WorkState::Empty;
        refreshResourceList();
        return false;
    }

    state = WorkState::Loaded;
    rebuildTree();
    refreshResourceList();
    refreshStatusFromArchive();
    return true;
}

void WorktableStage::refreshStatusFromArchive()
{
    const auto count  = archive.getEntries().size();
    const auto issues = archive.validate();
    const auto undecl = archive.countUndeclaredBytes();
    const auto &name  = archive.getPackageName();

    if (!archive.hasManifest())
    {
        setStatus(std::format("已打开 {}（{} 字节）但缺少清单，只能按数据段整体导出",
                              name, archive.getDataLength()),
                  theme::Warning);
        return;
    }

    if (!issues.empty())
    {
        setStatus(std::format("已打开 {}：{} 个资源，发现 {} 处清单问题：{}", name,
                              static_cast<int>(count),
                              static_cast<int>(issues.size()), issues.front()),
                  theme::Danger);
        return;
    }

    if (undecl > 0)
    {
        setStatus(std::format("已打开 {}：{} 个资源，另有 {} 未声明的数据", name,
                              static_cast<int>(count), humanBytes(undecl)),
                  theme::Warning);
        return;
    }

    setStatus(std::format("已打开 {}：{} 个资源，数据段 {}，清单完整", name,
                          static_cast<int>(count),
                          humanBytes(archive.getDataLength())),
              theme::Success);
}

// ──────────────────────────────────────────────
//  选择与翻页
// ──────────────────────────────────────────────
void WorktableStage::selectEntry(int index)
{
    const auto total = static_cast<int>(archive.getEntries().size());
    if (total == 0)
        return;

    selectedIndex = std::clamp(index, 0, total - 1);

    // 选中项的父目录可能被收起，先保证它在可见范围内
    const auto rows = visibleRows();
    int        rowInView = -1;
    for (std::size_t i = 0; i < rows.size(); ++i)
    {
        if (tree.allNodes()[rows[i]].entryIndex == selectedIndex)
        {
            rowInView = static_cast<int>(i);
            break;
        }
    }

    if (rowInView < 0)
    {
        // 被收起了，展开父目录后重算
        for (std::size_t i = 0; i < tree.size(); ++i)
        {
            if (tree.allNodes()[i].entryIndex == selectedIndex)
            {
                tree.expandPathTo(i);
                break;
            }
        }
        const auto retryRows = visibleRows();
        rowInView            = -1;
        for (std::size_t i = 0; i < retryRows.size(); ++i)
        {
            if (tree.allNodes()[retryRows[i]].entryIndex == selectedIndex)
            {
                rowInView = static_cast<int>(i);
                break;
            }
        }
    }

    // 选中项滚出可视范围时跟随滚动
    if (rowInView >= 0)
    {
        const auto rowPos = static_cast<std::size_t>(rowInView);
        if (rowPos < listOffset)
            listOffset = rowPos;
        else if (rowPos >= listOffset + listPageRows)
            listOffset = rowPos - listPageRows + 1;
    }

    refreshResourceList();
}

void WorktableStage::scrollList(int delta)
{
    const auto rows = visibleRows();
    if (rows.empty())
        return;

    // 没有选中项时从第一行开始
    int current = visibleRowOfEntry(selectedIndex);
    if (current < 0)
        current = 0;
    else
        current += delta;

    current = std::clamp(current, 0, static_cast<int>(rows.size()) - 1);

    // 目录行本身不可「选中」，向下找最近的文件行
    auto pickFileEntry = [this, &rows](int from, int step) -> int
    {
        for (int i = from; i >= 0 && i < static_cast<int>(rows.size()); i += step)
        {
            const auto &node = tree.allNodes()[rows[static_cast<std::size_t>(i)]];
            if (!node.isDirectory())
                return node.entryIndex;
        }
        return -1;
    };

    int entry = pickFileEntry(current, 1);
    if (entry < 0)
        entry = pickFileEntry(current, -1);
    if (entry < 0)
        return;

    selectEntry(entry);
}

// ──────────────────────────────────────────────
//  提取
// ──────────────────────────────────────────────
void WorktableStage::extractSelected()
{
    if (!archive.isOpen())
    {
        setStatus("尚未打开资源包，无法提取", theme::Danger);
        return;
    }

    if (selectedIndex < 0)
    {
        setStatus("请先在列表中选择要提取的资源", theme::Warning);
        return;
    }

    const auto  index = static_cast<std::size_t>(selectedIndex);
    const auto &entry = archive.getEntries()[index];

    const auto outputDir =
        toPath(kExtractRoot) / toPath(archive.getPackageName());

    const auto written = archive.extractEntry(index, outputDir);
    if (!written)
    {
        setStatus("提取失败：" + entry.name, theme::Danger);
        return;
    }

    state = WorkState::Extracted;
    setStatus("已提取 " + entry.name + " 到 " + pathToText(*written),
              theme::Success);
}

void WorktableStage::extractAll()
{
    if (!archive.isOpen())
    {
        setStatus("尚未打开资源包，无法提取", theme::Danger);
        return;
    }

    if (!archive.hasManifest())
    {
        setStatus("缺少清单，无法按资源提取", theme::Danger);
        return;
    }

    const auto outputDir =
        toPath(kExtractRoot) / toPath(archive.getPackageName());

    std::vector<fs::path> written;
    const auto            count = archive.extractAll(outputDir, true, &written);

    const auto total = archive.getEntries().size();
    if (count == 0)
    {
        setStatus("提取失败，没有写出任何文件", theme::Danger);
        return;
    }

    state = WorkState::Extracted;

    if (count < total)
    {
        setStatus(std::format("提取完成 {}/{} 个资源到 {}，其余读取失败",
                              static_cast<int>(count), static_cast<int>(total),
                              pathToText(outputDir)),
                  theme::Warning);
    }
    else
    {
        setStatus(std::format("提取完成：{} 个资源已写出到 {}", count,
                              pathToText(outputDir)),
                  theme::Success);
    }
}

// ──────────────────────────────────────────────
//  删除与保存
// ──────────────────────────────────────────────
void WorktableStage::removeSelected()
{
    if (!archive.isOpen())
    {
        setStatus("尚未打开资源包，无法删除", theme::Danger);
        return;
    }

    if (selectedIndex < 0)
    {
        setStatus("请先在列表中选择要删除的资源", theme::Warning);
        return;
    }

    const auto index = static_cast<std::size_t>(selectedIndex);
    const auto name  = archive.getEntries()[index].name;

    if (!archive.removeEntry(index))
    {
        setStatus("删除失败：" + name, theme::Danger);
        return;
    }

    // 删掉最后一条时把选中项往前挪
    const auto remaining = static_cast<int>(archive.getEntries().size());
    selectedIndex =
        remaining == 0 ? -1 : std::min(selectedIndex, remaining - 1);

    // 条目表变了，树与选中下标都要跟着更新
    rebuildTree();
    if (selectedIndex >= 0 && archive.getEntries().empty())
        selectedIndex = -1;

    refreshResourceList();
    setStatus("已从清单移除 " + name + "，点击「保存修改」写回文件",
              theme::Warning);
}

void WorktableStage::savePackage()
{
    if (!archive.isOpen())
    {
        setStatus("尚未打开资源包，无法保存", theme::Danger);
        return;
    }

    if (!archive.isDirty())
    {
        setStatus("没有需要保存的修改", theme::TextSecondary);
        return;
    }

    string error;
    if (!archive.save(true, &error))
    {
        setStatus("保存失败：" + error, theme::Danger);
        return;
    }

    refreshResourceList();
    setStatus(std::format("已保存 {}：数据段 {}，原文件已备份为 .bak",
                          archive.getPackageName(),
                          humanBytes(archive.getDataLength())),
              theme::Success);
}

// ──────────────────────────────────────────────
//  状态栏
// ──────────────────────────────────────────────
void WorktableStage::setStatus(string text, const Color &color)
{
    statusText  = std::move(text);
    statusColor = color;

    if (auto *bar =
            dynamic_cast<OpenUI::TextArea *>(Elements->find("statusBar")))
    {
        TextAttribute attribute = makeTextAttribute(color, theme::kFontBody);
        bar->setAttribute(attribute);
        bar->setText(statusText);
    }
}
