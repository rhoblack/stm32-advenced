# Ch15. UART CLI 구현 — Phase 1 기획 회의 통합 결과

작성일: 2026-03-17
버전: v1.5

---

## Sub-agent 1: 기술 저자 관점 — CLI 아키텍처 설계 계획

### CLI 파서 설계 방향

**핵심 설계 결정:**
- 입력 버퍼: Ch06에서 구현한 Ring Buffer(`ring_buf_t`)를 그대로 재사용 (UART RX DMA → Ring Buffer)
- 라인 에디터: Ring Buffer에서 `\n` 또는 `\r\n` 감지 시 라인 버퍼(`char line_buf[CLI_LINE_MAX]`)로 복사
- 파서: `strtok_r()` 기반 토큰화 (재진입 안전한 버전 사용)
- 디스패처: 커맨드 테이블 (`cmd_entry_t` 배열) 선형 탐색, 최대 16개 명령어 충분

**커맨드 테이블 구조:**
```c
typedef struct {
    const char *name;          /* 명령어 이름 (예: "time") */
    const char *usage;         /* 사용법 (예: "time set HH:MM:SS") */
    const char *description;   /* 설명 */
    int (*handler)(int argc, char *argv[]);  /* 핸들러 함수 포인터 */
} cmd_entry_t;
```

**App 레이어 인터페이스 설계 (cli_app.h):**
```c
/* CLI 초기화 — Ring Buffer + UART 연결 */
void CLI_Init(ring_buf_t *rx_buf, UART_HandleTypeDef *huart);

/* 메인 루프에서 주기적 호출 (폴링) */
void CLI_Process(void);

/* 사용 가능한 Service API (CLI → Service 호출) */
HAL_StatusTypeDef RTC_SetTime(uint8_t hour, uint8_t min, uint8_t sec);
HAL_StatusTypeDef RTC_GetTime(RTC_TimeTypeDef *time);
HAL_StatusTypeDef RTC_SetAlarm(uint8_t hour, uint8_t min);
HAL_StatusTypeDef Motor_SetPosition(int32_t steps);
HAL_StatusTypeDef Sensor_ReadNow(float *temp, float *humi);
void LCD_ClearScreen(void);
```

**파일 구성:**
- `cli_app.c` / `cli_app.h` — CLI 초기화, 메인 루프, 라인 에디터
- `cmd_table.c` / `cmd_table.h` — 커맨드 엔트리 테이블, 핸들러 구현

**지원 명령어 목록:**
| 명령어 | 예시 | 연결 Service |
|--------|------|-------------|
| `time get` | `time get` → `12:30:00` | rtc_service |
| `time set HH:MM:SS` | `time set 09:30:00` | rtc_service |
| `alarm set HH:MM` | `alarm set 07:00` | rtc_service |
| `motor pos N` | `motor pos 2048` | clock_service |
| `sensor read` | `sensor read` → `T:25.3 H:60.1` | sensor_service |
| `lcd clear` | `lcd clear` | ui_service |
| `help` | `help` | 내부 |
| `ver` | `ver` → `v1.5` | 내부 |

**ANSI 이스케이프 코드 활용:**
- `\033[32m` 초록색 → 성공 메시지
- `\033[31m` 빨간색 → 에러 메시지
- `\033[33m` 노란색 → 경고 메시지
- `\033[0m` 리셋
- `\033[2J\033[H` 화면 클리어
- 프롬프트: `\033[1;36mSTM32>\033[0m ` (굵은 청록색)

---

## Sub-agent 2: 교육 설계자 관점 — 학습 목표 및 누적 연결

### 학습 목표 (블룸 분류체계 적용)

1. **[이해]** 링 버퍼(Ring Buffer) 기반 비동기 UART 입력 처리 흐름을 Ch06 코드와 연결하여 설명할 수 있다.
2. **[적용]** `strtok_r()` 기반 명령어 파서를 구현하고 `time set/get`, `sensor read` 명령어가 동작하도록 커맨드 테이블을 작성할 수 있다.
3. **[분석]** 커맨드 테이블 패턴(함수 포인터 배열)이 `switch-case` 방식 대비 유지보수성에서 유리한 이유를 설명할 수 있다.
4. **[평가]** ANSI 이스케이프 코드를 활용하여 CLI UX를 개선하고, 임베디드 환경에서의 사용 제약을 판단할 수 있다.

### Ch06 → Ch15 누적 연결점

**Ch04 UART 기초:**
- `HAL_UART_Transmit` / `HAL_UART_Receive_IT` → Ch15에서 DMA 수신 경로 완성
- `UART_Send()` Driver API → CLI 출력에 그대로 활용

**Ch06 링 버퍼:**
- `ring_buf_t` 구조체, `ring_buf_push()`, `ring_buf_pop()` → CLI 입력 버퍼로 직접 재사용
- UART DMA RX IDLE 인터럽트 → Ring Buffer 채우기 → CLI 라인 감지
- 링 버퍼 복습을 절 도입부에서 자연스럽게 수행

**누적 연결 학습 흐름:**
```
Ch04 UART 기초(Driver)
    → Ch06 Ring Buffer + DMA(Driver 업그레이드)
        → Ch15 CLI Parser + Command Table(App 레이어)
```

