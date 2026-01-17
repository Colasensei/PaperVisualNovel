// parser.h
#pragma once
#ifndef PARSER_H
#define PARSER_H

#include "gamestate.h"
#include "condition.h"
#include <map>

//12
/**
 * @brief 选择选项结构
 */
struct ChoiceOption {
    std::string label;      // 跳转的标签
    std::string text;       // 显示的文本
};

/**
 * @brief 解析标签映射
 */
std::map<std::string, int> parseLabels(const std::vector<std::string>& lines);

/**
 * @brief 解析跳转目标
 */
int parseJumpTarget(const std::string& target, const std::map<std::string, int>& labels,
                    bool& isLabel);




/**
 * @brief 执行单行PGN命令
 * @return 执行状态：-1表示退出游戏，0表示正常执行，1表示跳转到指定行
 */
std::pair<int, size_t> executeLine(const std::string& line, GameState& gameState,
                                   size_t currentLine, const std::vector<std::string>& allLines,
                                   const std::string& where, int indentLevel,
                                   const std::map<std::string, int>& labels);

#endif // PARSER_H