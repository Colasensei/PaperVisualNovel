# PGNCode 技术文档 v4.0

## PaperVisualNovel 脚本语言完整规范

---

## 一、核心设计原则

### 1.1 空格规则（强制）

- **所有命令和参数之间必须用空格分隔**
- **字符串中不能包含空格**（使用中文或连接符）
- **标签定义必须独占一行，且以冒号结尾**
- **禁止使用任何缩进**（Tab或空格）

### 1.2 注释规则

```
// 单行注释（推荐）
# 单行注释（可选）

// ❌ 错误：注释前有缩进
    // 这是错误的注释
```

### 1.3 文件结构

```
游戏文件夹/
├── 游戏名称.pgn          # 主脚本文件（必须与文件夹同名）
├── endings.dat          # 结局收集数据文件（自动生成）
└── archive/            # 资源目录
```

---

## 二、基础命令

### 2.1 say - 显示文本（支持变量混合）

```
语法：say [文本] [显示时间] [颜色]

参数：
  文本：显示的内容，支持变量占位符 ${变量名}
  显示时间：秒（小数），控制打字机速度
  颜色：black, blue, green, aqua, red, purple, yellow, white

示例：
  // 基础文本
  say 你好世界 0.5 white

  // 包含变量
  set score = 100
  say 你的分数是：${score}分 0.5 green

  // 多个变量
  say HP:${hp}/${max_hp}|MP:${mp}/${max_mp} 0.5 aqua

说明：
  若要使用空格，需添加""，如：say "你好，世界" 0.5 white
  若要使用特殊字符，需添加\"，如：say "你好，\"世界\"" 0.5 white
```

### 2.2 wait - 等待

```
语法：wait [毫秒数]

功能：暂停指定毫秒数

示例：
  wait 1000      // 等待1秒
  wait 500       // 等待0.5秒
```

### 2.3 show - 显示文件

```
语法：show [文件名]

功能：打开archive目录中的文件

支持格式：
  图片：.jpg, .png, .bmp, .gif
  文档：.txt, .pdf, .doc, .docx
  音频：.mp3, .wav, .ogg
  视频：.mp4, .avi, .mkv
```

---

## 三、变量系统

### 3.1 set - 变量操作

```
语法：set [变量名] [操作符] [值]

操作符：
  =   赋值
  +=  加法赋值
  -=  减法赋值
  *=  乘法赋值
  /=  除法赋值

示例：
  set score = 100           // 初始化
  set score += 10           // 增加10
  set score -= 20           // 减少20
```

### 3.2 random - 生成随机数

```
语法：random [变量名] [最小值] [最大值]

功能：生成[min, max]范围内的随机整数

示例：
  random dice 1 6           // 掷骰子：1-6
  random luck 1 100         // 幸运值：1-100
```

---

## 四、流程控制

### 4.1 标签定义

```
语法：[标签名]:

规则：
  1. 标签名后必须跟冒号
  2. 独占一行，不能缩进
  3. 标签名不能包含空格
  4. 大小写敏感
```

### 4.2 jump - 无条件跳转

```
语法：jump [目标]

目标格式：
  1. 标签名（可带或不带冒号）
  2. 行号（正整数）

示例：
  jump START_SCENE:         // 跳转到标签（带冒号）
  jump BATTLE_BEGIN         // 跳转到标签（不带冒号）
  jump 25                   // 跳转到第25行
```

### 4.3 if - 条件跳转

```
语法：if [变量名] [比较符] [值] [目标]

比较符：
  ==  等于
  >   大于
  <   小于
  >=  大于等于
  <=  小于等于
  !=  不等于

也可使用较复杂的双目运算符：
  &&  逻辑与
  ||  逻辑或
  如
  if hp > 50 && mp > 20 BATTLE_BEGIN

示例：
  if hp > 50 BATTLE_BEGIN
  if mp < 10 LOW_MP

  if morality >= 80 GOOD_ENDING
  if secrets >= 3 SECRET_ENDING

```

