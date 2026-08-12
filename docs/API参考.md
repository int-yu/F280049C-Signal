# F280049C Signal API 参考

公开 ABI 只有 [`include/sigf32.h`](../include/sigf32.h)。本文原型与该头文件一致；完整可编译程序见 [API 调用教程](API调用教程.md)。除后端适配器外，所有函数均不分配内存，状态和工作区完全由调用者持有。采样率单位为 Hz，`count`、`state_count` 是**元素数**；只有 `*_workspace_size()` 返回**字节数**。相位内部使用 per-unit（pu）：一周为 `[0, 1)`。

## 通用规则

`SIGF32_OK` 表示结果有效；`SIGF32_EINVAL` 是空指针、零长度或参数范围错误；`SIGF32_EWORKSPACE` 是调用者提供的元素数不足；`SIGF32_ENOSIGNAL` 表示结果无有效信号；`SIGF32_ENOTLOCKED`、`SIGF32_ENOTCONVERGED` 可作为上层质量状态；`SIGF32_ERANGE` 是频率或运行范围越界。初始化失败时不得继续 process。`sigf32_diag_t` 可由调用者清零后复用：`valid` 表示最近输出有限，`invalid_count`/`saturation_count` 供上层诊断；库不隐式清零它。

ISR 中仅执行常数时间逐点函数；`stats`、相关、Goertzel、频谱、FLL 块更新应在 DMA 满块回调或后台任务运行。不要让两个 ISR/任务同时写同一个状态对象。

## 标量、相位与标定

| API | 原型与语义 | 参数/结果与失败处理 |
|---|---|---|
| `sigf32_phase_wrap_pu` | `float sigf32_phase_wrap_pu(float phase_pu);` | 任意 pu 映射到 `[0,1)`；无状态。 |
| `sigf32_phase_pu_to_rad` | `float sigf32_phase_pu_to_rad(float phase_pu);` | 先环绕再转弧度 `[0,2π)`。 |
| `sigf32_phase_pu_to_deg` | `float sigf32_phase_pu_to_deg(float phase_pu);` | 先环绕再转角度 `[0,360)`。 |
| `sigf32_phase_unwrap` | `float sigf32_phase_unwrap(float previous_rad,float current_rad);` | 输入/输出均为 rad；把 current 加减 `2π`，使其最接近 previous。 |
| `sigf32_linear_calibrate` | `float sigf32_linear_calibrate(float sample,float gain,float offset);` | 返回 `sample * gain + offset`；例如 ADC code 转伏特。 |
| `sigf32_median3` | `float sigf32_median3(float a,float b,float c);` | 返回三点中值，用于孤立毛刺前处理。 |

最小用法：`float volts = sigf32_linear_calibrate(adc_code, lsb_volt, offset_volt); float phase_deg = sigf32_phase_pu_to_deg(sigf32_phase_wrap_pu(phase));`。这些纯函数没有状态、没有状态码；NaN/Inf 的输入应在采样边界拒绝。

## DC、均值与 AGC

| API | 原型与生命周期 | 参数、结果和工作区 |
|---|---|---|
| `sigf32_dc_blocker_init` | `sigf32_status_t sigf32_dc_blocker_init(sigf32_dc_blocker_t *state,float alpha);` | `alpha` 必须 `[0,1)`；成功时清历史。 |
| `sigf32_dc_blocker_reset` | `void sigf32_dc_blocker_reset(sigf32_dc_blocker_t *state);` | 清 `x1/y1`，用于换量程或重新开始记录。 |
| `sigf32_dc_blocker_process` | `float sigf32_dc_blocker_process(sigf32_dc_blocker_t *state,float input,sigf32_diag_t *diag);` | 逐样点，`alpha` 越接近 1 截止越低；空状态返回 0 并增加 `invalid_count`。 |
| `sigf32_moving_average_workspace_size` | `size_t sigf32_moving_average_workspace_size(size_t length);` | 返回 `length*sizeof(float)` 字节，仅用于分配评估。 |
| `sigf32_moving_average_init` | `sigf32_status_t sigf32_moving_average_init(sigf32_moving_average_t *filter,float *state,size_t state_count,size_t length);` | `state_count >= length`（元素）；初始化清零，前 `length` 点包含零填充。 |
| `sigf32_moving_average_reset` | `void sigf32_moving_average_reset(sigf32_moving_average_t *filter);` | 清 state、累计和与索引。 |
| `sigf32_moving_average_process` | `float sigf32_moving_average_process(sigf32_moving_average_t *filter,float input);` | 逐样点输出平均；无效对象返回 0。 |
| `sigf32_agc_init` | `sigf32_status_t sigf32_agc_init(sigf32_agc_t *agc,float target_peak,float attack,float release,float min_gain,float max_gain);` | `target_peak/min_gain>0`，`attack/release∈(0,1]`，且 `max_gain>=min_gain`。 |
| `sigf32_agc_reset` | `void sigf32_agc_reset(sigf32_agc_t *agc);` | 恢复 `gain=1`、包络为 0。 |
| `sigf32_agc_process` | `float sigf32_agc_process(sigf32_agc_t *agc,float input,sigf32_diag_t *diag);` | 逐样点；仅适用于显示/判决链，不能用于保真幅值测量。 |

