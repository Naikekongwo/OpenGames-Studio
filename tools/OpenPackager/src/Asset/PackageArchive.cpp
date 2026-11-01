/**
 * @file PackageArchive.cpp
 * @author Naikekongwo
 * @brief ocdata 资源包读写核心的实现。
 * @version 0.1
 * @date 2026-11-01
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "OpenPackager/Asset/PackageArchive.hpp"
#include "OpenCore.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <iterator>
#include <sstream>
#include <system_error>

namespace fs = std::filesystem;

using std::string;
using std::string_view;
using std::vector;

namespace
{

/// ocdata 的魔数（OpenCore 26.x 起统一为 OCDT）
constexpr std::array<char, 4> kMagic = {'O', 'C', 'D', 'T'};
constexpr std::size_t         kMagicSize = kMagic.size();

/// 清单文件的后缀
constexpr string_view kPackedManifestSuffix = "_packagemanifest.txt";
/// data 包的后缀（旧格式，以魔数 DATA 区分）
constexpr string_view kPlainManifestSuffix = "_manifest.txt";

/**
 * @brief 以 UTF-8 语义构造路径。
 * @details Windows 下 std::filesystem 的窄字符构造依赖本地代码页，
 *          中文路径会乱码，因此统一走 char8_t。
 */
fs::path toPath(std::string_view utf8)
{
    return fs::path(reinterpret_cast<const char8_t *>(string(utf8).c_str()));
}

/**
 * @brief 路径转回 UTF-8 字符串。
 * @details std::format 不认识 char8_t 字符串，必须先转成 char。
 */
string toUtf8(const fs::path &path)
{
    const std::u8string utf8 = path.u8string();
    return string(reinterpret_cast<const char *>(utf8.data()), utf8.size());
}

/** @brief 把整段文件读进内存 */
bool readWholeFile(const fs::path &path, vector<char> &out)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open())
        return false;

    file.seekg(0, std::ios::end);
    const auto size = file.tellg();
    if (size < 0)
        return false;
    file.seekg(0, std::ios::beg);

    out.resize(static_cast<std::size_t>(size));
    if (!out.empty())
        file.read(out.data(), static_cast<std::streamsize>(out.size()));

    return file.good() || file.eof();
}

/** @brief 把内存写出到文件（自动创建父目录） */
bool writeWholeFile(const fs::path &path, const char *data, std::size_t size)
{
    std::error_code ec;
    if (path.has_parent_path())
        fs::create_directories(path.parent_path(), ec);

    std::ofstream file(path, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file.is_open())
        return false;

    if (size > 0)
        file.write(data, static_cast<std::streamsize>(size));

    file.flush();
    return file.good();
}

/** @brief 去掉首尾空白 */
string_view trim(string_view text)
{
    const auto begin = text.find_first_not_of(" \t\r\n");
    if (begin == string_view::npos)
        return {};
    const auto end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

/** @brief 解析形如 "123" 的非负整数 */
bool parseOffset(string_view text, int &out)
{
    text = trim(text);
    if (text.empty())
        return false;

    long long value = 0;
    for (const char ch : text)
    {
        if (ch < '0' || ch > '9')
            return false;
        value = value * 10 + (ch - '0');
        if (value > 2147483647LL)
            return false;
    }

    out = static_cast<int>(value);
    return true;
}

} // namespace

// ──────────────────────────────────────────────
//  类型与字符串的映射
// ──────────────────────────────────────────────
string_view ArchiveTypeToString(ArchiveResourceType type)
{
    switch (type)
    {
    case ArchiveResourceType::Texture:
        return "Texture";
    case ArchiveResourceType::Audio:
        return "Audio";
    case ArchiveResourceType::Font:
        return "Font";
    case ArchiveResourceType::Asset:
        return "Asset";
    }
    return "Asset";
}

