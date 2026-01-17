// condition.cpp
#include "condition.h"
#include <cctype>
#include <algorithm>

// ==================== 条件表达式分词 ====================

std::vector<ConditionToken> tokenizeCondition(const std::string& expr) {
    std::vector<ConditionToken> tokens;
    std::string current;

    for (size_t i = 0; i < expr.length(); i++) {
        char c = expr[i];

        if (std::isspace(c)) {
            if (!current.empty()) {
                // 处理当前token
                ConditionToken token;

                // 检查是否为操作符
                if (current == "&&") {
                    token.type = ConditionToken::OPERATOR;
                    token.value = current;
                    token.op = OP_AND;
                }
                else if (current == "||") {
                    token.type = ConditionToken::OPERATOR;
                    token.value = current;
                    token.op = OP_OR;
                }
                else if (current == "==") {
                    token.type = ConditionToken::OPERATOR;
                    token.value = current;
                    token.op = OP_EQ;
                }
                else if (current == "!=") {
                    token.type = ConditionToken::OPERATOR;
                    token.value = current;
                    token.op = OP_NE;
                }
                else if (current == "<") {
                    token.type = ConditionToken::OPERATOR;
                    token.value = current;
                    token.op = OP_LT;
                }
                else if (current == ">") {
                    token.type = ConditionToken::OPERATOR;
                    token.value = current;
                    token.op = OP_GT;
                }
                else if (current == "<=") {
                    token.type = ConditionToken::OPERATOR;
                    token.value = current;
                    token.op = OP_LE;
                }
                else if (current == ">=") {
                    token.type = ConditionToken::OPERATOR;
                    token.value = current;
                    token.op = OP_GE;
                }
                else if (current == "(") {
                    token.type = ConditionToken::PAREN_OPEN;
                    token.value = current;
                }
                else if (current == ")") {
                    token.type = ConditionToken::PAREN_CLOSE;
                    token.value = current;
                }
                else {
                    // 检查是否为数字
                    bool isNumber = true;
                    for (char ch : current) {
                        if (!std::isdigit(ch) && ch != '-' && ch != '+') {
                            isNumber = false;
                            break;
                        }
                    }

                    if (isNumber) {
                        token.type = ConditionToken::NUMBER;
                        token.value = current;
                    }
                    else {
                        token.type = ConditionToken::VAR;
                        token.value = current;
                    }
                }

                tokens.push_back(token);
                current.clear();
            }
            continue;
        }

        // 处理操作符
        if (c == '&' || c == '|' || c == '=' || c == '!' || c == '<' || c == '>' ||
            c == '(' || c == ')') {

            if (!current.empty()) {
                // 处理当前token
                ConditionToken token;
                token.type = ConditionToken::VAR; // 假设是变量
                token.value = current;
                tokens.push_back(token);
                current.clear();
            }

            // 处理操作符
            std::string opStr(1, c);
            if (i + 1 < expr.length()) {
                char next = expr[i + 1];
                if ((c == '&' && next == '&') ||
                    (c == '|' && next == '|') ||
                    (c == '=' && next == '=') ||
                    (c == '!' && next == '=') ||
                    (c == '<' && next == '=') ||
                    (c == '>' && next == '=')) {
                    opStr += next;
                    i++; // 跳过下一个字符
                }
            }

            ConditionToken token;
            if (opStr == "&&") {
                token.type = ConditionToken::OPERATOR;
                token.value = opStr;
                token.op = OP_AND;
            }
            else if (opStr == "||") {
                token.type = ConditionToken::OPERATOR;
                token.value = opStr;
                token.op = OP_OR;
            }
            else if (opStr == "==") {
                token.type = ConditionToken::OPERATOR;
                token.value = opStr;
                token.op = OP_EQ;
            }
            else if (opStr == "!=") {
                token.type = ConditionToken::OPERATOR;
                token.value = opStr;
                token.op = OP_NE;
            }
            else if (opStr == "<") {
                token.type = ConditionToken::OPERATOR;
                token.value = opStr;
                token.op = OP_LT;
            }
            else if (opStr == ">") {
                token.type = ConditionToken::OPERATOR;
                token.value = opStr;
                token.op = OP_GT;
            }
            else if (opStr == "<=") {
                token.type = ConditionToken::OPERATOR;
                token.value = opStr;
                token.op = OP_LE;
            }
            else if (opStr == ">=") {
                token.type = ConditionToken::OPERATOR;
                token.value = opStr;
                token.op = OP_GE;
            }
            else if (opStr == "(") {
                token.type = ConditionToken::PAREN_OPEN;
                token.value = opStr;
            }
            else if (opStr == ")") {
                token.type = ConditionToken::PAREN_CLOSE;
                token.value = opStr;
            }

            tokens.push_back(token);
        }
        else {
            current += c;
        }
    }

    // 处理最后一个token
    if (!current.empty()) {
        ConditionToken token;
        token.type = ConditionToken::VAR;
        token.value = current;
        tokens.push_back(token);
    }

    return tokens;
}

