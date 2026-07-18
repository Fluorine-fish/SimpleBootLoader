# SimpleBootLoader
> 这是一个用于验证的BootLoader工程，由于片内Flash大小限制，预期仅实现IAP基础功能，不做备份恢复分区等
## 目录结构
- Application 应用工程
- BootLoader bootloader工程
- PythonScrip python脚本以供便捷操作IAP
## Flash 分区设置
| 分区 | 地址范围 | 大小 | 页号 | 作用 |
|---|---|---:|---:|---|
| BootLoader | `0x08000000 ~ 0x08005FFF` | 24KB | Page0 ~ Page11 | BootLoader程序 |
| Application | `0x08006000 ~ 0x0801DFFF` | 96KB | Page12 ~ Page59 | Application运行区 |
| Data | `0x0801E000 ~ 0x0801EFFF` | 4KB | Page60 ~ Page61 | 驱动校准数据等 |
| BootInfo | `0x0801F000 ~ 0x0801FFFF` | 4KB | Page62 ~ Page63 | 固件元数据和升级状态 |
## 如何进入Jlink串口
``` shell 
screen /dev/cu.usbmodem0006027126361 115200
```
## IAP 串口指令
| 命令 | 数值 | 请求数据 | 作用 |
|---|---:|---|---|
| `IAP_CMD_GET_INFO` | `0xA1` | 无 | 读取BootInfo |
| `IAP_CMD_ERASE_APP` | `0xA2` | size、crc32、version | 标记升级中并擦除Application |
| `IAP_CMD_WRITE_BLOCK` | `0xA3` | offset、固件数据 | 将一个数据块写入Application |
| `IAP_CMD_VERIFY` | `0xA4` | 无 | 计算CRC32并写入有效BootInfo |
| `IAP_CMD_JUMP_APP` | `0xA5` | 无 | 校验并跳转Application |

## Application
用作在SPIN32G4验证FDriverCore的工程