std::optional<ArchiveResourceType> ArchiveTypeFromString(string_view str)
{
    if (str == "Texture")
        return ArchiveResourceType::Texture;
    if (str == "Audio")
        return ArchiveResourceType::Audio;
    if (str == "Font")
        return ArchiveResourceType::Font;
    if (str == "Asset")
        return ArchiveResourceType::Asset;

    // 兼容 OpenCore 26.1 之前的旧类型名
    if (str == "Music")
        return ArchiveResourceType::Audio;

    return std::nullopt;
}

// ──────────────────────────────────────────────
//  打开与关闭
// ──────────────────────────────────────────────
fs::path PackageArchive::guessManifestPath(const fs::path &ocdata)
{
    // <包名>_00.ocdata → <包名>_packagemanifest.txt
    string stem = toUtf8(ocdata.stem()); // 去掉 .ocdata，得到 xxx_00
    constexpr string_view zeroSuffix = "_00";
    if (stem.size() >= zeroSuffix.size() &&
        stem.compare(stem.size() - zeroSuffix.size(), zeroSuffix.size(),
                     zeroSuffix) == 0)
    {
        stem.erase(stem.size() - zeroSuffix.size());
    }

    return ocdata.parent_path() / toPath(stem + string(kPackedManifestSuffix));
}

string_view PackageArchive::defaultExtension(ArchiveResourceType type)
{
    switch (type)
    {
    case ArchiveResourceType::Texture:
        return ".png";
    case ArchiveResourceType::Audio:
        return ".ogg";
    case ArchiveResourceType::Font:
        return ".ttf";
    case ArchiveResourceType::Asset:
        return ".bin";
    }
    return ".bin";
}

bool PackageArchive::open(const fs::path &targetOcdata,
                         const fs::path &targetManifest)
{
    close();

    std::error_code ec;
    if (!fs::exists(targetOcdata, ec) || !fs::is_regular_file(targetOcdata, ec))
    {
        warnings.push_back("文件不存在：" + toUtf8(targetOcdata));
        return false;
    }

    // ① 校验魔数
    std::ifstream file(targetOcdata, std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        warnings.push_back("无法打开文件：" + toUtf8(targetOcdata));
        return false;
    }

    std::array<char, kMagicSize> magic{};
    file.read(magic.data(), static_cast<std::streamsize>(magic.size()));
    if (file.gcount() != static_cast<std::streamsize>(magic.size()) ||
        magic != kMagic)
    {
        warnings.push_back("魔数不匹配，不是 OCDT 资源包");
        return false;
    }

    // ② 统计数据段长度
    file.seekg(0, std::ios::end);
    const auto total = file.tellg();
    file.close();

    if (total < 0)
    {
        warnings.push_back("无法读取文件长度");
        return false;
    }

    const auto totalSize = static_cast<std::size_t>(total);
    if (totalSize < kMagicSize)
    {
        warnings.push_back("文件长度不足魔数长度");
        return false;
    }

    ocdataPath = targetOcdata;
    dataLength = totalSize - kMagicSize;

    // ③ 包名与清单路径
    string stem = toUtf8(ocdataPath.stem()); // xxx_00
    constexpr string_view zeroSuffix = "_00";
    if (stem.size() >= zeroSuffix.size() &&
        stem.compare(stem.size() - zeroSuffix.size(), zeroSuffix.size(),
                     zeroSuffix) == 0)
    {
        packageName = stem.substr(0, stem.size() - zeroSuffix.size());
    }
    else
    {
        packageName = stem;
    }

    manifestPath = targetManifest.empty() ? guessManifestPath(ocdataPath)
                                          : targetManifest;

    // ④ 解析清单
    opened = true;
    loadManifest();

    // 待写数据表与条目表等长
    newData.assign(entries.size(), std::nullopt);
    dirty = false;

    return true;
}

void PackageArchive::close()
{
    opened         = false;
    ocdataPath.clear();
    manifestPath.clear();
    packageName.clear();
    dataLength      = 0;
    manifestLoaded  = false;
    dirty           = false;
    entries.clear();
    newData.clear();
    warnings.clear();
}

