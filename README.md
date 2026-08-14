# 机器狗视觉循迹导航系统(原基础改良版，非原创)

基于 **Qt5 + OpenCV5 + LCM** 实现的四足机器狗视觉导航与赛道识别系统，支持多模式运动控制、彩色条带识别、分岔路/住户区检测、限高通行、上楼梯等复杂场景。

---

## ✨ 功能特性

| 功能模块 | 说明 |
|---------|------|
| **视觉循迹** | 基于摄像头实时提取赛道边缘，计算中线偏差，通过横移 + 转向复合控制实现稳定循迹 |
| **彩色条带识别** | 支持蓝、绿、红、棕、紫、白、黄、橙 8 种颜色阈值二值化识别，用于触发不同任务模式 |
| **住户区任务** | 棕色 / 紫色条带触发住户区左右进入流程，含站起、转身、恢复循迹等状态机 |
| **分岔路识别** | 绿色 / 红色条带触发分岔路左右转向，支持目标中线偏移与转向角速度复合控制 |
| **限高区通行** | 蓝色条带触发限高模式，通过动态调整机身高度 (`body_height`) 完成低矮区域通过 |
| **上楼梯模式** | 步态 API 切换（1002 → 1101 → 1021）完成楼梯爬升动作 |
| **集散区矫正** | 基于中线斜率 `k` 与偏移量 `average` 的双闭环位姿矫正 |
| **二维码识别** | 集成微信开源 WeChat QRCode 识别库（可配置启用） |
| **上位机通信** | UDP 通道实现实时图像回传、颜色阈值在线标定与保存 |
| **摄像头热插拔** | 自动枚举 /dev/video0~3，支持断线重连，程序不崩溃 |

---

## 📁 目录结构

```
velocity=0.2/              # 循迹速度 0.2 m/s 版本
velocity=0.3/              # 循迹速度 0.3 m/s 版本（推荐）
└── track1.15/
    ├── track/             # 源代码目录
    │   ├── main.cpp              # 主程序入口 + 图像处理核心逻辑
    │   ├── colorgroup.h/cpp      # 颜色阈值管理 + UDP图像发送线程
    │   ├── mythread.h/cpp        # 定时/状态机线程，定义所有 Mode 枚举
    │   ├── lcmutil.h/cpp         # LCM 通信封装（发送 gait API & 运动指令）
    │   ├── udputil.h/cpp         # UDP 接收线程（上位机指令）
    │   ├── qrcode_recognize_thread.h/cpp  # 二维码识别线程
    │   ├── sport_client_cmd_t.hpp         # LCM 消息结构体定义
    │   ├── sport_client_cmd_t.lcm         # LCM schema 文件
    │   ├── form.ui               # Qt UI 表单
    │   └── track.pro             # qmake 构建脚本
    ├── build/             # 构建输出 & linuxdeployqt 打包目录
    │   ├── track                  # 编译生成的可执行文件
    │   ├── lib/                   # 依赖的 .so 库（Qt5 / OpenCV / LCM / FFmpeg 等）
    │   ├── colorGroup.txt         # 颜色阈值持久化文件
    │   └── linuxdeployqt          # AppImage 打包工具
    ├── 更新日志.txt       # 版本更新历史
    └── send.sh            # 一键打包 + SCP 部署脚本
板子.skp                   # 赛道/结构设计 SketchUp 模型
```

---

## 🚀 快速开始

### 环境依赖

| 依赖 | 版本要求 | 备注 |
|-----|---------|------|
| OS | Ubuntu 16.04 / 18.04 | 程序默认链接 `libavcodec-ffmpeg56` 等 Xenial 版本库 |
| Qt | 5.x | `core gui network widgets` 模块 |
| OpenCV | 5.x | 需要编译 `wechat_qrcode` 模块（`opencv_contrib`） |
| LCM | ≥ 1.4 | 轻量级通信中间件，安装到 `/usr/local/{lib,include}` |
| 编译器 | GCC ≥ 4.8+ | C++11 标准 |
| CMake | ≥ 3.5 | (用于 OpenCV 编译) |

