#!/usr/bin/env python3
"""Loopback RSP/CPU regression tests; supply a legally obtained CE ROM."""
import argparse
import os
from pathlib import Path
import select
import socket
import subprocess
import tempfile
import time


class RSP:
    def __init__(self, port):
        self.socket = socket.create_connection(("127.0.0.1", port), timeout=5)
        self.no_ack = False

    def byte(self):
        value = self.socket.recv(1)
        if not value:
            raise EOFError("GDB connection closed")
        return value

    def packet(self, payload):
        data = payload.encode("ascii")
        return b"$" + data + b"#" + f"{sum(data) & 255:02x}".encode("ascii")

    def send(self, payload):
        self.socket.sendall(self.packet(payload))
        if not self.no_ack:
            assert self.byte() == b"+", "missing RSP acknowledgment"

    def receive(self):
        assert self.byte() == b"$", "unexpected/unsolicited response"
        data = b""
        while True:
            value = self.byte()
            if value == b"#":
                break
            data += value
        checksum = self.byte() + self.byte()
        assert int(checksum, 16) == sum(data) & 255, "bad reply checksum"
        if not self.no_ack:
            self.socket.sendall(b"+")
        return data.decode("ascii")

    def request(self, payload):
        self.send(payload)
        return self.receive()

    def reg(self, number):
        return int.from_bytes(bytes.fromhex(self.request(f"p{number:x}")), "little")

    def set_reg(self, number, value):
        assert self.request(f"P{number:x}=" + value.to_bytes(3, "little").hex()) == "OK"

    def resume(self, payload):
        self.send(payload)
        stop = self.receive()
        assert stop.startswith("T05"), stop
        return stop

    def close(self):
        self.socket.close()


class Emulator:
    def __init__(self, binary, rom, log):
        with socket.socket() as temporary:
            temporary.bind(("127.0.0.1", 0))
            self.port = temporary.getsockname()[1]
        self.process = subprocess.Popen(
            [binary, "--rom", rom], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=log, text=True, env=dict(os.environ, CEMU_GDB_PORT=str(self.port)))
        assert self.response().startswith("CEMU_HEADLESS_READY")
        self.command("run 6000")
        self.command("key enter")
        self.command("key clear")
        self.process.stdin.write("run-realtime 3600000\n")
        self.process.stdin.flush()

    def response(self):
        ready, _, _ = select.select([self.process.stdout], [], [], 20)
        assert ready, "headless response timed out"
        line = self.process.stdout.readline().strip()
        assert line, f"emulator exited: {self.process.poll()}"
        return line

    def command(self, command):
        self.process.stdin.write(command + "\n")
        self.process.stdin.flush()
        result = self.response()
        assert result.startswith("OK"), result
        return result

    def close(self):
        self.process.kill()
        self.process.wait(timeout=5)
        self.process.stdin.close()
        self.process.stdout.close()