bool PackageArchive::parseManifestLine(string_view line, ArchiveEntry &out,
                                      vector<string> &warnings)
{
    line = trim(line);
    if (line.empty())
        return false;

    // CSV 五段：类型,资源名,原始路径,起始偏移,结束偏移
    // 资源名与路径不含逗号，直接从两端切分即可
    const auto firstComma = line.find(',');
    if (firstComma == string_view::npos)
    {
        warnings.push_back("清单行缺少字段：" + string(line));
        return false;
    }

    const auto lastComma = line.rfind(',');
    if (lastComma == firstComma)
    {
        warnings.push_back("清单行字段不足：" + string(line));
        return false;
    }

    const auto secondLastComma = line.rfind(',', lastComma - 1);
    if (secondLastComma == string_view::npos || secondLastComma == firstComma)
    {
        warnings.push_back("清单行字段不足：" + string(line));
        return false;
    }

    const string_view typeText = trim(line.substr(0, firstComma));
    const string_view body     = line.substr(firstComma + 1);

    // body = 资源名,原始路径,起始,结束，其中原始路径可能为空
    const auto bodySecondLast = body.rfind(',');
    const auto bodyThirdLast  = body.rfind(',', bodySecondLast - 1);
    const auto bodyFirst      = body.find(',');
    if (bodyFirst == string_view::npos || bodyThirdLast == string_view::npos ||
        bodyFirst == bodyThirdLast)
    {
        warnings.push_back("清单行字段不足：" + string(line));
        return false;
    }

    auto type = ArchiveTypeFromString(typeText);
    if (!type)
    {
        warnings.push_back("未知的资源类型：" + string(typeText));
        type = ArchiveResourceType::Asset;
    }

    ArchiveEntry entry;
    entry.type       = *type;
    entry.name       = string(trim(body.substr(0, bodyFirst)));
    entry.filePath   = string(trim(body.substr(bodyFirst + 1,
                                               bodyThirdLast - bodyFirst - 1)));

    if (!parseOffset(body.substr(bodyThirdLast + 1,
                                 bodySecondLast - bodyThirdLast - 1),
                     entry.startIndex) ||
        !parseOffset(body.substr(bodySecondLast + 1), entry.endIndex))
    {
        warnings.push_back("清单行的偏移无法解析：" + string(line));
        return false;
    }

    out = std::move(entry);
    return true;
}

bool PackageArchive::loadManifest()
{
    entries.clear();
    manifestLoaded = false;

    std::error_code ec;
    if (!fs::exists(manifestPath, ec))
    {
        // 清单缺失不是致命错误：仍可按数据段整体导出
        warnings.push_back("清单文件不存在：" + toUtf8(manifestPath));
        return false;
    }

    std::ifstream file(manifestPath, std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        warnings.push_back("无法读取清单文件：" + toUtf8(manifestPath));
        return false;
    }

    string line;
    int    lineNo = 0;
    while (std::getline(file, line))
    {
        ++lineNo;

        ArchiveEntry entry;
        if (!parseManifestLine(line, entry, warnings))
            continue;

        if (entry.startIndex < 0 || entry.endIndex < entry.startIndex)
        {
            warnings.push_back("清单第 " + std::to_string(lineNo) +
                               " 行偏移非法：" + line);
            continue;
        }

        entries.push_back(std::move(entry));
    }

    file.close();
    manifestLoaded = true;

    // 旧版清单可能残留 Music 类型，这里只做提示，不阻断
    return true;
}

