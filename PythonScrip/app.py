#!/usr/bin/env python3
"""Interactive host tool for the STM32G431 SimpleBootLoader."""

from __future__ import annotations

import argparse
import cmd
import queue
import shlex
import struct
import sys
import threading
import time
import zlib
from dataclasses import dataclass
from pathlib import Path
from typing import Callable

import serial
from serial.tools import list_ports


REQUEST_SOF = 0x5A
RESPONSE_SOF = 0xA5

CMD_GET_INFO = 0xA1
CMD_ERASE_APP = 0xA2
CMD_WRITE_BLOCK = 0xA3
CMD_VERIFY = 0xA4
CMD_JUMP_APP = 0xA5

STATUS_OK = 0x00
BLOCK_SIZE = 256
MAX_APP_SIZE = 96 * 1024
MAX_RESPONSE_PAYLOAD = 4096

BOOTINFO_MAGIC = 0x434C4931
BOOTINFO_STATE_UPDATING = 0x55504454
BOOTINFO_STATE_VALID = 0x56414C44

COMMAND_NAMES = {
    CMD_GET_INFO: "GET_INFO",
    CMD_ERASE_APP: "ERASE_APP",
    CMD_WRITE_BLOCK: "WRITE_BLOCK",
    CMD_VERIFY: "VERIFY",
    CMD_JUMP_APP: "JUMP_APP",
}

STATUS_NAMES = {
    0x00: "OK",
    0x01: "BAD_FRAME",
    0x02: "BAD_LENGTH",
    0x03: "BAD_STATE",
    0x04: "FLASH_ERROR",
    0x05: "CRC_ERROR",
    0x06: "INVALID_APP",
    0x07: "UNKNOWN_COMMAND",
}

STATE_NAMES = {
    BOOTINFO_STATE_UPDATING: "UPDATING",
    BOOTINFO_STATE_VALID: "VALID",
    0xFFFFFFFF: "ERASED",
}

_PRINT_LOCK = threading.Lock()


def console_print(message: str = "") -> None:
    with _PRINT_LOCK:
        print(message, flush=True)


def crc16_ccitt(data: bytes) -> int:
    """CRC-16/CCITT-FALSE used by the BootLoader UART protocol."""
    crc = 0xFFFF
    for value in data:
        crc ^= value << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


def build_request_frame(command: int, payload: bytes = b"") -> bytes:
    if not 0 <= command <= 0xFF:
        raise ValueError("command must fit in one byte")
    if len(payload) > 0xFFFF:
        raise ValueError("payload is too large")

    body = bytes([command]) + struct.pack("<H", len(payload)) + payload
    return bytes([REQUEST_SOF]) + body + struct.pack("<H", crc16_ccitt(body))


@dataclass(frozen=True)
class Response:
    command: int
    status: int
    payload: bytes
    received_crc: int
    calculated_crc: int

    @property
    def crc_valid(self) -> bool:
        return self.received_crc == self.calculated_crc


@dataclass(frozen=True)
class BootInfo:
    magic: int
    state: int
    app_addr: int
    app_size: int
    app_crc32: int
    app_version: int
    reserved0: int
    reserved1: int

    @classmethod
    def from_payload(cls, payload: bytes) -> "BootInfo":
        if len(payload) != 32:
            raise ValueError(f"BootInfo应为32字节，实际收到{len(payload)}字节")
        return cls(*struct.unpack("<8I", payload))

    def format(self) -> str:
        magic_state = "MATCH" if self.magic == BOOTINFO_MAGIC else "MISMATCH"
        state_name = STATE_NAMES.get(self.state, "UNKNOWN")
        return "\n".join(
            [
                "BootInfo:",
                f"  magic       : 0x{self.magic:08X} ({magic_state})",
                f"  state       : 0x{self.state:08X} ({state_name})",
                f"  app_addr    : 0x{self.app_addr:08X}",
                f"  app_size    : {self.app_size} bytes",
                f"  app_crc32   : 0x{self.app_crc32:08X}",
                f"  app_version : {self.app_version}",
                f"  reserved    : 0x{self.reserved0:08X}, 0x{self.reserved1:08X}",
            ]
        )