完整的 ADC 标定、DC、FIR、IIR、抽取链见 [`preprocess_filter.c`](../examples/api_usage/preprocess_filter.c)。每个 `init` 必须检查 `SIGF32_OK`；失败后不得 process。

## FIR、抽取与 SOS IIR

| API | 原型与生命周期 | 参数、结果和工作区 |
|---|---|---|
| `sigf32_fir_workspace_size` | `size_t sigf32_fir_workspace_size(size_t taps);` | 返回 `taps*sizeof(float)` 字节。 |
| `sigf32_fir_init` | `sigf32_status_t sigf32_fir_init(sigf32_fir_t *fir,const float *coeffs,size_t taps,float *state,size_t state_count);` | `coeffs[0]` 对应当前样点，`state_count>=taps`；成功会清 state。 |
| `sigf32_fir_reset` | `void sigf32_fir_reset(sigf32_fir_t *fir);` | 清历史/索引；换滤波系数时重新 init。 |
| `sigf32_fir_process` | `float sigf32_fir_process(sigf32_fir_t *fir,float input);` | ISR 逐样点入口；init 后调用。 |
| `sigf32_fir_process_block` | `sigf32_status_t sigf32_fir_process_block(sigf32_fir_t *fir,const float *input,float *output,size_t count,sigf32_diag_t *diag);` | DMA block 入口，`input/output` 可用不同缓冲区；成功置 `diag->valid`。 |
| `sigf32_decimator_init` | `sigf32_status_t sigf32_decimator_init(sigf32_decimator_t *decimator,const float *coeffs,size_t taps,float *state,size_t state_count,size_t factor);` | `factor>=2`、状态元素不少于 taps；内部先建 FIR。 |
| `sigf32_decimator_reset` | `void sigf32_decimator_reset(sigf32_decimator_t *decimator);` | 清 FIR 历史和抽取相位。 |
| `sigf32_decimator_process` | `uint16_t sigf32_decimator_process(sigf32_decimator_t *decimator,float input,float *output);` | 每点先 FIR；仅返回 1 时 `*output` 有效。前端 FIR 必须在新 Nyquist 以下提供足够阻带。 |
| `sigf32_biquad_workspace_size` | `size_t sigf32_biquad_workspace_size(size_t stages);` | 返回 `stages*sizeof(sigf32_biquad_state_t)` 字节。 |
| `sigf32_biquad_init` | `sigf32_status_t sigf32_biquad_init(sigf32_biquad_t *filter,const sigf32_biquad_coeffs_t *coeffs,size_t stages,sigf32_biquad_state_t *state,size_t state_count);` | 每节系数 `(b0,b1,b2,a1,a2)`，`state_count>=stages`，成功清状态。 |
| `sigf32_biquad_reset` | `void sigf32_biquad_reset(sigf32_biquad_t *filter);` | 清所有节 `d1/d2`。 |
| `sigf32_biquad_process` | `float sigf32_biquad_process(sigf32_biquad_t *filter,float input);` | DF2T，`y=b0*x+d1; d1=b1*x-a1*y+d2; d2=b2*x-a2*y`。 |

状态数组必须独占，不能让 FIR 和 decimator 共用。示例展示 reset 和诊断；在 DMA 回调使用 `fir_process_block`，在 ADC ISR 使用各逐样点 API。

## 时域统计、频率与延时

