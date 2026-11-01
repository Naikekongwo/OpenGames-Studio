/**
 * @file PackageArchive.hpp
 * @author Naikekongwo
 * @brief ocdata 资源包的读写核心。
 * @version 0.1
 * @date 2026-11-01
 *
 * @details .ocdata 的文件结构（OpenCore 26.x）：
 *          [4 字节魔数 "OCDT"] + [连续拼接的资源二进制]，
 *          资源在数据段中的位置由随包的 <包名>_packagemanifest.txt 描述，
 *          清单每行一条 CSV：类型,资源名,原始路径,起始偏移,结束偏移。
 *
 *          本类只做「读 / 写 / 校验」三件事，不涉及渲染，便于单独测试：
 *            - 打开：读取并校验魔数，解析同目录的清单文件；
 *            - 提取：按节点的偏移把单个资源还原成独立文件；
 *            - 修改：替换 / 删除 / 新增节点，并把数据段整体重排后回写
 *              （.ocdata 与清单同时更新，保证两者一致）。
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

/**
 * @brief 资源类型，与 PackageManager 清单中的类型字符串一一对应。
 * @note 字符串映射集中在 ArchiveResourceType 相关的两个函数里，
 *       新增类型只需在这两处各加一项（与引擎的序列化保持同构）。
 */
enum class ArchiveResourceType
{
    Texture,
    Audio,
    Font,
    Asset
};

/** @brief 资源类型 → 清单字符串 */
std::string_view ArchiveTypeToString(ArchiveResourceType type);

/** @brief 清单字符串 → 资源类型，未知类型返回 nullopt */
std::optional<ArchiveResourceType> ArchiveTypeFromString(std::string_view str);

/**
 * @brief 资源在包内的一条索引记录。
 */
struct ArchiveEntry
{
    ArchiveResourceType type = ArchiveResourceType::Asset;
    std::string         name;     ///< 资源名（引擎内查找用的键）
    std::string         filePath; ///< 原始相对路径（保留目录结构用）
    int                 startIndex = 0; ///< 数据段起始偏移（不含魔数）
    int                 endIndex   = 0; ///< 数据段结束偏移（不含）

    /** @brief 资源字节数 */
    int  size() const { return endIndex - startIndex; }
    /** @brief 该资源是否需要保留原始路径才能还原目录结构 */
    bool hasPath() const { return !filePath.empty(); }
};

/**
 * @brief ocdata 资源包读写器。
 * @details 一个实例绑定一对 .ocdata 与清单文件；调用 open() 成功后方可
 *          使用其余接口。
 */
class PackageArchive
{
  public:
    PackageArchive() = default;

    // ──────────────────────────────────────────────
    //  打开与查询
    // ──────────────────────────────────────────────

    /**
     * @brief 打开一个 .ocdata 资源包。
     * @details 校验魔数、统计数据段长度、解析同目录的清单文件。
     *          清单缺失时仍然算打开成功（dataLength 已知，可以按偏移
     *          导出），但 entries 为空，需要调用 rebuildEntriesFromRaw()
     *          之类的兜底逻辑由界面层决定。
     * @param ocdataPath .ocdata 文件路径
     * @param manifestPath 清单文件路径，传空串则按包名自动推断
     * @return 成功返回 true
     */
    bool open(const std::filesystem::path &ocdataPath,
              const std::filesystem::path &manifestPath = {});

    /** @brief 关闭当前包，清空所有状态 */
    void close();

    /** @brief 当前是否已打开一个合法的包 */
    bool isOpen() const { return opened; }

    /** @brief 包的魔数是否符合预期（打开成功即为 true） */
    bool isMagicValid() const { return opened; }

    const std::filesystem::path &getOcdataPath() const { return ocdataPath; }
    const std::filesystem::path &getManifestPath() const { return manifestPath; }

    /** @brief 包名前缀（去掉 _00.ocdata 后缀） */
    const std::string &getPackageName() const { return packageName; }

    /** @brief 数据段总字节数（不含 4 字节魔数） */
    std::size_t getDataLength() const { return dataLength; }

    /** @brief 清单中的资源条目 */
    const std::vector<ArchiveEntry> &getEntries() const { return entries; }
    std::vector<ArchiveEntry>       &getEntries() { return entries; }

    /** @brief 清单文件是否存在并成功解析 */
    bool hasManifest() const { return manifestLoaded; }

    /** @brief 打开过程中记录的问题，供界面展示 */
    const std::vector<std::string> &getWarnings() const { return warnings; }

    /**
     * @brief 校验条目完整性（越界、重叠、长度为零等）。
     * @return 问题列表，为空表示全部合法
     */
    std::vector<std::string> validate() const;

