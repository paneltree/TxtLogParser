#ifndef CORE_FILESYSTEM_H
#define CORE_FILESYSTEM_H

#include <filesystem>
#include <string>
#include <functional>
#include <unordered_map>
#include <chrono>
#include <future>
#include <thread>
#include <vector>
#include <fstream>
#include <system_error>
#include <stdexcept>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace Core {

/**
 * @brief 文件系统操作异常类
 */
class FileSystemError : public std::runtime_error {
public:
    enum class ErrorType {
        NotFound,
        AccessDenied,
        AlreadyExists,
        InvalidPath,
        IOError,
        Unknown
    };

    FileSystemError(ErrorType type, const std::string& message)
        : std::runtime_error(message), type_(type) {}

    ErrorType getType() const { return type_; }

private:
    ErrorType type_;
};

/**
 * @brief 路径转换工具类
 */
class PathConverter {
public:
    /**
     * @brief 将字符串转换为文件系统路径
     */
    static std::filesystem::path fromString(const std::string& path) {
        return std::filesystem::path(path);
    }

    /**
     * @brief 将文件系统路径转换为字符串
     */
    static std::string toString(const std::filesystem::path& path) {
        return path.string();
    }
    
    /**
     * @brief 将路径转换为平台特定格式
     */
    static std::string toPlatformPath(const std::string& path) {
        std::string result = path;
        #ifdef _WIN32
        // 在Windows上将正斜杠转换为反斜杠
        std::replace(result.begin(), result.end(), '/', '\\');
        #else
        // 在Unix/Linux/Mac上将反斜杠转换为正斜杠
        std::replace(result.begin(), result.end(), '\\', '/');
        #endif
        return result;
    }
    
    /**
     * @brief 将路径转换为标准格式（使用正斜杠）
     */
    static std::string toStandardPath(const std::string& path) {
        std::string result = path;
        std::replace(result.begin(), result.end(), '\\', '/');
        return result;
    }
    
    /**
     * @brief 获取相对路径
     */
    static std::string toRelativePath(const std::string& path, const std::string& basePath) {
        std::filesystem::path p(path);
        std::filesystem::path base(basePath);
        
        try {
            std::filesystem::path rel = std::filesystem::relative(p, base);
            return rel.string();
        } catch (const std::exception&) {
            // 如果无法计算相对路径，则返回原始路径
            return path;
        }
    }
    
    /**
     * @brief 获取绝对路径
     */
    static std::string toAbsolutePath(const std::string& path) {
        try {
            std::filesystem::path p(path);
            return std::filesystem::absolute(p).string();
        } catch (const std::exception&) {
            // 如果无法计算绝对路径，则返回原始路径
            return path;
        }
    }
    
    /**
     * @brief 获取路径的文件名部分
     */
    static std::string getFileName(const std::string& path) {
        std::filesystem::path p(path);
        return p.filename().string();
    }
    
    /**
     * @brief 获取路径的目录部分
     */
    static std::string getDirectory(const std::string& path) {
        std::filesystem::path p(path);
        return p.parent_path().string();
    }
    
    /**
     * @brief 获取路径的扩展名
     */
    static std::string getExtension(const std::string& path) {
        std::filesystem::path p(path);
        return p.extension().string();
    }
    
    /**
     * @brief 连接路径
     */
    static std::string joinPath(const std::string& path1, const std::string& path2) {
        std::filesystem::path p1(path1);
        std::filesystem::path p2(path2);
        return (p1 / p2).string();
    }

private:
    PathConverter() = delete;
};

/**
 * @brief 文件系统操作工具类
 */
class FileSystem {
public:
    // 文件信息查询
    static bool exists(const std::filesystem::path& path) {
        return tryOperation([&]() {
            return std::filesystem::exists(path);
        }, false);
    }

    static bool isRegularFile(const std::filesystem::path& path) {
        return tryOperation([&]() {
            return std::filesystem::is_regular_file(path);
        }, false);
    }

    static bool isDirectory(const std::filesystem::path& path) {
        return tryOperation([&]() {
            return std::filesystem::is_directory(path);
        }, false);
    }

