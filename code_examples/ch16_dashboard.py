#!/usr/bin/env python3
"""
ch16_dashboard.py — STM32 v1.6 PC 대시보드 (완성 템플릿)
================================================
대상: STM32 NUCLEO-F411RE + Ch16 protocol_app.c

프레임 구조:
  [STX 0xAA][TYPE 1B][LEN 1B][PAYLOAD nB][CRC16_HI 1B][CRC16_LO 1B][ETX 0x55]

CRC-16/CCITT: 다항식 0x1021, 초기값 0xFFFF
CRC 계산 범위: TYPE + LEN + PAYLOAD

사용 방법:
  1. pip install pyserial PyQt5 pyqtgraph
  2. COM 포트 번호 확인 (장치 관리자 → COM & LPT 포트)
  3. python ch16_dashboard.py --port COM3 --baud 115200

학습 포인트 (코드 분석):
  - SerialReader: QThread 기반 수신 스레드 → GUI 스레드와 분리
  - FrameParser: 바이트 스트림 → 프레임 파싱 (STX 탐색 → 길이 수집 → ETX 확인)
  - CRC 검증: MCU 측과 동일 알고리즘으로 무결성 확인
  - PyQtGraph: 실시간 스크롤 그래프 (배터리 소모 없는 고속 렌더링)
"""

import sys
import struct
import threading
import argparse
from collections import deque

import serial
from PyQt5.QtWidgets import (QApplication, QMainWindow, QWidget,
                              QVBoxLayout, QHBoxLayout, QLabel, QGroupBox)
from PyQt5.QtCore import QThread, pyqtSignal, Qt, QTimer
from PyQt5.QtGui import QFont
import pyqtgraph as pg

# ─────────────────────────────────────────────────
# 프로토콜 상수 (MCU protocol_app.h 와 동일)
# ─────────────────────────────────────────────────
PROTO_STX           = 0xAA
PROTO_ETX           = 0x55
PROTO_MAX_PAYLOAD   = 32
PROTO_FRAME_OVERHEAD = 6  # STX + TYPE + LEN + CRC16(2) + ETX

TYPE_SENSOR_DATA    = 0x01
TYPE_TIME_DATA      = 0x02
TYPE_MOTOR_STATUS   = 0x03
TYPE_SYSTEM_STATUS  = 0x04
TYPE_ACK            = 0x05

# ─────────────────────────────────────────────────
# CRC-16/CCITT 계산 (MCU Protocol_CalcCRC16 와 동일)
# ─────────────────────────────────────────────────
def calc_crc16(data: bytes) -> int:
    """CRC-16/CCITT: 다항식 0x1021, 초기값 0xFFFF"""
    crc = 0xFFFF
    for byte in data:
        crc ^= (byte << 8)
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


