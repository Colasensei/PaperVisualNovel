// parser.cpp
#include "parser.h"
#include "ui.h"
#include "fileutils.h"
#include <Windows.h>
#include <sstream>
#include <map>

// ==================== 标签解析 ====================

std::map<std::string, int> parseLabels(const std::vector<std::string>& lines) {
    std::map<std::string, int> labels;

    for (size_t i = 0; i < lines.size(); i++) {
        std::string line = lines[i];
        std::stringstream ss(line);
        std::string token;

        if (ss >> token) {
            // 检查是否是标签（以冒号结尾的单词）
            if (token.back() == ':') {
                std::string labelName = token.substr(0, token.length() - 1);
                labels[labelName] = i + 1; // 行号（从1开始）
            }
        }
    }

    return labels;
}

// ==================== 跳转目标解析 ====================

int parseJumpTarget(const std::string& target, const std::map<std::string, int>& labels,
                    bool& isLabel) {
    // 先检查是否是数字（行号）
    try {
        int lineNum = std::stoi(target);
        isLabel = false;
        return lineNum;
    }
    catch (const std::exception&) {
        // 不是数字，检查是否是标签
        std::string labelName = target;

        // 直接查找标签
        auto it = labels.find(labelName);
        if (it != labels.end()) {
            isLabel = true;
            return it->second;
        }

        // 也尝试查找去掉冒号的标签
        if (!labelName.empty() && labelName.back() == ':') {
            labelName.pop_back();
            it = labels.find(labelName);
            if (it != labels.end()) {
                isLabel = true;
                return it->second;
            }
        }

        return -1; // 无效的目标
    }
}



// ==================== 执行行 ====================

