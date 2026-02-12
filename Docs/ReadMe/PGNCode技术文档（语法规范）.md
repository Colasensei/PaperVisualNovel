# PGN脚本语言语法文档

## 概述

**PGN** 是一种为视觉小说游戏设计的脚本语言，语法简单直观，支持分支剧情、变量操作和多媒体控制。

## 基础语法规则

### 1. 文件结构

```
*.pgn 文件
├── 注释（以#或//开头）
├── 标签定义（以:结尾）
├── 命令语句
└── 空行（被忽略）
```

### 2. 注释

```pgn
# 单行注释
// 也是单行注释
```

### 3. 大小写敏感

- 命令关键字不区分大小写：`say` = `SAY` 
- 变量名区分大小写：`score` ≠ `Score`
- 标签名区分大小写：`start:` ≠ `Start:`

### 4.空格规范（重要）

* **所有命令和参数之间必须用空格分隔**
* **标签定义必须独占一行，且以冒号结尾**
* **可使用缩进**（Tab或空格）

## 命令参考

### 文本显示命令

#### `say` - 显示对话文本

```pgn
# 基本语法
say "文本内容" [显示时间] [颜色]

# 示例
say "你好，世界！"               # 默认白色，0.5秒/字符
say "紧急情况！" 0.2 red         # 红色，快速显示
say "神秘信息..." 1.0 purple     # 紫色，慢速显示

# 变量插值
set score = 100
say "${score}分"             
```

**参数说明：**

- `文本内容`：要显示的文本，支持变量插值 `${变量名}`
- `显示时间`：可选，每个字符显示时间（秒），默认0.5
- `颜色`：可选，文本颜色（见颜色表），默认white

#### `sayvar` - 显示变量值

历史遗留命令，不建议使用，最佳实践为say的插值新风格

```pgn
sayvar 变量名 显示时间 颜色

# 示例
set score = 100
sayvar score 0.3 yellow          # 显示：100
```

### 游戏流程控制

#### `wait` - 等待

谨慎使用，可能影响体验

```pgn
wait 毫秒数

# 示例
wait 1000     # 等待1秒
wait 500      # 等待0.5秒
```

#### `end` - 结束游戏

此处提醒，顺序编写

```pgn
end           # 结束游戏，返回主菜单
```

#### `endname` - 标记结局

不代表结束游戏，仍需要end命令

```pgn
endname "结局名称"

# 示例
endname "美好结局"      # 达成"美好结局"
endname "悲剧收场"      # 达成"悲剧收场"
```

### 变量操作

#### `set` - 设置变量

暂只支持int

```pgn
set 变量名 操作符 值

# 示例
set score = 100        # 赋值
set score += 10        # 加法
set score -= 5         # 减法
set score *= 2         # 乘法
set score /= 4         # 除法
```

**支持的操作符：**

- `=`：赋值
- `+=`：加法赋值
- `-=`：减法赋值
- `*=`：乘法赋值
- `/=`：除法赋值

#### `random` - 生成随机数

生成即赋值，仅供单次使用

```pgn
random 变量名 最小值 最大值

# 示例
random dice 1 6        # 生成1-6的随机数
random luck 0 100      # 生成0-100的随机数
```

### `input` - 输入字符串

```pgn
input "提示文本" 变量名

```



### 流程控制

#### `jump` - 无条件跳转

支持标签与数字（代表行号）

```pgn
jump 目标

# 示例
jump chapter2          # 跳转到标签 chapter2
jump 50                # 跳转到第50行
```

#### `choose` - 分支选择

请注意，写在一行以内！

```pgn
choose 选项数量 标签1:显示文本1 标签2:显示文本2
...

# 示例
choose 2 go_library:去图书馆 go_park:去公园

# 错误示例（没在一行内）
choose 2 go_library:去图书馆
???????? go_park:去公园
```

**选择界面：**

```
请选择：
1. 去图书馆
2. 去公园
请输入数字选择 (1-2):
```

#### `if` - 条件跳转

```pgn
if 条件表达式 目标

# 示例
if score > 80 good_end
if score <= 80 bad_end
if flag == 1 && money >= 100 secret_shop
```

