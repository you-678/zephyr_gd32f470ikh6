# my_blinky

基于 **Zephyr RTOS** 的 GD32F470 三路 LED 流水灯示例，目标板 `gd32f470i_eval`。
工程名 `blinky`（见 `CMakeLists.txt`），仅依赖 Zephyr GPIO 驱动，无外部库。

> 本 README 按仓库当前代码与 `build/` 中的实际构建配置逐项核对编写（核对日期：2026-11-10）。仓库中原本没有 README.md，此文件为新建。

---

## 1. 项目解决什么问题

| 问题 | 本项目的做法 |
| --- | --- |
| 验证 Zephyr 工具链能否在 GD32F470 上跑通「构建 → 烧录 → 运行」全链路 | 提供一个最小可运行固件，上电后无需任何配置即可自检 |
| 验证设备树 overlay 与 `led0/led1/led2` 别名是否正确生效 | 代码通过 `DT_ALIAS` + `GPIO_DT_SPEC_GET` 取引脚，灯亮即说明设备树链路通 |
| 验证板级 GPIO 驱动、引脚极性与延时是否可用 | 三路 LED 按固定时序轮流点亮，肉眼即可判断 |
| 给后续例程（PWM、串口、传感器）提供最小骨架 | 保留 `CMakeLists.txt` / `prj.conf` / `boards/*.overlay` 的 Zephyr 标准结构 |

**定位**：板级 bring-up 冒烟测试（smoke test），不是产品固件。它不采集数据、不响应指令、不联网，也没有任何运行时交互。

---

## 2. 主要功能

| 功能 | 实现位置 | 状态 |
| --- | --- | --- |
| 3 路 LED 引脚由设备树别名解析 | `src/main.c` 的 `DT_ALIAS(led0..2)` + `GPIO_DT_SPEC_GET` | 已启用 |
| GPIO 就绪检查与输出初始化 | `gpio_is_ready_dt()`；失败直接 `return -1`；`GPIO_OUTPUT_INACTIVE`（上电全灭） | 已启用 |
| 三灯单灯流水循环 | `app_led_flow_task()`，每步 `k_msleep(500)`，循环周期 1.5 s | 已启用，`main()` 中 `while(1)` 调用 |
| 全亮 / 全灭 / 闪烁 N 次 | `app_led_all_on()` / `app_led_all_off()` / `app_led_flash(uint8_t times)` | 已实现但 **`main()` 未调用**；`flash()` 为「先灭 500 ms → 亮 500 ms → 灭」重复 N 次 |
| 引脚映射与有效电平 | `boards/gd32f470i_eval.overlay` | 已启用 |
| 使能 GPIO 驱动 | `prj.conf` 中唯一一行 `CONFIG_GPIO=y` | 已启用 |
| 串口打印 / Shell / 日志 / 按键输入 / PWM / 多线程 | — | 未实现（板级配置虽默认开了 USART0 控制台，但应用代码不打印任何字符） |

### 引脚映射

| 别名 | overlay 节点 | GPIO | 有效电平 | 板级 DTS 对应 LED |
| --- | --- | --- | --- | --- |
| `led0` | `my_led1` | `GPIOE` 引脚 2（PE2） | 高电平亮 | LED1 |
| `led1` | `my_led2` | `GPIOE` 引脚 3（PE3） | 高电平亮 | LED2 |
| `led2` | `my_led3` | `GPIOF` 引脚 10（PF10） | 高电平亮 | LED3 |

三个引脚与板级 `gd32f470i_eval.dts` 自带 `leds` 节点一致，overlay 的作用是重新定义 `led0/led1/led2` 别名指向自定义节点。

### 目录结构

| 路径 | 作用 |
| --- | --- |
| `CMakeLists.txt` | 要求 CMake ≥ 3.28.0，`find_package(Zephyr)`，源文件 `src/main.c` |
| `prj.conf` | `CONFIG_GPIO=y` |
| `src/main.c` | 初始化 + 流水灯主循环（约 77 行） |
| `boards/gd32f470i_eval.overlay` | 三个 LED 的别名与 GPIO 定义；文件名须与板名一致才会被自动加载 |
| `flash.ps1` | OpenOCD 一键烧录脚本（相对路径定位 hex，见第 4 节；已加入 `.gitignore`） |
| `build/` | 已生成的构建产物，可用 `west build` 覆盖重建 |
| `LICENSE` | Apache License 2.0 官方全文 |

---

## 3. 安装方法

### 3.1 环境要求