    /**
     * @brief 统计没有被任何条目覆盖的数据段字节数。
     * @details 清单缺失或条目被裁剪时用来提示「包内还有数据未声明」。
     */
    std::size_t countUndeclaredBytes() const;

    // ──────────────────────────────────────────────
    //  提取
    // ──────────────────────────────────────────────

    /**
     * @brief 把单个资源读取为内存字节。
     * @param index entries 下标
     * @return 成功返回字节，失败返回 nullopt
     */
    std::optional<std::vector<char>> readEntry(std::size_t index) const;

    /**
     * @brief 按条目把整个包提取到目录。
     * @details 目标路径为 outputDir / 条目.原始路径；原始路径为空时退化为
     *          outputDir / 类型目录 / 资源名.扩展名。
     * @param outputDir 输出根目录（不存在会自动创建）
     * @param overwrite 目标已存在时是否覆盖
     * @param written 成功写出的文件路径（追加）
     * @return 成功写出的文件数量
     */
    std::size_t extractAll(const std::filesystem::path &outputDir,
                           bool overwrite = true,
                           std::vector<std::filesystem::path> *written = nullptr) const;

    /**
     * @brief 把单个条目提取到目录（不复刻原始路径，直接用原始文件名）。
     * @return 成功返回写出的路径
     */
    std::optional<std::filesystem::path>
    extractEntry(std::size_t index, const std::filesystem::path &outputDir,
                 bool overwrite = true) const;

    /**
     * @brief 按偏移把整个数据段打包成单个文件（清单缺失时的兜底导出）。
     * @details 产物为「4 字节 OCDT + 数据段」，与原包逐字节一致，只用于
     *          确认数据完整性，不产生可用资源。
     */
    bool dumpRawData(const std::filesystem::path &outputFile) const;

    // ──────────────────────────────────────────────
    //  修改与回写
    // ──────────────────────────────────────────────

    /**
     * @brief 用新的文件替换第 index 个条目的内容。
     * @details 只修改内存中的待写数据；调用 save() 才会落盘。
     *          startIndex / endIndex 会在 save() 时统一重排。
     * @return 成功返回 true
     */
    bool replaceEntry(std::size_t index, const std::filesystem::path &newFile);

    /** @brief 标记删除第 index 个条目（save() 时不再写出） */
    bool removeEntry(std::size_t index);

    /**
     * @brief 追加一个新资源。
     * @param type 资源类型
     * @param name 资源名（重复时返回 false）
     * @param filePath 原始相对路径（打包时写入清单）
     * @param sourceFile 资源文件
     * @return 成功返回 true
     */
    bool appendEntry(ArchiveResourceType type, std::string_view name,
                     std::string_view filePath,
                     const std::filesystem::path &sourceFile);

    /**
     * @brief 把内存中的条目重排并写回 .ocdata 与清单。
     * @details 数据段按条目顺序重新拼接，偏移从 0 开始重算；
     *          写出顺序为「先写临时文件再替换」，避免中途失败把原包写坏。
     * @param keepBackup 是否把原文件备份为 *.bak
     * @param error 失败原因
     * @return 成功返回 true
     */
    bool save(bool keepBackup = true, std::string *error = nullptr);

    /** @brief 是否有尚未保存的修改 */
    bool isDirty() const { return dirty; }

    /**
     * @brief 从磁盘重新加载，丢弃所有未保存的修改
     */
    bool reload();

  private:
    // ── 打开状态 ──
    bool                 opened = false;
    std::filesystem::path ocdataPath;
    std::filesystem::path manifestPath;
    std::string          packageName;
    std::size_t          dataLength = 0;

    // ── 清单状态 ──
    bool                     manifestLoaded = false;
    std::vector<ArchiveEntry> entries;
    std::vector<std::string>  warnings;

    // ── 待写数据（只在修改后填充） ──
    // 键为 entries 下标；缺失表示沿用原始字节
    std::vector<std::optional<std::vector<char>>> newData;
    bool                                          dirty = false;

    // ── 内部工具 ──

    /** @brief 读取条目原始字节（优先使用待写数据） */
    std::optional<std::vector<char>> readEntryRaw(std::size_t index) const;

    /** @brief 解析清单文件 */
    bool loadManifest();

    /** @brief 从 <包名>_00.ocdata 推导清单路径 */
    static std::filesystem::path guessManifestPath(
        const std::filesystem::path &ocdata);

    /** @brief 按资源类型推断扩展名 */
    static std::string_view defaultExtension(ArchiveResourceType type);

    /** @brief 拆解一行清单；兼容旧版 Music 类型名 */
    static bool parseManifestLine(std::string_view line, ArchiveEntry &out,
                                  std::vector<std::string> &warnings);
};
