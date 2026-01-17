// gamestate.h
#pragma once
#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <string>
#include <vector>
#include <map>

/**
 * @brief 游戏状态管理器类
 * 
 * 负责管理游戏的变量、选择历史、结局收集等信息
 */
class GameState {
private:
    std::map<std::string, int> variables;      // 变量存储
    std::vector<std::string> choiceHistory;    // 选择历史
    std::vector<std::string> collectedEndings; // 已收集的结局
    std::vector<std::string> allEndings;       // 所有可能的结局

public:
    GameState() = default;
    
    // 变量操作
    void setVar(const std::string& name, int value);
    void addVar(const std::string& name, int value);
    int getVar(const std::string& name) const;
    bool hasVar(const std::string& name) const;
    const std::map<std::string, int>& getAllVariables() const;
    
    // 选择历史管理
    void recordChoice(const std::string& choice);
    const std::vector<std::string>& getChoiceHistory() const;
    
    // 结局管理
    void addEnding(const std::string& endingName);
    void registerEnding(const std::string& endingName);
    const std::vector<std::string>& getCollectedEndings() const;
    const std::vector<std::string>& getAllEndings() const;
    int getTotalEndingsCount() const;
    int getCollectedEndingsCount() const;
    
    // 状态管理
    void clear();
    
    // 序列化/反序列化
    std::string serialize() const;
    void deserialize(const std::string& data);
};

#endif // GAMESTATE_H