#!/usr/bin/env python3

from __future__ import annotations

import argparse
import socket
import subprocess
import sys
import threading
import time
from pathlib import Path

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    print(
        "[serial] pyserial is not installed.\n"
        "[serial] Install it on the host with:\n"
        "         python -m pip install pyserial",
        file=sys.stderr,
    )
    sys.exit(0)


DEFAULT_TCP_PORT = 3333
DEFAULT_BAUD = 115200

SERIAL_READ_TIMEOUT = 0.05
DISCOVERY_INTERVAL = 1.0
TCP_CHUNK_SIZE = 1024


KNOWN_VIDS = {
    0x067B,  # Prolific / PL2303
    0x0403,  # FTDI
    0x1A86,  # CH340 / CH341
    0x10C4,  # CP210x
    0x2341,  # Arduino
    0x2A03,  # Arduino
}


DESCRIPTION_HINTS = (
    "usb serial",
    "usb-serial",
    "uart",
    "pl2303",
    "prolific",
    "ftdi",
    "ch340",
    "ch341",
    "cp210",
)


def log(message: str) -> None:
    print(f"[serial] {message}", flush=True)


def bridge_is_running(host: str, port: int) -> bool:
    try:
        with socket.create_connection((host, port), timeout=0.3):
            return True
    except OSError:
        return False


def port_score(port) -> int:
    score = 0

    if port.vid in KNOWN_VIDS:
        score += 100

    text = " ".join(
        str(x or "")
        for x in (
            port.description,
            port.manufacturer,
            port.product,
            port.hwid,
        )
    ).lower()

    for hint in DESCRIPTION_HINTS:
        if hint in text:
            score += 10

    if port.vid is not None:
        score += 20

    return score


def find_serial_port(
    requested_vid: int | None,
    requested_pid: int | None,
    requested_serial: str | None,
) -> str | None:
    ports = list(list_ports.comports())
    candidates = []

    for port in ports:
        if requested_vid is not None and port.vid != requested_vid:
            continue

        if requested_pid is not None and port.pid != requested_pid:
            continue

        if requested_serial is not None and port.serial_number != requested_serial:
            continue

        score = port_score(port)

        if (
            requested_vid is None
            and requested_pid is None
            and requested_serial is None
            and score == 0
        ):
            continue

        candidates.append((score, port.device, port))

    if not candidates:
        return None

    candidates.sort(key=lambda x: (-x[0], x[1]))

    best_score = candidates[0][0]
    best = [x for x in candidates if x[0] == best_score]

    if len(best) > 1:
        log("Multiple suitable serial adapters found:")

        for _, device, port in best:
            identity = []

            if port.vid is not None:
                identity.append(f"VID={port.vid:04x}")

            if port.pid is not None:
                identity.append(f"PID={port.pid:04x}")

            if port.serial_number:
                identity.append(f"SN={port.serial_number}")

            extra = f" ({', '.join(identity)})" if identity else ""
            log(f"  {device}: {port.description}{extra}")

        log(f"Using {best[0][1]}")

    return best[0][1]


def open_serial(device: str, baud: int) -> serial.Serial:
    return serial.Serial(
        port=device,
        baudrate=baud,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=SERIAL_READ_TIMEOUT,
        write_timeout=None,
        xonxoff=False,
        rtscts=False,
        dsrdtr=False,
    )


def close_quietly(obj) -> None:
    try:
        obj.close()
    except Exception:
        pass


def shutdown_socket_quietly(sock: socket.socket) -> None:
    try:
        sock.shutdown(socket.SHUT_RDWR)
    except OSError:
        pass


def write_serial_all(ser: serial.Serial, data: bytes) -> None:
    view = memoryview(data)

    while view:
        written = ser.write(view)

        if written is None:
            raise serial.SerialException(
                "serial write returned no byte count"
            )

        if written <= 0:
            raise serial.SerialException(
                "serial write made no progress"
            )

        view = view[written:]


