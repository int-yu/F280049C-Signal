# F280049C-Signal

面向 TI 杯模拟电子系统设计邀请赛的 `TMS320F280049C`（C28x + FPU + TMU0 + CLA）信号处理算法库。它提供无动态内存的、可在主机回归测试和 CCS/SysConfig 工程中复用的浮点算法模块。

> 这是**算法库**，不是某块开发板的完整外设工程。ADC、DAC、DMA、ePWM、eCAP 与 GPIO 由 `main.syscfg` 图形化配置；算法接口不绑定任何封装脚号。

## 能力与约束

- 预处理：线性校准、中值去毛刺、DC blocker、滑动平均、AGC（仅显示/判决链）。
- 滤波/抽取：FIR、SOS biquad、抽取器；状态和工作区由调用者提供。
- 测量：均值、RMS、峰值、过零频率、互相关亚采样时延（三点抛物线插值）。
- 频域：窗函数、Goertzel、多频检测、频谱指标（THD、THD+N、SNR、SINAD、SFDR）。
- 同步处理：NCO/DDS、I/Q 解调、PLL、FLL 辅助捕获、LMS/NLMS、单频传递函数。
- 平台适配示例：TMU0 正弦余弦、CLA 逐样点算法入口、RFFT 接口边界。
- 不使用 `malloc/calloc/free`；目标采样率由各接口显式给出。典型应用频段为 1 Hz–100 kHz，前提是采样率、模拟前端和记录长度满足奈奎斯特条件。

算法精度验收指标仅覆盖数字算法：典型情况下频率 ≤0.05%、幅值 ≤0.5%、相位 ≤0.5°；ADC 基准、采样时钟、模拟前端及布局误差另计。

## 快速开始

1. 安装 CCS、DigitalPower SDK/C2000Ware `26.01.00.00` 与 C2000 编译器 `25.11.1.LTS`。
2. CCS：**File → Import CCS Projects**，选择 `F280049C_Signal.projectspec`。
3. 双击 `main.syscfg`，用 SysConfig 配置时钟、ADC、ePWM/DMA 触发及引脚；构建 `CPU1_RAM` 后先下载验证。
4. 在自己的采样中断或 DMA block 回调中包含 `sigf32.h`，为所有状态对象提供静态/全局工作区。详见 [快速开始](docs/快速开始.md)。

工程使用 `${COM_TI_DIGITAL_POWER_C2000WARE_SDK_SOFTWARE_PACKAGE_INSTALL_DIR}/c2000ware`，不含本机绝对 SDK 路径；`products="sysconfig"` 保持 SysConfig 产品依赖。

## 目录

- `include/`：唯一公开头文件 `sigf32.h`。
- `src/`：可移植 FPU 算法实现。
- `backends/c2000/`：TMU、CLA、RFFT 的接入边界与可编译示例。
- `examples/`：无硬件依赖的合成数据示例。
- `tests/`：GCC 回归测试和 Python 黄金测试。
- `tools/`：NumPy/SciPy 黄金模型与 SOS 设计报告生成器。
- `docs/`：中文接口、接线、调参与现场排障手册。

## 重要说明

- `__sinpuf32()` / `__cospuf32()` 接收 `[0, 1)` 的 per-unit 周期量；本库相位内部也统一用该单位，不可直接传弧度。
- CLA 不是 CPU 的 TMU0。CLA 路径只能调用 CLA 可用的数学例程；与 CPU 交换数据时需要消息 RAM 和同步设计。
- F280049C 有 **2 路 12 位 DAC**，不是 3 路。
- 默认频谱实现以可验证性为优先；真机性能请使用 `backends/c2000` 的 RFFT 接入方式并实测周期、RAM、栈。

许可证：[MIT](LICENSE)。