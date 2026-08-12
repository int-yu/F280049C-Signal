# API 参考

唯一公开头文件是 `include/sigf32.h`。除明确说明外，`SIGF32_OK` 成功；`SIGF32_EINVAL` 参数非法；`SIGF32_EWORKSPACE` 调用者提供的状态/工作区不足；`SIGF32_ENOSIGNAL` 信号不足；`SIGF32_ENOTLOCKED` 未锁定；`SIGF32_ENOTCONVERGED` 未收敛；`SIGF32_ERANGE` 超出可处理范围。

## 数值与预处理

| 接口 | 输入/输出 | 用途 |
|---|---|---|
| `sigf32_phase_wrap_pu` | 相位 → `[0,1)` | 相位连续累加后的归一化 |
| `sigf32_phase_pu_to_rad/deg` | pu → rad/° | 显示边界转换 |
| `sigf32_linear_calibrate` | `x*gain+offset` | ADC 校准；gain/offset 由标定得到 |
| `sigf32_median3` | 三个样点 | 抑制单点毛刺 |
| `sigf32_dc_blocker_*` | `alpha∈[0,1)` | 一阶 DC blocker；`alpha` 越接近 1 截止越低 |
| `sigf32_moving_average_*` | 调用者 `float state[length]` | 滑动平均；输出包含初始零填充 |
| `sigf32_agc_*` | target、attack/release、增益边界 | 自动增益；**不可放在幅值保真测量链** |

初始化后可调用 `reset` 清状态；所有工作区大小查询函数返回字节数，初始化参数 `state_count` 以元素数计。

## 滤波与抽取

- `sigf32_fir_init/process/process_block`：系数顺序为 `b[0]`（当前样点）到最旧样点。状态数至少 `taps`。
- `sigf32_biquad_init/process`：每节系数为 `(b0,b1,b2,a1,a2)`，实现方程为 `y=b0*x+d1; d1=b1*x-a1*y+d2; d2=b2*x-a2*y`。SOS 状态数至少 `stages`。
- `sigf32_decimator_init/process`：先 FIR 后抽取；`process` 返回 1 时 `*output` 有效。抽取前必须将阻带抑制设计到新奈奎斯特频率以下。

## 时域测量

- `sigf32_stats(input,count,result)`：返回均值、RMS、峰峰值、波峰因数；零输入返回 `ENOSIGNAL`，但统计字段仍被填充。
- `sigf32_frequency_zero_cross(...threshold,...)`：仅计正向过零；相邻差值低于 threshold 的过零被拒绝。至少要有两个有效过零。
- `sigf32_cross_correlate(x,y,count,max_lag,result)`：在 `[-max_lag,+max_lag]` 搜索最高归一化相关，`delay_samples` 的符号定义为 `y[i+lag]` 相对 `x[i]` 的 lag。

## 频域

- `sigf32_window_value/apply_window/window_metrics`：支持 Rect、Hann、Blackman-Harris、Flat-top；metrics 给出 coherent gain 与 ENBW（bin）。
- `sigf32_goertzel/multi`：目标频点可为非 FFT bin，`amplitude_peak` 是近似峰值振幅；非相干采样时须按应用需求提高记录长度或用 FFT 插值。
- `sigf32_spectrum_analyze`：工作区至少 `sigf32_spectrum_workspace_size(N)` 字节，传入 `workspace_count` 时单位是 **float 元素数**。返回基波频率、峰值幅度、相位、THD、THD+N、SNR、SINAD、SFDR 和噪声底。

`DC` 和 `Nyquist` 没有双边镜像，不能套用普通单边谱的 2 倍幅度规则。窗函数会降低相干增益；本接口已经作 coherent gain 补偿。THD/THD+N 只在明确基波、足够 SNR、且谐波落在可分析带宽内时有意义。

## NCO、解调与跟踪

- `sigf32_nco_init/set_frequency/next`：`next` 输出当前相位的 `sin/cos` 后推进相位；改频不重置相位，因此扫频连续。
- `sigf32_iq_init/process`：以本振相乘、低通后输出 I/Q、峰值幅度与相位。`lowpass_alpha` 取 `(0,1]`，小值更慢但抑制更强。
- `sigf32_pll_init/process/frequency_hz/locked`：二阶数字 PLL。`kp/ki` 必须针对采样率和允许频偏调节；`locked` 仅为算法状态，不能替代测量质量判断。
- `sigf32_fll_init/update_block`：用过零频率作块级初始捕获，适合作 PLL 初值；无有效过零或频率越界会返回诊断。
- `sigf32_lms_init/process`：`mu` 必须在 `(0,2)`；NLMS (`normalized=1`) 在输入幅度变化大时较稳，`error` 是期望信号减预测输出。
- `sigf32_transfer_point`：对输入/输出同频 Goertzel 之比，给出单频复数传递函数；coherence 为单段指示，不可代替多段 Welch 相干估计。

## 后端

`sigf32_tmu_sin_cos_pu` 在 C28x 且 `--tmu_support=tmu0` 时使用 `__sinpuf32/__cospuf32`；host 使用 `sinf/cosf`。CLA 适配器的 FIR/IIR/IQ/LMS 是逐样点入口，生产工程应把状态放入可访问 RAM，并通过消息 RAM 把输入输出与 CPU 隔离。RFFT 适配器提供输出约定参考；将其换成 C2000Ware `RFFT_f32` 时，不得改变 bin 的缩放与相位定义。