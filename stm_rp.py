import cv2
import numpy as np
import can
import time
import threading
from flask import Flask, Response

app = Flask(__name__)
cap = cv2.VideoCapture(0)

# 웹캠 해상도 설정
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

# -------------------------------------------------------------------
# CAN 초기화 (STM32와 검증 완료된 방식과 동일: can0, 표준ID 0x100, data[0]=0/1/2)
# -------------------------------------------------------------------
bus = can.interface.Bus(channel='can0', bustype='socketcan')

CAN_ARB_ID = 0x100
CAN_KEEPALIVE_SEC = 1.0  # 같은 값이어도 이 주기마다 한 번씩 재전송 (STM32의 age 끊김감지 대응)

_can_lock = threading.Lock()
_last_sent_cmd = None
_last_sent_time = 0.0

_last_debug_print = 0.0
DEBUG_PRINT_PERIOD_SEC = 1.0  # ✅ 추가: 1초마다 R/Y/G 카운트를 터미널에도 출력 (튜닝용)


def send_speed_cmd(speed_cmd: int):
    """STM32로 속도명령(0=정지/1=저속/2=정속) 전송.
    값이 바뀌었을 때, 또는 마지막 전송 후 KEEPALIVE 주기가 지났을 때만 보냄."""
    global _last_sent_cmd, _last_sent_time

    now = time.time()
    with _can_lock:
        changed = (speed_cmd != _last_sent_cmd)
        timed_out = (now - _last_sent_time) >= CAN_KEEPALIVE_SEC

        if not (changed or timed_out):
            return

        msg = can.Message(
            arbitration_id=CAN_ARB_ID,
            data=[speed_cmd, 0, 0, 0, 0, 0, 0, 0],
            is_extended_id=False
        )
        try:
            bus.send(msg)
            _last_sent_cmd = speed_cmd
            _last_sent_time = now
        except can.CanError as e:
            print(f"⚠️ CAN 송신 실패: {e}")


