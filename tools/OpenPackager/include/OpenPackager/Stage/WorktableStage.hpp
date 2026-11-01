/**
 * @file WorktableStage.hpp
 * @author Naikekongwo
 * @brief 打包工坊场景 - 打开/修改 ocdata 资源包，以及提取资源文件
 * @version 0.1
 * @date 2026-11-01
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include "OpenPackager/Asset/PackageArchive.hpp"
#include "OpenPackager/Ui/ResourceTree.hpp"
#include "World/Stage/BaseStage.hpp"

#include <atomic>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

class WorktableStage : public BaseStage
{
  public:
    WorktableStage();
    ~WorktableStage() override = default;

    bool parseEvents(Event *event) override;

    void onEnter() override;
    void initializeComponents() override;

    void onUpdate() override;

    void onRender() override;

    void onExit() override;

    void onDestroy() override;

  protected:
    /// 场景状态机
    enum class WorkState
    {
        Empty,    ///< 尚未打开资源包
        Loaded,   ///< 已打开资源包
        Extracted ///< 已提取完成
    };

    /// 详情面板拆成的单行文本框数量（引擎 Text 不支持换行）
    static constexpr std::size_t kDetailRowCount = 5;

    /// 顶部工具栏高度（相对屏幕高度）
    static constexpr float kToolbarHeightRatio = 0.075f;
    /// 主内容区（左右两块面板）高度（相对屏幕高度）
    static constexpr float kListHeightRatio = 0.795f;

  private:
    // ── 场景 ──
    void            buildToolbar();
    void            buildResourceList();
    void            buildStatusBar();

    // ── 数据 ──
    PackageArchive  archive;
    WorkState       state = WorkState::Empty;

    /// 清单条目的目录树
    ResourceTree    tree;

    /// 当前选中条目在 archive.entries 中的下标；-1 表示未选中
    int             selectedIndex = -1;

    /// 列表当前展示的起始行（用于翻页，单位是「可见行」）
    std::size_t     listOffset = 0;

    /// 列表每页行数（与列表面板高度、行高匹配）
    std::size_t     listPageRows = 8;

    /// 状态栏文字与颜色
    std::string     statusText = "请选择 data 目录下要打开的 .ocdata 资源包";
    Color           statusColor{};

    // ── 文件对话框状态 ──
    // 回调由 SDL 可能在任何线程触发，这里用原子标志 + 互斥保护的路径串，
    // 真正的打开动作始终在 onUpdate（主线程）完成。
    /// 已请求打开对话框，等待结果
    std::atomic<bool> dialogPending{false};
    /// 用户取消了对话框
    std::atomic<bool> dialogCancelled{false};
    /// 回调写入的选中路径（UTF-8），由 dialogMutex 保护
    std::string       dialogResult;
    std::mutex        dialogMutex;

    // ── 列表元素 id 缓存 ──
    /// 行文字元素的 id，重建列表前用于清理
    std::vector<std::string> rowIdCache;
    /// 行点击热区的 id，重建列表前用于清理
    std::vector<std::string> rowHitIdCache;
    /// 选中行强调竖条的 id，重建列表前用于清理
    std::vector<std::string> rowBarIdCache;
    /// 行底板的 id，重建列表前用于清理
    std::vector<std::string> rowPanelIdCache;

    // ── 操作 ──

    /// 扫描 data 目录，自动打开第一个 .ocdata
    void openAuto();

    /**
     * @brief 弹出系统文件对话框，选择任意位置的 .ocdata 打开。
     * @details 走 SDL_ShowOpenFileDialog（平台无关），结果在回调里拿。
     */
    void openViaDialog();

    /// 打开指定资源包
    bool openPackage(const std::filesystem::path &ocdataPath);

    /// 重新绘制资源列表（条目变动、树展开变化后调用）
    void refreshResourceList();

    /// 刷新右侧详情的单行文本框内容
    void refreshDetailRows();

    /// 重建目录树并刷新列表
    void rebuildTree();

    /// 让当前选中项展开父目录并滚动到可见
    void revealSelection();

    /// 当前压平后的可见行（节点下标）
    std::vector<std::size_t> visibleRows() const;

    /// 在可见行里查到某个条目下标所在的行号；找不到返回 -1
    int visibleRowOfEntry(int entryIndex) const;

    /// 把当前选中的条目提取到 *extract* 目录
    void extractSelected();

    /// 把整个包提取到 *extract/<包名>* 目录
    void extractAll();

    /// 删除当前选中的条目并保存
    void removeSelected();

    /// 保存修改（重排偏移并回写）
    void savePackage();

    // ── 延迟执行的界面请求 ──
    // 按钮的回调运行在元素自己的 Draw/parseEvents 栈上，
    // 若在回调里重建列表会销毁正在执行回调的按钮自身，
    // 因此所有会改动元素集合的操作用延迟请求交给 onUpdate 执行。

    /// 按钮触发的待执行动作
    enum class Action
    {
        None,
        ExtractAll, ///< 提取整个包
        ExtractOne, ///< 提取当前选中条目
        Remove,     ///< 删除当前选中条目
        Save,       ///< 保存修改
        PageUp,     ///< 列表上一页
        PageDown    ///< 列表下一页
    };

    Action pendingAction = Action::None;

    /// 需要重建资源列表
    bool pendingRefresh = false;

    /// 重建后要把选中项滚动到可见
    bool pendingReveal = false;

    /// 执行挂起的动作
    void runPendingAction();

    /// 选中第 index 个条目（archive 下标）
    void selectEntry(int index);

    /// 选中某个树节点（目录则切换展开，文件则选中）
    void activateNode(std::size_t nodeIndex);

    /// 列表向上/向下翻页
    void scrollList(int delta);

    /// 更新状态栏
    void setStatus(std::string text, const Color &color);

    /// 状态栏文案
    void refreshStatusFromArchive();

    /// 资源类型对应的图标纹理名
    static std::string_view iconForType(ArchiveResourceType type);
};
