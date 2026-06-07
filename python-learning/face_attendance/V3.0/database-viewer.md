# 考勤数据库查看工具
## Database Viewer for Attendance System

---

> **用途**：以人类可读的格式查看 `attendance.db` 中的所有表和数据
>
> **适用版本**：人脸识别考勤系统 3.0（SQLite 版）

---

## 目录

```
1. 工具概述
2. 文件说明
3. 环境要求
4. 使用方法
5. 输出格式说明
6. 字段含义解释
7. 常见使用场景
8. 常见问题
9. 扩展用法
```

---

## 1. 工具概述

本工具是一个轻量级的 SQLite 数据库查看脚本，专为考勤系统的
`attendance.db` 数据库设计。

**核心功能：**
```
✔ 自动发现数据库中的所有表
✔ 逐表打印所有数据，格式清晰
✔ 自动将 last_time（Unix 时间戳）转换为可读时间格式
✔ 空表友好提示，不报错
✔ 无需安装任何第三方库
```

**不能做的事：**
```
✘ 不能修改数据库内容（只读工具）
✘ 不能实时刷新（运行一次查看一次）
✘ 不能按条件过滤数据（查看全量数据）
✘ 不能导出为 Excel（请使用系统内置导出功能）
```

---

## 2. 文件说明

### 脚本文件

将以下代码保存为 `view_db.py`，放在与 `attendance.db` **同一目录**下：

```python
import sqlite3
from datetime import datetime

DB_FILE = "attendance.db"

conn = sqlite3.connect(DB_FILE)
conn.row_factory = sqlite3.Row
cur = conn.cursor()

cur.execute("""
SELECT name
FROM sqlite_master
WHERE type='table'
ORDER BY name;
""")

tables = cur.fetchall()

for table in tables:
    table_name = table["name"]

    print("\n" + "=" * 80)
    print(f"表: {table_name}")
    print("=" * 80)

    cur.execute(f"SELECT * FROM {table_name}")
    rows = cur.fetchall()

    if not rows:
        print("空表")
        continue

    columns = rows[0].keys()
    print(" | ".join(columns))

    for row in rows:
        values = []

        for col in columns:
            value = row[col]

            # attendance_status 表中的 last_time 转换为可读时间
            if table_name == "attendance_status" and col == "last_time":
                try:
                    value = datetime.fromtimestamp(float(value)).strftime(
                        "%Y-%m-%d %H:%M:%S"
                    )
                except:
                    pass

            values.append(str(value))

        print(" | ".join(values))

conn.close()
```

### 目录结构要求

```
项目根目录/
├── attendance_system.py   主程序
├── attendance.db          数据库文件（运行主程序后自动生成）
├── view_db.py             本查看工具         ← 放在这里
├── known_faces/
└── attendance_reports/
```

---

## 3. 环境要求

| 项目 | 要求 |
|------|------|
| Python 版本 | 3.6 及以上 |
| 第三方库 | **无需安装任何第三方库** |
| 标准库依赖 | `sqlite3`（内置）、`datetime`（内置）|
| 数据库文件 | `attendance.db` 必须存在于同一目录 |

> ✅ **无需激活虚拟环境**，系统自带 Python 即可运行

---

## 4. 使用方法

### 4.1 基本运行

打开命令行（终端），切换到脚本所在目录，执行：

```bash
python view_db.py
```

### 4.2 完整操作步骤

**Windows：**
```bash
# 第一步：打开命令提示符（Win + R → 输入 cmd → 回车）

# 第二步：切换到项目目录（根据实际路径修改）
cd C:\Users\你的用户名\Desktop\face-attendance-system

# 第三步：运行脚本
python view_db.py
```

**直接双击运行（Windows）：**
```
在文件资源管理器中找到 view_db.py
双击运行（窗口会一闪而过）

建议改用命令行运行，方便查看输出内容
或在脚本末尾添加：
  input("\n按 Enter 键退出...")
防止窗口自动关闭
```

### 4.3 将输出保存到文件

```bash
# 输出重定向到文本文件
python view_db.py > db_snapshot.txt

# 同时在屏幕显示并保存（Windows）
python view_db.py | tee db_snapshot.txt

# 带时间戳的文件名（PowerShell）
python view_db.py > "db_snapshot_$(Get-Date -Format 'yyyyMMdd_HHmmss').txt"
```

---

## 5. 输出格式说明

### 5.1 输出示例

运行脚本后，输出格式如下：

```
================================================================================
表: attendance_log
================================================================================
id | name | type | time | date
1 | 张三 | 签到 | 2024-01-15 09:02:35 | 2024-01-15
2 | 李四 | 签到 | 2024-01-15 09:05:12 | 2024-01-15
3 | 张三 | 签退 | 2024-01-15 18:03:47 | 2024-01-15
4 | 王五 | 签到 | 2024-01-15 09:15:20 | 2024-01-15
5 | 李四 | 签退 | 2024-01-15 17:58:30 | 2024-01-15

================================================================================
表: attendance_status
================================================================================
name | status | last_time | date
张三 | 已签退 | 2024-01-15 18:03:47 | 2024-01-15
李四 | 已签退 | 2024-01-15 17:58:30 | 2024-01-15
王五 | 已签到 | 2024-01-15 09:15:20 | 2024-01-15
```

