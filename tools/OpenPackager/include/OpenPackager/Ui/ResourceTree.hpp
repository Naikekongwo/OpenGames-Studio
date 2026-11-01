/**
 * @file ResourceTree.hpp
 * @author Naikekongwo
 * @brief 把 ocdata 清单压成按目录层级组织的树，供界面展示。
 * @version 0.1
 * @date 2026-11-01
 *
 * @details 清单里的条目带原始路径（如 assets/ui/font/xxx.otf），
 *          这里按「/」切分把路径还原成目录树。没有原始路径的条目
 *          统一挂到一个虚拟的「(未分类)」根目录下，保证都能被看到。
 *
 *          树只保存结构与下标，不复制条目本身；叶子节点的 entryIndex
 *          指向 PackageArchive 的 entries 下标。
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include "OpenPackager/Asset/PackageArchive.hpp"

#include <cstddef>
#include <string>
#include <vector>

/**
 * @brief 树中的一个节点（目录或文件）。
 */
struct TreeNode
{
    std::string name; ///< 显示名（目录名或文件名）
    std::string path; ///< 从根到本节点的完整路径，用于详情展示

    /// 指向 archive 条目的下标；目录为 -1
    int entryIndex = -1;

    /// 父节点下标；根节点为 -1
    int parent = -1;

    /// 子节点下标
    std::vector<std::size_t> children;

    /// 目录是否展开（文件节点恒为 false）
    bool expanded = true;

    /// 是否目录
    bool isDirectory() const { return entryIndex < 0; }
};

/**
 * @brief 由清单条目构建的目录树。
 */
class ResourceTree
{
  public:
    ResourceTree() = default;

    /**
     * @brief 按 entries 重建整棵树。
     * @details 会尽量保留同名目录的展开状态，避免刷新后全部收起。
     * @param entries 清单条目
     */
    void rebuild(const std::vector<ArchiveEntry> &entries);

    /// 清空
    void clear();

    /// 根节点（虚拟根，name 为空）
    const TreeNode &root() const { return nodes[rootIndex_]; }

    /// 全部节点
    const std::vector<TreeNode> &allNodes() const { return nodes; }

    /// 节点总数
    std::size_t size() const { return nodes.size(); }

    /**
     * @brief 按当前展开状态，把可见节点按显示顺序压平成列表。
     * @details 目录先于其子项，同级按目录在前、名称升序排列。
     * @return 可见节点下标序列
     */
    std::vector<std::size_t> flattenVisible() const;

    /**
     * @brief 节点在树中的深度（根的子节点为 0）。
     */
    int depthOf(std::size_t index) const;

    /**
     * @brief 切换目录的展开/收起。文件节点无效果。
     * @return 是否是需要刷新的目录
     */
    bool toggleExpanded(std::size_t index);

    /**
     * @brief 依次展开某条路径上的所有目录（用于选中后展开父级）。
     */
    void expandPathTo(std::size_t index);

    /**
     * @brief 当前展开状态快照，用于 rebuild 后恢复。
     */
    std::vector<std::string> expandedPaths() const;

  private:
    std::vector<TreeNode> nodes;
    std::size_t           rootIndex_ = 0;

    /// 收集当前所有展开的目录路径
    void collectExpanded(std::size_t index, std::vector<std::string> &out) const;

    /// 在 parent 下查找同名子目录，找到返回下标，否则 -1
    int findChildDir(std::size_t parent, const std::string &name) const;

    /// 在 parent 下新建一个子节点
    std::size_t addChild(std::size_t parent, std::string name, std::string path);

    /**
     * @brief 对每个节点的子项排序：目录在前，然后按名称升序。
     */
    void sortChildren();
};