### 4.4 choose - 标签式选择

```
语法：choose [选项数量] [标签1:文本1] [标签2:文本2] ... [标签N:文本N]

功能：显示选择菜单，根据选择跳转到对应标签

示例：
  choose 2 FIGHT:战斗 RUN:逃跑
  choose 3 TOWN:去镇上 FOREST:去森林 HOME:回家
```

### 4.5 end - 结束游戏

```
语法：end 或 END

功能：结束当前游戏，返回主菜单

示例：
  say 游戏结束 1.0 red
  end
```

---

## 五、结局收集系统 ⭐️新功能⭐️

### 5.1 endname - 记录结局

```
语法：endname [结局名称]

功能：
  1. 记录玩家达成的结局名称
  2. 将结局保存到游戏文件夹的 data.inf 文件中
  3. 显示已收集的结局进度
  4. 不结束游戏（需单独使用 end 命令）

特点：
  - 自动去重：同一结局只会记录一次
  - 进度显示：显示"已收集结局 X/Y"
  - 独立运行：不影响游戏流程，需配合 end 命令
  - 跨会话保存：结局数据永久保存

数据存储：
  文件位置：Novel/游戏文件夹/data.inf
  文件格式：每行一个结局名称

示例：
  endname 英雄结局
  // 显示：结局达成：英雄结局 | 已收集结局：1/5

  endname 隐藏结局
  // 显示：结局达成：隐藏结局 | 已收集结局：2/5
```

### 5.2 结局收集系统使用示例

#### 基本用法

```pgn
// 简单结局记录
say 恭喜通关！ 0.8 yellow
endname 普通结局
say 感谢游玩 0.5 white
end
```

#### 多结局游戏

```pgn
// 根据变量决定结局
if morality >= 80 GOOD_ENDING
if morality <= 20 BAD_ENDING
if secrets >= 3 SECRET_ENDING

// 普通结局
say 游戏结束-平凡的生活 0.5 white
endname 平凡结局
end

GOOD_ENDING:
say 游戏结束-英雄之路 0.8 yellow
endname 英雄结局
end

BAD_ENDING:
say 游戏结束-堕落之路 0.8 red
endname 堕落结局
end

SECRET_ENDING:
say 游戏结束-隐藏真相 0.8 purple
endname 隐藏结局
end
```

#### 复杂多结局系统

```pgn
// RPG游戏多结局示例
set reputation = 75
set completed_quests = 8
set relationships = {
  "alice": 80,
  "bob": 60,
  "charlie": 40
}

// 结局判断逻辑
if reputation >= 90 REPUTATION_ENDING
if completed_quests >= 10 QUEST_MASTER_ENDING
if relationships["alice"] >= 90 ROMANCE_ENDING

// 根据多个条件组合
if reputation >= 70 && completed_quests >= 8 COMBINATION_ENDING

// 默认结局
endname 冒险者结局
say 你的冒险暂时告一段落... 0.5 white
end

REPUTATION_ENDING:
endname 传奇英雄结局
say 你的名声传遍了整个大陆... 0.8 gold
end

QUEST_MASTER_ENDING:
endname 任务大师结局
say 你完成了无数委托，成为传奇... 0.8 blue
end

ROMANCE_ENDING:
endname 真爱结局
say 你和爱丽丝幸福地生活在一起... 0.8 pink
end

COMBINATION_ENDING:
endname 完美结局
say 你达成了完美的冒险之旅！ 1.0 rainbow
end
```

#### 收集型游戏

```pgn
// 收集所有结局的示例
set endings_found = 0

END_01:
endname 结局1：初出茅庐
set endings_found += 1
say 找到结局 ${endings_found}/8 0.5 green
end

END_02:
endname 结局2：勇者之路
set endings_found += 1
say 找到结局 ${endings_found}/8 0.5 green
end

// ... 更多结局

END_08:
endname 结局8：终极真相
set endings_found += 1
say 恭喜！收集了全部8个结局！ 1.0 gold
end
```

