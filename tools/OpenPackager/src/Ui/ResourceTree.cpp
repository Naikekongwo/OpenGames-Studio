/**
 * @file ResourceTree.cpp
 * @author Naikekongwo
 * @brief 资源目录树的实现。
 * @version 0.1
 * @date 2026-11-01
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "OpenPackager/Ui/ResourceTree.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

using std::string;
using std::string_view;
using std::vector;

namespace
{

/// 没有原始路径的条目统一放进这个虚拟目录
constexpr char kUnclassifiedName[] = "(未分类)";

/**
 * @brief 把 UTF-8 路径按 '/' 切成若干段，忽略空段。
 * @details 只做分隔，不处理 '\\'（清单里统一用 '/'）。
 */
vector<string> splitPath(string_view path)
{
    vector<string> parts;
    string         current;

    for (const char ch : path)
    {
        if (ch == '/')
        {
            if (!current.empty())
                parts.push_back(current);
            current.clear();
        }
        else
        {
            current.push_back(ch);
        }
    }

    if (!current.empty())
        parts.push_back(current);

    return parts;
}

} // namespace

void ResourceTree::clear()
{
    nodes.clear();
    rootIndex_ = 0;
}

vector<string> ResourceTree::expandedPaths() const
{
    vector<string> out;
    if (!nodes.empty())
        collectExpanded(rootIndex_, out);
    return out;
}

void ResourceTree::collectExpanded(std::size_t index,
                                   vector<string> &out) const
{
    if (index >= nodes.size())
        return;

    const auto &node = nodes[index];
    if (node.isDirectory() && node.expanded && index != rootIndex_)
        out.push_back(node.path);

    for (const auto child : node.children)
        collectExpanded(child, out);
}

int ResourceTree::findChildDir(std::size_t          parent,
                               const std::string   &name) const
{
    for (const auto child : nodes[parent].children)
    {
        if (nodes[child].isDirectory() && nodes[child].name == name)
            return static_cast<int>(child);
    }
    return -1;
}

std::size_t ResourceTree::addChild(std::size_t parent, std::string name,
                                   std::string path)
{
    TreeNode node;
    node.name   = std::move(name);
    node.path   = std::move(path);
    node.parent = static_cast<int>(parent);

    nodes.push_back(std::move(node));
    const auto index = nodes.size() - 1;
    nodes[parent].children.push_back(index);
    return index;
}

void ResourceTree::sortChildren()
{
    for (auto &node : nodes)
    {
        if (node.children.empty())
            continue;

        std::sort(node.children.begin(), node.children.end(),
                  [this](std::size_t a, std::size_t b)
                  {
                      const auto &la = nodes[a];
                      const auto &lb = nodes[b];

                      // 目录在前
                      if (la.isDirectory() != lb.isDirectory())
                          return la.isDirectory();

                      return la.name < lb.name;
                  });
    }
}

void ResourceTree::rebuild(const std::vector<ArchiveEntry> &entries)
{
    // 记住旧的展开状态，重建后按路径恢复
    const auto previouslyExpanded = expandedPaths();

    nodes.clear();
    rootIndex_ = 0;

    // 虚拟根
    TreeNode rootNode;
    rootNode.name   = "";
    rootNode.path   = "";
    rootNode.parent = -1;
    nodes.push_back(std::move(rootNode));

    for (std::size_t i = 0; i < entries.size(); ++i)
    {
        const auto &entry = entries[i];

        // 没有原始路径的条目挂到虚拟目录下
        if (!entry.hasPath())
        {
            auto dirIndex = findChildDir(rootIndex_, kUnclassifiedName);
            std::size_t parent = 0;
            if (dirIndex < 0)
            {
                parent = addChild(rootIndex_, kUnclassifiedName,
                                  kUnclassifiedName);
            }
            else
            {
                parent = static_cast<std::size_t>(dirIndex);
            }

            const auto leaf = addChild(parent, entry.name, entry.name);
            nodes[leaf].entryIndex = static_cast<int>(i);
            continue;
        }

        // 有路径：逐级建目录，最后一段作为文件名
        const auto parts = splitPath(entry.filePath);
        if (parts.empty())
        {
            // 路径不合法时退化为挂在根下
            const auto leaf = addChild(rootIndex_, entry.name, entry.name);
            nodes[leaf].entryIndex = static_cast<int>(i);
            continue;
        }

        std::size_t    parent   = rootIndex_;
        string         pathSoFar;
        const auto     dirCount = parts.size() - 1;

        for (std::size_t d = 0; d < dirCount; ++d)
        {
            if (!pathSoFar.empty())
                pathSoFar += '/';
            pathSoFar += parts[d];

            auto existing = findChildDir(parent, parts[d]);
            if (existing >= 0)
            {
                parent = static_cast<std::size_t>(existing);
            }
            else
            {
                parent = addChild(parent, parts[d], pathSoFar);
            }
        }

        if (!pathSoFar.empty())
            pathSoFar += '/';
        pathSoFar += parts.back();

        const auto leaf = addChild(parent, parts.back(), pathSoFar);
        nodes[leaf].entryIndex = static_cast<int>(i);
    }

    sortChildren();

    // 恢复展开状态：默认全展开，只有明确收起过的路径保持收起
    for (auto &node : nodes)
    {
        if (!node.isDirectory())
            continue;

        // 新目录默认展开；旧目录沿用旧状态
        const bool wasKnown =
            std::find(previouslyExpanded.begin(), previouslyExpanded.end(),
                      node.path) != previouslyExpanded.end();
        const bool hadOldState = !previouslyExpanded.empty();

        node.expanded = hadOldState ? wasKnown : true;
    }

    // 虚拟根始终展开
    nodes[rootIndex_].expanded = true;
}

int ResourceTree::depthOf(std::size_t index) const
{
    int depth = 0;
    auto current = static_cast<int>(index);

    while (current >= 0 && nodes[static_cast<std::size_t>(current)].parent >= 0)
    {
        ++depth;
        current = nodes[static_cast<std::size_t>(current)].parent;
    }

    // 根的直接子节点深度为 0
    return depth > 0 ? depth - 1 : 0;
}

vector<std::size_t> ResourceTree::flattenVisible() const
{
    vector<std::size_t> out;
    if (nodes.empty())
        return out;

    // 显式栈遍历，避免深目录递归过深
    vector<std::size_t> stack;
    const auto          &rootChildren = nodes[rootIndex_].children;

    // 逆序压栈，保证弹出顺序与 children 顺序一致
    for (auto it = rootChildren.rbegin(); it != rootChildren.rend(); ++it)
        stack.push_back(*it);

    while (!stack.empty())
    {
        const auto index = stack.back();
        stack.pop_back();

        out.push_back(index);

        const auto &node = nodes[index];
        if (!node.isDirectory() || !node.expanded)
            continue;

        for (auto it = node.children.rbegin(); it != node.children.rend(); ++it)
            stack.push_back(*it);
    }

    return out;
}

bool ResourceTree::toggleExpanded(std::size_t index)
{
    if (index >= nodes.size() || !nodes[index].isDirectory())
        return false;

    nodes[index].expanded = !nodes[index].expanded;
    return true;
}

void ResourceTree::expandPathTo(std::size_t index)
{
    if (index >= nodes.size())
        return;

    auto current = nodes[index].parent;
    while (current >= 0)
    {
        nodes[static_cast<std::size_t>(current)].expanded = true;
        current = nodes[static_cast<std::size_t>(current)].parent;
    }
}
