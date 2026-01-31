// gamestate.cpp
#include "gamestate.h"
#include <sstream>

// ==================== 变量操作 ====================

void GameState::setVar(const std::string& name, int value) {
    variables[name] = value;
}

void GameState::addVar(const std::string& name, int value) {
    if (variables.find(name) == variables.end()) {
        variables[name] = 0;
    }
    variables[name] += value;
}

int GameState::getVar(const std::string& name) const {
    auto it = variables.find(name);
    if (it != variables.end()) {
        return it->second;
    }
    return 0;
}

bool GameState::hasVar(const std::string& name) const {
    return variables.find(name) != variables.end();
}

const std::map<std::string, int>& GameState::getAllVariables() const {
    return variables;
}

// ==================== 选择历史管理 ====================

void GameState::recordChoice(const std::string& choice) {
    choiceHistory.push_back(choice);
}

const std::vector<std::string>& GameState::getChoiceHistory() const {
    return choiceHistory;
}

// ==================== 结局管理 ====================

void GameState::addEnding(const std::string& endingName) {
    // 检查是否已经收集过这个结局
    for (const auto& ending : collectedEndings) {
        if (ending == endingName) {
            return; // 已经收集过，不再添加
        }
    }
    collectedEndings.push_back(endingName);
}

void GameState::registerEnding(const std::string& endingName) {
    // 注册一个可能的结局（用于统计总数）
    for (const auto& ending : allEndings) {
        if (ending == endingName) {
            return; // 已经注册过
        }
    }
    allEndings.push_back(endingName);
}

const std::vector<std::string>& GameState::getCollectedEndings() const {
    return collectedEndings;
}

const std::vector<std::string>& GameState::getAllEndings() const {
    return allEndings;
}

int GameState::getTotalEndingsCount() const {
    return allEndings.size();
}

int GameState::getCollectedEndingsCount() const {
    return collectedEndings.size();
}

// ==================== 状态管理 ====================

void GameState::clear() {
    variables.clear();
    choiceHistory.clear();
}


// ==================== 字符串变量操作 ====================

void GameState::setStringVar(const std::string& name, const std::string& value) {
    stringVars[name] = value;
}

std::string GameState::getStringVar(const std::string& name) const {
    auto it = stringVars.find(name);
    if (it != stringVars.end()) {
        return it->second;
    }
    return "";
}

bool GameState::hasStringVar(const std::string& name) const {
    return stringVars.find(name) != stringVars.end();
}

const std::map<std::string, std::string>& GameState::getAllStringVariables() const {
    return stringVars;
}
// ==================== 更新序列化/反序列化 ====================

std::string GameState::serialize() const {
    std::stringstream ss;

    // 序列化整数变量
    ss << "[VARIABLES]" << std::endl;
    for (const auto& var : variables) {
        ss << var.first << "=" << var.second << std::endl;
    }

    // 序列化字符串变量
    ss << "[STRING_VARIABLES]" << std::endl;
    for (const auto& var : stringVars) {
        // 对字符串进行转义，避免换行符等问题
        std::string escaped = var.second;
        // 替换换行符
        size_t pos = 0;
        while ((pos = escaped.find("\n", pos)) != std::string::npos) {
            escaped.replace(pos, 1, "\\n");
            pos += 2;
        }
        ss << var.first << "=" << escaped << std::endl;
    }

    // 序列化选择历史
    ss << "[CHOICE_HISTORY]" << std::endl;
    for (const auto& choice : choiceHistory) {
        ss << choice << std::endl;
    }

    // 序列化已收集的结局
    ss << "[COLLECTED_ENDINGS]" << std::endl;
    for (const auto& ending : collectedEndings) {
        ss << ending << std::endl;
    }
    return ss.str();
}

void GameState::deserialize(const std::string& data) {
    clear(); // 清空当前状态

    std::stringstream ss(data);
    std::string line;
    std::string currentSection;

    while (getline(ss, line)) {
        if (line.empty()) continue;

        // 处理章节标记
        if (line[0] == '[' && line.back() == ']') {
            currentSection = line;
            continue;
        }

        if (currentSection == "[VARIABLES]") {
            size_t equalsPos = line.find('=');
            if (equalsPos != std::string::npos) {
                std::string varName = line.substr(0, equalsPos);
                std::string varValueStr = line.substr(equalsPos + 1);
                try {
                    int varValue = std::stoi(varValueStr);
                    variables[varName] = varValue;
                }
                catch (...) {
                    // 忽略转换错误
                }
            }
        }
        else if (currentSection == "[STRING_VARIABLES]") {
            size_t equalsPos = line.find('=');
            if (equalsPos != std::string::npos) {
                std::string varName = line.substr(0, equalsPos);
                std::string varValue = line.substr(equalsPos + 1);

                // 反转义字符串
                size_t pos = 0;
                while ((pos = varValue.find("\\n", pos)) != std::string::npos) {
                    varValue.replace(pos, 2, "\n");
                    pos += 1;
                }

                stringVars[varName] = varValue;
            }
        }
        else if (currentSection == "[CHOICE_HISTORY]") {
            choiceHistory.push_back(line);
        }
        else if (currentSection == "[COLLECTED_ENDINGS]") {
            collectedEndings.push_back(line);
        }
    }
}