#### 插件系统语法规范

##### `use` 命令

###### 语法格式

```
use <插件名> [<版本号>]
```

###### 参数说明

| 参数  | 必需  | 说明              |
| --- | --- | --------------- |
| 插件名 | 是   | 插件文件夹的名称，不区分大小写 |
| 版本号 | 否   | 插件版本号，用于版本兼容性检查 |

###### 版本号格式

- 数字格式：`1.0.0`, `2.1.3`
- 带引号格式：`"1.0.0"`, `"2.0.0-beta"`
- 多单词格式：`1.0.0 release`, `2.1.3 stable`

###### 功能描述

`use` 命令用于在PGN脚本开头声明插件依赖。当脚本执行到 `use` 命令时，系统会：

1. 检查 `Plugins/<插件名>/` 目录是否存在
2. 检查 `about.cfg` 配置文件是否存在
3. 如果指定了版本号，验证插件版本是否兼容
4. 如果检查失败，显示错误信息并可能中断游戏执行

###### 示例

```pgn
# 基本用法 - 只检查插件存在
use Calculator

# 带版本号检查
use Calculator 1.0.0

# 带引号的版本号
use ImageViewer "2.1.0"

# 多个插件依赖
use DataProcessor 2.1.0
use AIChat 1.5.0
use Network "3.0.0-beta"
```

###### 错误处理

| 错误类型   | 错误信息                         | 用户操作          |
| ------ | ---------------------------- | ------------- |
| 插件不存在  | "插件 'XXX' 未安装"               | 显示安装指导，返回主菜单  |
| 配置文件缺失 | "插件配置文件缺失"                   | 警告，可选择继续      |
| 版本不兼容  | "版本不匹配: 需要vX.X.X, 已安装vY.Y.Y" | 显示警告，可选择继续或返回 |

---

##### `plugin` 命令

###### 语法格式

```
plugin <插件名> "<参数>"
```

###### 参数说明

| 参数  | 必需  | 说明                   |
| --- | --- | -------------------- |
| 插件名 | 是   | 要执行的插件名称             |
| 参数  | 否   | 传递给插件的参数字符串，必须用双引号包裹 |

###### 特殊语法标记

1. 路径转换标记

| 标记          | 说明          | 示例                     | 转换结果                                 |
| ----------- | ----------- | ---------------------- | ------------------------------------ |
| `$file{路径}` | 转换为游戏目录绝对路径 | `$file{images/bg.png}` | `C:\Game\Novel\MyGame\images\bg.png` |
| `$log`      | 转换为日志文件绝对路径 | `$log`                 | `C:\Game\pvn_engine.log`             |

2. 变量替换标记

| 标记       | 说明       | 示例              |
| -------- | -------- | --------------- |
| `${变量名}` | 替换为游戏变量值 | `${playerName}` |

3. 转义字符

| 转义序列 | 实际字符 | 说明               |
| ---- | ---- | ---------------- |
| `\"` | `"`  | 双引号              |
| `\\` | `\`  | 反斜杠              |
| `\n` | 换行符  | 换行               |
| `\t` | 制表符  | 制表               |
| `\r` | 回车符  | 回车               |
| `\$` | `$`  | 美元符号（防止被识别为特殊标记） |
| `\{` | `{`  | 左花括号             |
| `\}` | `}`  | 右花括号             |

###### 参数解析规则

1. 引号处理规则

2. 参数必须用双引号 `"` 包裹

3. 引号内的 `\"` 会被转义为 `"`

4. 支持多行参数（通过 `\n`）

5. 标记嵌套规则

6. `$file{}` 标记内部支持转义：
   
   ```pgn
   # 正确：路径包含花括号
   plugin Test "$file{data/{config}.json}"
   
   # 正确：转义花括号
   plugin Test "$file{data/\{config\}.txt}"
   ```

7. `${}` 变量替换标记内部不支持嵌套

###### 执行优先级

1. 转义字符处理
2. 变量替换 `${}`
3. 路径转换 `$file{}` 和 `$log`

