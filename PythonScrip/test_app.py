import struct
import tempfile
import unittest
import zlib
from pathlib import Path
from unittest.mock import Mock

from app import (
    BOOTINFO_MAGIC,
    BOOTINFO_STATE_VALID,
    BootInfo,
    CMD_GET_INFO,
    FirmwareImage,
    IapShell,
    ResponseStreamParser,
    build_request_frame,
    crc16_ccitt,
)


def build_response(command: int, status: int, payload: bytes = b"") -> bytes:
    body = bytes([command, status]) + struct.pack("<H", len(payload)) + payload
    return b"\xA5" + body + struct.pack("<H", crc16_ccitt(body))


class ProtocolTests(unittest.TestCase):
    def test_get_info_request_matches_firmware_protocol(self) -> None:
        self.assertEqual(
            build_request_frame(CMD_GET_INFO),
            bytes.fromhex("5A A1 00 00 30 46"),
        )

    def test_stream_parser_separates_logs_and_fragmented_response(self) -> None:
        responses = []
        logs = []
        parser = ResponseStreamParser(responses.append, logs.append)
        frame = build_response(CMD_GET_INFO, 0, b"abc")

        parser.feed(b"[IAP] Start Get Info...\r\n" + frame[:3])
        parser.feed(frame[3:])

        self.assertEqual(logs, ["[IAP] Start Get Info..."])
        self.assertEqual(len(responses), 1)
        self.assertEqual(responses[0].command, CMD_GET_INFO)
        self.assertEqual(responses[0].payload, b"abc")
        self.assertTrue(responses[0].crc_valid)

    def test_boot_info_payload_is_little_endian(self) -> None:
        payload = struct.pack(
            "<8I",
            BOOTINFO_MAGIC,
            BOOTINFO_STATE_VALID,
            0x08006000,
            1234,
            0x89ABCDEF,
            7,
            0xFFFFFFFF,
            0xFFFFFFFF,
        )
        info = BootInfo.from_payload(payload)
        self.assertEqual(info.app_addr, 0x08006000)
        self.assertEqual(info.app_size, 1234)
        self.assertEqual(info.app_version, 7)


class FirmwareTests(unittest.TestCase):
    def test_firmware_crc_and_padding(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "Application.bin"
            path.write_bytes(b"123456789")
            image = FirmwareImage.load(str(path))

        self.assertEqual(image.crc32, zlib.crc32(b"123456789") & 0xFFFFFFFF)
        self.assertEqual(len(image.padded_data), 16)
        self.assertEqual(image.padded_data[9:], b"\xFF" * 7)


class ShellTests(unittest.TestCase):
    def test_plain_u_sends_iap_trigger(self) -> None:
        shell = IapShell()
        shell.client.send_trigger = Mock()

        result = shell.onecmd("U")

        self.assertFalse(result)
        shell.client.send_trigger.assert_called_once_with()


if __name__ == "__main__":
    unittest.main()