def run(binary, rom):
    with tempfile.TemporaryFile(mode="w+") as log:
        emulator = Emulator(binary, rom, log)
        client = None
        try:
            client = RSP(emulator.port)
            assert "vContSupported+" in client.request("qSupported")
            assert client.request("vCont?") == "vCont;c;C;s;S"
            xml = ""
            while True:
                part = client.request(f"qXfer:features:read:target.xml:{len(xml):x},11")
                xml += part[1:]
                if part[0] == "l":
                    break
            assert "<architecture>ez80-adl</architecture>" in xml
            assert xml.count("<reg ") == 14
            assert client.request("qXfer:features:read:target.xml:0,0") == "E01"
            assert client.request("qXfer:features:read:target.xml:ffffffff,ffffffff") == "l"
            assert client.request("vMustReplyEmpty") == ""
            assert client.request("") == ""
            assert client.request("Hg2") == "E01"
            print("PASS handshake, XML chunks, thread validation, unknown packets", flush=True)

            saved = client.request("g")
            assert len(saved) == 84
            for packet in ("G" + "01" * 41, "G" + "01" * 41 + "zz", "P0=01",
                           "P0=01020304", "P0junk=010203", "p0junk", "p100000000",
                           "mffffff,2", "m1000000,1", "m0,800", "m0,1junk",
                           "M0,2:00", "Z2,ffffff,2", "Z2,d11000,0", "c1000000",
                           "vCont;s:2"):
                assert client.request(packet) == "E01", packet
            assert client.request("g") == saved, "rejected register packet changed CPU state"
            assert client.request("Md11000,3:112233") == "OK"
            assert client.request("Md11000,3:aabbzz") == "E01"
            assert client.request("md11000,3") == "112233"
            print("PASS bounded parsing and atomic rejection of register/memory writes", flush=True)

            # DI; LD HL,123456; LD (D11000),HL; LD DE,(D11000); JP loop.
            # Keep interrupts disabled so stops have deterministic PCs.
            base, data, loop = 0xd10000, 0xd11000, 0xd1000e
            code = "f321563412220010d1ed5b0010d1c30e00d1"
            assert client.request(f"M{base:x},{len(code)//2:x}:{code}") == "OK"
            client.set_reg(5, base)
            client.resume("vCont;s:1;c")
            assert client.reg(5) == base + 1
            client.resume("S05")  # The signal must never be mistaken for a PC.
            assert client.reg(5) == base + 5
            assert client.reg(3) == 0x123456
            print("PASS instruction stepping, vCont selection, signal resume", flush=True)

            assert client.request(f"Z2,{data:x},3") == "OK"
            assert client.request(f"Z2,{data:x},3") == "OK"  # idempotent
            stop = client.resume("vCont;c")
            assert f"watch:{data:x};" in stop, stop
            assert client.reg(5) == base + 9
            assert client.request(f"m{data:x},3") == "563412"
            assert client.request("?") == stop
            assert client.request(f"z2,{data:x},3") == "OK"
            assert client.request(f"Z3,{data:x},3") == "OK"
            stop = client.resume("s")
            assert f"rwatch:{data:x};" in stop, stop
            assert client.reg(5) == loop
            assert client.reg(2) == 0x123456
            assert client.request(f"z3,{data:x},3") == "OK"
            client.set_reg(5, base + 9)
            assert client.request(f"Z4,{data+1:x},2") == "OK"
            stop = client.resume("c")
            assert f"awatch:{data+1:x};" in stop, stop
            assert client.reg(5) == loop
            assert client.request(f"z4,{data+1:x},2") == "OK"
            print("PASS completed-instruction write/read/access watchpoints", flush=True)

            # Repeated block instructions stop after each completed iteration.
            block, destination = 0xd10200, 0xd12000
            assert client.request(f"M{block:x},6:edb0c30202d1") == "OK"
            client.set_reg(3, data)
            client.set_reg(2, destination)
            client.set_reg(1, 3)
            client.set_reg(5, block)
            assert client.request(f"Z2,{destination:x},3") == "OK"
            for count in (2, 1, 0):
                stop = client.resume("c" if count == 2 else "s")
                assert f"watch:{destination:x};" in stop, stop
                assert client.reg(1) == count
                assert client.reg(2) == destination + 3 - count
                assert client.reg(3) == data + 3 - count
                assert client.reg(5) == (block if count else block + 2)
            assert client.request(f"m{destination:x},3") == "563412"
            assert client.request(f"z2,{destination:x},3") == "OK"
            client.set_reg(5, loop)
            print("PASS watchpoints on repeated block instructions", flush=True)

            assert client.request(f"Z0,{loop:x},3") == "OK"
            client.resume("c")
            assert client.reg(5) == loop
            assert client.request(f"z0,{loop:x},3") == "OK"
            # Poke the prefetched instruction. A step must execute the new NOP.
            assert client.request(f"M{loop:x},1:00") == "OK"
            client.resume("s")
            assert client.reg(5) == loop + 1
            assert client.request(f"M{loop:x},4:c30e00d1") == "OK"
            client.set_reg(5, loop)
            print("PASS execution breakpoints and prefetched instruction writes", flush=True)

            # Native RSP stepping of RET does not use GDB's software stepper.
            ret, stack = 0xd10300, 0xd14000
            assert client.request(f"M{ret:x},1:c9") == "OK"
            assert client.request(f"M{stack:x},3:" + loop.to_bytes(3, "little").hex()) == "OK"
            client.set_reg(13, 0xabcd)
            client.set_reg(4, stack)
            assert client.reg(13) == 0xabcd
            client.set_reg(5, ret)
            client.resume("s")
            assert client.reg(5) == loop
            assert client.reg(4) == stack + 3
            print("PASS native RET step and independent stack-pointer writes", flush=True)

            # A corrupt packet is rejected; the connection stays synchronized.
            client.socket.sendall(b"$?#00")
            assert client.byte() == b"-"
            assert client.request("?").startswith("T05")
            client.send("QStartNoAckMode")
            client.no_ack = True
            assert client.receive() == "OK"
            client.socket.sendall(b"+")  # final ACK of the mode transition
            assert client.request("qC") == "QC1"
            client.send("C05")
            client.socket.sendall(b"\x03")
            assert client.receive().startswith("T02")
            assert client.reg(5) == loop
            print("PASS checksum recovery, no-ACK transition, asynchronous interrupt", flush=True)

            # Leave a breakpoint installed, then detach and reconnect in ACK mode.
            assert client.request(f"Z0,{loop:x},3") == "OK"
            assert client.request("D") == "OK"
            client.close()
            client = RSP(emulator.port)
            assert client.request("qC") == "QC1"
            assert client.reg(5) == loop
            assert client.request("D") == "OK"
            client.close()
            # An incomplete request and a reset connection must not open the GUI.
            client = RSP(emulator.port)
            client.socket.sendall(b"$g")
            time.sleep(0.01)
            client.close()
            client = RSP(emulator.port)
            assert client.request("qC") == "QC1"
            client.close()
            client = None
            time.sleep(0.05)
            log.flush()
            log.seek(0)
            diagnostics = log.read()
            assert "[CEmu debug open]" not in diagnostics, "GDB disconnect reached GUI"
            assert "AddressSanitizer" not in diagnostics and "runtime error:" not in diagnostics
            print("PASS detach cleanup and reconnect", flush=True)
        finally:
            if client:
                client.close()
            emulator.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--emulator", required=True)
    parser.add_argument("--rom", required=True)
    args = parser.parse_args()
    run(str(Path(args.emulator).resolve()), str(Path(args.rom).resolve()))