# ─────────────────────────────────────────────────
# 프레임 파서 (바이트 스트림 → 구조체)
# ─────────────────────────────────────────────────
class FrameParser:
    """
    STX 탐색 → TYPE/LEN 수집 → PAYLOAD 수집 → CRC 검증 → ETX 확인
    상태 머신 방식으로 동작 (불완전 수신 대응)
    """
    STATE_HUNT_STX  = 0  # STX 탐색 중
    STATE_HEADER    = 1  # TYPE + LEN 수신 대기
    STATE_PAYLOAD   = 2  # PAYLOAD 수신 중
    STATE_CRC       = 3  # CRC 2바이트 수신
    STATE_ETX       = 4  # ETX 확인

    def __init__(self):
        self._state = self.STATE_HUNT_STX
        self._buf = bytearray()
        self._payload_len = 0
        self._frames_ok = 0
        self._frames_err = 0

    def feed(self, byte: int):
        """
        1바이트씩 입력 받아 완성된 프레임 반환.
        완성 시 dict 반환, 미완성/오류 시 None 반환.
        """
        if self._state == self.STATE_HUNT_STX:
            if byte == PROTO_STX:
                self._buf = bytearray([byte])
                self._state = self.STATE_HEADER
            return None

        elif self._state == self.STATE_HEADER:
            self._buf.append(byte)
            if len(self._buf) == 3:  # STX + TYPE + LEN
                self._payload_len = self._buf[2]
                if self._payload_len > PROTO_MAX_PAYLOAD:
                    # 잘못된 LEN — 재동기
                    self._state = self.STATE_HUNT_STX
                    return None
                self._state = (self.STATE_PAYLOAD
                               if self._payload_len > 0 else self.STATE_CRC)
            return None

        elif self._state == self.STATE_PAYLOAD:
            self._buf.append(byte)
            # STX(1) + TYPE(1) + LEN(1) + PAYLOAD(n) = 3 + n
            if len(self._buf) == 3 + self._payload_len:
                self._state = self.STATE_CRC
            return None

        elif self._state == self.STATE_CRC:
            self._buf.append(byte)
            if len(self._buf) == 3 + self._payload_len + 2:  # +CRC(2)
                self._state = self.STATE_ETX
            return None

        elif self._state == self.STATE_ETX:
            self._buf.append(byte)
            self._state = self.STATE_HUNT_STX

            if byte != PROTO_ETX:
                self._frames_err += 1
                return None

            # CRC 검증: TYPE + LEN + PAYLOAD 범위
            frame = bytes(self._buf)
            crc_range = frame[1 : 3 + self._payload_len]
            crc_calc  = calc_crc16(crc_range)
            crc_recv  = (frame[-3] << 8) | frame[-2]

            if crc_calc != crc_recv:
                self._frames_err += 1
                print(f"[CRC ERR] calc=0x{crc_calc:04X} recv=0x{crc_recv:04X}")
                return None

            self._frames_ok += 1
            payload = frame[3 : 3 + self._payload_len]
            return {
                "type"    : frame[1],
                "payload" : payload,
                "crc"     : crc_recv,
            }
        return None

    @property
    def stats(self):
        return {"ok": self._frames_ok, "err": self._frames_err}


# ─────────────────────────────────────────────────
# 시리얼 수신 스레드 (QThread)
# ─────────────────────────────────────────────────
class SerialReader(QThread):
    """
    별도 스레드에서 pyserial 수신 — GUI 스레드를 블로킹하지 않음.
    완성된 프레임을 Qt 시그널(frame_received)로 Main Thread에 전달.
    """
    frame_received = pyqtSignal(dict)   # 완성 프레임
    status_msg     = pyqtSignal(str)    # 상태 메시지

    def __init__(self, port: str, baud: int):
        super().__init__()
        self._port   = port
        self._baud   = baud
        self._running = False
        self._parser  = FrameParser()

    def run(self):
        """스레드 진입점 — pyserial 수신 루프"""
        self._running = True
        try:
            with serial.Serial(self._port, self._baud, timeout=0.1) as ser:
                self.status_msg.emit(f"연결됨: {self._port} @ {self._baud}bps")
                while self._running:
                    data = ser.read(64)  # 최대 64바이트씩 읽기
                    for byte in data:
                        frame = self._parser.feed(byte)
                        if frame is not None:
                            self.frame_received.emit(frame)
        except serial.SerialException as e:
            self.status_msg.emit(f"시리얼 오류: {e}")

    def stop(self):
        self._running = False
        self.wait()