下表为从 `build/build_info.yml`、`build/CMakeCache.txt` 可确认的硬件/环境要求，换机器时按实际安装路径替换 `<你的 Zephyr 源码路径>` 即可：

| 项 | 说明 |
| --- | --- |
| 主机系统 | Windows / Linux 均可（Zephyr 跨平台） |
| Zephyr 源码 | 通过 `ZEPHYR_BASE` 环境变量或 Zephyr workspace 定位，例如 `<你的 Zephyr 源码路径>/zephyr` |
| Zephyr SDK | 安装 Zephyr SDK（含 `arm-zephyr-eabi` 工具链、OpenOCD hosttools） |
| west | 安装于 Python venv 或已加入 `PATH`，用于 `west build` / `west flash` |
| CMake / 生成器 | ≥ 3.28.0 / Ninja |
| 目标板 | `gd32f470i_eval`（qualifier `gd32f470`） |
| 烧录调试器 | ST-Link（OpenOCD `interface/stlink.cfg`） |

### 3.2 从零准备（已搭好环境可跳过）

1. 按 Zephyr 官方 Getting Started 流程安装 Python venv + `west`，用 `west init` / `west update` 拉取 Zephyr 源码树，安装 Zephyr SDK（勾选 ARM 工具链与 OpenOCD hosttools）。
2. 安装 CMake ≥ 3.28.0、Ninja，并加入 `PATH`。
3. 设置环境变量（`CMakeLists.txt` 通过 `$ENV{ZEPHYR_BASE}` 定位 Zephyr）：

```powershell
$env:ZEPHYR_BASE = "<你的 Zephyr 源码路径>/zephyr"
```

4. 将本仓库放到任意目录（例如 `~/my_blinky`），接好 `gd32f470i_eval` 开发板与 ST-Link。

> 无需额外安装第三方库：`prj.conf` 只开启 `CONFIG_GPIO`。

---

## 4. 使用方法

### 4.1 构建

在本仓库根目录打开 PowerShell：

```powershell
# 先设置 Zephyr 源码路径（一次性，或在 shell 配置里持久化）
$env:ZEPHYR_BASE = "<你的 Zephyr 源码路径>/zephyr"
# 用 west 构建（west 已加入 PATH 时）
west build -b gd32f470i_eval -d build .
# 或直接用 cmake（等效写法）
# cmake -B build -GNinja -DBOARD=gd32f470i_eval -DZEPHYR_BASE="<你的 Zephyr 源码路径>/zephyr" -S .
# cmake --build build
```

需要彻底重构建时加 `-p always`：`west build -b gd32f470i_eval -d build -p always .`

产物位于 `build/zephyr/`：`zephyr.elf`、`zephyr.hex`、`zephyr.bin`。

### 4.2 烧录（二选一）

**方式 A：west flash（推荐，默认 openocd runner）**

```powershell
west flash -d build
```

`build/zephyr/runners.yaml` 中已配置 `openocd`（默认）与 `jlink`（`--device GD32F450IK`），需要 J-Link 时加 `--runner jlink`。

**方式 B：仓库自带的 `flash.ps1`**

```powershell
./flash.ps1
```

脚本实际内容：通过 `$PSScriptRoot` 自动定位到 `build/zephyr/zephyr.hex`（相对路径，项目挪到哪台电脑哪个盘都能用），调用 OpenOCD 加载 `interface/stlink.cfg` 与 `target/stm32f4x.cfg` 执行 `program ... verify reset exit`（烧录 + 校验 + 复位）。OpenOCD 可执行文件本身仍指向工具链安装位置（例如 `D:\zephyr-sdk\...`），若该路径在你机器上不同，请按实际情况修改脚本顶部的 `$OpenOCD` 变量即可。
其中 `stm32f4x.cfg` 是脚本为复用 STM32F4 的 OpenOCD Flash 驱动而采用的写法（GD32F4 系列调试时常见做法），并非笔误；若该配置在你的调试器上不适用，请改用方式 A 的 `west flash`。

> 说明：`flash.ps1` 中的 hex 路径已相对化；仅 `$OpenOCD` 变量指向工具链安装位置，属于环境相关项，已在 `.gitignore` 中排除本脚本以免泄露本机路径，方式 A（`west flash`）不受此限制。

### 4.3 运行

烧录完成后板子自动复位运行，**无需任何命令或按键输入**，直接观察三颗 LED。想改效果就改 `src/main.c`：