### 5.2 输出结构说明

```
每张表的输出由三部分组成：

① 分隔线 + 表名
   ================================================================================
   表: attendance_log
   ================================================================================

② 列名行（第一行，用 " | " 分隔）
   id | name | type | time | date

③ 数据行（每条记录一行，用 " | " 分隔）
   1 | 张三 | 签到 | 2024-01-15 09:02:35 | 2024-01-15

空表时显示：
   空表
```

### 5.3 时间戳转换说明

```
attendance_status 表中的 last_time 字段原始值是 Unix 时间戳（浮点数）：
  原始值：1705280627.432156
  转换后：2024-01-15 09:02:35

脚本自动完成此转换，无需手动计算。
其他表的时间字段（如 attendance_log.time）已是字符串格式，不需要转换。
```

---

## 6. 字段含义解释

### 6.1 attendance_log 表（打卡流水）

| 字段名 | 类型 | 含义 | 示例值 |
|------|------|------|------|
| id | INTEGER | 自增主键，每条记录唯一编号 | 1, 2, 3... |
| name | TEXT | 打卡人员姓名 | 张三 |
| type | TEXT | 打卡类型 | 签到 / 签退 |
| time | TEXT | 打卡完整时间 | 2024-01-15 09:02:35 |
| date | TEXT | 打卡日期（冗余字段，方便按日查询）| 2024-01-15 |

```
说明：
  ✔ 每次打卡产生一行记录，历史数据永久保留，不会被覆盖
  ✔ 同一天多次打卡（签到+签退+签到+签退）会产生多行
  ✔ id 全局递增，可判断打卡顺序
  ✔ date 字段与 time 字段的日期部分完全一致（冗余设计，提升查询效率）
```

### 6.2 attendance_status 表（当前状态）

| 字段名 | 类型 | 含义 | 示例值 |
|------|------|------|------|
| name | TEXT | 人员姓名（主键，每人只有一行）| 张三 |
| status | TEXT | 当前考勤状态 | 已签到 / 已签退 |
| last_time | REAL | 最后一次打卡的 Unix 时间戳 | 2024-01-15 09:02:35（转换后）|
| date | TEXT | 该状态对应的日期 | 2024-01-15 |

```
说明：
  ✔ 每人只有一行，记录最新状态
  ✔ 每次打卡后该行被 UPDATE（INSERT OR REPLACE 语义）
  ✔ 已签到：人员已打卡进入，尚未签退
  ✔ 已签退：人员已完成本次签退
  ✔ date 字段用于区分当天数据（跨日后新打卡会更新此字段）
  ✔ last_time 用于冷却期计算（距上次打卡是否超过5秒）
```

### 6.3 sqlite_master 表（系统表）

```
SQLite 内置系统表，存储数据库结构信息（表、索引、视图等）。
本脚本查询此表来自动发现所有用户表。
正常情况下不会在输出中显示此表（WHERE type='table' 会过滤系统表）。
```

---

## 7. 常见使用场景

### 场景1：确认打卡记录是否正常写入

```bash
python view_db.py
```

```
查看 attendance_log 表：
  ✔ 能看到刚才的签到/签退记录 → 写入正常
  ✘ 看不到最新记录            → 可能 DBWriter 队列未落盘，
                                  等待几秒后重试
```

---

### 场景2：排查某人未打卡或打卡异常

```
在 attendance_log 中查找该人的记录：
  只有签到，没有签退 → 该人忘记签退（"未签退"异常）
  只有签退，没有签到 → 数据异常（"无对应签到"异常）
  签退时间早于签到   → 时钟异常

在 attendance_status 中查看该人状态：
  status = 已签到，但已下班 → 确认未签退
  date 不是今天              → 今日尚未打卡
```

---

### 场景3：验证历史数据保留

```
修改脚本中的查询，查看历史日期数据：
  在 attendance_log 中看到多个不同 date 的记录 → 历史数据正常保留
  所有记录 date 都是今天                        → 可能数据库刚初始化
```

---

### 场景4：程序崩溃后检查数据完整性

```
程序意外退出后，运行本工具：

检查步骤：
  1. 查看 attendance_log 最后几条记录的 id 是否连续
  2. 对比 attendance_status 中的 last_time 与 attendance_log 最后一条的 time
  3. 若两者一致 → 数据完整，崩溃前已落盘
  4. 若不一致   → 可能丢失了崩溃前最后一次打卡记录
```

---

### 场景5：数据库文件不存在时

```
错误信息：
  sqlite3.OperationalError: unable to open database file

原因：
  attendance.db 不在脚本同一目录
  或考勤系统从未运行过（数据库尚未创建）

解决：
  确认 view_db.py 与 attendance.db 在同一目录
  或先运行考勤主程序生成数据库文件
```