@dataclass(frozen=True)
class FirmwareImage:
    path: Path
    data: bytes
    crc32: int

    @classmethod
    def load(cls, path_text: str) -> "FirmwareImage":
        path = Path(path_text).expanduser().resolve()
        data = path.read_bytes()
        if not data:
            raise ValueError("固件文件为空")
        if len(data) > MAX_APP_SIZE:
            raise ValueError(
                f"固件大小为{len(data)}字节，超过Application分区上限{MAX_APP_SIZE}字节"
            )
        return cls(path=path, data=data, crc32=zlib.crc32(data) & 0xFFFFFFFF)

    @property
    def padded_data(self) -> bytes:
        return self.data + b"\xFF" * ((-len(self.data)) % 8)


class ResponseStreamParser:
    """Separates BootLoader text logs from binary response frames."""

    def __init__(
        self,
        on_response: Callable[[Response], None],
        on_log: Callable[[str], None],
    ) -> None:
        self._on_response = on_response
        self._on_log = on_log
        self._buffer = bytearray()
        self._log_buffer = bytearray()

    def feed(self, data: bytes) -> None:
        self._buffer.extend(data)

        while self._buffer:
            sof_index = self._buffer.find(bytes([RESPONSE_SOF]))
            if sof_index < 0:
                self._feed_log(bytes(self._buffer))
                self._buffer.clear()
                return

            if sof_index > 0:
                self._feed_log(bytes(self._buffer[:sof_index]))
                del self._buffer[:sof_index]

            if len(self._buffer) < 5:
                return

            payload_length = struct.unpack_from("<H", self._buffer, 3)[0]
            if payload_length > MAX_RESPONSE_PAYLOAD:
                self._feed_log(bytes([self._buffer[0]]))
                del self._buffer[0]
                continue

            frame_length = 1 + 4 + payload_length + 2
            if len(self._buffer) < frame_length:
                return

            frame = bytes(self._buffer[:frame_length])
            del self._buffer[:frame_length]

            header = frame[1:5]
            payload = frame[5 : 5 + payload_length]
            received_crc = struct.unpack_from("<H", frame, 5 + payload_length)[0]
            response = Response(
                command=header[0],
                status=header[1],
                payload=payload,
                received_crc=received_crc,
                calculated_crc=crc16_ccitt(header + payload),
            )
            self._on_response(response)

    def flush_log(self) -> None:
        if self._buffer:
            self._feed_log(bytes(self._buffer))
            self._buffer.clear()
        if self._log_buffer:
            self._emit_log_line(bytes(self._log_buffer))
            self._log_buffer.clear()

    def _feed_log(self, data: bytes) -> None:
        self._log_buffer.extend(data)
        while True:
            newline_index = self._log_buffer.find(b"\n")
            if newline_index < 0:
                break
            line = bytes(self._log_buffer[:newline_index])
            del self._log_buffer[: newline_index + 1]
            self._emit_log_line(line)

        if len(self._log_buffer) > 1024:
            self._emit_log_line(bytes(self._log_buffer))
            self._log_buffer.clear()

    def _emit_log_line(self, line: bytes) -> None:
        text = line.rstrip(b"\r").decode("utf-8", errors="replace")
        if text:
            self._on_log(text)


class ReaderFailure(RuntimeError):
    pass