| 想改什么 | 改哪里 |
| --- | --- |
| 流水速度 | `app_led_flow_task()` 中三处 `k_msleep(500)` |
| LED 顺序 | `app_led_flow_task()` 中 `leds[0..2]` 的操作次序 |
| 换成全亮 / 全灭 / 闪烁 | 在 `main()` 的 `while(1)` 中改调用 `app_led_all_on()` / `app_led_all_off()` / `app_led_flash(N)` |
| 换引脚 | `boards/gd32f470i_eval.overlay` 中的 `gpios` 属性，改后需 `west build -p always` 重新生成设备树 |

---

## 5. 输入输出示例

### 5.1 输入

**运行期没有用户输入接口**：无按键、无串口命令、无 Shell、无参数。唯一的「输入」是上电/复位本身。

构建期的输入示例：

```powershell
west build -b gd32f470i_eval -d build .
```

| 输入项 | 示例值 | 来源 |
| --- | --- | --- |
| 板型参数 `-b` | `gd32f470i_eval` | 决定加载 `boards/gd32f470i_eval.overlay` |
| 设备树片段 | led0→PE2、led1→PE3、led2→PF10，高有效 | `boards/gd32f470i_eval.overlay` |
| Kconfig 片段 | `CONFIG_GPIO=y` | `prj.conf` |
| 硬件条件 | 开发板供电正常、SWD 与 ST-Link 连通 | 烧录阶段 |

### 5.2 输出

**(a) 构建输出**：`build/zephyr/zephyr.hex`、`zephyr.elf`、`zephyr.bin`（本机 2026-11-10 一次构建实测分别为 46,474 / 472,244 / 16,484 字节，仅供参照，随工具链版本会变）。

**(b) 运行输出（唯一的可见输出是 LED 电平）**：

| 时刻 | LED1 (PE2) | LED2 (PE3) | LED3 (PF10) |
| --- | --- | --- | --- |
| 复位后初始化 | 灭 | 灭 | 灭 |
| 0.0 – 0.5 s | 亮 | 灭 | 灭 |
| 0.5 – 1.0 s | 灭 | 亮 | 灭 |
| 1.0 – 1.5 s | 灭 | 灭 | 亮 |
| 1.5 – 2.0 s | 亮 | 灭 | 灭（开始下一轮循环） |

即 `LED1 → LED2 → LED3 → LED1 ...` 单灯轮流点亮，每颗亮 500 ms，一轮 1.5 s，无限循环。

**(c) 串口输出：无。** 代码中没有任何 `printk` / `printf`，也未启用 Shell 与日志模块；即使板级 DTS 默认配置了 `zephyr,console = &usart0`（`current-speed = 115200`），串口也收不到任何字符。GPIO 未就绪时 `main()` 返回 `-1`，同样不会有任何提示，表现为三灯全灭。

### 5.3 现象对照

| 观察结果 | 说明 |
| --- | --- |
| 三灯按上表轮流亮 | 正常：工具链、烧录、设备树、GPIO 驱动全部打通 |
| 三灯常灭 | 检查供电/复位、`gpio_is_ready_dt()` 是否失败、overlay 引脚与板上 LED 是否对应 |
| 亮灯顺序与预期不同 | 检查 overlay 中 `led0/led1/led2` 别名与实际丝印的对应关系 |
| 构建报找不到 Zephyr | 检查 `ZEPHYR_BASE` 是否设置、`CMakeLists.txt` 要求的 CMake 版本 |
| 烧录失败 | 检查 ST-Link 连接；用 `flash.ps1` 时确认脚本顶部 `$OpenOCD` 路径与你机器上的 Zephyr SDK 安装位置一致 |

---

## 6. 当前边界（避免误解）

- 应用层只有一个 `main()` 主循环 + `k_msleep()` 阻塞延时，没有创建线程，也没有使用中断、硬件定时器、DMA 等外设。
- `app_led_all_on()` / `app_led_all_off()` / `app_led_flash()` 是已写好的工具函数，但默认固件不会执行它们。
- 板级 DTS 里存在 PWM、DAC、USART 等节点，只是 Zephyr 板级支持的默认描述，**本项目未使用**。

---

## 7. 许可证

本项目采用 **Apache License 2.0**：

- 根目录 `LICENSE` 为官方全文（202 行，来源 `https://www.apache.org/licenses/LICENSE-2.0.txt`），与 `CMakeLists.txt` 首行的 `# SPDX-License-Identifier: Apache-2.0` 一致。
- `LICENSE` 附录中的 `Copyright [yyyy] [name of copyright owner]` 是 Apache 官方模板占位符，仓库未指定具体版权人；如需声明版权，替换该占位符或新增 `NOTICE` 文件即可。
- 除 `CMakeLists.txt` 的 SPDX 标识外，源文件未逐文件添加许可证头。
