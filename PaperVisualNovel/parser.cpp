// parser.cpp
#include "parser.h"
#include "ui.h"
#include "fileutils.h"
#include <Windows.h>
#include <sstream>
#include <map>
#include <chrono>

// ==================== 标签解析 ====================

std::map<std::string, int> parseLabels(const std::vector<std::string>& lines) {
    std::map<std::string, int> labels;

    for (size_t i = 0; i < lines.size(); i++) {
        std::string line = lines[i];
        std::stringstream ss(line);
        std::string token;

        if (ss >> token) {
            if (token.back() == ':') {
                std::string labelName = token.substr(0, token.length() - 1);
                labels[labelName] = i + 1;
            }
        }
    }

    return labels;
}

// ==================== 跳转目标解析 ====================

int parseJumpTarget(const std::string& target, const std::map<std::string, int>& labels,
    bool& isLabel) {
    try {
        int lineNum = std::stoi(target);
        isLabel = false;
        return lineNum;
    }
    catch (const std::exception&) {
        std::string labelName = target;

        auto it = labels.find(labelName);
        if (it != labels.end()) {
            isLabel = true;
            return it->second;
        }

        if (!labelName.empty() && labelName.back() == ':') {
            labelName.pop_back();
            it = labels.find(labelName);
            if (it != labels.end()) {
                isLabel = true;
                return it->second;
            }
        }

        return -1;
    }
}

// ==================== 执行行 ====================