class SerialClient:
    def __init__(self, baudrate: int = 115200) -> None:
        self.baudrate = baudrate
        self._port: serial.Serial | None = None
        self._reader_thread: threading.Thread | None = None
        self._stop_event = threading.Event()
        self._responses: queue.Queue[Response | ReaderFailure] = queue.Queue()
        self._write_lock = threading.Lock()
        self._parser = ResponseStreamParser(self._responses.put, self._print_device_log)

    @property
    def connected(self) -> bool:
        return self._port is not None and self._port.is_open

    @property
    def port_name(self) -> str | None:
        return self._port.port if self._port is not None else None

    def connect(self, port_name: str) -> None:
        self.disconnect()
        self._responses = queue.Queue()
        self._parser = ResponseStreamParser(self._responses.put, self._print_device_log)
        self._stop_event.clear()
        self._port = serial.Serial(
            port=port_name,
            baudrate=self.baudrate,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=0.1,
            write_timeout=2.0,
            xonxoff=False,
            rtscts=False,
            dsrdtr=False,
        )
        self._reader_thread = threading.Thread(
            target=self._reader_loop,
            name="iap-serial-reader",
            daemon=True,
        )
        self._reader_thread.start()

    def disconnect(self) -> None:
        self._stop_event.set()
        port = self._port
        self._port = None
        if port is not None:
            try:
                port.close()
            except serial.SerialException:
                pass
        if self._reader_thread is not None:
            self._reader_thread.join(timeout=1.0)
            self._reader_thread = None
        self._parser.flush_log()

    def send_trigger(self) -> None:
        self._write(b"U")

    def send_raw(self, data: bytes) -> None:
        self._write(data)

    def request(
        self,
        command: int,
        payload: bytes = b"",
        timeout: float = 5.0,
    ) -> Response:
        self._discard_stale_responses()
        self._write(build_request_frame(command, payload))
        deadline = time.monotonic() + timeout

        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise TimeoutError(
                    "等待设备响应超时；确认设备已经进入IAP，且串口未被其他程序占用"
                )
            try:
                item = self._responses.get(timeout=remaining)
            except queue.Empty as exc:
                raise TimeoutError(
                    "等待设备响应超时；确认设备已经进入IAP，且串口未被其他程序占用"
                ) from exc

            if isinstance(item, ReaderFailure):
                raise item
            if item.command != command:
                console_print(
                    f"[警告] 忽略非预期响应0x{item.command:02X}，"
                    f"当前等待0x{command:02X}"
                )
                continue
            if not item.crc_valid:
                raise RuntimeError(
                    f"响应CRC16错误：收到0x{item.received_crc:04X}，"
                    f"计算值0x{item.calculated_crc:04X}"
                )
            return item

    def _write(self, data: bytes) -> None:
        port = self._port
        if port is None or not port.is_open:
            raise RuntimeError("串口未连接，请先使用/connect命令")
        with self._write_lock:
            port.write(data)
            port.flush()

    def _discard_stale_responses(self) -> None:
        while True:
            try:
                self._responses.get_nowait()
            except queue.Empty:
                return

    def _reader_loop(self) -> None:
        port = self._port
        if port is None:
            return
        try:
            while not self._stop_event.is_set():
                data = port.read(256)
                if data:
                    self._parser.feed(data)
        except (serial.SerialException, OSError) as exc:
            if not self._stop_event.is_set():
                self._responses.put(ReaderFailure(f"串口读取失败：{exc}"))
                console_print(f"\n[串口] 读取失败：{exc}")

    @staticmethod
    def _print_device_log(line: str) -> None:
        console_print(f"[设备] {line}")


def available_ports() -> list[list_ports.ListPortInfo]:
    return sorted(list_ports.comports(), key=lambda item: item.device)


def choose_default_port() -> str | None:
    ports = available_ports()
    preferred = []
    for port in ports:
        description = " ".join(
            filter(
                None,
                [
                    port.device,
                    port.description,
                    port.manufacturer,
                    port.product,
                    port.interface,
                ],
            )
        ).lower()
        if port.device.startswith("/dev/cu.") and (
            "usbmodem" in description or "j-link" in description or "segger" in description
        ):
            preferred.append(port.device)

    if len(preferred) == 1:
        return preferred[0]
    if len(ports) == 1:
        return ports[0].device
    return None