---

## 8. 常见问题

### Q1：运行后只看到"空表"，没有数据

```
原因1：考勤系统今天还没有运行过，数据库存在但无打卡记录
原因2：数据库是刚创建的（建表后还未打卡）
原因3：DBWriter 队列中的数据还未落盘

解决：
  先运行考勤主程序，产生打卡记录后再运行本工具
```

---

### Q2：last_time 显示的是数字而不是时间

```
原因：时间戳转换失败（极少见，通常是数据异常导致）

手动转换方法（Python）：
  >>> from datetime import datetime
  >>> datetime.fromtimestamp(1705280627.432156)
  datetime.datetime(2024, 1, 15, 9, 2, 7)
```

---

### Q3：中文姓名显示乱码

```
原因：Windows 命令提示符默认编码为 GBK，Python 输出 UTF-8 时冲突

解决方案：

方案A：使用 PowerShell 替代 cmd（PowerShell 支持 UTF-8）

方案B：在脚本开头添加
  import sys
  import io
  sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')

方案C：运行前在 cmd 中执行
  chcp 65001
```

---

### Q4：表的顺序每次不一样

```
原因：SQLite 返回表名的顺序由内部存储决定
      脚本使用 ORDER BY name 按字母排序，顺序是固定的：
        attendance_log    （l 排在 s 前面）
        attendance_status
```

---

### Q5：想只查看某一张表

```
修改脚本，在循环内添加过滤条件：

for table in tables:
    table_name = table["name"]

    # 只查看 attendance_log 表
    if table_name != "attendance_log":
        continue

    # ... 其余代码不变
```

---

## 9. 扩展用法

### 9.1 查看指定日期的打卡记录

在脚本末尾添加如下代码，可按日期过滤：

```python
# 在 conn.close() 之前添加

target_date = "2026-06-07"   # 修改为要查询的日期

print(f"\n{'=' * 80}")
print(f"指定日期 [{target_date}] 的打卡流水")
print("=" * 80)

cur.execute(
    "SELECT * FROM attendance_log WHERE date=? ORDER BY time ASC",
    (target_date,)
)
rows = cur.fetchall()

if not rows:
    print(f"  {target_date} 无打卡记录")
else:
    columns = rows[0].keys()
    print(" | ".join(columns))
    for row in rows:
        print(" | ".join(str(row[col]) for col in columns))
```

---

### 9.2 统计某天各人打卡次数

```python
target_date = "2024-01-15"

cur.execute("""
    SELECT name,
           COUNT(*) AS total,
           SUM(CASE WHEN type='签到' THEN 1 ELSE 0 END) AS sign_in,
           SUM(CASE WHEN type='签退' THEN 1 ELSE 0 END) AS sign_out
    FROM attendance_log
    WHERE date=?
    GROUP BY name
    ORDER BY name
""", (target_date,))

rows = cur.fetchall()
print(f"\n{target_date} 打卡统计：")
print("姓名 | 总次数 | 签到次数 | 签退次数")
for row in rows:
    print(f"{row['name']} | {row['total']} | {row['sign_in']} | {row['sign_out']}")
```

---

### 9.3 查询有历史数据的所有日期

```python
cur.execute("""
    SELECT date, COUNT(DISTINCT name) AS person_count, COUNT(*) AS record_count
    FROM attendance_log
    GROUP BY date
    ORDER BY date DESC
""")

rows = cur.fetchall()
print("\n历史数据日期汇总：")
print("日期 | 打卡人数 | 打卡记录总条数")
for row in rows:
    print(f"{row['date']} | {row['person_count']} 人 | {row['record_count']} 条")
```

---

### 9.4 防止窗口自动关闭（双击运行时）

在脚本最末尾添加：

```python
input("\n查看完毕，按 Enter 键关闭窗口...")
```

---

### 9.5 定时自动刷新（实时监控模式）

```python
import time

while True:
    # 清屏
    os.system('cls' if os.name == 'nt' else 'clear')

    # 重新查询并打印（将原有查询代码封装成函数后调用）
    print_all_tables()

    print(f"\n最后刷新时间：{datetime.now().strftime('%H:%M:%S')}")
    print("按 Ctrl+C 退出监控模式")

    time.sleep(5)   # 每 5 秒刷新一次
```

---

## 快速参考卡

```
运行命令：        python view_db.py
保存输出：        python view_db.py > output.txt
需要的文件：      view_db.py 和 attendance.db 在同一目录
需要安装的库：    无（纯标准库）

输出的表：
  attendance_log     打卡流水（每次打卡一行，历史永久保留）
  attendance_status  当前状态（每人一行，记录最新状态）

last_time 字段：  Unix 时间戳，脚本自动转换为 YYYY-MM-DD HH:MM:SS 格式

中文乱码解决：    使用 PowerShell 运行，或 cmd 中先执行 chcp 65001
```