| API | 原型与结果有效性 |
|---|---|
| `sigf32_stats` | `sigf32_status_t sigf32_stats(const float *input,size_t count,sigf32_stats_result_t *result);` 返回 mean、RMS、min/max、peak、p-p、crest；全零块返回 `ENOSIGNAL` 且 `valid=0`。 |
| `sigf32_frequency_zero_cross` | `sigf32_status_t sigf32_frequency_zero_cross(const float *input,size_t count,float sample_rate_hz,float threshold,sigf32_frequency_result_t *result);` 只计正向过零，线性插值；至少两个有效 crossing，否则 `ENOSIGNAL`。`threshold>=0` 是相邻差分的最低幅度。 |
| `sigf32_cross_correlate` | `sigf32_status_t sigf32_cross_correlate(const float *x,const float *y,size_t count,int max_lag,sigf32_correlation_result_t *result);` 搜索 `[-max_lag,+max_lag]` 并作三点插值；`delay_samples=lag` 表示比较 `x[i]` 与 `y[i+lag]`。 |

时间块测量、子样点延迟处理见 [`measure_spectrum.c`](../examples/api_usage/measure_spectrum.c)。结果只在 `status==SIGF32_OK && result.valid` 时送显示或控制；信号不足时保留上一次结果或显示无效，不要把零当测量值。

## 窗、单频与频谱指标

| API | 原型与关键约束 |
|---|---|
| `sigf32_window_value` | `float sigf32_window_value(sigf32_window_t window,size_t index,size_t count);` 支持 `RECT/HANN/BLACKMAN_HARRIS/FLAT_TOP`；`count<=1` 返回 1。 |
| `sigf32_window_metrics` | `sigf32_status_t sigf32_window_metrics(sigf32_window_t window,size_t count,float *coherent_gain,float *enbw_bins);` 返回相干增益和 ENBW（bin）；输出指针、count 必须有效。 |
| `sigf32_apply_window` | `void sigf32_apply_window(const float *input,float *output,size_t count,sigf32_window_t window);` 将窗逐点写入 output，可在块任务调用。 |
| `sigf32_goertzel` | `sigf32_status_t sigf32_goertzel(const float *input,size_t count,float sample_rate_hz,float target_hz,sigf32_goertzel_result_t *result);` `target_hz∈[0,fs/2]`；给 real/imag/power/近似峰值幅度/pu 相位，无信号为 `ENOSIGNAL`。 |
| `sigf32_goertzel_multi` | `sigf32_status_t sigf32_goertzel_multi(const float *input,size_t count,float sample_rate_hz,const float *targets_hz,sigf32_goertzel_result_t *results,size_t target_count);` 按 target 数组逐频调用；任一无信号会向上传递非 OK。 |
| `sigf32_spectrum_workspace_size` | `size_t sigf32_spectrum_workspace_size(size_t fft_size);` 返回 `2*(N/2+1)*sizeof(float)` **字节**。 |
| `sigf32_spectrum_analyze` | `sigf32_status_t sigf32_spectrum_analyze(const float *input,size_t count,float sample_rate_hz,sigf32_window_t window,unsigned harmonic_count,float *workspace,size_t workspace_count,sigf32_spectrum_metrics_t *result);` `count>=8`；`workspace_count` 是 float 元素，至少 `2*(N/2+1)`；输出基波、THD/THDN/SNR/SINAD/SFDR/噪声底。 |

DC 与 Nyquist 不按普通单边谱作 2 倍幅值换算；频谱函数已按 coherent gain 修正。非相干记录会泄漏，选择合适窗、增加记录长度或改用单频 Goertzel。完整工作区与 THD/SNR 判断见 [`measure_spectrum.c`](../examples/api_usage/measure_spectrum.c)。

## NCO、IQ、PLL/FLL 与自适应