```pgn
# 无参数执行
plugin Calculator

# 简单参数
plugin Calculator "1+2*3"

# 带路径转换
plugin ImageViewer "$file{scenes/opening.png}"
```

###### 复杂参数示例

```pgn
# 多参数、转义、路径转换组合
plugin DataExporter "
--input \"$file{saves/autosave.sav}\" 
--output \"$file{exports/data.json}\" 
--log \"$log\" 
--format JSON 
--verbose
"

# 包含引号和换行的参数
plugin ChatBot "你好！\n我是AI助手。\n你说：\"今天天气真好\"\n我回复：\"是的！\""

# 使用游戏变量
set playerLevel 5
plugin Achievement "恭喜达到 ${playerLevel} 级！"

# 特殊字符处理
plugin Test "特殊路径：$file{data/\{test\}/file.txt} 字面量：\\$file\\{\\}"
```

###### 错误处理

| 错误类型   | 处理方式           |
| ------ | -------------- |
| 插件未安装  | 提示安装，继续执行下一行   |
| 执行失败   | 显示错误信息，继续执行下一行 |
| 参数解析错误 | 显示语法错误，继续执行下一行 |

###### 最佳实践

1. 参数格式化

```pgn
# 推荐：清晰的多参数格式
plugin AdvancedPlugin "
--mode interactive
--input \"$file{data/input.dat}\"
--output \"$file{results/output.json}\"
--log-level debug
--timeout 30
"

# 不推荐：难以阅读的单行复杂参数
plugin AdvancedPlugin "--mode interactive --input \"$file{data/input.dat}\" --output \"$file{results/output.json}\" --log-level debug --timeout 30"
```

2. 错误处理

```pgn
# 可以配合条件判断使用
if ${hasPlugin} == 1 plugin "SpecialEffect" "fireworks"
else say "特殊效果插件未安装" 0.5 yellow
```

3. 性能考虑

```pgn
# 避免在循环中频繁调用插件
# 不推荐：
jump loop_start
:loop_start
plugin Calculator "${counter} * 2"
add counter 1
if ${counter} < 10 jump loop_start

# 推荐：批量处理
plugin Calculator "batch_process ${counter}"
```

###### 注意事项

1. **安全性**：插件以系统权限执行，确保插件来源可信
2. **性能**：插件调用有启动开销，避免频繁调用
3. **兼容性**：不同操作系统可能影响路径处理和命令执行
4. **调试**：使用调试终端测试插件参数
5. **版本管理**：插件更新可能导致脚本不兼容

###### 扩展语法（未来可能支持）

```pgn
# 异步执行（建议）
plugin async ChatBot "处理中..."

# 获取返回值
set result = plugin Calculator "eval: 1+2"

# 管道操作
plugin DataGenerator "generate" | plugin DataProcessor "analyze"
```

---

######  完整示例脚本

```pgn
# AdventureGame.pgn
# 声明插件依赖
use ImageViewer "2.1.0"
use Calculator 1.0.0
use MusicPlayer "1.3.0"

say "欢迎来到奇幻冒险！" 0.5 white

# 显示开场图片
plugin ImageViewer "$file{images/opening.jpg} --fullscreen --duration 3000"

# 属性计算
set strength 10
set agility 8
plugin Calculator "combat_score: ${strength} * 2 + ${agility}"

# 背景音乐
plugin MusicPlayer "play --file \"$file{music/theme.mp3}\" --volume 80 --loop"

# 复杂对话系统
set playerName "冒险者"
plugin DialogSystem "
speaker: 老人
message: \"你好，${playerName}！\n欢迎来到我们的村庄。\"
choices: [\"询问任务\", \"了解历史\", \"离开\"]
timeout: 30
"

say "游戏开始！" 0.3 green
```

这个语法规范为插件系统的使用提供了完整的指导，确保开发者能够正确、安全地使用插件功能。 

### 多媒体控制

#### `show` - 显示文件

```pgn
show 文件名

# 示例
show background.jpg    # 显示图片
show letter.txt        # 显示文本文件
```

**支持的文件类型：**

