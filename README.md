# SimpleBootLoader
> 这是一个用于验证的BootLoader工程，由于片内Flash大小限制，预期仅实现IAP基础功能，不做备份恢复分区等
## 目录结构
- Application 应用工程
- BootLoader bootloader工程
## Flash 分区设置
| 分区 | 地址范围 | 大小 | 页号 | 作用 |
|---|---|---:|---:|---|
| BootLoader | `0x08000000 ~ 0x08005FFF` | 24KB | Page0 ~ Page11 | BootLoader程序 |
| Application | `0x08006000 ~ 0x0801DFFF` | 96KB | Page12 ~ Page59 | Application运行区 |
| Data | `0x0801E000 ~ 0x0801EFFF` | 4KB | Page60 ~ Page61 | 驱动校准数据等 |
| BootInfo | `0x0801F000 ~ 0x0801FFFF` | 4KB | Page62 ~ Page63 | 固件元数据和升级状态 |
