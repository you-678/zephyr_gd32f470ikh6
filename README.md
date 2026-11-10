# my_blinky

基于 **Zephyr RTOS** 的 GD32F470 板级示例，目标板 `gd32f470i_eval`。工程名 `blinky`（见 `CMakeLists.txt`），仅依赖 Zephyr GPIO 驱动，无外部库。

> 本 README 按仓库当前代码与设备树逐项核对编写（核对日期：2026-11-10）。

---

## 1. 项目解决什么问题

| 问题 | 本项目的做法 |
| --- | --- |
| 验证 Zephyr 工具链能否在 GD32F470 上跑通「构建 → 烧录 → 运行」全链路 | 提供一个最小可运行固件，上电后无需任何配置即可自检 |
| 验证设备树 overlay 中 `led0~led2` 与 `sw0~sw2` 别名是否生效 | 代码用 `DT_ALIAS` + `GPIO_DT_SPEC_GET` 取 LED 与按键引脚，灯随按键变化即说明链路通 |
| 验证板级 GPIO 驱动、引脚极性、以及 **GPIO 中断（callback）** 是否可用 | 三颗用户按键分别挂中断，按下即翻转对应 LED，肉眼即可判断 |
| 给后续例程（PWM、串口、传感器）提供最小骨架 | 保留 `CMakeLists.txt` / `prj.conf` / `boards/*.overlay` 的 Zephyr 标准结构 |

**定位**：板级 bring-up 冒烟测试（smoke test），不是产品固件。它不采集数据、不响应指令、不联网，也没有任何运行时交互（主循环空闲，全靠中断响应）。

---

## 2. 主要功能

| 功能 | 实现位置 | 状态 |
| --- | --- | --- |
| 3 路 LED 引脚由设备树别名解析 | `src/main.c` 的 `DT_ALIAS(led0..2)` + `GPIO_DT_SPEC_GET` | 已启用 |
| 3 路用户按键引脚由设备树别名解析 | `DT_ALIAS(sw0..2)` + `GPIO_DT_SPEC_GET` | 已启用 |
| GPIO 就绪检查与输出/输入初始化 | LED 用 `GPIO_OUTPUT_INACTIVE`（上电全灭）；按键用 `GPIO_INPUT`；任一未就绪 `main()` 直接 `return -1` | 已启用 |
| 按键中断配置 | `gpio_pin_interrupt_configure_dt(..., GPIO_INT_EDGE_TO_ACTIVE)`（边沿触发，按键有效电平时触发） | 已启用 |
| 共享中断回调，按引脚翻转对应 LED | `button_pressed_cb()` 中用 `pins & BIT(...)` 判断，调用 `gpio_pin_toggle_dt()` | 已启用 |
| 主循环空闲 | `while(1) k_msleep(1000)`，所有响应由中断完成 | 已启用 |
| 使能 GPIO 驱动 | `prj.conf` 仅 `CONFIG_GPIO=y`（中断属于 GPIO 驱动，无需额外开关） | 已启用 |
| 流水灯 / 全亮全灭 / 闪烁 / 串口打印 / Shell / 日志 / PWM / 多线程 | — | 未实现 |

### 引脚映射

**LED（高电平亮，与板级 `gd32f470i_eval.dts` 自带 LED1/2/3 一致，overlay 重定义别名）**

| 别名 | overlay 节点 | GPIO | 有效电平 |
| --- | --- | --- | --- |
| `led0` | `my_led1` | `GPIOE` 引脚 2（PE2） | 高 |
| `led1` | `my_led2` | `GPIOE` 引脚 3（PE3） | 高 |
| `led2` | `my_led3` | `GPIOF` 引脚 10（PF10） | 高 |

**按键（低电平有效，对应板载 WAKEUP / TAMPER / USER 键）**

| 别名 | overlay 节点 | GPIO | 有效电平 | 板上按键 |
| --- | --- | --- | --- | --- |
| `sw0` | `user_button_0` | `GPIOA` 引脚 0（PA0） | 低 | WAKEUP |
| `sw1` | `user_button_1` | `GPIOC` 引脚 13（PC13） | 低 | TAMPER |
| `sw2` | `user_button_2` | `GPIOB` 引脚 14（PB14） | 低 | USER |

三者共用一个 `gpio_callback`，分别注册到端口 A / C / B；按下对应按键时翻转对应 LED。

### 目录结构

| 路径 | 作用 |
| --- | --- |
| `CMakeLists.txt` | 要求 CMake ≥ 3.28.0，`find_package(Zephyr)`，源文件 `src/main.c` |
| `prj.conf` | `CONFIG_GPIO=y` |
| `src/main.c` | LED + 按键中断初始化与回调（约 75 行） |
| `boards/gd32f470i_eval.overlay` | LED 与按键的别名、GPIO 定义；文件名须与板名一致才会被自动加载 |
| `flash.ps1` | 本机 OpenOCD 一键烧录脚本（相对路径定位 hex；已加入 `.gitignore`，不随仓库发布） |
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

**方式 B：仓库自带的 `flash.ps1`（本机脚本，不随仓库发布）**

```powershell
./flash.ps1
```

脚本通过 `$PSScriptRoot` 自动定位 `build/zephyr/zephyr.hex`（相对路径），调用 OpenOCD 加载 `interface/stlink.cfg` 与 `target/stm32f4x.cfg` 执行烧录 + 校验 + 复位。OpenOCD 可执行文件本身仍指向工具链安装位置（例如 `D:\zephyr-sdk\...`），若该路径在你机器上不同，请修改脚本顶部 `$OpenOCD` 变量。因含本机环境信息，该脚本已加入 `.gitignore`，克隆仓库后需自行准备等效烧录方式（推荐方式 A）。