def tcp_to_serial(
    conn: socket.socket,
    ser: serial.Serial,
    stop: threading.Event,
    failures: list[BaseException],
) -> None:
    """
    Container -> physical UART.

    The critical part is ser.flush() after every bounded chunk.
    This applies backpressure so the TCP/PTY path cannot run
    far ahead of the actual 115200-baud UART.
    """

    try:
        while not stop.is_set():
            data = conn.recv(TCP_CHUNK_SIZE)

            if not data:
                break

            write_serial_all(ser, data)

            # Wait until queued serial TX data has actually drained.
            #
            # This makes the flasher's apparent progress much closer
            # to the real physical UART transfer progress.
            ser.flush()

    except (OSError, serial.SerialException) as exc:
        failures.append(exc)

    finally:
        stop.set()
        shutdown_socket_quietly(conn)


def serial_to_tcp(
    ser: serial.Serial,
    conn: socket.socket,
    stop: threading.Event,
    failures: list[BaseException],
) -> None:
    """
    Physical UART -> container.
    """

    try:
        while not stop.is_set():
            data = ser.read(4096)

            if data:
                conn.sendall(data)

    except (OSError, serial.SerialException) as exc:
        failures.append(exc)

    finally:
        stop.set()
        shutdown_socket_quietly(conn)


def handle_connection(
    conn: socket.socket,
    ser: serial.Serial,
) -> bool:
    """
    Handle a single socat/client connection.

    Returns True if the physical serial port appears to have failed.
    """

    conn.setsockopt(
        socket.IPPROTO_TCP,
        socket.TCP_NODELAY,
        1,
    )

    # Keep receive buffering reasonably small so the TCP stack cannot
    # absorb the entire firmware image while the UART is still draining.
    try:
        conn.setsockopt(
            socket.SOL_SOCKET,
            socket.SO_RCVBUF,
            4096,
        )
    except OSError:
        pass

    conn.settimeout(None)

    stop = threading.Event()
    failures: list[BaseException] = []

    tx_thread = threading.Thread(
        target=tcp_to_serial,
        args=(conn, ser, stop, failures),
        daemon=True,
        name="tcp-to-serial",
    )

    rx_thread = threading.Thread(
        target=serial_to_tcp,
        args=(ser, conn, stop, failures),
        daemon=True,
        name="serial-to-tcp",
    )

    tx_thread.start()
    rx_thread.start()

    tx_thread.join()

    stop.set()
    shutdown_socket_quietly(conn)

    rx_thread.join(
        timeout=SERIAL_READ_TIMEOUT * 4 + 0.25
    )

    # One final drain before considering the connection finished.
    try:
        ser.flush()
    except (OSError, serial.SerialException) as exc:
        failures.append(exc)

    return any(
        isinstance(exc, serial.SerialException)
        for exc in failures
    )


def serial_still_present(ser: serial.Serial) -> bool:
    try:
        _ = ser.in_waiting
        return True
    except (OSError, serial.SerialException):
        return False


def run_bridge(args) -> None:
    server = socket.socket(
        socket.AF_INET,
        socket.SOCK_STREAM,
    )

    # Important on Windows:
    # do NOT allow multiple Python bridge instances to bind port 3333.
    if sys.platform == "win32":
        server.setsockopt(
            socket.SOL_SOCKET,
            socket.SO_EXCLUSIVEADDRUSE,
            1,
        )
    else:
        server.setsockopt(
            socket.SOL_SOCKET,
            socket.SO_REUSEADDR,
            1,
        )

    try:
        server.bind((args.bind, args.port))
    except OSError as exc:
        log(
            f"Cannot listen on "
            f"{args.bind}:{args.port}: {exc}"
        )
        return

    server.listen(1)
    server.settimeout(1.0)

    log(
        f"TCP bridge listening on "
        f"{args.bind}:{args.port}"
    )

    current_device: str | None = None
    ser: serial.Serial | None = None

    while True:
        if ser is None:
            device = find_serial_port(
                args.vid,
                args.pid,
                args.serial_number,
            )

            if device is None:
                if current_device is not None:
                    log("Serial adapter disconnected")
                    current_device = None

                time.sleep(DISCOVERY_INTERVAL)
                continue

            try:
                ser = open_serial(
                    device,
                    args.baud,
                )

                current_device = device

                log(
                    f"Opened {device} "
                    f"@ {args.baud} 8N1"
                )

            except (
                OSError,
                serial.SerialException,
            ) as exc:
                log(
                    f"Could not open "
                    f"{device}: {exc}"
                )

                ser = None
                time.sleep(DISCOVERY_INTERVAL)
                continue

        try:
            conn, addr = server.accept()

        except socket.timeout:
            if not serial_still_present(ser):
                log(
                    f"Serial adapter "
                    f"{current_device} disconnected"
                )

                close_quietly(ser)

                ser = None
                current_device = None

            continue

        log(
            f"Client connected from "
            f"{addr[0]}:{addr[1]}"
        )

        serial_failed = False

        try:
            with conn:
                serial_failed = handle_connection(
                    conn,
                    ser,
                )

        except (
            BrokenPipeError,
            ConnectionResetError,
            ConnectionAbortedError,
            OSError,
        ):
            pass

        finally:
            log("Client disconnected")

        if (
            serial_failed
            or not serial_still_present(ser)
        ):
            log(
                f"Serial adapter "
                f"{current_device} disconnected"
            )

            close_quietly(ser)

            ser = None
            current_device = None