### 安装依赖（参考）

```bash
# Qt5
sudo apt install qt5-default qtbase5-dev

# LCM
git clone https://github.com/lcm-proj/lcm.git && cd lcm
mkdir build && cd build && cmake .. && make -j$(nproc) && sudo make install

# OpenCV5 + wechat_qrcode (需自行编译)
# 确保编译选项包含 opencv_contrib/modules/wechat_qrcode
```

### 编译构建

```bash
cd velocity=0.3/track1.15/track
qmake track.pro
make -j$(nproc)
```

编译完成后，可执行文件位于 `track/` 目录下。如需打包部署：

```bash
cd ../..
# 使用 linuxdeployqt 打包 AppImage 并复制到目标机器
bash send.sh
```

---

## 🎮 使用方法

### 命令行参数

```bash
# 语法
./track [模式/住户色 [分岔色/showImage]]
```

| 参数 | 示例 | 说明 |
|-----|------|------|
| (无参数) | `./track` | 默认 `track` 模式，住户色 = `brown` |
| `track` | `./track track` | 纯循迹模式 |
| `brown` | `./track brown` | 循迹 + 棕色住户区触发 |
| `violet` | `./track violet` | 循迹 + 紫色住户区触发 |
| `stop` | `./track stop` | 立即停止（API 1006） |
| 第2参数 `red` | `./track brown red` | 循迹 + 棕色住户 + 红色分岔路（右岔） |
| 第2参数 `green` | `./track violet green` | 循迹 + 紫色住户 + 绿色分岔路（左岔） |
| 第2参数 `showImage` | `./track track showImage` | 启用 UDP 图像回传 + 上位机交互 |

### 运行模式说明

| 模式 (Mode) | 触发条件 | 行为 |
|------------|---------|------|
| `import` | 集散区矫正完成后 | 导入初始姿态 |
| `revise` | 手动/初始进入 | 基于中线斜率 + 偏移双闭环矫正位姿 |
| `track` | 默认模式 | 标准循迹：前进 0.3m/s + 横移 + 转向 |
| `limitHeight` | 识别到蓝色条带 | 机身高度降至 `-0.15` 后恢复通过 |
| `upstair` | (预留) 手动切换 | 步态 1002 → 1101 → 1021 上楼梯 |
| `residenceleft` | 棕色条带 | 左住户区：前进 → 停 → 左转 → 回正 → 恢复 |
| `residenceright` | 紫色条带 | 右住户区：横移 → 前进 → 停 → 右转 → 回正 |
| `divergeleft` | 绿色条带 | 左岔路：目标中线 180 + 左转 0.08 rad/s |
| `divergeright` | 红色条带 | 右岔路：目标中线 180 + 右转 0.20 rad/s |
| `stop` | `stop` 参数进入 | API 1006，速度清零 |

### 控制律（循迹模式）

```cpp
velocity[0] = TRACK_VELOCITY;               // 前进速度 (0.2 / 0.3 m/s)
velocity[1] = 0.0005 * (average - goalAverage);   // 横向平移
velocity[2] = 0.004  * (goalAverage - average);   // 转向角速度 (Z轴)
```
其中 `average` 为赛道中线像素均值，`goalAverage` 目标中心 = 200。

---

## 🔧 颜色阈值标定

1. 运行时加 `showImage` 参数：
   ```bash
   ./track track showImage
   ```
2. 上位机通过 UDP 发送以下指令（见 `UdpUtil::ifReceiveInfoFlag`）：

   | Flag | 功能 |
   |------|------|
   | 1 | 选择颜色 → 返回当前阈值 |
   | 2 | 设置颜色阈值 (`setColorThreadhold`) |
   | 3 | 保存阈值到 `colorGroup.txt` |