// ──────────────────────────────────────────────
//  校验
// ──────────────────────────────────────────────
vector<string> PackageArchive::validate() const
{
    vector<string> issues;

    if (!opened)
    {
        issues.push_back("尚未打开资源包");
        return issues;
    }

    if (!manifestLoaded)
        issues.push_back("缺少清单文件，只能按数据段整体导出");

    // 按起始偏移排序后检查重叠，同时检测越界
    vector<std::size_t> order(entries.size());
    for (std::size_t i = 0; i < order.size(); ++i)
        order[i] = i;

    std::sort(order.begin(), order.end(),
              [this](std::size_t a, std::size_t b)
              { return entries[a].startIndex < entries[b].startIndex; });

    int lastEnd = -1;
    for (const auto index : order)
    {
        const auto &entry = entries[index];

        if (entry.endIndex > static_cast<int>(dataLength))
        {
            issues.push_back("条目越界：" + entry.name + " [" +
                             std::to_string(entry.startIndex) + ", " +
                             std::to_string(entry.endIndex) + ") 超出数据段 " +
                             std::to_string(dataLength) + " 字节");
        }

        if (entry.size() == 0)
            issues.push_back("条目长度为 0：" + entry.name);

        if (entry.startIndex < lastEnd)
        {
            issues.push_back("条目区间重叠：" + entry.name + " 起始 " +
                             std::to_string(entry.startIndex) + "，上一个条目结束于 " +
                             std::to_string(lastEnd));
        }

        lastEnd = std::max(lastEnd, entry.endIndex);
    }

    return issues;
}

std::size_t PackageArchive::countUndeclaredBytes() const
{
    if (!opened)
        return 0;

    // 把所有条目区间排序合并，求未覆盖的长度
    vector<std::pair<int, int>> ranges;
    ranges.reserve(entries.size());
    for (const auto &entry : entries)
    {
        const int begin = std::clamp(entry.startIndex, 0,
                                     static_cast<int>(dataLength));
        const int end =
            std::clamp(entry.endIndex, 0, static_cast<int>(dataLength));
        if (end > begin)
            ranges.emplace_back(begin, end);
    }

    std::sort(ranges.begin(), ranges.end());

    std::size_t covered = 0;
    int         cursor  = 0;
    for (const auto &[begin, end] : ranges)
    {
        const int from = std::max(begin, cursor);
        if (end > from)
            covered += static_cast<std::size_t>(end - from);
        cursor = std::max(cursor, end);
    }

    return dataLength > covered ? dataLength - covered : 0;
}

// ──────────────────────────────────────────────
//  提取
// ──────────────────────────────────────────────
std::optional<vector<char>> PackageArchive::readEntryRaw(std::size_t index) const
{
    if (!opened || index >= entries.size())
        return std::nullopt;

    // 已被修改过的条目直接取内存里的新内容
    if (index < newData.size() && newData[index])
        return newData[index];

    const auto &entry = entries[index];
    if (entry.size() <= 0)
        return std::nullopt;

    std::ifstream file(ocdataPath, std::ios::in | std::ios::binary);
    if (!file.is_open())
        return std::nullopt;

    file.seekg(static_cast<std::streamoff>(kMagicSize + entry.startIndex),
               std::ios::beg);
    if (!file.good())
        return std::nullopt;

    vector<char> buffer(static_cast<std::size_t>(entry.size()));
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    if (file.gcount() != static_cast<std::streamsize>(buffer.size()))
        return std::nullopt;

    return buffer;
}

std::optional<vector<char>> PackageArchive::readEntry(std::size_t index) const
{
    return readEntryRaw(index);
}

std::optional<fs::path>
PackageArchive::extractEntry(std::size_t index, const fs::path &outputDir,
                             bool overwrite) const
{
    if (!opened || index >= entries.size())
        return std::nullopt;

    const auto &entry = entries[index];
    const auto  data  = readEntryRaw(index);
    if (!data)
        return std::nullopt;

    // 优先使用原始文件名，缺失时用资源名 + 类型扩展名
    string fileName;
    if (entry.hasPath())
        fileName = toUtf8(fs::path(toPath(entry.filePath)).filename());
    if (fileName.empty())
        fileName = entry.name + string(defaultExtension(entry.type));

    const auto target = outputDir / toPath(fileName);

    std::error_code ec;
    if (!overwrite && fs::exists(target, ec))
        return std::nullopt;

    if (!writeWholeFile(target, data->data(), data->size()))
        return std::nullopt;

    return target;
}

