// condition.h
#pragma once
#ifndef CONDITION_H
#define CONDITION_H

#include "gamestate.h"
#include <vector>
#include <string>

/**
 * @brief 条件运算符类型
 */
enum ConditionOp {
    OP_NONE = 0,
    OP_AND,     // &&
    OP_OR,      // ||
    OP_EQ,      // ==
    OP_NE,      // !=
    OP_LT,      // <
    OP_GT,      // >
    OP_LE,      // <=
    OP_GE       // >=
};

/**
 * @brief 条件表达式Token结构
 */
struct ConditionToken {
    enum Type {
        VAR,
        NUMBER,
        OPERATOR,
        PAREN_OPEN,
        PAREN_CLOSE
    } type;

    std::string value;
    ConditionOp op;
};

// 条件表达式解析函数
std::vector<ConditionToken> tokenizeCondition(const std::string& expr);
int getOpPriority(ConditionOp op);
bool evaluateSimpleCondition(const std::string& leftStr, ConditionOp op, 
                             const std::string& rightStr, const GameState& gameState);
bool evaluateCondition(const std::vector<ConditionToken>& tokens, 
                       const GameState& gameState, size_t& index);

#endif // CONDITION_H