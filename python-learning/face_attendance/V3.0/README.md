# 人脸识别考勤系统版本演进对比
## 1.0 → 2.0 → 3.0 完整修改总结

---

## 目录

1. [版本概览](#1-版本概览)
2. [1.0 → 2.0 修改汇总](#2-10--20-修改汇总)
3. [2.0 → 3.0 修改汇总](#3-20--30-修改汇总)
4. [三版本横向对比](#4-三版本横向对比)
5. [各版本适用场景](#5-各版本适用场景)
6. [版本演进总结](#6-版本演进总结)

---

## 1. 版本概览

| 版本 | 核心定位 | 代码行数 | 类数量 |
|------|------|------|------|
| 1.0 | 功能原型，单类实现 | ~550 行 | 4 个辅助类 + 1 个主类 |
| 2.0 | 工程优化，职责拆分 | ~900 行 | 7 个类 |
| 3.0 | 存储升级，SQLite 持久化 | ~1100 行 | 9 个类 |

---

## 2. 1.0 → 2.0 修改汇总

### 2.1 新增：日志系统

**修改性质：全新引入**

```
1.0版：全部使用 print() 输出
2.0版：引入 logging 模块，新增 setup_logger() 函数

具体内容：
  (1) 新增 setup_logger() 函数
     - 控制台输出 INFO 及以上
     - 文件按天滚动，保留 7 天，输出 DEBUG 及以上
     - 格式：[时间戳] 级别 模块名 - 消息内容
     - 日志目录 logs/ 自动创建

  (2) 全局 logger 单例
     - 所有 print() 替换为对应级别的 logger 调用
     - DEBUG   → 文件写入完成、字体缓存淘汰
     - INFO    → 注册成功、打卡记录、导出完成
     - WARNING → 字体加载失败、文件读取失败
     - ERROR   → 文件写入失败、导出失败
     - CRITICAL→ InsightFace初始化失败、摄像头打开失败
```

---

### 2.2 新增：字体缓存 LRU 升级

**修改性质：重构升级**

```
1.0版：普通字典 _font_cache: dict = {}，无上限，无淘汰
2.0版：新增 _LRUFontCache 类，基于 OrderedDict 实现 LRU

具体内容：
  (1) 新增配置项 FONT_CACHE_MAX = 32
  (2) 新增 _LRUFontCache 类
     - OrderedDict 维护访问顺序
     - 命中时 move_to_end() 更新顺序
     - 超出上限时 popitem(last=False) 淘汰最久未用
     - threading.Lock 保证多线程安全
  (3) 字体加载三级降级策略
     粗体失败 → 常规体 → PIL 内置字体
     1.0版：只有两级且静默吞异常
     2.0版：每级失败记录日志
```

---

### 2.3 新增：完善错误处理

**修改性质：全面补充**

| 位置 | 1.0 处理方式 | 2.0 处理方式 |
|------|------|------|
| InsightFace 初始化 | 无处理，直接崩溃 | 捕获 Exception，记录 CRITICAL，重新抛出 |
| 人脸库加载 | 单张失败中断全部 | 每张独立 try/except，跳过失败文件 |
| 人脸检测 | 无处理 | 捕获 Exception，返回空列表，记录 ERROR |
| 考勤状态加载 | 只检查文件存在 | 捕获 OSError/JSONDecodeError/KeyError |
| 文件写入 | print 错误信息 | 捕获 OSError，记录 ERROR 级别日志 |
| Excel 保存 | 无处理 | 捕获 OSError，记录 ERROR，重新抛出 |
| 合并单元格 | except: pass 静默 | 记录 DEBUG 日志后跳过 |
| 摄像头打开 | 不检测，后续崩溃 | isOpened() 检测，失败记录 CRITICAL |
| 读帧失败 | break 退出循环 | continue 跳过，记录 WARNING |
| Ctrl+C | 无处理，资源泄漏 | 捕获 KeyboardInterrupt，finally 释放 |

---

### 2.4 新增：类职责拆分（架构重构）

**修改性质：架构级重构**

```
1.0版：所有逻辑集中在单个 AttendanceSystem 类（约600行）

2.0版：拆分为四个独立类

┌──────────────────┬──────────────────────────────────────────────┐
│ 类名             │ 职责                                          │
├──────────────────┼──────────────────────────────────────────────┤
│ FaceRecognizer   │ InsightFace模型加载、人脸库构建、身份识别      │
│ AttendanceManager│ 打卡状态读写、日志管理、工时统计（含锁保护）   │
│ UIRenderer       │ 所有OpenCV/PIL绘制逻辑，不持有业务数据        │
│ AttendanceSystem │ 组合以上三者，驱动主循环，处理键盘事件        │
└──────────────────┴──────────────────────────────────────────────┘

迁移对应关系：
  _build_face_db()  → FaceRecognizer._build_face_db()
  _recognize()      → FaceRecognizer._recognize()
  _do_attendance()  → AttendanceManager.do_attendance()
  _get_stats()      → AttendanceManager.get_stats()
  _get_work_info()  → AttendanceManager.get_work_info()
  _draw_faces()     → UIRenderer.draw_faces()
  _draw_notif...()  → UIRenderer.draw_notifications()
  _draw_panel()     → UIRenderer.draw_panel()

新增查询接口（1.0版没有）：
  AttendanceManager.get_status()         单人状态查询
  AttendanceManager.get_recent_records() 最近N条记录
  AttendanceManager.snapshot_log()       导出用日志快照
```

---

### 2.5 新增：并发安全（RLock 保护）

**修改性质：全新引入**

```
1.0版：无任何锁保护，主线程打卡与导出线程存在竞态

2.0版：AttendanceManager 内引入 threading.RLock

(1) 使用 RLock 而非 Lock 的原因
   do_attendance() 内部调用 get_work_info()
   get_work_info() 也需要获取锁
   RLock（可重入锁）允许同一线程多次获取，避免死锁

(2) 关键设计
   do_attendance()：
     "读状态→判断冷却→切换状态→写日志" 四步原子完成
     消除 TOCTOU 竞态条件

   _save_state() / _save_log()：
     锁内制作数据快照，锁外交给 AsyncWriter
     最小化锁持有时间

   snapshot_log()：
     导出时获取完整日志深拷贝
     导出过程（可能耗时数秒）在锁外执行
```

---

### 2.6 新增：面板分层刷新

**修改性质：性能优化**

```
1.0版：单一缓存，0.5s TTL，时钟更新触发完整重绘，有闪烁

2.0版：双层缓存，分离刷新频率

新增配置项：
  PANEL_STATUS_TTL = 0.5   打卡统计层刷新间隔
  PANEL_CLOCK_TTL  = 0.2   时钟层刷新间隔

层1（status 缓存）：
  包含：标题、日期、统计、最近打卡、在场人员
  触发重建：缓存超期 或 窗口尺寸变化 或 打卡事件
  重建代价：完整 PIL 文字渲染（较高）

层2（时钟/FPS 覆盖）：
  每帧无条件擦除并重绘时钟/FPS 区域
  开销极低，彻底消除秒针跳变时的闪烁

姓名截断优化：
  1.0版：按字符数截断 rn[:7] + ".."
  2.0版：按像素宽度截断，二分查找，支持中英文混排

时间戳对齐：
  1.0版：固定偏移 (pw-86, y)
  2.0版：预计算像素宽度，动态右对齐
```

---

### 2.7 新增：主循环细节

**修改性质：稳定性补充**

```
(1) face_db 为空保护
   1.0版：_recognize() 对空字典也遍历
   2.0版：直接返回 None

(2) 资源释放保证
   1.0版：仅正常退出路径释放
   2.0版：finally 块保证任意退出路径释放

(3) 打卡后缓存失效
   1.0版：在 _do_attendance() 内部设置
   2.0版：在主循环中设置（职责更清晰）

(4) 导出数据源
   1.0版：直接读 self.attendance_log（无锁）
   2.0版：先调用 snapshot_log() 获取快照
```

---

## 3. 2.0 → 3.0 修改汇总

### 3.1 存储层：JSON → SQLite（核心变化）

**修改性质：架构级替换**

```
2.0版存储方案：
  attendance_state.json  当日考勤状态
  attendance_log.json    当日打卡流水
  每日覆盖写入，历史数据不保留

3.0版存储方案：
  attendance.db（SQLite）
  ├── attendance_status 表  当前考勤状态
  └── attendance_log 表     打卡流水（永久保留）

影响：
  (1) 配置项变化
     去掉：STATE_FILE = "attendance_state.json"
     去掉：LOG_FILE   = "attendance_log.json"
     新增：DB_FILE    = "attendance.db"

  (2) 历史数据
     2.0版：只保留当天，跨日自动重置
     3.0版：按 date 字段区分每天，历史数据永久保留
             可按日期查询任意历史考勤记录

  (3) 数据查询
     2.0版：从内存字典读取，无法查历史
     3.0版：SQL 查询，支持任意日期范围
```

---

### 3.2 新增：DBWriter 类

**修改性质：替换 AsyncWriter**

```
2.0版：AsyncWriter
  - 异步写入 JSON 文件
  - 写操作：json.dump() 到文件
  - 无读写分离概念

3.0版：DBWriter（全新设计）
  - 异步写入 SQLite 数据库
  - 解决多线程 SQLite 竞争问题

核心设计：
  写线程：持有唯一写连接，串行执行所有 INSERT/UPDATE/DELETE
  读连接：主线程独立持有（WAL 模式下读写并发无冲突）

新增方法（AsyncWriter 没有）：
  executemany(sql, params_list)  批量写操作
  flush(timeout)                 阻塞等待队列清空（退出前调用）
  stop()                         通知写线程优雅退出

特殊队列消息（内部协议）：
  ("__many__", sql, params_list) → 批量执行
  ("__flush__", event)           → 同步等待落盘
  _STOP 哨兵对象                 → 写线程退出

WAL 模式配置：
  PRAGMA journal_mode=WAL       写不阻塞读
  PRAGMA synchronous=NORMAL     WAL模式下平衡性能与安全
```

---

### 3.3 新增：AttendanceDB 类

**修改性质：全新引入**

```
2.0版：无独立数据库访问层，读写混在 AttendanceManager 中

3.0版：新增 AttendanceDB 类，专门负责建表和只读查询

职责划分：
  AttendanceDB  → 建表（DDL）+ 只读查询（SELECT）+ 写操作投递
  DBWriter      → 执行所有写操作（INSERT/UPDATE）

表结构：
  attendance_status
    name      TEXT PRIMARY KEY   人员姓名
    status    TEXT               已签到 / 已签退
    last_time REAL               Unix 时间戳
    date      TEXT               YYYY-MM-DD

  attendance_log
    id        INTEGER PK AUTOINCREMENT
    name      TEXT NOT NULL
    type      TEXT NOT NULL      签到 / 签退
    time      TEXT NOT NULL      YYYY-MM-DD HH:MM:SS
    date      TEXT NOT NULL      冗余存储，方便按日查询

索引：
  idx_log_name_date  (name, date) → 单人单日查询，O(log n)
  idx_log_date       (date)       → 当日所有记录查询，O(log n)

新增查询方法：
  get_status(name, today)          单人今日状态
  get_all_status_today(today)      所有人今日状态列表
  get_log_today(name, today)       单人今日流水
  get_log_by_date(date)            指定日期所有人流水（导出用）
  get_recent_records(today, n)     最近N条流水（SQL层排序）

写操作投递方法：
  upsert_status(name, status, last_time, today)  INSERT OR REPLACE
  insert_log(name, action, time_str, today)      INSERT
```

---

### 3.4 修改：AttendanceManager 内部重写

**修改性质：内部重构，对外接口不变**

```
对外接口完全一致（方法签名未改变）：
  do_attendance()     ✔ 签名不变
  get_stats()         ✔ 签名不变
  get_work_info()     ✔ 签名不变
  get_status()        ✔ 签名不变
  get_recent_records()✔ 签名不变

内部变化：

(1) 构造函数参数变化
   2.0版：def __init__(self, writer: AsyncWriter)
   3.0版：def __init__(self, db: AttendanceDB)

(2) 数据加载：JSON → 数据库查询
   2.0版：_load_state() / _load_log() 读取 JSON 文件
   3.0版：_load_today() 从数据库恢复今日数据到内存缓存

(3) 数据保存：JSON 文件 → 异步写库
   2.0版：_save_state() / _save_log() 通过 AsyncWriter 写 JSON
   3.0版：do_attendance() 内直接调用
           db.upsert_status() / db.insert_log()

(4) 内存缓存策略（新引入）
   2.0版：attendance_status/attendance_log 直接作为权威数据源
   3.0版：引入 _status_cache / _log_cache 两个内存缓存
           写操作：先更新缓存，再异步写库
           读操作：直接读缓存，零数据库访问
           重启：从数据库恢复今日数据到缓存

(5) get_recent_records() 实现变化
   2.0版：从内存字典汇总排序，锁内完成
   3.0版：从内存缓存汇总排序（与2.0相同逻辑，但数据源是内存缓存而非原字典）

(6) snapshot_log() → get_log_for_export()
   2.0版：snapshot_log() 从内存制作深拷贝快照
   3.0版：get_log_for_export(date) 直接从数据库读取
           - 无需加锁（数据库层面线程安全）
           - 支持指定日期参数（可导出历史数据）
           - 数据永远是最新的（不存在缓存与数据库不一致）
```

---

### 3.5 修改：AttendanceSystem 初始化顺序

**修改性质：依赖链扩展**

```
2.0版初始化顺序：
  AsyncWriter → AttendanceManager(writer)
  FaceRecognizer
  UIRenderer

3.0版初始化顺序：
  DBWriter → AttendanceDB(DB_FILE, writer) → AttendanceManager(db)
  FaceRecognizer
  UIRenderer

新增退出流程：
  2.0版：cap.release() → destroyAllWindows() → _do_export()
  3.0版：cap.release() → destroyAllWindows() → _do_export()
          → db_writer.flush()   ← 新增：等待写入队列清空
          → db_writer.stop()    ← 新增：通知写线程优雅退出
  保证程序退出时所有打卡数据已安全落库
```

---

### 3.6 修改：导出数据源

**修改性质：可靠性提升**

```
2.0版：
  log_snapshot = self.manager.snapshot_log()   内存快照
  存在风险：内存缓存与JSON文件之间存在短暂不一致窗口

3.0版：
  log_data = self.manager.get_log_for_export() 直接读数据库
  优势：
    ① 数据唯一来源，无一致性问题
    ② 支持指定日期参数，可导出历史
    ③ 无需加锁（数据库读写分离）
```

---

### 3.7 修改：FaceRecognizer（小幅简化）

**修改性质：代码优化**

```
2.0版：
  results = []
  for face in faces:
      x1, y1, x2, y2 = face.bbox.astype(int)
      name = self._recognize(face.normed_embedding)
      results.append((x1, y1, x2, y2, name))
  return results

3.0版：
  return [(*(face.bbox.astype(int)),
           self._recognize(face.normed_embedding))
          for face in faces]

使用解包星号语法（*）简化列表构建，逻辑完全一致
```

---

### 3.8 删除的内容

**修改性质：移除**

```
3.0版相比2.0版删除的内容：

(1) 删除配置项
   STATE_FILE = "attendance_state.json"
   LOG_FILE   = "attendance_log.json"

(2) 删除 AsyncWriter 类（被 DBWriter 替代）

(3) 删除 AttendanceManager 中的方法
   _load_state()    → 被 _load_today() 替代
   _save_state()    → 被 db.upsert_status() 直接调用替代
   _load_log()      → 被 _load_today() 替代
   _save_log()      → 被 db.insert_log() 直接调用替代
   snapshot_log()   → 被 get_log_for_export() 替代

(4) 删除 PANEL_CLOCK_TTL 配置项
   2.0版定义了但实际未使用条件判断（每帧无条件覆盖）
   3.0版直接去掉，更诚实
```

---

## 4. 三版本横向对比

### 4.1 架构对比

| 模块 | 1.0 | 2.0 | 3.0 |
|------|------|------|------|
| 人脸识别 | AttendanceSystem 内部方法 | FaceRecognizer 类 | FaceRecognizer 类（小幅简化）|
| 打卡管理 | AttendanceSystem 内部方法 | AttendanceManager 类 | AttendanceManager 类（内部重写）|
| UI 渲染 | AttendanceSystem 内部方法 | UIRenderer 类 | UIRenderer 类（完全不变）|
| 主循环 | AttendanceSystem | AttendanceSystem | AttendanceSystem（扩展退出流程）|
| 异步写入 | AsyncWriter（JSON）| AsyncWriter（JSON）| DBWriter（SQLite）|
| 数据库层 | 无 | 无 | AttendanceDB（新增）|

---

### 4.2 存储对比

| 特性 | 1.0 | 2.0 | 3.0 |
|------|------|------|------|
| 存储格式 | JSON 文件 | JSON 文件 | SQLite 数据库 |
| 文件数量 | 2 个 JSON | 2 个 JSON | 1 个 .db 文件 |
| 历史数据 | ❌ 每日覆盖 | ❌ 每日覆盖 | ✅ 永久保留 |
| 历史查询 | ❌ 不支持 | ❌ 不支持 | ✅ 按日期查询 |
| 并发安全 | ❌ 无保护 | ⚠️ 内存锁保护 | ✅ 读写分离 + WAL |
| 崩溃恢复 | ⚠️ 依赖文件完整性 | ⚠️ 依赖文件完整性 | ✅ SQLite 事务保证 |
| 写入性能 | 每次全量重写 | 每次全量重写 | 增量 INSERT，性能更好 |

---

### 4.3 并发安全对比

| 场景 | 1.0 | 2.0 | 3.0 |
|------|------|------|------|
| 主线程打卡 vs 导出线程读取 | ❌ 竞态 | ✅ RLock 保护 | ✅ 内存锁 + 数据库事务 |
| 多人同帧并发打卡 | ⚠️ 无显式保护 | ✅ RLock 原子操作 | ✅ RLock 原子操作 |
| SQLite 多线程写入 | 无 SQLite | 无 SQLite | ✅ 单写线程串行 |
| 导出时数据一致性 | ❌ 无保证 | ⚠️ 内存快照 | ✅ 数据库读（唯一数据源）|

---

### 4.4 错误处理对比

| 位置 | 1.0 | 2.0 | 3.0 |
|------|------|------|------|
| 模型初始化失败 | 崩溃无提示 | CRITICAL + 重抛 | CRITICAL + 重抛（同2.0）|
| 照片加载失败 | 中断全部 | 跳过单张，记录 WARNING | 跳过单张，记录 WARNING（同2.0）|
| 人脸检测失败 | 崩溃 | 返回空列表，记录 ERROR | 返回空列表，记录 ERROR（同2.0）|
| 数据加载失败 | 部分处理 | 捕获三类异常，降级 | 数据库事务保证，不会损坏 |
| 写入失败 | print | 记录 ERROR | 记录 ERROR（同2.0）|
| 摄像头失败 | 崩溃 | 检测+优雅退出 | 检测+优雅退出（同2.0）|
| 程序退出 | 可能泄漏资源 | finally 保证释放 | finally + flush + stop |

---

### 4.5 性能对比

| 指标 | 1.0 | 2.0 | 3.0 |
|------|------|------|------|
| 字体缓存 | 无上限字典 | LRU 32条上限 | LRU 32条上限（同2.0）|
| 面板渲染 | 单层缓存 | 双层缓存，分离刷新 | 双层缓存（同2.0）|
| 数据写入 | 全量 JSON 重写 | 全量 JSON 重写 | 增量 SQL INSERT，更高效 |
| 数据读取 | 内存字典，O(1) | 内存字典，O(1) | 内存缓存，O(1)（无变化）|
| 导出性能 | 内存读取 | 内存快照读取 | 数据库查询（略慢但更准确）|

---

### 4.6 代码量对比

| 版本 | 总行数 | 相比上版增加 | 主要增量来源 |
|------|------|------|------|
| 1.0 | ~550 行 | 基准 | — |
| 2.0 | ~900 行 | +350 行（+64%）| 日志/错误处理/类拆分/锁/分层缓存 |
| 3.0 | ~1100 行 | +200 行（+22%）| DBWriter/AttendanceDB/SQL查询方法 |

---

### 4.7 3.0 版本缺点与已知问题

#### (1) 内存缓存与数据库的双写一致性风险

```
问题描述：
  do_attendance() 中先更新内存缓存（锁内），
  再通过 DBWriter 异步写库（锁外）。

  极端场景：
    内存缓存已更新 → 程序在写库之前崩溃
    重启后从数据库恢复 → 内存缓存的那次打卡记录丢失

风险等级：🟡 低概率，但确实存在

根本原因：
  引入了两个数据源（内存缓存 + 数据库），
  任何双写架构都面临这个问题
```

---

#### (2) DBWriter 队列无界，极端情况可能内存溢出

```
问题描述：
  DBWriter 使用 queue.Queue()（无界队列）
  若磁盘 I/O 持续阻塞，写入速度跟不上打卡速度
  队列会无限增长，最终耗尽内存

实际风险：
  正常使用场景（每天几十次打卡）完全不会触发
  仅在极端压测或磁盘故障时才会出现

对比 AsyncWriter：
  2.0版 AsyncWriter 同样是无界队列，存在相同问题
  3.0版未修复此问题，直接继承
```

---

#### (3) 读连接无连接池，长期运行可能有锁等待

```
问题描述：
  AttendanceDB 持有单一读连接（self._rconn）
  所有读操作（get_stats/get_work_info/get_recent_records）
  共用同一个连接对象

  WAL 模式下读写并发无冲突，但：
    多个读操作之间若有并发（如导出线程 + 主线程同时读）
    单一连接串行处理，存在等待
    
当前实际影响：
  由于读操作都在主线程执行，实际上是串行的
  并不存在真正的并发读问题
  但这是一个隐患，若未来多线程化读操作会暴露
```

---

#### (4) _load_today() 恢复数据存在跨日边界问题

```
问题描述：
  _today 在 __init__ 时固定为当天日期字符串
  如果程序跨午夜 00:00 不间断运行：
    新一天的打卡记录写入数据库时 date 字段仍是昨天
    内存缓存也不会自动重置
    导致新一天的数据归到昨天的日期下

风险等级：🔴 实际场景中会发生（24小时运行场景）

2.0版同样存在此问题（_today 同样在初始化时固定）
3.0版未修复
```

---

#### (5) get_log_for_export() 查询全量数据，无分页

```
问题描述：
  get_log_by_date() 查询某天所有人的全部打卡记录
  一次性加载到内存并返回

  当注册人数很多（如 500 人）且每人多次打卡时
  单次查询数据量可能较大，导出时内存峰值升高

实际影响：
  当前场景（小型团队，几十人）完全不受影响
  仅在大规模部署时才需要考虑分页导出
```

---

#### (6) 数据库文件无备份机制

```
问题描述：
  attendance.db 是唯一数据存储文件
  若文件损坏（磁盘故障/强制断电）可能导致数据丢失

  SQLite WAL 模式已提供一定的崩溃保护
  但没有定期备份机制（如每日复制一份）

对比2.0版：
  JSON 文件同样无备份，且崩溃保护更弱
  3.0版在数据安全性上优于2.0，但仍不够完善
```

---

#### (7) UIRenderer 仍与 AttendanceManager 存在耦合

```
问题描述：
  draw_faces() 方法接收 manager 参数并直接调用：
    manager.get_work_info(name)
    manager.get_status(name)

  UIRenderer 本应是"纯渲染层"，不依赖业务对象
  这个设计缺陷从 2.0版继承，3.0版未修复

  同时 AttendanceSystem 主循环中：
    self.renderer._panel_status_ts = 0.0
  直接访问了 UIRenderer 的私有属性，违反封装原则

  这两个问题在2.0版就已存在，3.0版原样保留
```

---

#### (8) SQLite 不适合高并发多进程场景

```
问题描述：
  SQLite 是进程内数据库，设计上不支持多进程并发写入
  若未来需要：
    多摄像头 → 多进程分别写入同一数据库
    Web 后台 → 同时读写数据库

  SQLite 的并发能力会成为瓶颈
  需要升级到 MySQL / PostgreSQL

当前影响：
  单进程单摄像头场景完全没问题
  这是 SQLite 的固有限制，不是实现问题
```

---

#### 3.0 版缺点总结

| 缺点 | 严重程度 | 当前实际影响 | 修复方向 |
|------|------|------|------|
| 双写一致性风险 | 🟡 低 | 极小概率丢失1条记录 | 同步写库或 WAL 保护 |
| 队列无界 | 🟡 低 | 正常使用不触发 | 有界队列 + 背压 |
| 单一读连接 | 🟢 极低 | 当前串行读，无影响 | 连接池 |
| 跨日边界问题 | 🔴 中 | 24小时运行时发生 | 每帧检查日期变化 |
| 导出无分页 | 🟢 极低 | 小团队不受影响 | 分页查询 |
| 无数据库备份 | 🟡 低 | 依赖SQLite崩溃保护 | 定期文件备份 |
| UIRenderer 耦合 | 🟡 低 | 功能正常，设计不优雅 | 数据传值而非传对象 |
| SQLite 并发限制 | 🟢 极低 | 单进程场景无影响 | 升级 MySQL/PostgreSQL |

## 5. 各版本适用场景

| 场景 | 推荐版本 | 原因 |
|------|------|------|
| 学习/理解代码 | **1.0** | 逻辑集中，线性阅读 |
| 个人临时使用 | **1.0** | 简单够用 |
| 课程作业/展示 | **2.0** | 体现工程规范 |
| 团队小项目 | **2.0** | 可维护性好 |
| 需要历史数据查询 | **3.0** | SQLite 永久保留 |
| 长期稳定运行 | **3.0** | 事务保证，崩溃不丢数据 |
| 生产环境 | 三者都不够 | 还需 Web界面/用户管理/备份策略 |

---

## 6. 版本演进总结

```
1.0  →  能用的代码
         功能完整，但不健壮，不规范

2.0  →  工程化的代码
         健壮、规范、可维护，但数据不持久

3.0  →  可部署的代码
         数据永久保留，并发安全，崩溃不丢数据
         距离真正的生产系统又近了一步
```

> 三个版本代表了软件开发的三个成长阶段：
>
> **能跑** → **能用** → **可靠**