class IapShell(cmd.Cmd):
    prompt = "iap> "

    def __init__(self, baudrate: int = 115200) -> None:
        super().__init__()
        self.client = SerialClient(baudrate=baudrate)
        self.firmware: FirmwareImage | None = None
        self.version = 1

    def onecmd(self, line: str) -> bool | None:
        stripped = line.strip()
        if not stripped:
            return False
        if stripped == "EOF":
            return self.do_exit("")
        if stripped in ("U", "u"):
            self.do_trigger("")
            return False
        if not stripped.startswith("/"):
            console_print("直接输入U进入IAP；其他命令必须以/开头，输入/help查看帮助")
            return False
        return super().onecmd(stripped[1:])

    def emptyline(self) -> None:
        return None

    def do_help(self, arg: str) -> None:
        """显示命令帮助。"""
        del arg
        console_print(
            "\n".join(
                [
                    "可用命令：",
                    "  /ports                         列出串口",
                    "  /connect [PORT]                连接串口；省略PORT时自动选择J-Link VCOM",
                    "  /disconnect                    断开串口",
                    "  /status                        查看当前连接和固件状态",
                    "  U                              直接发送U进入IAP",
                    "  /trigger                       发送U进入IAP（复位后5秒内执行）",
                    "  /info                          读取并解析BootInfo",
                    "  /erase BIN [VERSION]           保存升级信息并擦除Application",
                    "  /write                         写入最近/erase选择的BIN",
                    "  /verify                        校验固件CRC32并标记有效",
                    "  /jump                          跳转Application",
                    "  /upload BIN [VERSION]          一键触发、擦除、写入、校验并跳转",
                    "  /raw HEX                       发送原始十六进制字节",
                    "  /exit                          退出",
                ]
            )
        )

    def do_ports(self, arg: str) -> None:
        """列出系统串口。"""
        del arg
        ports = available_ports()
        if not ports:
            console_print("未发现串口")
            return
        for port in ports:
            manufacturer = port.manufacturer or "未知厂商"
            console_print(f"{port.device}  {port.description}  {manufacturer}")

    def do_connect(self, arg: str) -> None:
        """连接串口。"""
        try:
            args = shlex.split(arg)
            if len(args) > 1:
                raise ValueError("用法：/connect [PORT]")
            port_name = args[0] if args else choose_default_port()
            if port_name is None:
                raise RuntimeError("无法唯一确定串口，请先使用/ports，再指定/connect PORT")
            self.client.connect(port_name)
            console_print(f"[串口] 已连接 {port_name} @ {self.client.baudrate} 8N1")
        except Exception as exc:
            self._print_error(exc)

    def do_disconnect(self, arg: str) -> None:
        """断开串口。"""
        del arg
        old_port = self.client.port_name
        self.client.disconnect()
        console_print(f"[串口] 已断开 {old_port or ''}".rstrip())

    def do_status(self, arg: str) -> None:
        """显示当前状态。"""
        del arg
        connection = self.client.port_name if self.client.connected else "未连接"
        console_print(f"串口：{connection}")
        if self.firmware is None:
            console_print("固件：未选择")
        else:
            console_print(
                f"固件：{self.firmware.path}，{len(self.firmware.data)}字节，"
                f"CRC32=0x{self.firmware.crc32:08X}，版本={self.version}"
            )

    def do_trigger(self, arg: str) -> None:
        """发送IAP触发字符。"""
        del arg
        try:
            self.client.send_trigger()
            console_print("[发送] 已发送IAP触发字符'U'")
        except Exception as exc:
            self._print_error(exc)

    def do_info(self, arg: str) -> None:
        """读取BootInfo。"""
        del arg
        try:
            response = self._request_ok(CMD_GET_INFO)
            console_print(BootInfo.from_payload(response.payload).format())
        except Exception as exc:
            self._print_error(exc)

    def do_erase(self, arg: str) -> None:
        """设置固件元数据并擦除Application。"""
        try:
            firmware, version = self._parse_firmware_args(arg)
            self.firmware = firmware
            self.version = version
            self._print_firmware(firmware, version)
            payload = struct.pack(
                "<III", len(firmware.data), firmware.crc32, version
            )
            console_print("[升级] 正在擦除Application...")
            self._request_ok(CMD_ERASE_APP, payload, timeout=20.0)
        except Exception as exc:
            self._print_error(exc)

    def do_write(self, arg: str) -> None:
        """写入最近选择的固件。"""
        try:
            if arg.strip():
                raise ValueError("用法：/write；请先使用/erase BIN [VERSION]")
            if self.firmware is None:
                raise RuntimeError("尚未选择固件，请先使用/erase BIN [VERSION]")
            self._write_firmware(self.firmware)
        except Exception as exc:
            self._print_error(exc)

    def do_verify(self, arg: str) -> None:
        """请求设备校验Application CRC32。"""
        del arg
        try:
            console_print("[升级] 正在校验CRC32...")
            self._request_ok(CMD_VERIFY, timeout=15.0)
        except Exception as exc:
            self._print_error(exc)

    def do_jump(self, arg: str) -> None:
        """跳转Application。"""
        del arg
        try:
            console_print("[升级] 请求跳转Application...")
            self._request_ok(CMD_JUMP_APP)
        except Exception as exc:
            self._print_error(exc)

    def do_upload(self, arg: str) -> None:
        """执行完整IAP升级流程。"""
        try:
            firmware, version = self._parse_firmware_args(arg)
            self.firmware = firmware
            self.version = version
            self._print_firmware(firmware, version)

            self.client.send_trigger()
            console_print("[升级] 已发送触发字符；设备必须处于复位后的5秒等待窗口或IAP中")
            time.sleep(0.2)

            erase_payload = struct.pack(
                "<III", len(firmware.data), firmware.crc32, version
            )
            console_print("[升级] 正在擦除Application...")
            self._request_ok(CMD_ERASE_APP, erase_payload, timeout=20.0)
            self._write_firmware(firmware)
            console_print("[升级] 正在校验CRC32...")
            self._request_ok(CMD_VERIFY, timeout=15.0)
            console_print("[升级] 校验通过，正在跳转Application...")
            self._request_ok(CMD_JUMP_APP)
            console_print("[升级] 完成")
        except Exception as exc:
            self._print_error(exc)

    def do_raw(self, arg: str) -> None:
        """发送原始十六进制数据。"""
        try:
            compact = "".join(shlex.split(arg)).replace("0x", "").replace("0X", "")
            if not compact:
                raise ValueError("用法：/raw 5A A1 00 00 30 46")
            data = bytes.fromhex(compact)
            self.client.send_raw(data)
            console_print(f"[发送] {data.hex(' ').upper()}")
        except Exception as exc:
            self._print_error(exc)

    def do_exit(self, arg: str) -> bool:
        """退出程序。"""
        del arg
        self.client.disconnect()
        console_print("已退出")
        return True

    do_quit = do_exit

    def _request_ok(
        self,
        command: int,
        payload: bytes = b"",
        timeout: float = 5.0,
    ) -> Response:
        response = self.client.request(command, payload, timeout)
        command_name = COMMAND_NAMES.get(command, f"0x{command:02X}")
        status_name = STATUS_NAMES.get(response.status, "UNKNOWN_STATUS")
        console_print(
            f"[响应] {command_name}: {status_name} (0x{response.status:02X}), "
            f"payload={len(response.payload)}字节, CRC16=OK"
        )
        if response.status != STATUS_OK:
            raise RuntimeError(
                f"设备拒绝{command_name}：{status_name} (0x{response.status:02X})"
            )
        return response

    def _write_firmware(self, firmware: FirmwareImage) -> None:
        padded = firmware.padded_data
        total_blocks = (len(padded) + BLOCK_SIZE - 1) // BLOCK_SIZE
        console_print(f"[升级] 开始写入，共{total_blocks}个数据块")

        for block_number, offset in enumerate(
            range(0, len(padded), BLOCK_SIZE), start=1
        ):
            block = padded[offset : offset + BLOCK_SIZE]
            payload = struct.pack("<I", offset) + block
            self._request_ok(CMD_WRITE_BLOCK, payload, timeout=8.0)
            console_print(
                f"[进度] {block_number}/{total_blocks} "
                f"({min(offset + len(block), len(firmware.data))}/{len(firmware.data)}字节)"
            )

    @staticmethod
    def _parse_firmware_args(arg: str) -> tuple[FirmwareImage, int]:
        args = shlex.split(arg)
        if not 1 <= len(args) <= 2:
            raise ValueError("用法：命令 BIN [VERSION]")
        version = int(args[1], 0) if len(args) == 2 else 1
        if not 0 <= version <= 0xFFFFFFFF:
            raise ValueError("VERSION必须在0到0xFFFFFFFF之间")
        return FirmwareImage.load(args[0]), version

    @staticmethod
    def _print_firmware(firmware: FirmwareImage, version: int) -> None:
        console_print(f"固件路径   : {firmware.path}")
        console_print(f"固件大小   : {len(firmware.data)} bytes")
        console_print(f"固件CRC32  : 0x{firmware.crc32:08X}")
        console_print(f"固件版本   : {version}")

    @staticmethod
    def _print_error(exc: Exception) -> None:
        console_print(f"[错误] {exc}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="STM32G431 SimpleBootLoader交互式IAP上位机"
    )
    parser.add_argument("--port", help="启动时连接的串口；省略时尝试自动选择")
    parser.add_argument("--baudrate", type=int, default=115200)
    parser.add_argument(
        "--no-auto-connect",
        action="store_true",
        help="启动后不自动连接串口",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    shell = IapShell(baudrate=args.baudrate)

    console_print("STM32G431 SimpleBootLoader IAP Terminal")
    console_print("直接输入U进入IAP；输入/help查看其他命令")
    console_print("使用本工具前请关闭screen、CoolTerm等串口程序")

    if not args.no_auto_connect:
        port_name = args.port or choose_default_port()
        if port_name is not None:
            shell.do_connect(shlex.quote(port_name))
        else:
            console_print("未自动连接串口，请使用/ports和/connect")

    try:
        shell.cmdloop()
    except KeyboardInterrupt:
        console_print("\n收到Ctrl-C，正在退出")
    finally:
        shell.client.disconnect()
    return 0


if __name__ == "__main__":
    sys.exit(main())