std::pair<int, size_t> executeLine(const std::string& line, GameState& gameState,
    size_t currentLine, const std::vector<std::string>& allLines,
    const std::string& where, int indentLevel,
    const std::map<std::string, int>& labels) {
    extern CurrentGameInfo g_currentGameInfo;

    Log(LogGrade::INFO, LogCode::EXEC_START, "Executing line: " + to_string(currentLine + 1));
    Log(LogGrade::DEBUG, LogCode::EXEC_START, "Now executing: " + line);

    std::stringstream ss(line);
    std::string cmd;
    ss >> cmd;
    Log(LogGrade::DEBUG, LogCode::EXEC_START, "Command: " + cmd);

    if (cmd.empty() || cmd == "//" || cmd == "#") {
        Log(LogGrade::DEBUG, LogCode::EXEC_COMPLETE, "Empty line or comment, skipping.");
        return { 0, currentLine + 1 };
    }

    if (cmd.back() == ':') {
        Log(LogGrade::DEBUG, LogCode::EXEC_COMPLETE, "Label found, skipping.");
        return { 0, currentLine + 1 };
    }

    // ==================== 游戏结束命令 ====================
    if (cmd == "end" || cmd == "END") {
        Log(LogGrade::DEBUG, LogCode::EXEC_COMPLETE, "Game end command found.");
        std::cout << "游戏结束" << std::endl;
        system("pause");
        Log(LogGrade::DEBUG, LogCode::EXEC_COMPLETE, "Exiting game.");
        return { -1, 0 };
    }

    // ==================== 结局名命令 ====================
    if (cmd == "endname" || cmd == "ENDNAME") {
        Log(LogGrade::DEBUG, LogCode::EXEC_START, "End name command found.");
        std::string endingName;
        getline(ss, endingName);

        size_t start = endingName.find_first_not_of(" ");
        if (start != std::string::npos) {
            endingName = endingName.substr(start);
        }

        if (!endingName.empty()) {
            std::string gameFolder = "";
            size_t novelPos = where.find("Novel\\");
            if (novelPos != std::string::npos) {
                size_t startPos = novelPos + 6;
                size_t endPos = where.find("\\", startPos);
                if (endPos != std::string::npos) {
                    gameFolder = where.substr(startPos, endPos - startPos);
                }
            }

            if (!gameFolder.empty()) {
                saveEnding(gameFolder, endingName, gameState);
                Log(LogGrade::INFO, LogCode::ENDING_SAVED, "Ending saved: " + endingName);

                int collected = gameState.getCollectedEndingsCount();
                int total = gameState.getTotalEndingsCount();

                std::cout << std::endl;
                std::cout << "==============================" << std::endl;
                std::cout << "结局达成：" << endingName << std::endl;
                std::cout << "已收集结局：" << collected << "/" << total << std::endl;
                std::cout << "==============================" << std::endl;
                std::cout << std::endl;

                std::cout << "按任意键继续..." << std::endl;
                getKeyName();
            }
        }

        Log(LogGrade::DEBUG, LogCode::EXEC_COMPLETE, "End name command executed.");
        return { 0, currentLine + 1 };
    }

    // ==================== 等待命令 ====================
    if (cmd == "wait" || cmd == "WAIT") {
        Log(LogGrade::DEBUG, LogCode::EXEC_START, "Wait command found.");
        int wait;
        if (ss >> wait) {
            Log(LogGrade::DEBUG, LogCode::EXEC_START, "Wait time: " + std::to_string(wait));
            Sleep(wait);
        }
        return { 0, currentLine + 1 };
    }

    // ==================== 说话命令（say） ====================
    if (cmd == "say" || cmd == "SAY") {
        Log(LogGrade::DEBUG, LogCode::EXEC_START, "Say command found.");
        std::string rest;
        getline(ss, rest);

        size_t start = rest.find_first_not_of(" ");
        if (start == std::string::npos) {
            return { 0, currentLine + 1 };
        }
        rest = rest.substr(start);

        std::string text = "";
        double time_val = 0.5;
        std::string incolor = "white";

        if (rest[0] == '"') {
            Log(LogGrade::DEBUG, LogCode::EXEC_START, "Quoted string found.");
            size_t quote_end = 0;
            bool escaped = false;
            size_t errorPos = 0;

            for (size_t i = 1; i < rest.length(); i++) {
                errorPos = i;
                if (escaped) {
                    switch (rest[i]) {
                    case '"': text += '"'; break;
                    case 'n': text += '\n'; break;
                    case 't': text += '\t'; break;
                    case 'r': text += '\r'; break;
                    case '\\': text += '\\'; break;
                    default: text += rest[i]; break;
                    }
                    escaped = false;
                }
                else if (rest[i] == '\\') {
                    escaped = true;
                }
                else if (rest[i] == '"') {
                    quote_end = i;
                    break;
                }
                else {
                    text += rest[i];
                }
            }

            if (quote_end == 0) {
                Log(LogGrade::ERR, LogCode::PARSE_ERROR,
                    "Missing closing quote in say command at line " + std::to_string(currentLine + 1));

                formatErrorOutput(
                    logCodeToString(LogCode::PARSE_ERROR),
                    "ParseError",
                    "Unterminated string literal at 'say' command",
                    line,
                    currentLine + 1,
                    errorPos,
                    "Add closing double quote (\") at the end of the string. "
                    "Use \\\" to include double quotes inside the string.",
                    "https://github.com/Colasensei/PaperVisualNovel/tree/master/Docs/errors/E3001.md"
                );

                MessageBoxA(NULL, "错误：say命令中的字符串缺少结束引号",
                    "错误", MB_ICONERROR | MB_OK);
                return { 0, currentLine + 1 };
            }

            std::string remaining = rest.substr(quote_end + 1);
            std::stringstream remainingSS(remaining);
            std::vector<std::string> tokens;
            std::string token;

            while (remainingSS >> token) {
                tokens.push_back(token);
            }

            if (!tokens.empty()) {
                std::string last_token = tokens.back();

                if (last_token == "black" || last_token == "blue" || last_token == "green" ||
                    last_token == "aqua" || last_token == "red" || last_token == "purple" ||
                    last_token == "yellow" || last_token == "white") {
                    incolor = last_token;
                    tokens.pop_back();
                }

                if (!tokens.empty()) {
                    last_token = tokens.back();
                    try {
                        time_val = std::stod(last_token);
                        tokens.pop_back();
                    }
                    catch (const std::exception&) {
                        time_val = 0.5;
                    }
                }
            }
        }
        else {
            Log(LogGrade::DEBUG, LogCode::EXEC_START, "Unquoted string found.");
            std::stringstream rest_ss(rest);
            std::vector<std::string> tokens;
            std::string token;

            while (rest_ss >> token) {
                tokens.push_back(token);
            }

            if (!tokens.empty()) {
                std::vector<std::string> text_parts;

                while (!tokens.empty()) {
                    std::string token = tokens.back();

                    if (token == "black" || token == "blue" || token == "green" ||
                        token == "aqua" || token == "red" || token == "purple" ||
                        token == "yellow" || token == "white") {
                        if (incolor == "white") {
                            incolor = token;
                            tokens.pop_back();
                            continue;
                        }
                    }

                    try {
                        char* end;
                        double time_test = strtod(token.c_str(), &end);
                        if (end != token.c_str() && *end == '\0') {
                            if (time_val == 0.5) {
                                time_val = time_test;
                                tokens.pop_back();
                                continue;
                            }
                        }
                    }
                    catch (...) {
                    }

                    text_parts.insert(text_parts.begin(), token);
                    tokens.pop_back();
                }

                for (size_t i = 0; i < text_parts.size(); i++) {
                    if (i > 0) text += " ";
                    text += text_parts[i];
                }
            }
        }

        std::string final_text = "";
        size_t pos = 0;

        while (pos < text.length()) {
            size_t var_start = text.find("${", pos);

            if (var_start == std::string::npos) {
                final_text += text.substr(pos);
                break;
            }

            final_text += text.substr(pos, var_start - pos);
            size_t var_end = text.find("}", var_start);
            if (var_end == std::string::npos) {
                final_text += text.substr(var_start);
                break;
            }

            std::string var_name = text.substr(var_start + 2, var_end - var_start - 2);

            std::string string_value = gameState.getStringVar(var_name);
            if (!string_value.empty()) {
                final_text += string_value;
            }
            else {
                int var_value = gameState.getVar(var_name);
                final_text += std::to_string(var_value);
            }

            pos = var_end + 1;
        }

        color text_color = white;
        if (incolor == "black") text_color = black;
        else if (incolor == "blue") text_color = blue;
        else if (incolor == "green") text_color = green;
        else if (incolor == "aqua") text_color = aqua;
        else if (incolor == "red") text_color = red;
        else if (incolor == "purple") text_color = purple;
        else if (incolor == "yellow") text_color = yellow;

        Log(LogGrade::DEBUG, LogCode::EXEC_START, "Text: " + final_text);
        vnout(final_text, time_val, text_color, false, true);

        int result = operate();
        if (result == 1) {
            Log(LogGrade::INFO, LogCode::GAME_SAVED, "Save and exit.");
            return { -1, 0 };
        }
        else if (result == 2) {
            Log(LogGrade::INFO, LogCode::GAME_START, "Exit without saving.");
            return { -2, 0 };
        }
        else if (result == 3) {
            Log(LogGrade::DEBUG, LogCode::EXEC_START,
                "Debug Terminal Jump to line " + std::to_string(g_currentGameInfo.currentLine + 1));
            return { 1, g_currentGameInfo.currentLine };
        }

        Log(LogGrade::DEBUG, LogCode::EXEC_COMPLETE, "Next line: " + std::to_string(currentLine + 1));
        return { 0, currentLine + 1 };
    }

    // ==================== 输入命令 ====================
    if (cmd == "input" || cmd == "INPUT") {
        Log(LogGrade::DEBUG, LogCode::EXEC_START, "INPUT command detected.");

        std::string prompt, varName;

        if (line.find('"') != std::string::npos) {
            size_t firstQuote = line.find('"');
            size_t secondQuote = line.find('"', firstQuote + 1);

            if (secondQuote == std::string::npos) {
                Log(LogGrade::ERR, LogCode::PARSE_ERROR,
                    "Invalid input command: missing closing quote at line " + std::to_string(currentLine + 1));

                formatErrorOutput(
                    logCodeToString(LogCode::PARSE_ERROR),
                    "ParseError",
                    "Unterminated string literal at 'input' command",
                    line,
                    currentLine + 1,
                    firstQuote + 1,
                    "Add closing double quote (\") at the end of the prompt string.",
                    "https://github.com/Colasensei/PaperVisualNovel/tree/master/Docs/errors/E3001.md"
                );

                MessageBoxA(NULL, "错误：input命令格式不正确，缺少结束引号",
                    "错误", MB_ICONERROR | MB_OK);
                return { 0, currentLine + 1 };
            }

            prompt = line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
            std::string rest = line.substr(secondQuote + 1);

            std::stringstream restSS(rest);
            if (!(restSS >> varName)) {
                Log(LogGrade::ERR, LogCode::PARSE_ERROR,
                    "Invalid input command: missing variable name at line " + std::to_string(currentLine + 1));
                MessageBoxA(NULL, "错误：input命令格式不正确，缺少变量名",
                    "错误", MB_ICONERROR | MB_OK);
                return { 0, currentLine + 1 };
            }
        }
        else {
            std::string rest;
            getline(ss, rest);
            std::stringstream restSS(rest);

            if (!(restSS >> prompt >> varName)) {
                Log(LogGrade::ERR, LogCode::PARSE_ERROR,
                    "Invalid input command: missing parameters at line " + std::to_string(currentLine + 1));
                MessageBoxA(NULL, "错误：input命令格式不正确，参数不足",
                    "错误", MB_ICONERROR | MB_OK);
                return { 0, currentLine + 1 };
            }
        }

        if (varName.empty()) {
            Log(LogGrade::ERR, LogCode::PARSE_ERROR,
                "Invalid input command: empty variable name at line " + std::to_string(currentLine + 1));
            MessageBoxA(NULL, "错误：input命令变量名不能为空",
                "错误", MB_ICONERROR | MB_OK);
            return { 0, currentLine + 1 };
        }

        std::cout << std::endl;
        std::cout << "\033[32m" << prompt << "\033[37m";

        std::string userInput;
        std::getline(std::cin, userInput);

        size_t start = userInput.find_first_not_of(" \t\n\r");
        if (start != std::string::npos) {
            size_t end = userInput.find_last_not_of(" \t\n\r");
            userInput = userInput.substr(start, end - start + 1);
        }

        if (!userInput.empty()) {
            gameState.setStringVar(varName, userInput);
            Log(LogGrade::INFO, LogCode::GAME_START,
                "Input saved to string variable: " + varName + " = \"" + userInput + "\"");
            std::cout << std::endl;
        }
        else {
            Log(LogGrade::WARNING, LogCode::GAME_START,
                "User input is empty for variable: " + varName);
            gameState.setStringVar(varName, "");
            std::cout << std::endl;
        }

        return { 0, currentLine + 1 };
    }

    // ==================== 显示变量值命令 ====================
    if (cmd == "sayvar" || cmd == "SAYVAR") {
        Log(LogGrade::DEBUG, LogCode::EXEC_START, "SAYVAR command detected.");
        std::string varName;
        double time_val;
        std::string incolor;

        if (ss >> varName >> time_val >> incolor) {
            Log(LogGrade::DEBUG, LogCode::EXEC_START, "Variable name: " + varName);
            int varValue = gameState.getVar(varName);
            std::string text = std::to_string(varValue);

            color text_color = white;
            if (incolor == "black") text_color = black;
            else if (incolor == "blue") text_color = blue;
            else if (incolor == "green") text_color = green;
            else if (incolor == "aqua") text_color = aqua;
            else if (incolor == "red") text_color = red;
            else if (incolor == "purple") text_color = purple;
            else if (incolor == "yellow") text_color = yellow;

            vnout(text, time_val, text_color, false, true);
            int result = operate();
            if (result == 1) {
                Log(LogGrade::INFO, LogCode::GAME_SAVED, "Save and exit.");
                return { -1, 0 };
            }
            else if (result == 2) {
                Log(LogGrade::INFO, LogCode::GAME_START, "Exit without saving.");
                return { -2, 0 };
            }
            else if (result == 3) {
                Log(LogGrade::DEBUG, LogCode::EXEC_START,
                    "Debug Terminal Jump to line " + std::to_string(g_currentGameInfo.currentLine + 1));
                return { 1, g_currentGameInfo.currentLine };
            }
            Log(LogGrade::DEBUG, LogCode::EXEC_COMPLETE, "Next line: " + std::to_string(currentLine + 1));
            return { 0, currentLine + 1 };
        }
    }

    // ==================== 显示文件命令 ====================
    if (cmd == "show" || cmd == "SHOW") {
        Log(LogGrade::DEBUG, LogCode::EXEC_START, "SHOW command detected.");
        std::string file_to_show;
        if (ss >> file_to_show) {
            Log(LogGrade::DEBUG, LogCode::EXEC_START, "File to show: " + file_to_show);
            std::string path = where + "archive\\" + file_to_show;
            safeViewFile(path);

            Log(LogGrade::DEBUG, LogCode::EXEC_COMPLETE, "Next line: " + std::to_string(currentLine + 1));
        }
        return { 0, currentLine + 1 };
    }

    // ==================== 选择命令 ====================
    if (cmd == "choose" || cmd == "CHOOSE") {
        Log(LogGrade::DEBUG, LogCode::EXEC_START, "CHOOSE command detected.");
        std::vector<ChoiceOption> options;
        std::vector<std::string> gumOptions;
        std::stringstream ss(line);
        std::string cmdWord;
        int optionCount;

        ss >> cmdWord >> optionCount;

        for (int i = 0; i < optionCount; i++) {
            std::string optionStr;
            if (ss >> optionStr) {
                size_t colonPos = optionStr.find(':');
                if (colonPos != std::string::npos) {
                    ChoiceOption option;
                    option.label = optionStr.substr(0, colonPos);
                    option.text = optionStr.substr(colonPos + 1);
                    options.push_back(option);

                    gumOptions.push_back(std::to_string(i + 1) + ". " + option.text);
                }
                else {
                    ChoiceOption option;
                    option.label = optionStr;
                    option.text = optionStr;
                    options.push_back(option);

                    gumOptions.push_back(std::to_string(i + 1) + ". " + optionStr);
                }
            }
        }

        Log(LogGrade::DEBUG, LogCode::EXEC_START, "Options parsed: " + std::to_string(options.size()));

        if (!gum::GumWrapper::is_available()) {
            Log(LogGrade::WARNING, LogCode::FALLBACK_USED, "Gum not available, falling back to original method");
            std::cout << "\033[90m" << "请选择：" << std::endl;
            for (size_t i = 0; i < options.size(); i++) {
                std::cout << i + 1 << ". ";
                vnout(options[i].text, 0.5, white, false, true);
                std::cout << "\033[90m";
                std::cout << std::endl;
            }
            std::cout << "请输入数字选择 (1-" << options.size() << "): ";

            int choice = -1;
            while (choice < 1 || choice > static_cast<int>(options.size())) {
                std::string input = getKeyName();
                std::cout << input;
                std::cout << std::endl;
                std::cout << "\033[0m";

                if (input.length() == 1 && std::isdigit(input[0])) {
                    choice = input[0] - '0';
                }
                else if (input == "ESC") {
                }
            }
            Log(LogGrade::INFO, LogCode::GAME_START, "User choice (fallback): " + std::to_string(choice));

            gameState.recordChoice(options[choice - 1].text);
            std::cout << options[choice - 1].text << endl;

            std::string targetLabel = options[choice - 1].label;
            bool isLabel;
            int jumpLine = parseJumpTarget(targetLabel, labels, isLabel);

            if (jumpLine > 0 && jumpLine <= static_cast<int>(allLines.size())) {
                Log(LogGrade::INFO, LogCode::EXEC_START, "Jump to line: " + std::to_string(jumpLine));
                return { 1, jumpLine - 1 };
            }
            else {
                Log(LogGrade::ERR, LogCode::JUMP_INVALID, "Invalid jump target: " + targetLabel);
                MessageBoxA(NULL, ("错误：选择目标无效 - " + targetLabel).c_str(),
                    "错误", MB_ICONERROR | MB_OK);
                return { -1, 0 };
            }
        }

        try {
            Log(LogGrade::DEBUG, LogCode::EXEC_START, "Using gum for selection");
            std::cout << endl;
            std::string selected = gum::GumWrapper::choose(gumOptions);
            std::string op = "";
            if (!selected.empty()) {
                op = selected.substr(0, 1);
                int choice = std::stoi(op);

                Log(LogGrade::INFO, LogCode::GAME_START, "User choice (gum): " + op);

                if (choice >= 1 && choice <= static_cast<int>(options.size())) {
                    gameState.recordChoice(options[choice - 1].text);
                    vnout("你选择了：" + options[choice - 1].text, 0.5, gray, true, true);

                    std::string targetLabel = options[choice - 1].label;
                    bool isLabel;
                    int jumpLine = parseJumpTarget(targetLabel, labels, isLabel);

                    if (jumpLine > 0 && jumpLine <= static_cast<int>(allLines.size())) {
                        Log(LogGrade::INFO, LogCode::EXEC_START, "Jump to line: " + std::to_string(jumpLine));
                        return { 1, jumpLine - 1 };
                    }
                    else {
                        Log(LogGrade::ERR, LogCode::JUMP_INVALID, "Invalid jump target: " + targetLabel);
                        MessageBoxA(NULL, ("错误：选择目标无效 - " + targetLabel).c_str(),
                            "错误", MB_ICONERROR | MB_OK);
                        return { -1, 0 };
                    }
                }
                else {
                    Log(LogGrade::ERR, LogCode::JUMP_INVALID, "Invalid choice index from gum: " + op);
                    throw std::runtime_error("Invalid choice index");
                }
            }
            else {
                Log(LogGrade::WARNING, LogCode::GAME_START, "Gum selection cancelled or empty");
                return { 0, currentLine + 1 };
            }
        }
        catch (const std::exception& e) {
            Log(LogGrade::ERR, LogCode::PLUGIN_EXEC_FAILED, "Gum selection error: " + std::string(e.what()));

            Log(LogGrade::WARNING, LogCode::FALLBACK_USED, "Falling back to original selection method");
            std::cout << "\033[90m" << "请选择：" << std::endl;
            for (size_t i = 0; i < options.size(); i++) {
                std::cout << i + 1 << ". ";
                vnout(options[i].text, 0.5, white, false, true);
                std::cout << "\033[90m";
                std::cout << std::endl;
            }
            std::cout << "请输入数字选择 (1-" << options.size() << "): ";

            int choice = -1;
            while (choice < 1 || choice > static_cast<int>(options.size())) {
                std::string input = getKeyName();
                std::cout << input;
                std::cout << std::endl;
                std::cout << "\033[0m";

                if (input.length() == 1 && std::isdigit(input[0])) {
                    choice = input[0] - '0';
                }
                else if (input == "ESC") {
                }
            }
            Log(LogGrade::INFO, LogCode::GAME_START, "User choice (fallback after gum error): " + std::to_string(choice));

            gameState.recordChoice(options[choice - 1].text);

            std::string targetLabel = options[choice - 1].label;
            bool isLabel;
            int jumpLine = parseJumpTarget(targetLabel, labels, isLabel);

            if (jumpLine > 0 && jumpLine <= static_cast<int>(allLines.size())) {
                Log(LogGrade::INFO, LogCode::EXEC_START, "Jump to line: " + std::to_string(jumpLine));
                return { 1, jumpLine - 1 };
            }
            else {
                Log(LogGrade::ERR, LogCode::JUMP_INVALID, "Invalid jump target: " + targetLabel);
                MessageBoxA(NULL, ("错误：选择目标无效 - " + targetLabel).c_str(),
                    "错误", MB_ICONERROR | MB_OK);
                return { -1, 0 };
            }
        }
    }

    // ==================== 清屏命令 ====================
    if (cmd == "cls" || cmd == "clean" || cmd == "CLS" || cmd == "CLEAN")
    {
        Log(LogGrade::DEBUG, LogCode::EXEC_START, "CLS command detected.");
        system("cls");
        return { 0, currentLine + 1 };
    }

    // ==================== 随机数命令 ====================
    if (cmd == "random" || cmd == "RANDOM") {
        Log(LogGrade::DEBUG, LogCode::EXEC_START, "RANDOM command detected.");
        std::string varName;
        int minVal, maxVal;
        if (ss >> varName >> minVal >> maxVal) {
            Log(LogGrade::DEBUG, LogCode::EXEC_START, "Variable name: " + varName);
            Log(LogGrade::DEBUG, LogCode::EXEC_START, "Min value: " + std::to_string(minVal));
            Log(LogGrade::DEBUG, LogCode::EXEC_START, "Max value: " + std::to_string(maxVal));

            if (minVal > maxVal) {
                std::swap(minVal, maxVal);
            }

            int range = maxVal - minVal + 1;
            int randomValue = minVal + (rand() % range);

            Log(LogGrade::DEBUG, LogCode::EXEC_START, "Random value: " + std::to_string(randomValue));
            gameState.setVar(varName, randomValue);
        }
        return { 0, currentLine + 1 };
    }

    // ==================== 设置变量命令 ====================
    if (cmd == "set" || cmd == "SET") {
        Log(LogGrade::DEBUG, LogCode::EXEC_START, "SET command detected.");
        std::string varName, op;
        int value;
        if (ss >> varName >> op >> value) {
            if (op == "=") {
                gameState.setVar(varName, value);
            }
            else if (op == "+=") {
                gameState.addVar(varName, value);
            }
            else if (op == "-=") {
                gameState.addVar(varName, -value);
            }
            else if (op == "*=") {
                if (gameState.hasVar(varName)) {
                    int current = gameState.getVar(varName);
                    gameState.setVar(varName, current * value);
                }
            }
            else if (op == "/=") {
                if (gameState.hasVar(varName) && value != 0) {
                    int current = gameState.getVar(varName);
                    gameState.setVar(varName, current / value);
                }
            }
            else {
                Log(LogGrade::ERR, LogCode::COMMAND_UNKNOWN, "Invalid operation: " + op);
                MessageBoxA(NULL, ("错误：无效的操作符 - " + op).c_str(),
                    "错误", MB_ICONERROR | MB_OK);
            }
        }
        Log(LogGrade::INFO, LogCode::GAME_START,
            "Did " + varName + " " + op + " " + std::to_string(value));
        Log(LogGrade::INFO, LogCode::GAME_START,
            "Variable " + varName + " set to " + std::to_string(gameState.getVar(varName)));
        return { 0, currentLine + 1 };
    }

    // ==================== 跳转命令 ====================
    if (cmd == "jump" || cmd == "JUMP") {
        Log(LogGrade::DEBUG, LogCode::EXEC_START, "JUMP command detected.");
        std::string target;
        if (ss >> target) {

            bool isLabel;
            int jumpLine = parseJumpTarget(target, labels, isLabel);

            if (jumpLine > 0 && jumpLine <= static_cast<int>(allLines.size())) {
                Log(LogGrade::INFO, LogCode::EXEC_START, "Jump to line: " + std::to_string(jumpLine));
                return { 1, jumpLine - 1 };
            }
            else {
                Log(LogGrade::ERR, LogCode::JUMP_INVALID, "Invalid jump target: " + target);
                MessageBoxA(NULL, ("错误：跳转目标无效 - " + target).c_str(),
                    "错误", MB_ICONERROR | MB_OK);
            }
        }

        Log(LogGrade::ERR, LogCode::PARSE_ERROR, "Invalid JUMP command format.");
        return { 0, currentLine + 1 };
    }

    // ==================== 条件命令 ====================
    if (cmd == "if" || cmd == "IF") {
        Log(LogGrade::DEBUG, LogCode::EXEC_START, "IF command detected.");
        std::string conditionExpr;
        getline(ss, conditionExpr);

        size_t lastSpace = conditionExpr.find_last_of(' ');
        if (lastSpace == std::string::npos) {
            Log(LogGrade::ERR, LogCode::CONDITION_INVALID,
                "Invalid IF command format at line " + std::to_string(currentLine + 1));
            Log(LogGrade::ERR, LogCode::CONDITION_INVALID, "Condition expression: " + conditionExpr);

            formatErrorOutput(
                logCodeToString(LogCode::CONDITION_INVALID),
                "ConditionError",
                "Missing jump target in 'if' command",
                line,
                currentLine + 1,
                conditionExpr.length(),
                "Add a jump target (line number or label) at the end of the condition. "
                "Example: if a > 10 end_label",
                "https://github.com/Colasensei/PaperVisualNovel/tree/master/Docs/errors/E3005.md"
            );

            MessageBoxA(NULL, "错误：if命令格式不正确", "错误", MB_ICONERROR | MB_OK);
            return { 0, currentLine + 1 };
        }

        std::string target = conditionExpr.substr(lastSpace + 1);
        conditionExpr = conditionExpr.substr(0, lastSpace);

        conditionExpr.erase(0, conditionExpr.find_first_not_of(" "));
        conditionExpr.erase(conditionExpr.find_last_not_of(" ") + 1);

        std::vector<ConditionToken> tokens = tokenizeCondition(conditionExpr);
        size_t index = 0;
        bool conditionMet = evaluateCondition(tokens, gameState, index);
        Log(LogGrade::DEBUG, LogCode::EXEC_START, "Condition met: " + std::to_string(conditionMet));

        if (conditionMet) {
            Log(LogGrade::INFO, LogCode::EXEC_START, "Condition met, jump to: " + target);
            bool isLabel;
            int jumpLine = parseJumpTarget(target, labels, isLabel);

            if (jumpLine > 0 && jumpLine <= static_cast<int>(allLines.size())) {
                Log(LogGrade::INFO, LogCode::EXEC_START, "Jump to line: " + std::to_string(jumpLine));
                return { 1, jumpLine - 1 };
            }
            else {
                Log(LogGrade::ERR, LogCode::JUMP_INVALID, "Invalid jump target: " + target);
                MessageBoxA(NULL, ("错误：跳转目标无效 - " + target).c_str(),
                    "错误", MB_ICONERROR | MB_OK);
            }
        }
        Log(LogGrade::INFO, LogCode::EXEC_START, "Condition not met, continue to next line.");
        return { 0, currentLine + 1 };
    }

    // ==================== 插件命令 ====================
    if (cmd == "plugin" || cmd == "PLUGIN" || cmd == "runplugin" || cmd == "RUNPLUGIN") {
        Log(LogGrade::INFO, LogCode::PLUGIN_LOADED,
            "PLUGIN command detected at line " + std::to_string(currentLine + 1));

        std::string pluginName, runArgs;

        std::string rest;
        getline(ss, rest);

        size_t firstQuote = std::string::npos;
        bool inQuotes = false;
        bool escaped = false;

        for (size_t i = 0; i < rest.length(); i++) {
            if (escaped) {
                escaped = false;
                continue;
            }

            if (rest[i] == '\\') {
                escaped = true;
                continue;
            }

            if (rest[i] == '"') {
                firstQuote = i;
                break;
            }
        }

        if (firstQuote == std::string::npos) {
            std::stringstream restSS(rest);
            if (!(restSS >> pluginName)) {
                Log(LogGrade::ERR, LogCode::PARSE_ERROR,
                    "Invalid plugin command format: missing plugin name at line " + std::to_string(currentLine + 1));
                MessageBoxA(NULL, "错误：plugin命令格式不正确，缺少插件名",
                    "错误", MB_ICONERROR | MB_OK);
                return { 0, currentLine + 1 };
            }
            runArgs = "";
        }
        else {
            std::string beforeQuote = rest.substr(0, firstQuote);
            std::stringstream beforeSS(beforeQuote);
            if (!(beforeSS >> pluginName)) {
                Log(LogGrade::ERR, LogCode::PARSE_ERROR,
                    "Invalid plugin command format: missing plugin name before quotes at line " +
                    std::to_string(currentLine + 1));
                MessageBoxA(NULL, "错误：plugin命令格式不正确，引号前缺少插件名",
                    "错误", MB_ICONERROR | MB_OK);
                return { 0, currentLine + 1 };
            }

            escaped = false;
            size_t secondQuote = std::string::npos;

            for (size_t i = firstQuote + 1; i < rest.length(); i++) {
                if (escaped) {
                    escaped = false;
                    continue;
                }

                if (rest[i] == '\\') {
                    escaped = true;
                    continue;
                }

                if (rest[i] == '"') {
                    secondQuote = i;
                    break;
                }
            }

            if (secondQuote == std::string::npos) {
                Log(LogGrade::ERR, LogCode::PARSE_ERROR,
                    "Invalid plugin command format: missing closing quote at line " +
                    std::to_string(currentLine + 1));

                formatErrorOutput(
                    logCodeToString(LogCode::PARSE_ERROR),
                    "ParseError",
                    "Unterminated string literal in 'plugin' command",
                    line,
                    currentLine + 1,
                    firstQuote + 1,
                    "Add closing double quote (\") at the end of the arguments.",
                    "https://github.com/Colasensei/PaperVisualNovel/tree/master/Docs/errors/E3001.md"
                );

                MessageBoxA(NULL, "错误：plugin命令格式不正确，缺少结束引号",
                    "错误", MB_ICONERROR | MB_OK);
                return { 0, currentLine + 1 };
            }

            std::string rawArgs = rest.substr(firstQuote + 1, secondQuote - firstQuote - 1);

            std::string unescapedArgs;
            escaped = false;

            for (size_t i = 0; i < rawArgs.length(); i++) {
                if (escaped) {
                    if (rawArgs[i] == '"') {
                        unescapedArgs += '"';
                    }
                    else if (rawArgs[i] == '\\') {
                        unescapedArgs += '\\';
                    }
                    else if (rawArgs[i] == 'n') {
                        unescapedArgs += '\n';
                    }
                    else if (rawArgs[i] == 't') {
                        unescapedArgs += '\t';
                    }
                    else if (rawArgs[i] == 'r') {
                        unescapedArgs += '\r';
                    }
                    else {
                        unescapedArgs += rawArgs[i];
                    }
                    escaped = false;
                }
                else if (rawArgs[i] == '\\') {
                    escaped = true;
                }
                else {
                    unescapedArgs += rawArgs[i];
                }
            }

            runArgs = unescapedArgs;

            std::string afterQuote = rest.substr(secondQuote + 1);
            size_t nonSpacePos = afterQuote.find_first_not_of(" \t\r\n");
            if (nonSpacePos != std::string::npos) {
                Log(LogGrade::WARNING, LogCode::PARSE_ERROR,
                    "Extra characters after closing quote in plugin command: " +
                    afterQuote.substr(nonSpacePos));
            }
        }

        pluginName = trim(pluginName);

        Log(LogGrade::DEBUG, LogCode::PLUGIN_LOADED, "Plugin name: " + pluginName);
        Log(LogGrade::DEBUG, LogCode::PLUGIN_LOADED,
            "Plugin arguments (raw, with escapes): \"" + runArgs + "\"");

        if (!runArgs.empty()) {
            std::string processedArgs = "";
            size_t pos = 0;
            bool inEscape = false;

            while (pos < runArgs.length()) {
                if (inEscape) {
                    processedArgs += runArgs[pos];
                    inEscape = false;
                    pos++;
                    continue;
                }

                if (runArgs[pos] == '\\') {
                    if (pos + 1 < runArgs.length()) {
                        char nextChar = runArgs[pos + 1];
                        if (nextChar == '$' || nextChar == '{' || nextChar == '}') {
                            processedArgs += nextChar;
                            pos += 2;
                        }
                        else {
                            processedArgs += runArgs[pos];
                            inEscape = true;
                            pos++;
                        }
                    }
                    else {
                        processedArgs += runArgs[pos];
                        pos++;
                    }
                }
                else if (pos < runArgs.length() - 5 && runArgs.substr(pos, 6) == "$file{") {
                    size_t braceStart = pos + 5;
                    size_t braceEnd = std::string::npos;
                    int braceDepth = 1;

                    for (size_t i = braceStart + 1; i < runArgs.length(); i++) {
                        if (runArgs[i] == '\\') {
                            i++;
                            continue;
                        }

                        if (runArgs[i] == '{') {
                            braceDepth++;
                        }
                        else if (runArgs[i] == '}') {
                            braceDepth--;
                            if (braceDepth == 0) {
                                braceEnd = i;
                                break;
                            }
                        }
                    }

                    if (braceEnd != std::string::npos) {
                        std::string rawPath = runArgs.substr(braceStart + 1, braceEnd - braceStart - 1);
                        std::string relativePath = "";

                        for (size_t i = 0; i < rawPath.length(); i++) {
                            if (rawPath[i] == '\\' && i + 1 < rawPath.length()) {
                                if (rawPath[i + 1] == '\\' || rawPath[i + 1] == '{' || rawPath[i + 1] == '}') {
                                    relativePath += rawPath[i + 1];
                                    i++;
                                }
                                else {
                                    relativePath += rawPath[i];
                                }
                            }
                            else {
                                relativePath += rawPath[i];
                            }
                        }

                        std::string absolutePath;
                        if (!where.empty()) {
                            absolutePath = where + relativePath;

                            std::replace(absolutePath.begin(), absolutePath.end(), '/', '\\');

                            size_t dotDotPos;
                            while ((dotDotPos = absolutePath.find("\\..\\")) != std::string::npos) {
                                size_t prevSlash = absolutePath.rfind('\\', dotDotPos - 1);
                                if (prevSlash != std::string::npos) {
                                    absolutePath = absolutePath.substr(0, prevSlash) +
                                        absolutePath.substr(dotDotPos + 3);
                                }
                                else {
                                    break;
                                }
                            }

                            Log(LogGrade::DEBUG, LogCode::PLUGIN_LOADED,
                                "Converted $file{" + rawPath + "} to: " + absolutePath);
                            processedArgs += absolutePath;
                        }
                        else {
                            Log(LogGrade::WARNING, LogCode::PLUGIN_LOADED,
                                "Cannot convert $file{} path: 'where' path is empty");
                            processedArgs += relativePath;
                        }

                        pos = braceEnd + 1;
                    }
                    else {
                        processedArgs += runArgs.substr(pos, 6);
                        pos += 6;
                    }
                }
                else if (pos < runArgs.length() - 3 && runArgs.substr(pos, 4) == "$log") {
                    std::string logPath;

                    char buffer[MAX_PATH];
                    if (GetCurrentDirectoryA(MAX_PATH, buffer) != 0) {
                        logPath = std::string(buffer) + "\\pvn_engine.log";
                        Log(LogGrade::DEBUG, LogCode::PLUGIN_LOADED, "Converted $log to: " + logPath);
                        processedArgs += logPath;
                    }
                    else {
                        Log(LogGrade::WARNING, LogCode::PLUGIN_LOADED,
                            "Failed to get current directory for $log conversion");
                        processedArgs += "pvn_engine.log";
                    }

                    pos += 4;
                }
                else if (pos < runArgs.length() - 2 && runArgs.substr(pos, 2) == "${") {
                    size_t varEnd = std::string::npos;
                    int braceDepth = 1;

                    for (size_t i = pos + 2; i < runArgs.length(); i++) {
                        if (runArgs[i] == '\\') {
                            i++;
                            continue;
                        }

                        if (runArgs[i] == '{') {
                            braceDepth++;
                        }
                        else if (runArgs[i] == '}') {
                            braceDepth--;
                            if (braceDepth == 0) {
                                varEnd = i;
                                break;
                            }
                        }
                    }

                    if (varEnd != std::string::npos) {
                        std::string varName = runArgs.substr(pos + 2, varEnd - pos - 2);

                        std::string stringValue = gameState.getStringVar(varName);
                        if (!stringValue.empty()) {
                            processedArgs += stringValue;
                        }
                        else {
                            int intValue = gameState.getVar(varName);
                            processedArgs += std::to_string(intValue);
                        }

                        pos = varEnd + 1;
                    }
                    else {
                        processedArgs += runArgs[pos];
                        pos++;
                    }
                }
                else {
                    processedArgs += runArgs[pos];
                    pos++;
                }
            }

            runArgs = processedArgs;
            Log(LogGrade::DEBUG, LogCode::PLUGIN_LOADED,
                "Plugin arguments (fully processed): \"" + runArgs + "\"");
        }

        auto pluginStartTime = std::chrono::high_resolution_clock::now();
        bool success = runPlugin(pluginName, runArgs);
        auto pluginEndTime = std::chrono::high_resolution_clock::now();
        auto pluginExecTime = std::chrono::duration_cast<std::chrono::milliseconds>(pluginEndTime - pluginStartTime).count();

        if (success) {
            Log(LogGrade::INFO, LogCode::PLUGIN_LOADED,
                "Plugin executed successfully: " + pluginName + " (took " +
                std::to_string(pluginExecTime) + "ms)");
        }
        else {
            Log(LogGrade::ERR, LogCode::PLUGIN_EXEC_FAILED,
                "Failed to execute plugin: " + pluginName + " (took " +
                std::to_string(pluginExecTime) + "ms)");
        }

        return { 0, currentLine + 1 };
    }

    // ==================== 插件依赖声明命令 ====================
    if (cmd == "use" || cmd == "USE") {
        Log(LogGrade::INFO, LogCode::PLUGIN_LOADED,
            "USE command detected at line " + std::to_string(currentLine + 1));

        std::string pluginName, pluginVersion = "";

        if (!(ss >> pluginName)) {
            Log(LogGrade::ERR, LogCode::PARSE_ERROR,
                "Invalid use command format: missing plugin name at line " +
                std::to_string(currentLine + 1));
            MessageBoxA(NULL, "错误：use命令格式不正确，缺少插件名",
                "错误", MB_ICONERROR | MB_OK);
            return { 0, currentLine + 1 };
        }

        if (ss >> pluginVersion) {
            std::string remaining;
            getline(ss, remaining);
            if (!remaining.empty() && remaining[0] == ' ') {
                remaining = remaining.substr(1);
            }
            if (!remaining.empty()) {
                pluginVersion += " " + remaining;
            }
            Log(LogGrade::DEBUG, LogCode::PLUGIN_LOADED,
                "Plugin version specified: " + pluginVersion);
        }

        std::string pluginDir = "Plugins\\" + pluginName;
        if (!fs::exists(pluginDir)) {
            Log(LogGrade::ERR, LogCode::PLUGIN_MISSING, "Plugin not found: " + pluginName);

            formatErrorOutput(
                logCodeToString(LogCode::PLUGIN_MISSING),
                "PluginError",
                "Required plugin '" + pluginName + "' is not installed",
                line,
                currentLine + 1,
                line.find(pluginName),
                "Download the plugin and place it in Plugins/" + pluginName + "/ directory",
                "https://github.com/Colasensei/PaperVisualNovel/tree/master/Docs/errors/W2002.md"
            );

            vnout("错误：所需的插件 '" + pluginName + "' 未安装", 0.5, red, true);
            std::cout << std::endl;

            std::cout << "==============================" << std::endl;
            std::cout << "插件 '" << pluginName << "' 未安装" << std::endl;
            std::cout << "请执行以下操作：" << std::endl;
            std::cout << "1. 下载插件到 Plugins/" << pluginName << "/ 目录" << std::endl;
            std::cout << "2. 确保目录中有 about.cfg 配置文件" << std::endl;
            std::cout << "3. 重新启动游戏" << std::endl;
            std::cout << "==============================" << std::endl;
            std::cout << std::endl << "按任意键返回主菜单..." << std::endl;
            getKeyName();
            return { -1, 0 };
        }

        std::string aboutFilePath = pluginDir + "\\about.cfg";
        if (!fs::exists(aboutFilePath)) {
            Log(LogGrade::ERR, LogCode::PLUGIN_MISSING,
                "Plugin configuration file not found: " + pluginName);
            vnout("错误：插件 '" + pluginName + "' 配置文件缺失", 0.5, red, true);
            std::cout << std::endl << "按任意键继续..." << std::endl;
            getKeyName();
            return { 0, currentLine + 1 };
        }

        if (!pluginVersion.empty()) {
            std::ifstream aboutFile(aboutFilePath);
            if (aboutFile.is_open()) {
                std::string fileVersion = "";
                std::string line;

                while (std::getline(aboutFile, line)) {
                    std::string trimmedLine = trim(line);
                    if (trimmedLine.empty() || trimmedLine[0] == '#') {
                        continue;
                    }

                    size_t equalsPos = trimmedLine.find('=');
                    if (equalsPos != std::string::npos) {
                        std::string key = trim(trimmedLine.substr(0, equalsPos));
                        std::string value = trim(trimmedLine.substr(equalsPos + 1));

                        if (value.length() >= 2 &&
                            ((value.front() == '"' && value.back() == '"') ||
                                (value.front() == '\'' && value.back() == '\''))) {
                            value = value.substr(1, value.length() - 2);
                        }

                        if (key == "Version") {
                            fileVersion = value;
                            break;
                        }
                    }
                }
                aboutFile.close();

                if (!fileVersion.empty()) {
                    if (fileVersion != pluginVersion) {
                        Log(LogGrade::WARNING, LogCode::VERSION_MISMATCH,
                            "Plugin version mismatch: required=" + pluginVersion +
                            ", installed=" + fileVersion);

                        std::cout << std::endl;
                        std::cout << "==============================" << std::endl;
                        std::cout << "插件版本警告" << std::endl;
                        std::cout << "脚本需要: " << pluginName << " v" << pluginVersion << std::endl;
                        std::cout << "已安装: " << pluginName << " v" << fileVersion << std::endl;
                        std::cout << std::endl;

                        try {
                            std::stringstream requiredSS(pluginVersion);
                            std::stringstream installedSS(fileVersion);
                            std::string requiredPart, installedPart;
                            bool versionOk = true;

                            while (std::getline(requiredSS, requiredPart, '.') &&
                                std::getline(installedSS, installedPart, '.')) {
                                try {
                                    int reqNum = std::stoi(requiredPart);
                                    int instNum = std::stoi(installedPart);

                                    if (instNum < reqNum) {
                                        versionOk = false;
                                        break;
                                    }
                                    else if (instNum > reqNum) {
                                        break;
                                    }
                                }
                                catch (...) {
                                    if (fileVersion != pluginVersion) {
                                        versionOk = false;
                                        break;
                                    }
                                }
                            }

                            if (!versionOk) {
                                std::cout << "警告：插件版本可能不兼容" << std::endl;
                                std::cout << "建议安装 " << pluginName << " v" << pluginVersion << std::endl;
                            }
                            else {
                                std::cout << "版本检查通过" << std::endl;
                            }
                        }
                        catch (...) {
                            std::cout << "警告：无法比较版本号" << std::endl;
                        }

                        std::cout << "==============================" << std::endl;
                        std::cout << std::endl << "按任意键继续（或按ESC返回主菜单）..." << std::endl;

                        std::string key = getKeyName();
                        if (key == "ESC") {
                            return { -1, 0 };
                        }
                    }
                }
            }
        }

        Log(LogGrade::INFO, LogCode::PLUGIN_LOADED,
            "Plugin dependency registered: " + pluginName +
            (pluginVersion.empty() ? "" : " v" + pluginVersion));

        return { 0, currentLine + 1 };
    }

    // ==================== 未知命令 ====================
    Log(LogGrade::ERR, LogCode::COMMAND_UNKNOWN, "Unknown command: " + cmd);

    formatErrorOutput(
        logCodeToString(LogCode::COMMAND_UNKNOWN),
        "CommandError",
        "Unknown PGN command",
        line,
        currentLine + 1,
        line.find(cmd),
        "Check the command spelling or refer to the documentation for valid commands",
        "https://github.com/Colasensei/PaperVisualNovel/tree/master/Docs/errors/E3002.md"
    );

    MessageBoxA(NULL, ("错误：未知的PGN命令 - " + cmd).c_str(),
        "错误", MB_ICONERROR | MB_OK);
    return { 0, currentLine + 1 };
}