3. 程序启动时 `colorGroup` 构造函数自动读取 `colorGroup.txt` 恢复标定参数。

内置默认阈值（HSV/BGR 区间）见 [colorgroup.h](velocity=0.3/track1.15/track/colorgroup.h#L22-L45)：
- 白色赛道：`Scalar(0,155,1)` ~ `Scalar(200,200,200)`
- 蓝色限高：`Scalar(158,109,0)` ~ `Scalar(255,209,84)`
- 红色分岔（右）：`Scalar(62,25,138)` ~ `Scalar(127,97,184)`
- 绿色分岔（左）：上限 `Scalar(98,116,114)`
- 棕色住户（左）：`Scalar(62,57,105)` ~ `Scalar(100,96,131)`
- 紫色住户（右）：`Scalar(116,70,96)` ~ `Scalar(151,102,135)`

---

## 📡 通信协议

### LCM 下行（机器狗控制）
通道与消息类型：`sport_client_cmd_t`

| API | 含义 |
|-----|------|
| 1002 | 站起 |
| 1006 | 停止 / 趴下 |
| 1021 | 正常行走步态 |
| 1101 | 上楼梯步态 |

发送字段：`velocity[3]`、`euler_angles[3]`、`body_height`、`step_height`。

### UDP 上行（图像回传）
- `colorGroup::showPicture()` 将当前帧 JPEG 编码后通过 QUdpSocket 发出
- 端口号 / 目标 IP：在 `colorgroup.cpp` 中配置

---

## 📝 更新日志

| 日期 | 内容 |
|-----|------|
| 2021/08/01 | 初版发布，可初步测试 |
| 2021/08/25 | colorGroup.txt 阈值文件持久化，启动自动读取 |
| 2021/09/09 | 摄像头自动枚举（0~3 号），支持热插拔断线重连 |
| 2021/10/08 | 二维码识别库替换为微信 `wechat_qrcode` |
| 2022/03/30 | 注释二维码模块，增加上位机视频回传链路 |
| 当前 | 拆分为 velocity=0.2 / 0.3 双版本，track1.15 算法迭代 |

---

## 🛠️ 关键算法说明

### 1. 赛道边缘提取
对二值化图像逐行扫描，`judgmentLeft/Right` 基于黑白跳变邻域模式（3 像素 / 5 像素模板）定位左右边缘点，过滤噪声。

### 2. 条带长度检测（长短条带区分）
`recognizeBlackStripe` 采用**递归扩散算法**（`RecursionLeft/Right`）沿连通域搜索最左/最右边界，长度 ≥ 398 且上沿白像素 > 80 判定为长条带。

### 3. 中线拟合
取 `rows/2` 范围的有效点（左右边缘连续性校验 |Δ| < 4），计算每列中点并存入 `value[]`，均值化得到 `average`，超范围 [0,400] 时重置为 200。

### 4. 集散区斜率矫正
将中线点**前后对半配对**计算斜率并取平均，偶数个点取 `(value[i]-value[i+n/2])/(position[i]-position[i+n/2])`，奇数个点跳过中间点；k 限幅 ±0.5。

---

## 📌 注意事项

1. **OpenCV 路径**：`track.pro` 默认链接 `/usr/local/lib/libopencv_*.so`，并 include `/usr/local/include/opencv5/`，如自定义安装路径请修改对应行。
2. **HSV/BGR 顺序**：`inRange` 阈值基于当前颜色空间，修改摄像头 `VideoCapture` 格式时需同步更新阈值。
3. **图像分辨率**：主循环输入 640×480 → `resize2` 裁剪 + 缩放到 200×150 处理；边缘检测代码中 `cols=400/200` 的硬编码需与预处理尺寸保持一致。
4. **部署目标机**：`send.sh` 中 SCP 目标地址为 `user@10.0.0.34:/home/user/`，请修改为实际机器狗 IP。

---

## 📄 License

本项目仅供学习与竞赛参考使用，记录自己的学习经历。