- 图片：.jpg, .jpeg, .png, .bmp, .gif
- 文本：.txt, .md, .log
- 文档：.pdf, .doc, .docx
- 音频：.mp3, .wav
- 视频：.mp4, .avi

## 颜色表

| 颜色  | 英文     | 示例                    |
| --- | ------ | --------------------- |
| 黑色  | black  | `say "文本" 0.5 black`  |
| 蓝色  | blue   | `say "文本" 0.5 blue`   |
| 绿色  | green  | `say "文本" 0.5 green`  |
| 青色  | aqua   | `say "文本" 0.5 aqua`   |
| 红色  | red    | `say "文本" 0.5 red`    |
| 紫色  | purple | `say "文本" 0.5 purple` |
| 黄色  | yellow | `say "文本" 0.5 yellow` |
| 白色  | white  | `say "文本" 0.5 white`  |
| 灰色  | gray   | `say "文本" 0.5 gray`   |

## 标签系统

### 定义标签

```pgn
标签名:
    命令1
    命令2
    ...

# 示例
start:
    say "游戏开始..."
    jump intro

intro:
    say "这是引子部分..."
```

### 使用标签

```pgn
jump start      # 跳转到start标签
if x > 5 start  # 条件跳转到start标签
```

## 条件表达式语法

### 比较运算符

```pgn
# 数值比较
a == b      # 等于
a != b      # 不等于
a < b       # 小于
a > b       # 大于
a <= b      # 小于等于
a >= b      # 大于等于

# 逻辑运算
a && b      # 逻辑与（AND）
a || b      # 逻辑或（OR）

# 示例
score >= 60 && score < 80
flag == 1 || money > 100
```

### 变量和常量

```pgn
# 变量
player_score
game_flag
item_count

# 常量
100         # 整数
0           # 零
-5          # 负数
```

### 复杂表达式

```pgn
# 支持括号
(score > 80 && flag == 1) || secret_unlocked

# 组合条件
if (hp <= 0) game_over
if (key_found && door_unlocked) enter_room
```

## 文件路径处理

### 相对路径

```pgn
# 相对于游戏目录的archive文件夹
show image.jpg          # 实际路径：游戏目录/archive/image.jpg
```

### 特殊目录

```
游戏目录/
├── 游戏名.pgn          # 脚本文件
├── archive/           # 资源文件夹
└── saves/            # 存档文件夹（自动创建）
```

## 转义字符

### 字符串中的转义

```pgn
say "他说：\"你好！\""        # 输出：他说："你好！"
say "第一行\n第二行"          # 输出：第一行（换行）第二行
say "制表符\t文本"            # 输出：制表符    文本
say "反斜杠：\\"              # 输出：反斜杠：\
```

**支持的转义序列：**

- `\"`：双引号
- `\\`：反斜杠
- `\n`：换行
- `\t`：制表符
- `\r`：回车

## 脚本执行流程

### 基本流程

```
开始
├── 按顺序执行命令
├── 遇到选择：暂停等待用户输入
├── 遇到跳转：跳转到指定位置
├── 遇到结束：停止执行
└── 脚本结束：返回主菜单
```

### 执行示例

```pgn
# 示例脚本
start:
    say "欢迎来到冒险游戏！" 0.5 blue
    set score = 0

    choose 2 left:向前走 right:向后走

left:
    say "你选择了向前..." 0.3
    set score += 10
    jump check_score

right:
    say "你选择了向后..." 0.3
    set score += 5
    jump check_score

check_score:
    if score >= 10 good_end
    if score < 10 bad_end

good_end:
    say "恭喜！完美结局！" 0.5 green
    endname "完美结局"
    end

bad_end:
    say "游戏结束..." 0.5 red
    endname "普通结局"
    end
```

## 语法限制和注意事项

### 1. 一行一条命令

```pgn
# 正确
say "你好"
wait 1000

# 错误
say "你好" wait 1000
```

### 2. 变量命名规则

- 只能包含字母、数字、下划线
- 不能以数字开头
- 区分大小写
- 示例：`score`, `player_name`, `item_1`

### 3. 字符串引号