### 5.3 结局进度显示

当执行 `endname` 命令时，系统会自动显示：

```
==============================
结局达成：[结局名称]
已收集结局：[已收集数]/[总数]
==============================
```

**示例输出：**

```
==============================
结局达成：英雄结局
已收集结局：3/7
==============================
```

### 5.4 结局数据管理

#### 数据文件位置

```
游戏根目录/
└── Novel/
    └── 你的游戏/
        ├── 你的游戏.pgn
        ├── endings.dat    ← 结局数据文件
        └── archive/
```

#### 数据文件格式

```
英雄结局
隐藏结局
悲剧结局
真实结局
```

#### 自动管理功能

1. **自动创建**：第一次达成结局时自动创建文件
2. **自动去重**：不会重复记录相同结局
3. **自动统计**：实时更新收集进度
4. **永久保存**：数据跨游戏会话保存

---

## 六、高级特性

### 6.1 变量混合输出

```
// 在文本中直接插入变量
set player_name = 冒险者
set level = 15
set exp = 1250

say 欢迎，${player_name}！你是${level}级冒险者，拥有${exp}点经验。 0.8 yellow
```

### 6.2 嵌套选择系统

```
// 多层决策树
choose 2 APPROACH:接近 AVOID:避开

APPROACH:
choose 3 TALK:对话 INSPECT:检查 ATTACK:攻击

TALK:
choose 2 FRIENDLY:友好 THREATEN:威胁
// 继续嵌套...
```

### 6.3 动态选择系统

```
// 根据游戏状态显示不同选项
set has_key = 1
set knows_password = 0

if has_key == 1 OPTIONS_WITH_KEY
choose 2 EXAMINE:检查 LEAVE:离开

OPTIONS_WITH_KEY:
choose 3 UNLOCK:使用钥匙 EXAMINE:检查 LEAVE:离开
```

### 6.4 循环系统

```
set attempts = 0

PUZZLE_LOOP:
if attempts >= 3 FAIL
choose 2 TRY_AGAIN:再次尝试 GIVE_UP:放弃

TRY_AGAIN:
set attempts += 1
jump PUZZLE_LOOP

GIVE_UP:
jump FAIL
```

---

## 七、最佳实践

### 7.1 命名规范

```
// 使用前缀系统
SCENE_VILLAGE:
DIALOGUE_MERCHANT:
CHOICE_CHAPTER1:
QUEST_MAIN:
ENDING_GOOD:
```

### 7.2 结局设计规范

#### 结局命名规范

```pgn
// 清晰易懂的结局名称
endname 英雄结局        // ✅ 明确
endname 真结局         // ✅ 清晰
endname 结局1          // ⚠️ 避免使用数字编号
endname end            // ❌ 避免使用关键字

// 分类结局
endname 战斗结局：胜利
endname 剧情结局：牺牲
endname 隐藏结局：真相
```

#### 多结局流程设计

```pgn
// 推荐结构：集中判断，分散处理
END_GAME:
// 集中判断所有结局条件
if score >= 1000 ENDING_S
if score >= 800 ENDING_A
if score >= 600 ENDING_B
if score >= 400 ENDING_C
endname 结局D
jump ENDING_D

// 各个结局的具体处理
ENDING_S:
endname 结局S
say S级评价！完美通关！ 1.0 gold
end

ENDING_A:
endname 结局A
say A级评价！优秀表现！ 0.8 yellow
end

// ... 其他结局
```

#### 结局解锁提示

```pgn
// 显示解锁的结局信息
SECRET_ENDING_UNLOCK:
say "成就解锁！" 0.8 yellow
wait 1000
endname 隐藏结局：古老秘密
say "你发现了被遗忘的古老秘密..." 0.5 purple
wait 2000
end
```

### 7.3 脚本组织结构