### 절 구성 및 인지 부하 분배

| 절 | 제목 | 새 개념 수 | 인지 부하 |
|----|------|-----------|----------|
| 1 | CLI 아키텍처 설계 | 2개 (App 레이어 위치, CLI 컴포넌트) | 낮음 |
| 2 | 명령어 파서 및 토큰화 | 2개 (strtok_r, argc/argv 패턴) | 중간 |
| 3 | 커맨드 테이블 패턴 | 2개 (함수 포인터, 테이블 패턴) | 중간 |
| 4 | 명령어별 구현 | 1개 (Service 연동) | 낮음 |
| 5 | ANSI 이스케이프 코드 | 2개 (VT100 코드, UX 설계) | 낮음 |

**챕터 4시간 강의 분배:**
- Phase 1 (1시간): CLI 아키텍처 설계 + 링 버퍼 복습
- Phase 2 (1.5시간): 파서 구현 + 커맨드 테이블 구현
- Phase 3 (1시간): 명령어별 구현 실습
- Phase 4 (0.5시간): ANSI 코드 + 최종 동작 확인

---

## Sub-agent 3: 교육 전문 강사 관점 — 수강생 막힘 포인트 Top 5

### 막힘 포인트 분석 및 해소 방법

**막힘 #1: strtok vs strtok_r 혼동**
- **증상**: `strtok()`을 인터럽트 핸들러와 메인 루프에서 동시 사용 → 글로벌 상태 손상
- **해소**: strtok의 내부 정적 포인터 동작 원리 다이어그램 제시 → strtok_r의 save_ptr 차이 명확화
- **안심 장치**: "임베디드에서 strtok 쓰다가 이상한 버그 만나면 열에 아홉은 이 문제입니다"

**막힘 #2: argc/argv 패턴을 C로 직접 구현하는 것이 생소함**
- **증상**: main(int argc, char *argv[])은 알지만 직접 파싱 구현이 처음인 경우
- **해소**: `argv[]` 배열이 토큰 문자열의 시작 주소를 저장하는 포인터 배열임을 그림으로 설명
- **실습 순서**: 먼저 고정 문자열로 파서 테스트 → 그 다음 UART 입력 연결

**막힘 #3: 함수 포인터 문법 생소함**
- **증상**: `int (*handler)(int argc, char *argv[])` 선언이 읽히지 않음
- **해소**: `typedef`로 먼저 단순화 → `cmd_handler_t` 타입 명시 → 배열 선언 단계적 접근
- **비유**: "TV 리모컨의 번호 버튼마다 다른 채널 함수가 연결된 것과 같습니다"

**막힘 #4: Ring Buffer에서 라인 감지 타이밍**
- **증상**: `\r\n` vs `\n` 처리, 백스페이스(`\b`) 처리 미구현
- **해소**: PuTTY/TeraTerm 기본 설정(CR+LF) 명시 + `\r` 무시 처리 코드 제공
- **실무 팁**: "minicom은 \n만 보내고, TeraTerm 기본값은 \r\n입니다 — 항상 양쪽 다 처리하세요"

**막힘 #5: ANSI 이스케이프 코드가 Windows 터미널에서 동작 안 함**
- **증상**: Windows CMD.EXE에서 색상 코드 미지원 (구형 환경)
- **해소**: Windows Terminal / PuTTY / VS Code 터미널 권장, 실습 환경 사전 통일 요청
- **폴백**: ANSI 지원 여부 감지 없이 그냥 전송 — 미지원 단말은 코드 문자열이 그대로 보임을 예고

### 면접 연결 포인트 (강사 추천)
1. "임베디드에서 커맨드 파서를 구현할 때 strtok 대신 strtok_r을 쓰는 이유는?"
2. "함수 포인터 배열 기반 커맨드 테이블의 장점을 switch-case와 비교하여 설명하라"
3. "링 버퍼 기반 CLI에서 ISR과 메인 루프 간 데이터 공유 시 주의사항은?"

---

## 통합 결론 및 집필 방향

### 핵심 설계 결정
1. **Ch06 링 버퍼 재사용**: 별도 구현 없이 `ring_buf_t` 그대로 사용 → 누적 성장 실증
2. **strtok_r 채택**: 재진입 안전성 명시 + strtok 함정 설명으로 교육 효과 극대화
3. **커맨드 테이블**: `typedef`로 함수 포인터 타입 정의 후 배열 선언 → 단계적 빌드업
4. **ANSI 코드**: 터미널 환경 이슈를 솔직하게 명시 + Windows Terminal 권장

### 각 절 핵심 메시지
- 1절: "CLI는 App 레이어의 관문 — 모든 Service를 명령어로 제어"
- 2절: "strtok_r 하나면 argc/argv 패턴 완성 — 복잡하지 않다"
- 3절: "함수 포인터 배열 = 새 명령어 추가 시 테이블에 한 줄만 추가"
- 4절: "Service API 호출만 하면 됨 — CLI는 조각들을 연결하는 접착제"
- 5절: "ANSI 코드로 터미널을 IDE처럼 — 4줄로 색상 입력"
