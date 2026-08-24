# VehicleTerminal TTS 模块

本模块为 VehicleTerminal 提供离线中文语音合成功能。它在 RK3588 上使用 RKNN Runtime 运行 Matcha-TTS 与 Vocos 模型，将 AI 助手生成的文字转换为 16 kHz 单声道 PCM，并通过 ALSA 连续播放。

## 模型下载

模型文件不建议直接提交到 Git 仓库，请从以下地址下载：

- 网盘地址：(https://pan.baidu.com/s/1vBkBEKNJrTvQb8xEQyxAlA)
- 提取码：c7q2

下载后，将完整的 `saved_v1` 文件夹放到：

```text
/home/firefly/VehicleTerminal/model/tts/saved_v1/
```

> 不要额外嵌套一层目录。例如不能放成 `saved_v1/saved_v1/*.rknn`。

## 模型目录结构

模型目录必须保持以下结构和文件名：

```text
model/tts/saved_v1/
├── deploy_models/
│   └── vocos-22khz-univ-fp16.rknn
├── lexicon.txt
├── matcha-encoder-768-no-ceil-fp16.rknn
├── matcha-estimator-128-fp16.rknn
├── matcha-estimator-256-fp16.rknn
├── matcha-estimator-384-fp16.rknn
├── matcha-estimator-512-fp16.rknn
├── matcha-estimator-768-fp16.rknn
├── matcha-noise-seed42-768.npy
├── time_emb_step0.npy
├── time_emb_step1.npy
├── time_emb_step2.npy
└── tokens.txt
```

各文件用途：

| 文件 | 用途 |
| --- | --- |
| `matcha-encoder-768-no-ceil-fp16.rknn` | 将 Token 序列编码为最多 768 帧的声学特征 |
| `matcha-estimator-*-fp16.rknn` | 128、256、384、512、768 帧 Estimator 桶，运行时按有效帧数选择 |
| `vocos-22khz-univ-fp16.rknn` | 将 Mel 频谱转换为 22.05 kHz 音频特征，固定 600 帧输入 |
| `matcha-noise-seed42-768.npy` | 固定随机噪声，保证合成结果稳定 |
| `time_emb_step*.npy` | 三步 ODE 推理所需的时间嵌入 |
| `lexicon.txt` | 发音词典 |
| `tokens.txt` | Token 表 |

当前模型目录共 13 个文件，大小约为 150 MiB。

## 部署检查

在开发板执行：

```bash
cd /home/firefly/VehicleTerminal
find model/tts/saved_v1 -type f -printf '%P  %s bytes\n' | sort
```

也可以用下面的命令检查所有必需文件：

```bash
cd /home/firefly/VehicleTerminal

for file in \
  model/tts/saved_v1/lexicon.txt \
  model/tts/saved_v1/tokens.txt \
  model/tts/saved_v1/matcha-noise-seed42-768.npy \
  model/tts/saved_v1/time_emb_step0.npy \
  model/tts/saved_v1/time_emb_step1.npy \
  model/tts/saved_v1/time_emb_step2.npy \
  model/tts/saved_v1/matcha-encoder-768-no-ceil-fp16.rknn \
  model/tts/saved_v1/matcha-estimator-128-fp16.rknn \
  model/tts/saved_v1/matcha-estimator-256-fp16.rknn \
  model/tts/saved_v1/matcha-estimator-384-fp16.rknn \
  model/tts/saved_v1/matcha-estimator-512-fp16.rknn \
  model/tts/saved_v1/matcha-estimator-768-fp16.rknn \
  model/tts/saved_v1/deploy_models/vocos-22khz-univ-fp16.rknn
do
  test -s "$file" || echo "缺失或为空: $file"
done
```

命令没有输出时，表示文件齐全且非空。

## 推理流程

```text
AI 助手文本
  → 文本清理与数字/英文规范化
  → 按语义、标点和帧数切分
  → Matcha Encoder-768
  → 按有效帧选择 Estimator 桶
  → Vocos-600
  → ISTFT 生成 22.05 kHz 音频
  → 重采样为 16 kHz
  → ALSA 流式播放
```

超过 Vocos 600 帧容量的句子会继续切分。标点切分会保留相应停顿；由于帧数上限而在非标点位置切分时，不插入人工停顿。

TTS 模型在服务初始化时加载并常驻内存，后续文本块复用 RKNN Context，避免反复初始化模型。播放与合成使用独立队列，支持流式播放和“停止回答”。

## 源码位置

主要文件如下：

```text
audio/TTSService.h
audio/TTSService.cpp
audio/MatchaRknnEngine.h
audio/MatchaRknnEngine.cpp
audio/VoiceController.h
audio/VoiceController.cpp
audio/VoiceCommandNormalizer.h
audio/VoiceCommandNormalizer.cpp
```

应用入口在 `main.cpp` 中使用以下模型目录初始化 TTS：

```text
/home/firefly/VehicleTerminal/model/tts/saved_v1
```

如果项目被移动到其他绝对路径，需要同步修改 `main.cpp` 中的 `ttsModelPath`，或者后续将它改为相对于程序目录的路径。

## 编译与运行

依赖包括 Qt 5、OpenCV、ALSA、RKNN Runtime 和 C++17 编译器。

```bash
cd /home/firefly/VehicleTerminal
cmake -S . -B build
cmake --build build -j"$(nproc)"
./run.sh
```

也可以直接启动：

```bash
cd /home/firefly/VehicleTerminal
export LD_LIBRARY_PATH="$PWD/lib:$LD_LIBRARY_PATH"
./build/VehicleTerminal
```

初始化成功时日志中应出现：

```text
[Main] TTSService ready
Encoder + 5 Estimator buckets + Vocos init = OK
```

## NPU 分配

当前实现将 TTS RKNN 模型绑定到 `RKNN_NPU_CORE_2`，疲劳检测绑定到 `RKNN_NPU_CORE_0`。RKLLM 的核心使用情况由其运行时配置决定。修改核心分配后，应重新观察三颗 NPU 核心的占用率以及实时播放是否出现卡顿。

## 常见问题

### `TTSService init failed` 或 `cannot open lexicon`

确认运行目录、`ttsModelPath` 和上述模型目录结构一致，尤其检查是否多嵌套了一层 `saved_v1`。

### `Cannot open model`

检查对应 `.rknn` 文件是否存在、是否完整，以及当前用户是否拥有读取权限：

```bash
ls -lh /home/firefly/VehicleTerminal/model/tts/saved_v1
ls -lh /home/firefly/VehicleTerminal/model/tts/saved_v1/deploy_models
```

### 程序能合成但没有声音

当前 ALSA 播放设备在 `audio/TTSService.h` 中配置。先检查设备名称：

```bash
aplay -l
```

如果声卡名称不同，需要修改 `kPlaybackDevice` 后重新编译。



README 和 TTS 源码正常提交，使用者通过上方网盘地址单独下载模型。