std::size_t PackageArchive::extractAll(const fs::path &outputDir, bool overwrite,
                                       vector<fs::path> *written) const
{
    if (!opened)
        return 0;

    std::error_code ec;
    fs::create_directories(outputDir, ec);

    std::size_t count = 0;
    for (std::size_t i = 0; i < entries.size(); ++i)
    {
        const auto &entry = entries[i];
        const auto  data  = readEntryRaw(i);
        if (!data)
        {
            LOG("提取失败，条目 {} 读取不到数据", entry.name);
            continue;
        }

        // 保留原始目录结构；没有原始路径时按类型分目录
        fs::path relative;
        if (entry.hasPath())
        {
            relative = toPath(entry.filePath);
        }
        else
        {
            relative = toPath(string(ArchiveTypeToString(entry.type))) /
                       toPath(entry.name + string(defaultExtension(entry.type)));
        }

        // 防止清单里出现 ../ 之类的越界路径
        auto target = outputDir / relative;
        target      = target.lexically_normal();
        const auto rootStr   = toUtf8(outputDir.lexically_normal());
        const auto targetStr = toUtf8(target);
        if (targetStr.rfind(rootStr, 0) != 0)
        {
            LOG("跳过越界路径 {}", targetStr);
            continue;
        }

        if (!overwrite && fs::exists(target, ec))
            continue;

        if (!writeWholeFile(target, data->data(), data->size()))
        {
            LOG("写出失败 {}", targetStr);
            continue;
        }

        ++count;
        if (written)
            written->push_back(target);
    }

    return count;
}

bool PackageArchive::dumpRawData(const fs::path &outputFile) const
{
    if (!opened)
        return false;

    std::ifstream source(ocdataPath, std::ios::in | std::ios::binary);
    if (!source.is_open())
        return false;

    // 原样复制整包（魔数 + 数据段），保证逐字节一致
    vector<char> buffer;
    return readWholeFile(ocdataPath, buffer) &&
           writeWholeFile(outputFile, buffer.data(), buffer.size());
}

// ──────────────────────────────────────────────
//  修改
// ──────────────────────────────────────────────
bool PackageArchive::replaceEntry(std::size_t index,
                                  const fs::path &newFile)
{
    if (!opened || index >= entries.size())
        return false;

    vector<char> data;
    if (!readWholeFile(newFile, data))
    {
        LOG("读取替换文件失败 {}", toUtf8(newFile));
        return false;
    }

    if (newData.size() < entries.size())
        newData.resize(entries.size());

    newData[index] = std::move(data);
    dirty          = true;
    return true;
}

bool PackageArchive::removeEntry(std::size_t index)
{
    if (!opened || index >= entries.size())
        return false;

    entries.erase(entries.begin() + static_cast<std::ptrdiff_t>(index));
    if (index < newData.size())
        newData.erase(newData.begin() + static_cast<std::ptrdiff_t>(index));

    dirty = true;
    return true;
}

bool PackageArchive::appendEntry(ArchiveResourceType type, string_view name,
                                 string_view filePath,
                                 const fs::path &sourceFile)
{
    if (!opened)
        return false;

    // 资源名必须唯一，否则引擎查找会命中前面的条目
    for (const auto &entry : entries)
    {
        if (entry.name == name)
        {
            LOG("资源名 {} 已存在", string(name));
            return false;
        }
    }

    vector<char> data;
    if (!readWholeFile(sourceFile, data))
    {
        LOG("读取新增文件失败 {}", toUtf8(sourceFile));
        return false;
    }

    ArchiveEntry entry;
    entry.type     = type;
    entry.name     = string(name);
    entry.filePath = string(filePath);

    entries.push_back(std::move(entry));
    newData.emplace_back(std::move(data));
    dirty = true;
    return true;
}