```pgn
// ===============================
// 游戏：命运的选择
// 作者：冒险者工作室
// 版本：2.0
// 总结局数：8
// ===============================

// 初始化
INIT:
set morality = 50
set strength = 50
set intelligence = 50

// 第一章
CHAPTER_1:
// ... 游戏内容

// 结局判断（游戏结尾处）
GAME_END:
// 判断所有可能的结局
if morality >= 80 && strength >= 70 HERO_ENDING
if morality <= 20 && strength >= 70 VILLAIN_ENDING
if intelligence >= 80 WISE_ENDING
if morality >= 60 && intelligence >= 60 BALANCED_ENDING
// 默认结局
endname 平凡旅行者
jump END_DEFAULT

// 各个结局的实现
HERO_ENDING:
endname 传奇英雄
say 你成为传颂千古的英雄... 1.0 gold
end

VILLAIN_ENDING:
endname 绝世魔王
say 你的恶名响彻整个世界... 1.0 crimson
end

WISE_ENDING:
endname 智慧贤者
say 你的智慧照亮了时代... 1.0 blue
end

BALANCED_ENDING:
endname 平衡守护者
say 你在力量与智慧间找到平衡... 1.0 purple
end

END_DEFAULT:
say 你的冒险告一段落... 0.5 white
end
```

### 7.4 调试与测试

```pgn
// 结局测试代码
TEST_ALL_ENDINGS:
say 测试所有结局... 0.5 yellow

// 测试结局1
endname 测试结局1
say 测试结局1记录完成 0.3 green
wait 500

// 测试结局2
endname 测试结局2
say 测试结局2记录完成 0.3 green
wait 500

// 显示当前进度
say 当前测试进度：2/5 0.5 aqua
end
```

---

## 八、完整游戏示例：多结局RPG

```pgn
// ===============================
// 游戏：龙之传说
// 结局数：6
// ===============================

// 初始化属性
set strength = 50    // 力量
set wisdom = 50      // 智慧
set courage = 50     // 勇气
set morality = 50    // 道德
set dragon_heart = 0 // 龙之心收集

// 游戏开始
START:
say 龙之传说 1.0 gold
wait 1500

// 第一章：英雄诞生
CHAPTER_1:
say 第一章：命运的召唤 0.8 yellow

choose 3 TRAIN:刻苦训练 STUDY:研究古籍 EXPLORE:探索世界

TRAIN:
set strength += 20
say 力量提升至${strength} 0.5 red
jump CHAPTER_2

STUDY:
set wisdom += 20
say 智慧提升至${wisdom} 0.5 blue
jump CHAPTER_2

EXPLORE:
set courage += 20
say 勇气提升至${courage} 0.5 green
jump CHAPTER_2

// 第二章：龙之试炼
CHAPTER_2:
say 第二章：龙之试炼 0.8 yellow

choose 2 FIGHT_DRAGON:挑战巨龙 HELP_DRAGON:帮助巨龙

FIGHT_DRAGON:
if strength >= 60 FIGHT_SUCCESS
say 你的力量不足，被巨龙击败... 0.5 red
set morality -= 10
jump CHAPTER_3

FIGHT_SUCCESS:
say 你击败了巨龙！ 0.8 gold
set dragon_heart += 1
set courage += 20
jump CHAPTER_3

HELP_DRAGON:
say 你选择帮助受伤的巨龙... 0.5 blue
set morality += 20
set wisdom += 10
jump CHAPTER_3

// 第三章：最终抉择
CHAPTER_3:
say 第三章：最终抉择 0.8 yellow
say 当前属性： 0.5 white
say 力量:${strength} 智慧:${wisdom} 勇气:${courage} 道德:${morality} 0.5 aqua

if dragon_heart > 0 HAS_DRAGON_HEART

choose 2 SAVE_WORLD:拯救世界 RULE_WORLD:统治世界

SAVE_WORLD:
if morality >= 70 TRUE_HERO
if courage >= 70 BRAVE_HERO
endname 普通英雄
say 你成为了英雄，但传说很快被遗忘... 0.5 white
end

RULE_WORLD:
if strength >= 70 TYRANT
if wisdom >= 70 SCHEMER
endname 野心家
say 你的野心未能实现... 0.5 gray
end

HAS_DRAGON_HEART:
choose 3 SAVE_WORLD:拯救世界  RULE_WORLD:统治世界 ASCEND:成为龙神

ASCEND:
endname 龙神结局
say 你融合龙之心，成为了新的龙神！ 1.0 rainbow
end

TRUE_HERO:
endname 真英雄结局
say 你的美德将被永远传颂！ 1.0 gold
end

BRAVE_HERO:
endname 勇者结局
say 你的勇气激励了无数后人！ 0.8 orange
end

TYRANT:
endname 暴君结局
say 你以铁腕统治了世界... 0.8 crimson
end

SCHEMER:
endname 智者结局
say 你以智慧掌控了一切... 0.8 purple
end

// 隐藏结局条件
HIDDEN_CONDITIONS:
if strength >= 80 && wisdom >= 80 && courage >= 80 && morality >= 80 PERFECT_ENDING
jump GAME_END

PERFECT_ENDING:
endname 完美结局
say 你达成了所有完美条件！ 1.0 diamond
say 力量:${strength} 智慧:${wisdom} 勇气:${courage} 道德:${morality} 0.5 white
end

GAME_END:
say 游戏结束 0.5 white
end
```

