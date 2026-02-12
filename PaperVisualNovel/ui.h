// ui.h
#pragma once
#ifndef UI_H
#define UI_H

#include <string>
#include "header.h"

/**
 * @brief 控制台颜色输出
 */
void vnout(const std::string& out, double time, color color = white,
    bool with_newline = false, bool use_typewriter_effect = true);



/**
 * @brief 显示进度条
 */
void simpleProgressBar(float progress);

/**
 * @brief 获取当前时间字符串
 */
std::string logtimer();

/**
 * @brief 获取按键名称
 */
std::string getKeyName();

/**
 * @brief 计算编辑距离（用于模糊匹配）
 */
int calculateEditDistance(const std::string& s1, const std::string& s2);

/**
 * @brief 检查字符串相似度
 */
bool isSimilar(const std::string& input, const std::string& target, int maxDistance = 2);

/**
 * @brief 操作处理函数（处理ESC菜单等）
 */
int operate();

/**
 * @brief 日志等级枚举
 */
enum class LogGrade {
    INFO,      // 信息
    WARNING,   // 警告
    ERR,     // 错误
    DEBUG      // 调试
};

/**
 * @brief 日志编号枚举
 */
enum class LogCode {
    // 信息类 (I1xxx)
    GAME_START = 1001,          // 游戏启动
    GAME_LOADED = 1002,         // 游戏加载成功
    GAME_SAVED = 1003,          // 游戏保存成功
    PLUGIN_LOADED = 1004,       // 插件加载成功
    ENDING_SAVED = 1005,        // 结局保存成功

    // 警告类 (W2xxx)
    FILE_NOT_FOUND = 2001,      // 文件不存在
    PLUGIN_MISSING = 2002,      // 插件缺失
    VERSION_MISMATCH = 2003,    // 版本不匹配
    SAVE_CORRUPTED = 2004,      // 存档可能损坏
    FALLBACK_USED = 2005,       // 回退到备用方法

    // 错误类 (E3xxx)
    PARSE_ERROR = 3001,         // 解析错误
    COMMAND_UNKNOWN = 3002,     // 未知命令
    JUMP_INVALID = 3003,        // 无效跳转
    FILE_OPEN_FAILED = 3004,    // 文件打开失败
    CONDITION_INVALID = 3005,   // 条件表达式无效
    PLUGIN_EXEC_FAILED = 3006,  // 插件执行失败
    MEMORY_ERROR = 3007,        // 内存错误

    // 调试类 (D4xxx)
    PERFORMANCE = 4001,         // 性能评估
    TOKEN_START = 4002,        // Tokenization开始
    TOKEN_COMPLETE = 4003,     // Tokenization完成
    EXEC_START = 4004,         // 执行开始
    EXEC_COMPLETE = 4005       // 执行完成
};

/**
 * @brief 日志输出函数（带编号）
 * @param logGrade 日志等级
 * @param code 日志编号
 * @param out 日志内容
 */
void Log(LogGrade logGrade, LogCode code, const std::string& out);
std::string logCodeToString(LogCode code);
/**
 * @brief 性能评估日志（便捷宏）
 */
#define LOG_PERF(operation, result, time_ms, memory_kb) \
    Log(LogGrade::DEBUG, LogCode::PERFORMANCE, \
        operation + " " + result + " (took " + \
        std::to_string(static_cast<int>(time_ms)) + "ms " + \
        std::to_string(static_cast<int>(memory_kb)) + "KB)")

 /**
  * @brief 格式化错误输出（带位置指示）
  */
void formatErrorOutput(const std::string& errorCode,
    const std::string& errorType,
    const std::string& message,
    const std::string& line,
    size_t lineNumber,
    size_t position,
    const std::string& hint,
    const std::string& docUrl);

#endif // UI_H