def spawn_detached(args) -> None:
    script = str(
        Path(__file__).resolve()
    )

    command = [
        sys.executable,
        script,
        "--daemon",
        "--port",
        str(args.port),
        "--baud",
        str(args.baud),
        "--bind",
        args.bind,
    ]

    if args.vid is not None:
        command += [
            "--vid",
            f"{args.vid:04x}",
        ]

    if args.pid is not None:
        command += [
            "--pid",
            f"{args.pid:04x}",
        ]

    if args.serial_number:
        command += [
            "--serial-number",
            args.serial_number,
        ]

    kwargs = {
        "stdin": subprocess.DEVNULL,
        "stdout": subprocess.DEVNULL,
        "stderr": subprocess.DEVNULL,
        "close_fds": True,
    }

    if sys.platform == "win32":
        kwargs["creationflags"] = (
            subprocess.CREATE_NEW_PROCESS_GROUP
            | subprocess.DETACHED_PROCESS
        )
    else:
        kwargs["start_new_session"] = True

    subprocess.Popen(
        command,
        **kwargs,
    )


def parse_hex(value: str) -> int:
    try:
        return int(value, 16)

    except ValueError:
        raise argparse.ArgumentTypeError(
            f"expected hexadecimal value, "
            f"got {value!r}"
        )


def parse_args():
    parser = argparse.ArgumentParser(
        description=(
            "Low-latency USB serial to TCP "
            "bridge for Docker devcontainers"
        )
    )

    parser.add_argument(
        "--port",
        type=int,
        default=DEFAULT_TCP_PORT,
        help=(
            f"TCP port "
            f"(default: {DEFAULT_TCP_PORT})"
        ),
    )

    parser.add_argument(
        "--bind",
        default="0.0.0.0",
        help=(
            "TCP bind address "
            "(default: 0.0.0.0)"
        ),
    )

    parser.add_argument(
        "--baud",
        type=int,
        default=DEFAULT_BAUD,
        help=(
            f"serial baud rate "
            f"(default: {DEFAULT_BAUD})"
        ),
    )

    parser.add_argument(
        "--vid",
        type=parse_hex,
        help=(
            "USB vendor ID in hex, "
            "e.g. 067b"
        ),
    )

    parser.add_argument(
        "--pid",
        type=parse_hex,
        help=(
            "USB product ID in hex, "
            "e.g. 2303"
        ),
    )

    parser.add_argument(
        "--serial-number",
        help="USB device serial number",
    )

    parser.add_argument(
        "--daemon",
        action="store_true",
        help=argparse.SUPPRESS,
    )

    return parser.parse_args()


def main() -> int:
    args = parse_args()

    if args.daemon:
        run_bridge(args)
        return 0

    # initializeCommand may run multiple times.
    #
    # If a bridge already responds locally, don't spawn another.
    if bridge_is_running(
        "127.0.0.1",
        args.port,
    ):
        log(
            f"Bridge already running "
            f"on TCP port {args.port}"
        )
        return 0

    device = find_serial_port(
        args.vid,
        args.pid,
        args.serial_number,
    )

    if device:
        log(
            f"Found serial adapter: "
            f"{device}"
        )

    else:
        log(
            "No USB serial adapter currently "
            "attached; bridge will wait for one."
        )

    spawn_detached(args)

    for _ in range(20):
        if bridge_is_running(
            "127.0.0.1",
            args.port,
        ):
            log(
                f"Bridge started on "
                f"TCP port {args.port}"
            )
            return 0

        time.sleep(0.05)

    log(
        "Warning: bridge process was started "
        "but TCP port is not responding yet."
    )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())