    static uintmax_t fileSize(const std::filesystem::path& path) {
        return tryOperation([&]() {
            return std::filesystem::file_size(path);
        }, 0);
    }

    /**
     * @brief 获取文件的最后修改时间
     */
    static std::filesystem::file_time_type lastWriteTime(const std::filesystem::path& path) {
        try {
            return std::filesystem::last_write_time(path);
        } catch (const std::filesystem::filesystem_error& e) {
            handleFilesystemError(e);
            return std::filesystem::file_time_type{};
        }
    }

    /**
     * @brief 将文件时间转换为time_t
     */
    static time_t toTimeT(const std::filesystem::file_time_type& ft) {
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ft - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
        return std::chrono::system_clock::to_time_t(sctp);
    }

    // 文件操作
    static bool remove(const std::filesystem::path& path) {
        return tryOperation([&]() {
            return std::filesystem::remove(path);
        }, false);
    }

    static bool removeAll(const std::filesystem::path& path) {
        return tryOperation([&]() {
            return std::filesystem::remove_all(path) > 0;
        }, false);
    }

    static bool copy(const std::filesystem::path& from, const std::filesystem::path& to,
                    std::filesystem::copy_options options = std::filesystem::copy_options::none) {
        return tryOperation([&]() {
            std::filesystem::copy(from, to, options);
            return true;
        }, false);
    }

    static bool rename(const std::filesystem::path& from, const std::filesystem::path& to) {
        return tryOperation([&]() {
            std::filesystem::rename(from, to);
            return true;
        }, false);
    }

    // 目录操作
    static bool createDirectory(const std::filesystem::path& path) {
        return tryOperation([&]() {
            return std::filesystem::create_directory(path);
        }, false);
    }

    static bool createDirectories(const std::filesystem::path& path) {
        return tryOperation([&]() {
            return std::filesystem::create_directories(path);
        }, false);
    }

    // 批量操作
    template<typename Func>
    static void parallelScan(const std::filesystem::path& dir, Func callback) {
        std::vector<std::filesystem::path> paths;
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            paths.push_back(entry.path());
        }

        const size_t threadCount = std::thread::hardware_concurrency();
        std::vector<std::thread> threads;
        
        for (size_t i = 0; i < threadCount; ++i) {
            threads.emplace_back([&, i]() {
                for (size_t j = i; j < paths.size(); j += threadCount) {
                    callback(paths[j]);
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }
    }

    static void copyFiles(const std::vector<std::filesystem::path>& sources,
                         const std::filesystem::path& targetDir) {
        std::vector<std::future<void>> futures;
        for (const auto& source : sources) {
            futures.push_back(std::async(std::launch::async, [&]() {
                std::filesystem::copy(source, targetDir / source.filename(),
                                    std::filesystem::copy_options::update_existing);
            }));
        }
        for (auto& future : futures) {
            future.wait();
        }
    }

private:
    template<typename Func, typename ReturnType>
    static ReturnType tryOperation(Func operation, ReturnType defaultValue) {
        try {
            return operation();
        } catch (const std::filesystem::filesystem_error& e) {
            handleFilesystemError(e);
            return defaultValue;
        }
    }

    static void handleFilesystemError(const std::filesystem::filesystem_error& e) {
        auto errorCode = e.code().value();
        auto type = FileSystemError::ErrorType::Unknown;

        switch (errorCode) {
            case static_cast<int>(std::errc::no_such_file_or_directory):
                type = FileSystemError::ErrorType::NotFound;
                break;
            case static_cast<int>(std::errc::permission_denied):
                type = FileSystemError::ErrorType::AccessDenied;
                break;
            case static_cast<int>(std::errc::file_exists):
                type = FileSystemError::ErrorType::AlreadyExists;
                break;
            case static_cast<int>(std::errc::invalid_argument):
                type = FileSystemError::ErrorType::InvalidPath;
                break;
            case static_cast<int>(std::errc::io_error):
                type = FileSystemError::ErrorType::IOError;
                break;
        }

        throw FileSystemError(type, e.what());
    }

    FileSystem() = delete;
};

} // namespace Core

#endif // CORE_FILESYSTEM_H 