// ==================== 获取操作符优先级 ====================

int getOpPriority(ConditionOp op) {
    switch (op) {
    case OP_OR: return 1;
    case OP_AND: return 2;
    case OP_EQ: case OP_NE: case OP_LT: case OP_GT: case OP_LE: case OP_GE: return 3;
    default: return 0;
    }
}

// ==================== 计算简单表达式 ====================

bool evaluateSimpleCondition(const std::string& leftStr, ConditionOp op, 
                             const std::string& rightStr, const GameState& gameState) {
    int leftVal, rightVal;

    // 获取左值
    if (std::isdigit(leftStr[0]) || 
        (leftStr[0] == '-' && leftStr.length() > 1 && std::isdigit(leftStr[1]))) {
        leftVal = std::stoi(leftStr);
    }
    else {
        leftVal = gameState.getVar(leftStr);
    }

    // 获取右值
    if (std::isdigit(rightStr[0]) || 
        (rightStr[0] == '-' && rightStr.length() > 1 && std::isdigit(rightStr[1]))) {
        rightVal = std::stoi(rightStr);
    }
    else {
        rightVal = gameState.getVar(rightStr);
    }

    // 进行比较
    switch (op) {
    case OP_EQ: return leftVal == rightVal;
    case OP_NE: return leftVal != rightVal;
    case OP_LT: return leftVal < rightVal;
    case OP_GT: return leftVal > rightVal;
    case OP_LE: return leftVal <= rightVal;
    case OP_GE: return leftVal >= rightVal;
    default: return false;
    }
}

// ==================== 递归计算条件表达式 ====================

bool evaluateCondition(const std::vector<ConditionToken>& tokens, 
                       const GameState& gameState, size_t& index) {
    std::vector<bool> values;
    std::vector<ConditionOp> ops;

    while (index < tokens.size()) {
        const auto& token = tokens[index];

        if (token.type == ConditionToken::PAREN_OPEN) {
            index++;
            bool subResult = evaluateCondition(tokens, gameState, index);
            values.push_back(subResult);
        }
        else if (token.type == ConditionToken::VAR || token.type == ConditionToken::NUMBER) {
            // 处理简单比较表达式
            if (index + 2 < tokens.size()) {
                const auto& opToken = tokens[index + 1];
                const auto& rightToken = tokens[index + 2];

                if (opToken.type == ConditionToken::OPERATOR &&
                    (opToken.op >= OP_EQ && opToken.op <= OP_GE)) {

                    bool result = evaluateSimpleCondition(token.value, opToken.op,
                                                          rightToken.value, gameState);
                    values.push_back(result);
                    index += 3;
                    continue;
                }
            }

            // 如果是单个变量，检查是否为真（非零）
            if (token.type == ConditionToken::VAR) {
                int varVal = gameState.getVar(token.value);
                values.push_back(varVal != 0);
            }
            else {
                // 数字直接转为bool
                int numVal = std::stoi(token.value);
                values.push_back(numVal != 0);
            }
            index++;
        }
        else if (token.type == ConditionToken::OPERATOR &&
                (token.op == OP_AND || token.op == OP_OR)) {
            ops.push_back(token.op);
            index++;
        }
        else if (token.type == ConditionToken::PAREN_CLOSE) {
            index++;
            break;
        }
        else {
            // 跳过其他token
            index++;
        }
    }

    // 从左到右计算逻辑运算（&& 优先级高于 ||）
    bool result = values.empty() ? false : values[0];
    size_t opIndex = 0;

    for (size_t i = 1; i < values.size(); i++) {
        if (opIndex < ops.size()) {
            ConditionOp op = ops[opIndex];
            if (op == OP_AND) {
                result = result && values[i];
            }
            else if (op == OP_OR) {
                result = result || values[i];
            }
            opIndex++;
        }
    }

    return result;
}