# ─────────────────────────────────────────────────
# 메인 대시보드 윈도우 (PyQt5)
# ─────────────────────────────────────────────────
class Dashboard(QMainWindow):
    GRAPH_HISTORY = 60  # 그래프 표시 데이터 포인트 수 (1초 × 60 = 1분)

    def __init__(self, port: str, baud: int):
        super().__init__()
        self.setWindowTitle(f"STM32 v1.6 대시보드 — {port}")
        self.setMinimumSize(900, 600)

        # 데이터 버퍼
        self._temp_buf  = deque([0.0] * self.GRAPH_HISTORY, maxlen=self.GRAPH_HISTORY)
        self._humi_buf  = deque([0.0] * self.GRAPH_HISTORY, maxlen=self.GRAPH_HISTORY)
        self._frame_cnt = 0

        self._build_ui()

        # 시리얼 수신 스레드 시작
        self._reader = SerialReader(port, baud)
        self._reader.frame_received.connect(self._on_frame)
        self._reader.status_msg.connect(self._status_bar.setText)
        self._reader.start()

        # 그래프 갱신 타이머 (100ms)
        self._timer = QTimer()
        self._timer.timeout.connect(self._update_graph)
        self._timer.start(100)

    # ── UI 구성 ──────────────────────────────────
    def _build_ui(self):
        central = QWidget()
        self.setCentralWidget(central)
        layout = QVBoxLayout(central)

        # 상단: 수치 표시
        top = QHBoxLayout()

        self._lbl_temp  = self._make_value_label("-- °C",  "#EA4335")
        self._lbl_humi  = self._make_value_label("-- %RH", "#1A73E8")
        self._lbl_time  = self._make_value_label("--:--:--", "#34A853")
        self._lbl_motor = self._make_value_label("Motor: --°", "#FBBC04")

        for title, widget in [("온도", self._lbl_temp), ("습도", self._lbl_humi),
                               ("시각", self._lbl_time), ("모터", self._lbl_motor)]:
            grp = QGroupBox(title)
            grp.setLayout(QVBoxLayout())
            grp.layout().addWidget(widget)
            top.addWidget(grp)

        layout.addLayout(top)

        # 중단: 온습도 실시간 그래프
        pg.setConfigOption("background", "w")
        pg.setConfigOption("foreground", "k")

        self._plot = pg.PlotWidget(title="온도 / 습도 추이 (최근 60초)")
        self._plot.addLegend()
        self._plot.setLabel("left",   "값")
        self._plot.setLabel("bottom", "시간 (초)")
        self._curve_temp = self._plot.plot(pen=pg.mkPen("#EA4335", width=2), name="온도 (°C)")
        self._curve_humi = self._plot.plot(pen=pg.mkPen("#1A73E8", width=2), name="습도 (%RH)")
        layout.addWidget(self._plot)

        # 하단: 상태 바
        self._status_bar = QLabel("대기 중...")
        self._status_bar.setStyleSheet("color: gray; font-size: 11px;")
        layout.addWidget(self._status_bar)

    def _make_value_label(self, text: str, color: str) -> QLabel:
        lbl = QLabel(text)
        lbl.setAlignment(Qt.AlignCenter)
        lbl.setFont(QFont("Arial", 22, QFont.Bold))
        lbl.setStyleSheet(f"color: {color};")
        return lbl

    # ── 프레임 수신 처리 ─────────────────────────
    def _on_frame(self, frame: dict):
        """Qt 메인 스레드에서 호출 (시그널-슬롯 큐 전달)"""
        self._frame_cnt += 1
        ftype   = frame["type"]
        payload = frame["payload"]

        if ftype == TYPE_SENSOR_DATA and len(payload) == 8:
            temp, humi = struct.unpack_from("<ff", payload)
            self._temp_buf.append(round(temp, 1))
            self._humi_buf.append(round(humi, 1))
            self._lbl_temp.setText(f"{temp:.1f} °C")
            self._lbl_humi.setText(f"{humi:.1f} %RH")

        elif ftype == TYPE_TIME_DATA and len(payload) == 3:
            h, m, s = payload[0], payload[1], payload[2]
            self._lbl_time.setText(f"{h:02d}:{m:02d}:{s:02d}")

        elif ftype == TYPE_MOTOR_STATUS and len(payload) == 3:
            angle = payload[0] | (payload[1] << 8)
            deg   = int(angle * 360 / 4096)
            self._lbl_motor.setText(f"Motor: {deg}°")

        self._status_bar.setText(
            f"수신 프레임: {self._frame_cnt} | "
            f"파서 통계: {self._reader._parser.stats}"
        )

    def _update_graph(self):
        self._curve_temp.setData(list(self._temp_buf))
        self._curve_humi.setData(list(self._humi_buf))

    # ── 종료 처리 ────────────────────────────────
    def closeEvent(self, event):
        self._timer.stop()
        self._reader.stop()
        event.accept()


# ─────────────────────────────────────────────────
# 진입점
# ─────────────────────────────────────────────────
def main():
    parser = argparse.ArgumentParser(description="STM32 v1.6 PC 대시보드")
    parser.add_argument("--port",  default="COM3",   help="시리얼 포트 (예: COM3, /dev/ttyUSB0)")
    parser.add_argument("--baud",  default=115200, type=int, help="보드레이트")
    args = parser.parse_args()

    app = QApplication(sys.argv)
    win = Dashboard(args.port, args.baud)
    win.show()
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