bool PackageArchive::save(bool keepBackup, string *error)
{
    auto fail = [error](const string &reason)
    {
        if (error)
            *error = reason;
        LOG("{}", reason);
        return false;
    };

    if (!opened)
        return fail("尚未打开资源包");

    // ① 收集每个条目最终要写出的字节，并重排偏移
    vector<vector<char>> blobs;
    blobs.reserve(entries.size());

    for (std::size_t i = 0; i < entries.size(); ++i)
    {
        std::optional<vector<char>> data;
        if (i < newData.size() && newData[i])
            data = newData[i];
        else
            data = readEntryRaw(i);

        if (!data)
            return fail("条目 " + entries[i].name + " 读取不到数据");

        blobs.push_back(std::move(*data));
    }

    // ② 拼接数据段
    vector<char> dataSegment;
    std::size_t  cursor = 0;
    for (std::size_t i = 0; i < entries.size(); ++i)
    {
        entries[i].startIndex = static_cast<int>(cursor);
        cursor += blobs[i].size();
        entries[i].endIndex = static_cast<int>(cursor);

        if (cursor > 2147483647ULL)
            return fail("数据段超出 2GB，当前格式不支持");

        dataSegment.insert(dataSegment.end(), blobs[i].begin(), blobs[i].end());
    }

    // ③ 生成清单文本
    std::ostringstream manifestStream;
    for (const auto &entry : entries)
    {
        manifestStream << ArchiveTypeToString(entry.type) << ',' << entry.name
                       << ',' << entry.filePath << ',' << entry.startIndex
                       << ',' << entry.endIndex << '\n';
    }
    const string manifestText = manifestStream.str();

    // ④ 先写临时文件，成功后再替换原文件，避免中途失败把原包写坏
    const auto ocdataTmp  = ocdataPath;
    const auto ocdataNew  = fs::path(ocdataPath) += toPath(".tmp");
    const auto manifestNew = fs::path(manifestPath) += toPath(".tmp");

    std::error_code ec;

    {
        std::ofstream out(ocdataNew,
                          std::ios::out | std::ios::binary | std::ios::trunc);
        if (!out.is_open())
            return fail("无法创建临时文件 " + toUtf8(ocdataNew));

        out.write(kMagic.data(), static_cast<std::streamsize>(kMagic.size()));
        if (!dataSegment.empty())
            out.write(dataSegment.data(),
                      static_cast<std::streamsize>(dataSegment.size()));

        out.flush();
        if (!out.good())
        {
            fs::remove(ocdataNew, ec);
            return fail("写入临时文件失败 " + toUtf8(ocdataNew));
        }
    }

    if (!writeWholeFile(manifestNew, manifestText.data(), manifestText.size()))
    {
        fs::remove(ocdataNew, ec);
        return fail("无法创建清单临时文件 " + toUtf8(manifestNew));
    }

    // ⑤ 备份原文件
    if (keepBackup)
    {
        const auto ocdataBak   = fs::path(ocdataPath) += toPath(".bak");
        const auto manifestBak = fs::path(manifestPath) += toPath(".bak");

        if (fs::exists(ocdataPath, ec))
        {
            fs::remove(ocdataBak, ec);
            fs::copy_file(ocdataPath, ocdataBak, ec);
        }
        if (fs::exists(manifestPath, ec))
        {
            fs::remove(manifestBak, ec);
            fs::copy_file(manifestPath, manifestBak, ec);
        }
    }

    // ⑥ 替换
    fs::rename(ocdataNew, ocdataPath, ec);
    if (ec)
    {
        fs::remove(ocdataNew, ec);
        return fail("替换资源包失败");
    }

    fs::rename(manifestNew, manifestPath, ec);
    if (ec)
        return fail("替换清单失败");

    // ⑦ 重载，让内存状态与磁盘一致
    dataLength     = dataSegment.size();
    newData.assign(entries.size(), std::nullopt);
    dirty = false;

    LOG("资源包已保存：{} 个条目，数据段 {} 字节",
        static_cast<int>(entries.size()), dataSegment.size());

    return true;
}

bool PackageArchive::reload()
{
    if (!opened)
        return false;

    const auto ocdata   = ocdataPath;
    const auto manifest = manifestPath;
    return open(ocdata, manifest);
}