```pgn
# 正确 - 使用双引号
say "文本内容"

# 错误 - 单引号不支持
say '文本内容'
```

### 4. 数值范围

- 整数：-2,147,483,648 到 2,147,483,647
- 不支持小数和科学计数法

## 错误处理

### 常见错误

1. **语法错误**
   
   ```pgn
   say 文本内容      # 缺少引号，但因为向后兼容，也可使用，并非语法错误
   set x == 5       # 错误：应该用 = 而不是 ==
   ```

2. **运行时错误**
   
   ```pgn
   set x = y        # 错误：不支持增加变量
   jump unknown     # 错误：标签不存在
   show missing.jpg # 错误：文件不存在
   ```

3. **逻辑错误**
   
   ```pgn
   # 无限循环
   loop:
       jump loop
   ```

### 错误消息格式

```
错误：未知的PGN命令 - 命令名
错误：跳转目标无效 - 目标名
错误：say命令中的字符串缺少结束引号
```

## 最佳实践

### 1. 代码组织

```pgn
# 使用空行分隔逻辑块
start:
    # 初始化
    set score = 0
    set flag = 0

    # 剧情开始
    say "故事开始..."

    # 第一个选择
    choose 2 option1:选择1 option2:选择2
```

### 2. 注释规范

```pgn
# 章节：第一章
# 作者：小明
# 日期：2024-01-01

chapter1:
    # 场景：教室
    say "教室里静悄悄的..."

    # 选择：去向
    choose 2 stay:留在教室 leave:离开教室
```

### 3. 标签命名

```pgn
# 使用有意义的名称
introduction:      # 引子
battle_start:      # 战斗开始
good_ending:       # 好结局
secret_room:       # 秘密房间
```

### 4. 变量管理

```pgn
# 使用前缀区分变量类型
# str_ 字符串变量（未来支持）
# int_ 整数变量
# flag_ 标志变量

set int_score = 0
set flag_key_found = 1
set flag_door_unlocked = 0
```

## 调试技巧

### 1. 使用调试终端（F12）

```
Debug> vars        # 查看所有变量
Debug> info        # 查看游戏信息
Debug> goto 行号   # 跳转到指定行
Debug> log info 消息 # 输出日志
```

### 2. 临时调试命令

```pgn
# 显示变量值（调试用）
say "当前分数：${score}" 0.1 gray
wait 500

# 条件检查
if debug_mode == 1 show_debug_info
```

## 学习资源

### 示例脚本

```pgn
# hello_world.pgn
start:
    say "Hello, World!" 0.3 blue
    wait 1000
    say "这是你的第一个PGN脚本" 0.5
    end
```

### 模板项目

```
template_game/
├── game.pgn          # 主脚本
├── archive/
└── README.txt       # 说明文档
```

## 附录

### A. 保留关键字

```
say, sayvar, wait, end, endname
set, random
jump, choose, if
show
```

### B. 颜色值对照表

| 颜色     | RGB值          | 控制台代码    |
| ------ | ------------- | -------- |
| black  | (0,0,0)       | \033[30m |
| blue   | (0,0,255)     | \033[34m |
| green  | (0,255,0)     | \033[32m |
| aqua   | (0,255,255)   | \033[36m |
| red    | (255,0,0)     | \033[31m |
| purple | (255,0,255)   | \033[35m |
| yellow | (255,255,0)   | \033[33m |
| white  | (255,255,255) | \033[37m |
| gray   | (128,128,128) | \033[90m |

### C. 文件扩展名

- `.pgn`：PGN脚本文件
- `.sav`：存档文件
- `.inf`：游戏信息文件
- `.cfg`:   游戏设置键值文件

---

## 技术支持

### 获取帮助

1. 查看本文档
2. 查看其他文档
3. 运行示例脚本
4. 使用调试终端（F12）
5. 查看日志文件 `pvn_engine.log`
6. 发布issue

### 报告问题

```pgn
# 在脚本中重现问题
problem_test:
# 导致问题的命令
say "测试问题..."
# 问题描述
```

## 当前

**版本：** PGN语法 v1.0  
**最后更新：** 2026年1月17日  
**状态：** Beta

版本