std::pair<int, size_t> executeLine(const std::string& line, GameState& gameState,
    size_t currentLine, const std::vector<std::string>& allLines,
    const std::string& where, int indentLevel,
    const std::map<std::string, int>& labels) {
    extern CurrentGameInfo g_currentGameInfo; // 外部全局变量

    Log(LogGrade::INFO, "Executing line: " + to_string(currentLine + 1));
    Log(LogGrade::DEBUG, "Now executing: " + line);

    std::stringstream ss(line);
    std::string cmd;
    ss >> cmd;
    Log(LogGrade::DEBUG, "Command: " + cmd);

    if (cmd.empty() || cmd == "//" || cmd == "#") {
        Log(LogGrade::DEBUG, "Empty line or comment, skipping.");
        return { 0, currentLine + 1 };
    }

    // 处理标签
    if (cmd.back() == ':') {
        Log(LogGrade::DEBUG, "Label found, skipping.");
        return { 0, currentLine + 1 };
    }

    // ==================== 游戏结束命令 ====================
    if (cmd == "end" || cmd == "END") {
        Log(LogGrade::DEBUG, "Game end command found.");
        std::cout << "游戏结束" << std::endl;
        system("pause");
        Log(LogGrade::DEBUG, "Exiting game.");
        return { -1, 0 };
    }

    // ==================== 结局名命令 ====================
    if (cmd == "endname" || cmd == "ENDNAME") {

        Log(LogGrade::DEBUG, "End name command found.");
        std::string endingName;
        getline(ss, endingName);

        // 去除开头空格
        size_t start = endingName.find_first_not_of(" ");
        if (start != std::string::npos) {
            endingName = endingName.substr(start);
        }

        if (!endingName.empty()) {
            // 提取游戏文件夹名
            std::string gameFolder = "";
            size_t novelPos = where.find("Novel\\");
            if (novelPos != std::string::npos) {
                size_t startPos = novelPos + 6; // "Novel\\" 的长
                size_t endPos = where.find("\\", startPos);
                if (endPos != std::string::npos) {
                    gameFolder = where.substr(startPos, endPos - startPos);
                }
            }

            if (!gameFolder.empty()) {
                // 保存结局
                saveEnding(gameFolder, endingName, gameState);
                Log(LogGrade::INFO, "Ending saved: " + endingName);
                // 显示收集进度
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

        // endname 不结束游戏，继续执行下一行
        Log(LogGrade::DEBUG, "End name command executed.");
        return { 0, currentLine + 1 };
    }

    // ==================== 等待命令 ====================
    if (cmd == "wait" || cmd == "WAIT") {
        Log(LogGrade::DEBUG, "Wait command found.");
        int wait;
        if (ss >> wait) {
            Log(LogGrade::DEBUG, "Wait time: " + std::to_string(wait));
            Sleep(wait);
        }
        return { 0, currentLine + 1 };
    }

    // ==================== 说话命令（say） ====================
    //say是诗山，优化等于0 这是谁写的（恼）
    if (cmd == "say" || cmd == "SAY") {
        Log(LogGrade::DEBUG, "Say command found.");
        std::string rest;
        getline(ss, rest);

        // 去除开头的空白字符
        size_t start = rest.find_first_not_of(" ");
        if (start == std::string::npos) {
            // 整个命令为空
            return { 0, currentLine + 1 };
        }
        rest = rest.substr(start);

        std::string text = "";
        double time_val = 0.5;
        std::string incolor = "white";

        // 检查第一个字符是否是引号
        if (rest[0] == '"') {
            Log(LogGrade::DEBUG, "Quoted string found.");
            // 带引号语法
            size_t quote_end = 0;
            bool escaped = false;

            // 手动解析引号字符串，处理转义
            for (size_t i = 1; i < rest.length(); i++) {
                if (escaped) {
                    // 处理转义字符
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
                    // 遇到转义字符
                    escaped = true;
                }
                else if (rest[i] == '"') {
                    // 找到结束引号
                    quote_end = i;
                    break;
                }
                else {
                    text += rest[i];
                }
            }

            if (quote_end == 0) {
                // 没有找到结束引号
                Log(LogGrade::ERR, "Error: Missing closing quote in say command.At line " + std::to_string(currentLine));
                MessageBoxA(NULL, "错误：say命令中的字符串缺少结束引号",
                    "错误", MB_ICONERROR | MB_OK);
                return { 0, currentLine + 1 };
            }

            // 提取引号后的参数部分
            std::string remaining = rest.substr(quote_end + 1);

            // 解析时间和颜色
            std::stringstream remainingSS(remaining);
            std::vector<std::string> tokens;
            std::string token;

            while (remainingSS >> token) {
                tokens.push_back(token);
            }

            // 从后向前解析参数
            if (!tokens.empty()) {
                std::string last_token = tokens.back();

                // 检查是否是颜色
                if (last_token == "black" || last_token == "blue" || last_token == "green" ||
                    last_token == "aqua" || last_token == "red" || last_token == "purple" ||
                    last_token == "yellow" || last_token == "white") {
                    incolor = last_token;
                    tokens.pop_back();
                }

                // 检查是否是时间
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
            Log(LogGrade::DEBUG, "Unquoted string found.");
            // 原有的无引号语法（向后兼容）
            std::stringstream rest_ss(rest);
            std::vector<std::string> tokens;
            std::string token;

            while (rest_ss >> token) {
                tokens.push_back(token);
            }

            if (!tokens.empty()) {
                // 临时存储文本部分
                std::vector<std::string> text_parts;

                // 从后向前解析
                while (!tokens.empty()) {
                    std::string token = tokens.back();

                    // 检查是否是颜色
                    if (token == "black" || token == "blue" || token == "green" ||
                        token == "aqua" || token == "red" || token == "purple" ||
                        token == "yellow" || token == "white") {
                        if (incolor == "white") {
                            incolor = token;
                            tokens.pop_back();
                            continue;
                        }
                    }

                    // 检查是否是时间
                    try {
                        // 尝试转换为double
                        char* end;
                        double time_test = strtod(token.c_str(), &end);
                        if (end != token.c_str() && *end == '\0') {
                            // 成功转换且是完整的数字
                            if (time_val == 0.5) { // 默认值，说明还没设置过
                                time_val = time_test;
                                tokens.pop_back();
                                continue;
                            }
                        }
                    }
                    catch (...) {
                        // 转换失败，不是时间
                    }

                    // 既不是颜色也不是时间，就是文本的一部分
                    text_parts.insert(text_parts.begin(), token);
                    tokens.pop_back();
                }

                // 构建文本
                for (size_t i = 0; i < text_parts.size(); i++) {
                    if (i > 0) text += " ";
                    text += text_parts[i];
                }
            }
        }

        // 处理变量占位符（${var}）
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

            // 首先检查是否是字符串变量
            std::string string_value = gameState.getStringVar(var_name);
            if (!string_value.empty()) {
                // 如果是字符串变量，直接使用字符串值
                final_text += string_value;
            }
            else {
                // 否则尝试作为整数变量处理
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
        Log(LogGrade::DEBUG, "Text: " + final_text);
        vnout(final_text, time_val, text_color, false, true);
        int result = operate();
        if (result == 1) { // 保存并退出
            Log(LogGrade::INFO, "Save and exit.");
            return { -1, 0 };
        }
        else if (result == 2) { // 不保存退出
            Log(LogGrade::INFO, "Exit without saving.");
            return { -2, 0 };
        }
        else if (result == 3) { // 调试终端请求跳转
            Log(LogGrade::DEBUG, "Debug Terminal Jump to line " + std::to_string(g_currentGameInfo.currentLine + 1));
            return { 1, g_currentGameInfo.currentLine };
        }
        Log(LogGrade::DEBUG, "Next line: " + std::to_string(currentLine + 1));
        return { 0, currentLine + 1 };
    }

    // ==================== 输入命令 ====================
    if (cmd == "input" || cmd == "INPUT") {
        Log(LogGrade::DEBUG, "INPUT command detected.");

        std::string prompt, varName;

        // 解析命令格式：input "提示文本" varname
        if (line.find('"') != std::string::npos) {
            // 带引号的格式
            size_t firstQuote = line.find('"');
            size_t secondQuote = line.find('"', firstQuote + 1);

            if (secondQuote == std::string::npos) {
                Log(LogGrade::ERR, "Invalid input command: missing closing quote");
                MessageBoxA(NULL, "错误：input命令格式不正确，缺少结束引号",
                    "错误", MB_ICONERROR | MB_OK);
                return { 0, currentLine + 1 };
            }

            prompt = line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
            std::string rest = line.substr(secondQuote + 1);

            std::stringstream restSS(rest);
            if (!(restSS >> varName)) {
                Log(LogGrade::ERR, "Invalid input command: missing variable name");
                MessageBoxA(NULL, "错误：input命令格式不正确，缺少变量名",
                    "错误", MB_ICONERROR | MB_OK);
                return { 0, currentLine + 1 };
            }
        }
        else {
            // 无引号格式（简单格式）
            std::string rest;
            getline(ss, rest);
            std::stringstream restSS(rest);

            if (!(restSS >> prompt >> varName)) {
                Log(LogGrade::ERR, "Invalid input command: missing parameters");
                MessageBoxA(NULL, "错误：input命令格式不正确，参数不足",
                    "错误", MB_ICONERROR | MB_OK);
                return { 0, currentLine + 1 };
            }
        }

        if (varName.empty()) {
            Log(LogGrade::ERR, "Invalid input command: empty variable name");
            MessageBoxA(NULL, "错误：input命令变量名不能为空",
                "错误", MB_ICONERROR | MB_OK);
            return { 0, currentLine + 1 };
        }

        // 显示提示信息
        std::cout << std::endl;
        std::cout << "\033[32m" << prompt << "\033[37m" ;

        // 获取用户输入
        std::string userInput;
        std::getline(std::cin, userInput);

        // 去除首尾空白字符
        size_t start = userInput.find_first_not_of(" \t\n\r");
        if (start != std::string::npos) {
            size_t end = userInput.find_last_not_of(" \t\n\r");
            userInput = userInput.substr(start, end - start + 1);
        }

        if (!userInput.empty()) {
            // 存储到字符串变量中
            gameState.setStringVar(varName, userInput);
            Log(LogGrade::INFO, "Input saved to string variable: " + varName + " = \"" + userInput + "\"");

            
            std::cout << std::endl;
        }
        else {
            Log(LogGrade::WARNING, "User input is empty for variable: " + varName);
            // 如果用户输入为空，仍然设置变量为空字符串
            gameState.setStringVar(varName, "");
            
            std::cout << std::endl;
        }

        return { 0, currentLine + 1 };
    }
    // ==================== 显示变量值命令 ====================
    //别用，这玩意纯纯历史遗留，一堆bug
    if (cmd == "sayvar" || cmd == "SAYVAR") {
        Log(LogGrade::DEBUG, "SAYVAR command detected.");
        std::string varName;
        double time_val;
        std::string incolor;

        if (ss >> varName >> time_val >> incolor) {
            Log(LogGrade::DEBUG, "Variable name: " + varName);
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
            if (result == 1) { // 保存并退出

                Log(LogGrade::INFO, "Save and exit.");
                return { -1, 0 };
            }
            else if (result == 2) { // 不保存退出
                Log(LogGrade::INFO, "Exit without saving.");
                return { -2, 0 };
            }
            else if (result == 3) { // 调试终端请求跳转
                // 使用全局变量中的跳转行号
                Log(LogGrade::DEBUG, "Debug Terminal Jump to line " + std::to_string(g_currentGameInfo.currentLine + 1));
                return { 1, g_currentGameInfo.currentLine };
            }
            Log(LogGrade::DEBUG, "Next line: " + std::to_string(currentLine + 1));
            return { 0, currentLine + 1 };
        }
    }

    // ==================== 显示文件命令 ====================
    //超好用（喜）
    if (cmd == "show" || cmd == "SHOW") {
        Log(LogGrade::DEBUG, "SHOW command detected.");
        std::string file_to_show;
        if (ss >> file_to_show) {
            Log(LogGrade::DEBUG, "File to show: " + file_to_show);
            std::string path = where + "archive\\" + file_to_show;
            safeViewFile(path);

            Log(LogGrade::DEBUG, "Next line: " + std::to_string(currentLine + 1));
        }
        return { 0, currentLine + 1 };
    }

    // ==================== 选择命令 ====================
    // 好用（喜）
    if (cmd == "choose" || cmd == "CHOOSE") {
        Log(LogGrade::DEBUG, "CHOOSE command detected.");
        // 解析选项：格式为 标签:显示文本
        std::vector<ChoiceOption> options;
        std::vector<std::string> gumOptions;  // 用于gum显示的选项
        std::stringstream ss(line);
        std::string cmdWord;
        int optionCount;

        ss >> cmdWord >> optionCount;

        // 解析每个选项
        for (int i = 0; i < optionCount; i++) {
            std::string optionStr;
            if (ss >> optionStr) {
                // 分割标签和文本（格式：标签:文本）
                size_t colonPos = optionStr.find(':');
                if (colonPos != std::string::npos) {
                    ChoiceOption option;
                    option.label = optionStr.substr(0, colonPos);
                    option.text = optionStr.substr(colonPos + 1);
                    options.push_back(option);

                    // 构建gum显示的选项字符串 (格式: 1.文本)
                    gumOptions.push_back(std::to_string(i + 1) + ". " + option.text);
                }
                else {
                    // 如果没有冒号 使用标签作为文本
                    ChoiceOption option;
                    option.label = optionStr;
                    option.text = optionStr;
                    options.push_back(option);

                    // 构建gum显示的选项字符串
                    gumOptions.push_back(std::to_string(i + 1) + ". " + optionStr);
                }
            }
        }

        Log(LogGrade::DEBUG, "Options parsed: " + std::to_string(options.size()));

        // 检查gum是否可用
        if (!gum::GumWrapper::is_available()) {
            Log(LogGrade::WARNING, "Gum not available, falling back to original method");
            // 原有的显示逻辑
            std::cout << "\033[90m" << "请选择：" << std::endl;
            for (size_t i = 0; i < options.size(); i++) {
                std::cout << i + 1 << ". ";
                vnout(options[i].text, 0.5, white, false, true);
                std::cout << "\033[90m";
                std::cout << std::endl;
            }
            std::cout << "请输入数字选择 (1-" << options.size() << "): ";

            // 原有的输入逻辑
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
                    // 不做任何处理，继续等待有效输入
                }
            }
            Log(LogGrade::INFO, "User choice (fallback): " + std::to_string(choice));

            // 记录选择
            gameState.recordChoice(options[choice - 1].text);
            std::cout << options[choice - 1].text << endl;

            // 跳转到对应的标签
            std::string targetLabel = options[choice - 1].label;
            bool isLabel;
            int jumpLine = parseJumpTarget(targetLabel, labels, isLabel);

            if (jumpLine > 0 && jumpLine <= static_cast<int>(allLines.size())) {
                Log(LogGrade::INFO, "Jump to line: " + std::to_string(jumpLine));
                return { 1, jumpLine - 1 };
            }
            else {
                Log(LogGrade::ERR, "Invalid jump target: " + targetLabel);
                MessageBoxA(NULL, ("错误：选择目标无效 - " + targetLabel).c_str(),
                    "错误", MB_ICONERROR | MB_OK);
                return { -1, 0 };
            }
        }

        // 使用gum进行选择
        try {
            Log(LogGrade::DEBUG, "Using gum for selection");
            std::cout << endl;
            // 使用gum显示选择菜单
            std::string selected = gum::GumWrapper::choose(gumOptions);
            // 提取第一个字符作为选项数字
            std::string op = "";
            if (!selected.empty()) {
                // 提取第一个字符（如"1"、"2"等）
                op = selected.substr(0, 1);

                // 转换为choice索引 (1-based)
                int choice = std::stoi(op);

                Log(LogGrade::INFO, "User choice (gum): " + op);

                if (choice >= 1 && choice <= static_cast<int>(options.size())) {
                    // 记录选择
                    gameState.recordChoice(options[choice - 1].text);
                    vnout("你选择了：" + options[choice - 1].text, 0.5, gray, true, true);

                    // 跳转到对应的标签
                    std::string targetLabel = options[choice - 1].label;
                    bool isLabel;
                    int jumpLine = parseJumpTarget(targetLabel, labels, isLabel);

                    if (jumpLine > 0 && jumpLine <= static_cast<int>(allLines.size())) {
                        Log(LogGrade::INFO, "Jump to line: " + std::to_string(jumpLine));
                        return { 1, jumpLine - 1 };
                    }
                    else {
                        Log(LogGrade::ERR, "Invalid jump target: " + targetLabel);
                        MessageBoxA(NULL, ("错误：选择目标无效 - " + targetLabel).c_str(),
                            "错误", MB_ICONERROR | MB_OK);
                        return { -1, 0 };
                    }
                }
                else {
                    Log(LogGrade::ERR, "Invalid choice index from gum: " + op);
                    throw std::runtime_error("Invalid choice index");
                }
            }
            else {
                Log(LogGrade::WARNING, "Gum selection cancelled or empty");
                // 用户取消选择，重新显示当前选择
                return { 0, currentLine + 1 };  // 重新执行当前选择
            }
        }
        catch (const std::exception& e) {
            Log(LogGrade::ERR, "Gum selection error: " + std::string(e.what()));

            // gum失败，回退到原始方法
            Log(LogGrade::DEBUG, "Falling back to original selection method");
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
                    // 不做任何处理，继续等待有效输入
                }
            }
            Log(LogGrade::INFO, "User choice (fallback after gum error): " + std::to_string(choice));

            // 记录选择
            gameState.recordChoice(options[choice - 1].text);

            // 跳转到对应的标签
            std::string targetLabel = options[choice - 1].label;
            bool isLabel;
            int jumpLine = parseJumpTarget(targetLabel, labels, isLabel);

            if (jumpLine > 0 && jumpLine <= static_cast<int>(allLines.size())) {
                Log(LogGrade::INFO, "Jump to line: " + std::to_string(jumpLine));
                return { 1, jumpLine - 1 };
            }
            else {
                Log(LogGrade::ERR, "Invalid jump target: " + targetLabel);
                MessageBoxA(NULL, ("错误：选择目标无效 - " + targetLabel).c_str(),
                    "错误", MB_ICONERROR | MB_OK);
                return { -1, 0 };
            }
        }
    }   
    // ==================== 清屏命令 ====================
    if (cmd == "cls" || cmd == "clean" || cmd == "CLS" || cmd == "CLEAN")
    {

        Log(LogGrade::DEBUG, "CLS command detected.");
        system("cls");
        return { 0, currentLine + 1 };
    }
    // ==================== 随机数命令 ====================
    if (cmd == "random" || cmd == "RANDOM") {
        Log(LogGrade::DEBUG, "RANDOM command detected.");
        std::string varName;
        int minVal, maxVal;
        if (ss >> varName >> minVal >> maxVal) {
            Log(LogGrade::DEBUG, "Variable name: " + varName);

            Log(LogGrade::DEBUG, "Min value: " + std::to_string(minVal));
            Log(LogGrade::DEBUG, "Max value: " + std::to_string(maxVal));
            // 确保最小值不大于最大值
            if (minVal > maxVal) {
                std::swap(minVal, maxVal);
            }

            // 生成[minVal, maxVal]范围内的随机数
            int range = maxVal - minVal + 1;
            int randomValue = minVal + (rand() % range);

            Log(LogGrade::DEBUG, "Random value: " + std::to_string(randomValue));
            gameState.setVar(varName, randomValue);
        }
        return { 0, currentLine + 1 };
    }

    // ==================== 设置变量命令 ====================
    if (cmd == "set" || cmd == "SET") {
        Log(LogGrade::DEBUG, "SET command detected.");
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
                gameState.addVar(varName, -value);  // 添加负值
            }
            else if (op == "*=") {
                // 处理乘法
                if (gameState.hasVar(varName)) {
                    int current = gameState.getVar(varName);
                    gameState.setVar(varName, current * value);
                }
            }
            else if (op == "/=") {
                // 处理除法
                if (gameState.hasVar(varName) && value != 0) {
                    int current = gameState.getVar(varName);
                    gameState.setVar(varName, current / value);
                }
            }
            else {

                Log(LogGrade::ERR, "Invalid operation: " + op);
                MessageBoxA(NULL, ("错误：无效的操作符 - " + op).c_str(),
                    "错误", MB_ICONERROR | MB_OK);
            }
        }
        Log(LogGrade::INFO, "Did " + varName + " " + op + " " + std::to_string(value));
        Log(LogGrade::INFO, "Variable " + varName + " set to " + std::to_string(gameState.getVar(varName)));
        return { 0, currentLine + 1 };
    }

    // ==================== 跳转命令 ====================
    if (cmd == "jump" || cmd == "JUMP") {


        Log(LogGrade::DEBUG, "JUMP command detected.");
        std::string target;
        if (ss >> target) {

            bool isLabel;
            int jumpLine = parseJumpTarget(target, labels, isLabel);

            if (jumpLine > 0 && jumpLine <= static_cast<int>(allLines.size())) {
                // 跳转到指定行号（行号从1开始，但我们需要0-based索引）
                Log(LogGrade::INFO, "Jump to line: " + std::to_string(jumpLine));
                return { 1, jumpLine - 1 };
            }
            else {
                Log(LogGrade::ERR, "Invalid jump target: " + target);
                MessageBoxA(NULL, ("错误：跳转目标无效 - " + target).c_str(),
                    "错误", MB_ICONERROR | MB_OK);
            }
        }



        Log(LogGrade::ERR, "Invalid JUMP command format.");
        return { 0, currentLine + 1 };
    }

    // ==================== 条件命令 ====================
    if (cmd == "if" || cmd == "IF") {
        Log(LogGrade::DEBUG, "IF command detected.");
        // 读取整个条件表达式
        std::string conditionExpr;
        getline(ss, conditionExpr);

        // 分割条件表达式和跳转目标
        size_t lastSpace = conditionExpr.find_last_of(' ');
        if (lastSpace == std::string::npos) {
            Log(LogGrade::ERR, "Invalid IF command format.At "+std::to_string(currentLine));
            Log(LogGrade::ERR, "Condition expression: " + conditionExpr);
            Log(LogGrade::ERR, "Last space position: " + std::to_string(lastSpace));
            MessageBoxA(NULL, "错误：if命令格式不正确", "错误", MB_ICONERROR | MB_OK);
            return { 0, currentLine + 1 };
        }

        std::string target = conditionExpr.substr(lastSpace + 1);
        conditionExpr = conditionExpr.substr(0, lastSpace);

        // 去除多余空格
        conditionExpr.erase(0, conditionExpr.find_first_not_of(" "));
        conditionExpr.erase(conditionExpr.find_last_not_of(" ") + 1);

        // 分词并计算条件
        std::vector<ConditionToken> tokens = tokenizeCondition(conditionExpr);
        size_t index = 0;
        bool conditionMet = evaluateCondition(tokens, gameState, index);
        Log(LogGrade::DEBUG, "Condition met: " + std::to_string(conditionMet));
        if (conditionMet) {
            Log(LogGrade::INFO, "Condition met, jump to: " + target);
            bool isLabel;
            int jumpLine = parseJumpTarget(target, labels, isLabel);

            if (jumpLine > 0 && jumpLine <= static_cast<int>(allLines.size())) {
                Log(LogGrade::INFO, "Jump to line: " + std::to_string(jumpLine));
                return { 1, jumpLine - 1 }; // 跳转到指定行（0-based索引）
            }
            else {
                Log(LogGrade::ERR, "Invalid jump target: " + target);
                MessageBoxA(NULL, ("错误：跳转目标无效 - " + target).c_str(),
                    "错误", MB_ICONERROR | MB_OK);
            }
        }
        Log(LogGrade::INFO, "Condition not met, continue to next line.");
        return { 0, currentLine + 1 }; // 继续执行下一行
    }

    //欢迎来pull request（喜）
    //新命令开发指南见ReadMe/下的文档

    // ==================== 未知命令 ====================
    Log(LogGrade::ERR, "Unknown command: " + cmd);
    MessageBoxA(NULL, ("错误：未知的PGN命令 - " + cmd).c_str(),
        "错误", MB_ICONERROR | MB_OK);
    return { 0, currentLine + 1 }; // 继续执行下一行


}
