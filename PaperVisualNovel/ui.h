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
 * @brief 日志输出函数
 * @param logGrade 日志等级
 * @param out 日志内容
 */
void Log(LogGrade logGrade, const std::string& out);

#endif // UI_H