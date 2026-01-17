// fileutils.h
#pragma once
#ifndef FILEUTILS_H
#define FILEUTILS_H

#include "gamestate.h"
#include <string>
#include <vector>
#include <filesystem>
#include "header.h"

namespace fs = std::filesystem;

// 存档管理
bool saveGame(const std::string& scriptPath, size_t currentLine,
              const GameState& gameState, const std::string& saveName = "autosave");
bool loadGame(const std::string& savePath, SaveData& saveData);
bool hasSaveFile(const std::string& scriptPath);
std::string getSaveInfo(const std::string& scriptPath);

// 文件安全操作
bool safeViewFile(const std::string& filepath);
void overwriteLine(const std::string& filename, int lineToOverwrite, 
                   const std::string& newContent);

// 结局文件操作
std::vector<std::string> readCollectedEndings(const std::string& gameFolder);
void saveEnding(const std::string& gameFolder, const std::string& endingName, 
                GameState& gameState);
void loadAllEndings(const std::vector<std::string>& lines, GameState& gameState);

// 游戏统计
int countTotalEndingsInScript(const std::string& scriptPath);
std::pair<int, int> getGameEndingStats(const std::string& gameFolderPath);
std::string getGameFolderName(const std::string& fullPath);

// 配置文件
std::string readCfg(const std::string& key);
void updateFirstRunFlag(bool value);

#endif // FILEUTILS_H