def gen_frames():
    global _last_debug_print
    while True:
        success, frame = cap.read()
        if not success:
            break

        # ---------------------------------------------------------
        # [1단계] ROI 구역 정하기 (관심 영역 설정)
        # ---------------------------------------------------------
        x_start, y_start, width, height = 220, 160, 200, 150
        roi = frame[y_start:y_start+height, x_start:x_start+width]

        # ---------------------------------------------------------
        # [2단계] 색상 인식을 위해 BGR을 HSV 색 공간으로 변환
        # ---------------------------------------------------------
        hsv = cv2.cvtColor(roi, cv2.COLOR_BGR2HSV)

        # 초록색 범위
        # ✅ 변경: S(채도)/V(명도) 하한을 50 -> 120으로 상향.
        #    LED는 자체발광이라 채도/명도가 매우 높음. 나무책상/바닥처럼
        #    칙칙한 색(채도/명도 낮음)은 장소가 바뀌어도 이 기준에 안 걸리게 됨.
        lower_green = np.array([35, 120, 120])
        upper_green = np.array([85, 255, 255])

        # 노란색 범위
        lower_yellow = np.array([15, 120, 120])
        upper_yellow = np.array([35, 255, 255])

        # 빨간색 범위 (HSV 구조상 0 부근과 180 부근 두 영역으로 나뉨)
        lower_red1 = np.array([0, 120, 120])
        upper_red1 = np.array([15, 255, 255])
        lower_red2 = np.array([165, 120, 120])
        upper_red2 = np.array([180, 255, 255])

        # ---------------------------------------------------------
        # [3단계] 마스크 생성 및 색상 픽셀 개수 세기
        # ---------------------------------------------------------
        mask_green = cv2.inRange(hsv, lower_green, upper_green)
        mask_yellow = cv2.inRange(hsv, lower_yellow, upper_yellow)

        mask_red1 = cv2.inRange(hsv, lower_red1, upper_red1)
        mask_red2 = cv2.inRange(hsv, lower_red2, upper_red2)
        mask_red = cv2.bitwise_or(mask_red1, mask_red2)

        cnt_red = cv2.countNonZero(mask_red)
        cnt_yellow = cv2.countNonZero(mask_yellow)
        cnt_green = cv2.countNonZero(mask_green)

        # ---------------------------------------------------------
        # [4단계] 기준값(Threshold) 비교 후 결과 값 출력
        # ---------------------------------------------------------
        color_result = -1  # 아무것도 검출되지 않음
        color_name = "None"

        # LED는 점광원이라 ROI 안에서 차지하는 픽셀 수 자체가 크지 않음.
        # S/V 기준을 엄격하게 올린 만큼, 개수 기준은 낮춰서 LED 검출 민감도 확보.
        threshold = 150

        # ✅ 추가: 1등 색이 2등 색보다 충분히 우세할 때만 확정.
        #    배경(나무책상 등)이 두 색 범위에 동시에 살짝 걸치는 애매한 상황에서
        #    "어쩌다 더 많은 쪽"으로 잘못 확정되는 걸 막기 위함.
        #    1등이 2등의 1.5배 이상이어야 신뢰함.
        DOMINANCE_RATIO = 1.5

        counts = sorted(
            [(cnt_red, 0, "RED (0) -> STOP"),
             (cnt_yellow, 1, "YELLOW (1) -> SLOW"),
             (cnt_green, 2, "GREEN (2) -> NORMAL")],
            reverse=True
        )
        top_cnt, top_cmd, top_name = counts[0]
        second_cnt = counts[1][0]

        is_strong_enough = top_cnt > threshold
        is_dominant = (second_cnt == 0) or (top_cnt >= second_cnt * DOMINANCE_RATIO)

        if is_strong_enough and is_dominant:
            color_result = top_cmd
            color_name = top_name
        # 그 외(애매하거나 너무 약함)는 color_result=-1 그대로 -> 신호 안 보냄

        # ✅ 추가: 1초마다 터미널에도 R/Y/G 카운트 출력 (인식 안 될 때도 항상 보임)
        now_dbg = time.time()
        if now_dbg - _last_debug_print >= DEBUG_PRINT_PERIOD_SEC:
            _last_debug_print = now_dbg
            print(f"[DEBUG] R:{cnt_red} Y:{cnt_yellow} G:{cnt_green} "
                  f"(threshold={threshold}, dominance_ok={is_dominant}, strong_ok={is_strong_enough})")

        # ---------------------------------------------------------
        # [4-1단계] ✅ 추가: 인식된 색상을 CAN으로 STM32에 송신
        # ---------------------------------------------------------
        if color_result != -1:
            print(f"🚦 감지된 색상: {color_name} (값: {color_result})")
            send_speed_cmd(color_result)
        # 색이 인식 안 될 때는 일부러 안 보냄 -> STM32는 마지막 값을 계속 유지함
        # (예: 신호가 잠깐 화면 밖으로 나가도 갑자기 멈추지 않게)

        # ---------------------------------------------------------
        # [5단계] 브라우저 화면에 시각적으로 구역 그려주기
        # ---------------------------------------------------------
        cv2.rectangle(frame, (x_start, y_start), (x_start+width, y_start+height), (0, 255, 0), 2)

        if color_result != -1:
            cv2.putText(frame, f"Detect: {color_name}", (x_start, y_start - 10),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)
        else:
            cv2.putText(frame, "Detect: None", (x_start, y_start - 10),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)

        # ✅ 추가: 항상 R/Y/G 픽셀 카운트를 화면에 표시 (튜닝용 디버그).
        #    어느 조건(threshold 미달 vs dominance 미달)에서 막히는지 바로 확인 가능.
        debug_line = f"R:{cnt_red} Y:{cnt_yellow} G:{cnt_green} (th={threshold})"
        cv2.putText(frame, debug_line, (x_start, y_start + height + 25),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 255), 1)

        # JPEG 인코딩 및 스트리밍 전송
        ret, buffer = cv2.imencode('.jpg', frame)
        frame_bytes = buffer.tobytes()
        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + frame_bytes + b'\r\n')


@app.route('/')
def index():
    return Response(gen_frames(), mimetype='multipart/x-mixed-replace; boundary=frame')


if __name__ == '__main__':
    print("🚀 실시간 색상 인식 웹 서버 가동 중... 노트북 브라우저를 확인하세요!")
    print(f"📡 CAN 송신 준비 완료: channel=can0, arbitration_id=0x{CAN_ARB_ID:03X}")
    app.run(host='0.0.0.0', port=5000, debug=False)