> 注：`stm32f4x.cfg` 是复用 STM32F4 的 OpenOCD Flash 驱动的常见写法（GD32F4 调试时常用），并非笔误；若不适用请改用方式 A。

### 4.3 运行

烧录完成后板子自动复位运行，**无需任何命令或按键输入**，直接观察 LED 并操作按键。想改效果就改 `src/main.c`：

| 想改什么 | 改哪里 |
| --- | --- |
| 按键触发方式（上升/下降沿） | `gpio_pin_interrupt_configure_dt(...)` 的 `GPIO_INT_EDGE_TO_ACTIVE` |
| 按键与 LED 的对应关系 | `button_pressed_cb()` 中 `pins & BIT(buttons[n].pin)` → `gpio_pin_toggle_dt(&leds[m])` |
| 换按键 / LED 引脚 | `boards/gd32f470i_eval.overlay` 中的 `gpios` 属性，改后需 `west build -p always` 重新生成设备树 |
| 加流水灯 / 闪烁等效果 | 在 `while(1)` 中加入逻辑（当前主循环仅 `k_msleep(1000)` 空转） |

---

## 5. 输入输出示例

### 5.1 输入

**运行期没有用户输入接口**：无串口命令、无 Shell、无参数。唯一的「输入」是：

- **上电 / 复位** 本身（触发初始化）；
- **物理按键**：板载 WAKEUP（PA0）/ TAMPER（PC13）/ USER（PB14）任意一次按下。

构建期输入示例：

```powershell
west build -b gd32f470i_eval -d build .
```

| 输入项 | 示例值 | 来源 |
| --- | --- | --- |
| 板型参数 `-b` | `gd32f470i_eval` | 决定加载 `boards/gd32f470i_eval.overlay` |
| 设备树片段 | led0→PE2、led1→PE3、led2→PF10（高）；sw0→PA0、sw1→PC13、sw2→PB14（低） | `boards/gd32f470i_eval.overlay` |
| Kconfig 片段 | `CONFIG_GPIO=y` | `prj.conf` |
| 硬件条件 | 开发板供电正常、SWD 与 ST-Link 连通、按键可正常触发 | 烧录 / 运行阶段 |

### 5.2 输出

**(a) 构建输出**：`build/zephyr/zephyr.hex`、`zephyr.elf`、`zephyr.bin`（体积随工具链版本变化）。

**(b) 运行输出（唯一的可见输出是 LED 电平，由按键中断驱动）**：

| 操作 | 现象 |
| --- | --- |
| 上电 / 复位 | 三颗 LED 全灭（`GPIO_OUTPUT_INACTIVE`） |
| 按 WAKEUP（PA0） | LED1（PE2）翻转（灭↔亮） |
| 按 TAMPER（PC13） | LED2（PE3）翻转 |
| 按 USER（PB14） | LED3（PF10）翻转 |
| 连续按同一键 | 每按一次翻转一次，循环切换亮/灭 |

即「**按某键 → 对应 LED 翻转**」，三键相互独立，主循环不轮询。

**(c) 串口输出：无。** 代码中没有任何 `printk` / `printf`，也未启用 Shell 与日志模块；即使板级 DTS 默认配置了 `zephyr,console = &usart0`（`current-speed = 115200`），串口也收不到任何字符。GPIO 未就绪时 `main()` 返回 `-1`，表现为三灯全灭。

### 5.3 现象对照

| 观察结果 | 说明 |
| --- | --- |
| 按某键，对应 LED 翻转 | 正常：工具链、烧录、设备树、GPIO 中断全部打通 |
| 三灯常灭且按键无反应 | 检查供电/复位、`gpio_is_ready_dt()` 是否失败、overlay 引脚与板载按键是否对应 |
| 按 A 键却翻了 B 灯 | 检查 `button_pressed_cb()` 中的引脚—LED 映射 |
| 构建报找不到 Zephyr | 检查 `ZEPHYR_BASE` 是否设置、`CMakeLists.txt` 要求的 CMake 版本 |
| 烧录失败 | 检查 ST-Link 连接；用 `flash.ps1` 时确认脚本顶部 `$OpenOCD` 路径与你机器上的 Zephyr SDK 安装位置一致 |

---

## 6. 当前边界（避免误解）

- 应用层是「初始化 + 中断回调 + 空转主循环」，没有轮询、没有线程，也没有使用硬件定时器、DMA、PWM 等外设。
- LED 控制完全由按键中断驱动，代码里**没有**流水灯、`app_led_all_on/off/flash` 这类轮询式效果函数。
- 板级 DTS 里存在 PWM、DAC、USART 等节点，只是 Zephyr 板级支持的默认描述，**本项目未使用**。

---

## 7. 许可证

本项目采用 **Apache License 2.0**：

- 根目录 `LICENSE` 为官方全文（202 行，来源 `https://www.apache.org/licenses/LICENSE-2.0.txt`），与 `CMakeLists.txt` 首行的 `# SPDX-License-Identifier: Apache-2.0` 一致。
- `LICENSE` 附录中的 `Copyright [yyyy] [name of copyright owner]` 是 Apache 官方模板占位符，仓库未指定具体版权人；如需声明版权，替换该占位符或新增 `NOTICE` 文件即可。
- 除 `CMakeLists.txt` 的 SPDX 标识外，源文件未逐文件添加许可证头。
