# STM32G431 IAP上位机

该工具对应当前BootLoader的USART1二进制IAP协议。启动后可以直接输入`U`进入IAP，其他功能使用以`/`开头的命令。程序会自动解析设备输出的文本日志和二进制响应帧。

## 启动

在本目录中执行：

```bash
./run.sh
```

也可以手动激活虚拟环境：

```bash
source .venv/bin/activate
python app.py
```

指定串口启动：

```bash
./run.sh --port /dev/cu.usbmodem0006027126361
```

使用本工具前必须关闭`screen`、CoolTerm等占用同一串口的程序。

## 命令

```text
/ports                         列出串口
/connect [PORT]                连接串口
/disconnect                    断开串口
/status                        查看当前状态
U                              直接发送U进入IAP
/trigger                       发送U进入IAP
/info                          读取BootInfo
/erase BIN [VERSION]           擦除Application并记录升级信息
/write                         写入最近/erase选择的BIN
/verify                        校验CRC32
/jump                          跳转Application
/upload BIN [VERSION]          执行完整升级流程
/raw HEX                       发送原始十六进制数据
/exit                          退出
```

## 读取BootInfo

复位开发板，在BootLoader的5秒等待窗口内进入IAP：

```text
iap> U
[发送] 已发送IAP触发字符'U'
[设备] IAP ready
iap> /info
[设备] [IAP] Start Get Info...
[响应] GET_INFO: OK (0x00), payload=32字节, CRC16=OK
```

如果Application无效，BootLoader会自动停留在IAP，此时可以直接执行`/info`。

## 生成Application.bin

上位机发送的是原始二进制文件，不是ELF文件：

```bash
arm-none-eabi-objcopy \
  -O binary \
  ../Application/build/Debug/Application.elf \
  ../Application/build/Debug/Application.bin
```

## 一键升级

复位开发板后，在5秒等待窗口内执行：

```text
iap> /upload ../Application/build/Debug/Application.bin 1
```

该命令依次执行：

1. 发送`U`触发IAP
2. 计算Application.bin的大小和CRC32
3. 擦除Application分区
4. 按256字节分块写入，末块补`0xFF`到8字节对齐
5. 请求BootLoader校验CRC32
6. 将BootInfo标记为有效
7. 跳转Application

## 分步升级

需要单独观察每个阶段时使用：

```text
iap> /erase ../Application/build/Debug/Application.bin 1
iap> /write
iap> /verify
iap> /info
iap> /jump
```

`/erase`会直接擦除原Application。升级过程中断后，应重新执行`/erase`并从头写入。

## 状态码

| 数值 | 名称 | 含义 |
|---:|---|---|
| `0x00` | `OK` | 执行成功 |
| `0x01` | `BAD_FRAME` | 帧格式错误 |
| `0x02` | `BAD_LENGTH` | 数据长度错误 |
| `0x03` | `BAD_STATE` | 当前未处于可写或可校验状态 |
| `0x04` | `FLASH_ERROR` | Flash擦除或写入失败 |
| `0x05` | `CRC_ERROR` | CRC校验失败 |
| `0x06` | `INVALID_APP` | Application不可启动 |
| `0x07` | `UNKNOWN_COMMAND` | 未知命令 |

## 常见问题

### 等待响应超时

确认以下条件：

- 已经连接正确的`/dev/cu.usbmodem...`端口
- 串口参数为115200、8N1、无流控
- `screen`和CoolTerm已经关闭
- Application有效时，已在复位后的5秒内直接输入`U`、执行`/trigger`或执行`/upload`
- J-Link VCOM的TX连接PA10，RX连接PA9，并且共地

### 日志后出现二进制数据

当前BootLoader会先发送`[IAP] ...`调试文本，再发送`0xA5`开头的二进制响应。本工具会自动分离并解析二者，不需要手动切换十六进制显示。
