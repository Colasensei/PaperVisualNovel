# AiChat 插件 - DeepSeek AI 聊天机器人

## 📌 插件信息

- **插件名称**: AiChat
- **版本**: 1.0.0
- **作者**: colaSensei
- **依赖**: Python 3.6+, `requests` 库
- **API**: DeepSeek API

## 🚀 快速开始

### 1. 安装依赖
```bash
pip install requests
```

### 2. 配置API密钥
获取DeepSeek API密钥：[DeepSeek官网](https://platform.deepseek.com/)

### 3. 在PGN脚本中使用
```pgn
use AiChat 1.0.0
plugin AiChat "-1 \"$file{prompts/system.txt}\" \"sk-your-api-key\" \"小助手\" \"$log\""
```

## 📖 参数说明

程序接受**5个按顺序的位置参数**：

### 参数顺序表

| 序号 | 参数名 | 类型 | 必需 | 说明 |
|------|--------|------|------|------|
| 1 | 次数 | 整数 | 是 | 对话模式控制 |
| 2 | 提示词文件路径 | 字符串 | 是 | GB2312编码的文本文件 |
| 3 | API密钥 | 字符串 | 是 | DeepSeek API密钥 |
| 4 | AI名称 | 字符串 | 是 | 对话中显示的名称 |
| 5 | 日志文件路径 | 字符串 | 是 | 设为空/`none`/`null`禁用日志 |

### 详细说明

#### 1. **次数** (整数)
控制对话模式：
- **负数** (如 `-1`): 无限对话模式，持续对话直到用户输入 `/exit`
- **0**: 单次处理模式，使用提示词文件内容作为输入，只输出一次
- **正数** (如 `5`): 限制对话次数，达到指定次数后自动结束

#### 2. **提示词文件路径** (字符串)
- **格式**: GB2312编码的文本文件
- **内容**: 包含AI人设、系统提示词、初始对话设定
- **示例内容**:
```
你是一个乐于助人的AI助手，性格开朗、知识渊博。
请用中文回复，保持回答简洁明了。
当前用户是一名游戏玩家。
```

#### 3. **API密钥** (字符串)
- **格式**: DeepSeek API密钥，如 `sk-xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx`
- **获取**: 登录DeepSeek平台创建API密钥
- **注意**: 密钥需要包裹在引号中

#### 4. **AI名称** (字符串)
- **用途**: 在对话中显示的名称
- **示例**: `"助手"`、`"机器人"`、`"小智"`、`"AI导师"`
- **显示效果**: `[AI名称]: 回复内容...`

#### 5. **日志文件路径** (字符串)
- **格式**: GB2312编码的日志文件
- **特殊值**: 
  - 空字符串 `""`: 禁用日志
  - `"none"`: 禁用日志
  - `"null"`: 禁用日志
- **日志内容**: 记录对话历史、时间戳、运行状态

## 💡 使用示例

### 示例1: 无限对话模式
```pgn
# 无限对话，手动输入/exit退出
plugin AiChat "-1 \"$file{config/ai_persona.txt}\" \"sk-abc123...\" \"小助手\" \"$log\""
```

**运行效果**:
```
[小助手]: 你好！我是小助手，有什么可以帮你的吗？
你: 你好
[小助手]: 你好！今天天气不错，你打算做点什么呢？
你: /exit
对话结束。
```

### 示例2: 单次处理模式
```pgn
# 读取提示词文件，生成一次回复
plugin AiChat "0 \"$file{prompts/poem_request.txt}\" \"sk-abc123...\" \"诗人\" \"\""
```

**提示词文件内容** (poem_request.txt):
```
请写一首关于春天的五言绝句。
```

### 示例3: 限制次数对话
```pgn
# 最多对话5次
plugin AiChat "5 \"$file{config/assistant.txt}\" \"sk-abc123...\" \"导师\" \"$file{logs/chat.log}\""
```

### 示例4: 使用游戏变量
```pgn
# 从游戏变量获取API密钥
set apiKey "sk-your-actual-api-key"
plugin AiChat "-1 \"$file{ai/config.txt}\" \"${apiKey}\" \"游戏向导\" \"$log\""
```

### 示例5: 完整游戏集成
```pgn
# MyGame.pgn
use AiChat 1.0.0

say "欢迎来到AI互动游戏！" 0.5 white

# 选择AI类型
choose 3
  teacher:AI教师
  friend:AI朋友
  guide:游戏向导

if ${choice} == "teacher"
  plugin AiChat "10 \"$file{ai/teacher.txt}\" \"sk-...\" \"王老师\" \"$log\""
else if ${choice} == "friend"
  plugin AiChat "-1 \"$file{ai/friend.txt}\" \"sk-...\" \"小明\" \"$log\""
else
  plugin AiChat "0 \"$file{ai/guide.txt}\" \"sk-...\" \"向导\" \"\""
```

## 📂 文件结构示例

### 插件目录
```
Plugins/AiChat/
├── about.cfg          # 插件配置
├── AiChat.py          # 主程序
├── README.md          # 说明文档
└── examples/          # 示例文件
    ├── config/
    │   ├── ai_persona.txt     # AI人设示例
    │   ├── assistant.txt      # 助手角色
    │   └── game_guide.txt     # 游戏向导
    ├── prompts/
    │   ├── poem_request.txt   # 写诗请求
    │   └── story_start.txt    # 故事开头
    └── logs/                  # 日志目录
```

### 提示词文件示例

#### 1. 通用助手 (assistant.txt)
```
你是一个乐于助人的AI助手。
请用简洁明了的中文回答问题。
如果不知道答案，请诚实告知。
保持友好和专业的语气。
```

#### 2. 游戏向导 (game_guide.txt)
```
你是一个奇幻游戏的NPC向导。
你知道游戏的所有规则和秘密。
用神秘而有趣的语气说话。
适当给玩家提示，但不要剧透关键内容。
当前玩家正在第一章的森林中。
```

#### 3. 创意写作 (creative.txt)
```
你是一个创意写作助手。
擅长写故事、诗歌和对话。
语言优美、富有想象力。
根据用户的请求创作文学作品。
```

## 🔧 高级用法

### 1. 转义字符处理
```pgn
# 路径包含特殊字符
plugin AiChat "-1 \"$file{config/my\\{special\\}.txt}\" \"sk-...\" \"助手\" \"$log\""

# 参数包含引号
plugin AiChat "0 \"请解释'人工智能'这个概念\" \"sk-...\" \"专家\" \"\""
```

### 2. 多行提示词
```pgn
# 使用多行提示词文件
plugin AiChat "0 \"$file{prompts/long_request.txt}\" \"sk-...\" \"作家\" \"\""
```

### 3. 配合游戏剧情
```pgn
# 根据游戏进度切换AI角色
if ${chapter} == 1
  plugin AiChat "3 \"$file{ai/chapter1.txt}\" \"sk-...\" \"村长\" \"$log\""
else if ${chapter} == 2
  plugin AiChat "5 \"$file{ai/chapter2.txt}\" \"sk-...\" \"骑士\" \"$log\""
end
```

## ⚠️ 注意事项

### 1. **编码要求**
- 所有文本文件必须使用 **GB2312编码**
- Windows记事本保存时选择 "ANSI" 编码
- 避免使用GB2312不支持的字符

### 2. **API限制**
- DeepSeek API有调用频率限制
- 长时间对话注意token消耗
- 妥善保管API密钥

### 3. **性能建议**
- 无限对话模式可能消耗大量token
- 单次模式适合生成静态内容
- 限制次数模式平衡交互和控制

### 4. **错误处理**
```pgn
# 建议的错误处理
if ${hasApiKey} == 1
  plugin AiChat "-1 \"$file{ai/default.txt}\" \"${apiKey}\" \"助手\" \"$log\""
else
  say "AI聊天功能不可用，请检查API配置" 0.5 red
```

## 🐛 故障排除

### 常见问题

| 问题 | 可能原因 | 解决方法 |
|------|----------|----------|
| 编码错误 | 文件不是GB2312编码 | 用记事本另存为ANSI编码 |
| API错误 | 密钥无效或过期 | 检查API密钥，重新生成 |
| 无响应 | 网络问题 | 检查网络连接 |
| 程序崩溃 | 参数格式错误 | 检查参数顺序和引号 |
| 日志不记录 | 路径权限问题 | 检查目录写入权限 |

### 调试命令
```pgn
# 测试基本功能
plugin AiChat "0 \"测试\" \"sk-...\" \"测试员\" \"test.log\""

# 查看日志
show test.log
```

## 📝 日志格式

启用日志后，文件内容示例：
```
[2024-01-20 10:30:15] 会话开始
AI名称: 小助手
模式: 无限对话
提示词文件: C:\Game\config\ai_persona.txt

[2024-01-20 10:30:20] 用户: 你好
[2024-01-20 10:30:22] AI: 你好！我是小助手...

[2024-01-20 10:35:10] 用户输入: /exit
[2024-01-20 10:35:10] 会话结束
总对话轮次: 5
```

## 🔄 更新日志

### v1.0.0 (当前版本)
- 初始版本发布
- 支持三种对话模式
- GB2312编码强制输出
- 打字机效果显示
- 完整的日志系统
- PGN插件集成

## 📞 支持与反馈

如有问题或建议：
1. 检查日志文件获取详细错误信息
2. 确保所有参数格式正确
3. 验证文件编码为GB2312
4. 确认API密钥有效

---

**享受与AI的智能对话吧！** 🎮🤖