| API | 原型与生命周期 | 参数、结果和失败处理 |
|---|---|---|
| `sigf32_nco_init` | `sigf32_status_t sigf32_nco_init(sigf32_nco_t *nco,float sample_rate_hz,float frequency_hz,float phase_pu);` | `fs>0`、`|f|<=fs/2`；设置可连续推进的 pu 相位。 |
| `sigf32_nco_set_frequency` | `sigf32_status_t sigf32_nco_set_frequency(sigf32_nco_t *nco,float frequency_hz);` | 同一 Nyquist 边界；只改步进，不重置相位，适合扫频。 |
| `sigf32_nco_next` | `void sigf32_nco_next(sigf32_nco_t *nco,float *sine,float *cosine);` | 输出当前相位 sin/cos 后推进一个样点；空指针时不写入。 |
| `sigf32_iq_init` | `sigf32_status_t sigf32_iq_init(sigf32_iq_demod_t *demod,float sample_rate_hz,float carrier_hz,float lowpass_alpha);` | `lowpass_alpha∈(0,1]`；小值更慢、平滑更强。 |
| `sigf32_iq_process` | `sigf32_iq_result_t sigf32_iq_process(sigf32_iq_demod_t *demod,float sample);` | 逐样点给 I/Q、峰值幅度和 pu 相位；无状态指针时返回 `valid=0`。 |
| `sigf32_pll_init` | `sigf32_status_t sigf32_pll_init(sigf32_pll_t *pll,float sample_rate_hz,float initial_hz,float kp,float ki,float min_hz,float max_hz);` | 初值必须在 `[min_hz,max_hz]`，`kp/ki>=0`；成功清积分和锁定统计。 |
| `sigf32_pll_process` | `float sigf32_pll_process(sigf32_pll_t *pll,float sample);` | ISR 逐点，返回相位误差 pu；只在 init 成功后调用。 |
| `sigf32_pll_frequency_hz` | `float sigf32_pll_frequency_hz(const sigf32_pll_t *pll);` | 返回当前估计 Hz，空指针返回 0。 |
| `sigf32_pll_locked` | `uint16_t sigf32_pll_locked(const sigf32_pll_t *pll);` | 连续低误差超过内部窗口返回 1；仅算法状态，仍需应用层质量门限。 |
| `sigf32_fll_init` | `sigf32_status_t sigf32_fll_init(sigf32_fll_t *fll,float sample_rate_hz,float initial_hz,float smoothing,float min_hz,float max_hz);` | `smoothing∈(0,1]`、范围正且初值在范围内。 |
| `sigf32_fll_update_block` | `sigf32_status_t sigf32_fll_update_block(sigf32_fll_t *fll,const float *input,size_t count,float threshold);` | DMA 块级过零估计，用输出 `fll.frequency_hz` 种子 PLL；无信号或越界返回状态并清 `locked`。 |
| `sigf32_lms_workspace_size` | `size_t sigf32_lms_workspace_size(size_t taps);` | 总需求 `2*taps*sizeof(float)` 字节，对应独立 weights/history。 |
| `sigf32_lms_init` | `sigf32_status_t sigf32_lms_init(sigf32_lms_t *lms,float *weights,float *history,size_t taps,float mu,uint16_t normalized);` | `mu∈(0,2)`；清两个数组；`normalized=1` 启用 NLMS。 |
| `sigf32_lms_reset` | `void sigf32_lms_reset(sigf32_lms_t *lms);` | 清权重、历史和索引。 |
| `sigf32_lms_process` | `float sigf32_lms_process(sigf32_lms_t *lms,float reference,float desired,float *error);` | 逐样点，返回预测输出，`*error=desired-output`（若非空）；需已有 init。 |
| `sigf32_transfer_point` | `sigf32_status_t sigf32_transfer_point(const float *input,const float *output,size_t count,float sample_rate_hz,float frequency_hz,sigf32_transfer_result_t *result);` | 以同频 Goertzel 比值返回复传函、幅值、pu 相位；输入能量不足为 `ENOSIGNAL`；单块 `coherence=1` 只是指示，非 Welch 统计相干。 |

相位连续 NCO、FLL 捕获后启动 PLL、IQ 幅相和 NLMS 误差的完整调用见 [`tracking_adaptive.c`](../examples/api_usage/tracking_adaptive.c)。PLL/FLL 频率与锁定只应在结果通过状态和质量门限后发布。

## F280 后端边界

公共 `sigf32.h` 保持可移植。`backends/c2000/sigf32_tmu.h` 的 `sigf32_tmu_sin_cos_pu` 接收 `[0,1)` pu；在 C28x 且 `--tmu_support=tmu0` 使用 TMU0 intrinsic，主机用 `sinf/cosf`。`sigf32_cla_adapter.h` 提供 `sigf32_cla_fir_point`、`sigf32_cla_biquad_point`、`sigf32_cla_iq_point`、`sigf32_cla_pll_point`、`sigf32_cla_lms_point`，调用者仍拥有全部状态；生产工程须放入 CPU/CLA 可见 RAM，通过 message RAM 同步。

`sigf32_rfft_reference` 是主机参考 DFT。定义 `SIGF32_ENABLE_C2000WARE_RFFT` 后可用 `sigf32_rfft_c2000ware`：C2000Ware `RFFT_F32_STRUCT`、input、output、twiddles 由调用者提供，FFT 长度必须为 32 到 2048 的 2 次幂；适配器把 packed output 还原为 `real[0..N/2]` / `imag[0..N/2]`。完整可编译后端边界示例见 [`platform_backends.c`](../examples/api_usage/platform_backends.c)。