---

## 九、错误处理

### 常见错误

```
// 1. 空格错误
❌ endname英雄结局          // 缺少空格
✅ endname 英雄结局         // 正确

// 2. 结局名称包含空格
❌ endname 英雄 结局        // 包含空格
✅ endname 英雄结局         // 使用中文
✅ endname Hero_Ending      // 使用下划线

// 3. 缺少end命令
❌ endname 结局
// 游戏继续运行，不会结束

✅ endname 结局
end                    // 正确配合使用
```

### 结局系统特定错误

```
1. 文件写入失败：检查游戏文件夹权限
2. 重复记录：系统自动处理，不会重复
3. 进度显示错误：重启游戏重新加载
```

---

## 十、快速参考卡

### 命令速查

```
基础命令：
  say 文本 时间 颜色              // 显示文本
  wait 毫秒                      // 等待
  show 文件名                    // 显示文件

变量命令：
  set 变量 操作符 值              // 变量操作
  random 变量 最小 最大           // 随机数

流程控制：
  [标签名]:                      // 定义标签
  jump 目标                      // 跳转
  if 变量 比较符 值 目标          // 条件跳转
  choose N 标签1:文本1 ...        // 选择菜单
  end                            // 结束游戏

结局系统：⭐️
  endname 结局名称               // 记录结局
```

### 特殊语法

```
变量输出：${变量名}              // 在say命令中使用
结局显示：自动显示"已收集 X/Y"
颜色代码：black, blue, green, aqua, red, purple, yellow, white
比较符：==, >, <, >=, <=, !=
```

---

## 总结

PGNCode v4.0 引入了强大的结局收集系统，为多结局游戏提供了完整的解决方案：

### 核心优势：

1. **简单易用**：`endname` 命令直观易用
2. **自动管理**：系统自动处理数据存储和去重
3. **进度跟踪**：实时显示收集进度，增加重玩价值
4. **永久保存**：结局数据跨会话保存
5. **完全兼容**：不影响现有脚本功能

### 设计理念：

- **鼓励探索**：玩家可以追踪未达成的结局
- **增加重玩性**：明确的收集目标激励重复游玩
- **成就感**：显示收集进度提供成就感
- **灵活设计**：支持各种类型的多结局游戏

### 创作者建议：

1. 设计3-8个有意义的结局
2. 为每个结局提供独特的剧情和反馈
3. 考虑添加隐藏结局增加探索乐趣
4. 确保结局名称清晰易懂
5. 测试所有结局路径确保正常记录

**现在，开始创作你的史诗级多结局游戏吧！**

---

*文档版本：4.0 | 最后更新：2024年*
*PaperVisualNovel - 千页小说引擎 | 结局收集系统已就绪*
