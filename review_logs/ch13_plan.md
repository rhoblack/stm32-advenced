# Ch13 기획 회의 통합 결과
## ILI9341 GFX 레이어 + 화면 완성 (v1.3)

작성일: 2026-03-17
작성자: Phase 1 통합 기획 (기술 저자 + 교육 설계자 + 강사 관점)

---

## Sub-agent 1: 기술 저자 관점

### 각 절 핵심 메시지

| 절 | 제목 | 핵심 메시지 |
|----|------|------------|
| 도입부 | 아키텍처 위치 & 인터페이스 & FSM | GFX 레이어가 LCD Driver → Service → App을 어떻게 연결하는가 |
| 1절 | GFX 레이어 아키텍처 설계 원칙 | 그래픽 추상화 계층의 필요성 — "픽셀 조작 vs 의미 있는 UI" |
| 2절 | 폰트 렌더링 구현 | 비트맵 폰트를 LCD에 그리는 알고리즘 |
| 3절 | 화면 레이아웃 설계 및 구현 | TIME_VIEW / SENSOR_VIEW / STOPWATCH_VIEW 3개 화면 좌표계 설계 |
| 4절 | DISPLAY FSM 구현 | 버튼 이벤트 → 화면 전환 상태 머신 |
| 5절 | 프레임버퍼 DMA 최적화 | 부분 갱신(partial update)으로 깜박임 제거 |
| 6절 | 전체 아키텍처 다이어그램 갱신 | v1.3 완성본 — App → UI Service → GFX → LCD Driver → SPI DMA |

### HAL 코드 예제 목록

1. `ch13_gfx_service.c` — GFX Service 초기화 및 도형/텍스트 API 구현
2. `ch13_gfx_service.h` — GFX Service 공개 인터페이스
3. `ch13_ui_service.c` — UI Service (화면 3종 렌더링 함수)
4. `ch13_ui_service.h` — UI Service 공개 인터페이스
5. `ch13_display_fsm.c` — DISPLAY FSM (TIME_VIEW / SENSOR_VIEW / STOPWATCH_VIEW)
6. `ch13_font_renderer.c` — 비트맵 폰트 렌더링 엔진
7. `ch13_framebuffer_dma.c` — 프레임버퍼 부분 갱신 DMA 최적화

### SVG 다이어그램 목록

1. `ch13_sec00_architecture.svg` — v1.3 전체 레이어 아키텍처 (도입부)
2. `ch13_sec00_fsm.svg` — DISPLAY FSM 상태 전이 다이어그램 (도입부)
3. `ch13_sec00_sequence.svg` — UI 갱신 시퀀스 다이어그램 (도입부)
4. `ch13_sec01_gfx_layers.svg` — GFX 추상화 계층 상세
5. `ch13_sec02_font_bitmap.svg` — 비트맵 폰트 렌더링 원리
6. `ch13_sec03_layout.svg` — 화면 3종 좌표 레이아웃 설계도
7. `ch13_sec05_framebuffer.svg` — 프레임버퍼 부분 갱신 메커니즘
8. `ch13_sec_final_architecture.svg` — v1.3 완성 아키텍처 (핵심 정리)

### GFX Service / UI Service 인터페이스 API 설계

```c
/* gfx_service.h — GFX Service 공개 인터페이스 */

/* 초기화 */
void GFX_Init(void);

/* 기본 도형 */
void GFX_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void GFX_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void GFX_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void GFX_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void GFX_DrawCircle(uint16_t cx, uint16_t cy, uint16_t r, uint16_t color);

/* 폰트 렌더링 */
void GFX_DrawChar(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg, uint8_t scale);
void GFX_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t fg, uint16_t bg, uint8_t scale);
void GFX_DrawNumber(uint16_t x, uint16_t y, int32_t val, uint16_t fg, uint16_t bg, uint8_t scale);

/* 색상 팔레트 매크로 */
#define GFX_COLOR_BLACK    0x0000
#define GFX_COLOR_WHITE    0xFFFF
#define GFX_COLOR_RED      0xF800
#define GFX_COLOR_GREEN    0x07E0
#define GFX_COLOR_BLUE     0x001F
#define GFX_COLOR_YELLOW   0xFFE0
#define GFX_COLOR_CYAN     0x07FF
#define GFX_COLOR_ORANGE   0xFD20
#define GFX_COLOR_DARKGRAY 0x7BEF
#define GFX_COLOR_NAVY     0x000F

/* 화면 치수 */
#define LCD_WIDTH   240
#define LCD_HEIGHT  320

/* ui_service.h — UI Service 공개 인터페이스 */

typedef enum {
    UI_VIEW_TIME      = 0,  /* 디지털 시각 화면 */
    UI_VIEW_SENSOR    = 1,  /* 온습도 화면 */
    UI_VIEW_STOPWATCH = 2,  /* 스톱워치 화면 */
    UI_VIEW_MAX
} UI_ViewType_t;

void UI_Init(void);
void UI_SetView(UI_ViewType_t view);
UI_ViewType_t UI_GetView(void);
void UI_Refresh(void);          /* 현재 화면 전체 갱신 */
void UI_RefreshPartial(void);   /* 변경된 영역만 부분 갱신 */

/* 각 화면 렌더링 */
void UI_Render_TimeView(uint8_t hour, uint8_t min, uint8_t sec, uint8_t date);
void UI_Render_SensorView(float temp, float humi);
void UI_Render_StopwatchView(uint32_t elapsed_ms);
```

### DISPLAY FSM 상태 전이 설계

**상태(States)**: TIME_VIEW / SENSOR_VIEW / STOPWATCH_VIEW
**이벤트(Events)**: BTN_SHORT (짧게 누름), BTN_LONG (길게 누름)
**전이(Transitions)**:
- TIME_VIEW + BTN_SHORT → SENSOR_VIEW
- SENSOR_VIEW + BTN_SHORT → STOPWATCH_VIEW
- STOPWATCH_VIEW + BTN_SHORT → TIME_VIEW
- 모든 상태 + BTN_LONG → 현재 화면 강제 갱신 (진입 액션 재실행)

**액션(Actions)**:
- 상태 진입 시: 배경 지우기(GFX_FillRect 전체) + 화면 레이아웃 초기화
- 1초 주기 갱신: 현재 상태의 데이터 영역만 부분 갱신

---

## Sub-agent 2: 교육 설계자 관점

### 학습 목표 4개 ("~할 수 있다" 형태, 블룸 분류체계)

1. **[이해]** GFX 추상화 레이어가 필요한 이유와 LCD Driver / GFX Service / UI App 3계층 분리 원칙을 설명할 수 있다.
2. **[적용]** 비트맵 폰트 렌더링 함수를 구현하고 디지털 시각·온습도·스톱워치를 LCD에 표시할 수 있다.
3. **[적용]** DISPLAY FSM을 테이블 기반으로 구현하여 버튼 이벤트에 따른 화면 전환을 처리할 수 있다.
4. **[분석]** 프레임버퍼 전체 갱신과 부분 갱신(partial update)의 성능 차이를 측정하고 개선할 수 있다.

### Ch12(SPI DMA) → Ch13(GFX) 누적 성장 연결점

| Ch12에서 구축한 것 | Ch13에서 사용하는 방식 |
|-------------------|----------------------|
| `ILI9341_Fill_Color()` | GFX_FillRect 내부에서 호출 |
| `ILI9341_Draw_Pixel()` | GFX_DrawPixel 내부에서 호출 |
| `ILI9341_Draw_Rectangle()` | GFX 레이어가 이를 래핑 |
| `ILI9341_Draw_String()` | ch13에서 폰트 렌더링으로 고도화 |
| SPI DMA 전송 인프라 | 프레임버퍼 전송에 재사용 |
| CS 핀 타이밍 | 변경 없이 유지 |

**핵심 메시지**: "Ch12에서 만든 LCD Driver의 저수준 함수들 위에, Ch13에서 '의미 있는 UI'를 만드는 Service 레이어를 쌓는다. 코드를 버리지 않고 확장한다."

### 블룸 분류체계 기반 인지 부하 설계

| 절 | 블룸 수준 | 새 개념 수 | 인지 부하 |
|----|---------|---------|---------|
| 1절 (GFX 아키텍처) | 이해 | 2개 (추상화, 레이어 경계) | 낮음 |
| 2절 (폰트 렌더링) | 적용 | 3개 (비트맵 폰트, 스케일, 배경색) | 중간 |
| 3절 (화면 레이아웃) | 적용 | 2개 (좌표계, 영역 분할) | 중간 |
| 4절 (DISPLAY FSM) | 적용+분석 | 2개 (FSM 진입/이탈 액션, 이벤트 테이블) | 중간-높음 |
| 5절 (DMA 최적화) | 분석 | 2개 (dirty flag, partial update) | 높음 |
| 6절 (아키텍처 갱신) | 창조 | 0개 (통합 정리) | 낮음 (성취감) |

**설계 원칙**: 1~2절에서 개념을 쌓은 후 3~4절에서 실제 구현, 5절의 심화는 선택적 도전 과제로 배치. 매 절마다 실행 가능한 중간 산출물(빌드+실행 가능 코드) 제공.

---

## Sub-agent 3: 강사 관점

### 수강생이 막힐 Top 5 포인트 + 해소 방법

**1. GFX 레이어와 LCD Driver 중복 혼란**
- 막힘: "Ch12에서 ILI9341_Draw_String을 만들었는데 왜 또 GFX_DrawString을 만드나요?"
- 해소: "Ch12는 하드웨어 구체적(ILI9341 한정), Ch13 GFX는 하드웨어 무관(추상). 나중에 LCD를 교체해도 UI Service 코드는 그대로."
- 강의 전략: 칠판에 "ILI9341_xxx → GFX_xxx → UI_xxx" 3층 구조를 먼저 그리고 시작

**2. 비트맵 폰트 데이터 해석**
- 막힘: `uint8_t Font16_Table[][16]` 배열 구조가 처음에는 외계어처럼 보임
- 해소: 그리드 종이에 'A' 글자를 그려서 0/1 비트로 채우는 실습 먼저 수행
- 강의 전략: 8×16 폰트의 'A' 한 글자를 함께 바이너리로 분석하는 5분 실습

**3. DISPLAY FSM 진입/이탈 액션 구현 위치**
- 막힘: "배경 지우기를 state_enter에서 해야 하나, 각 Render 함수에서 해야 하나?"
- 해소: FSM 진입 액션 = 레이아웃 초기화, 1초 콜백 = 데이터 영역만 갱신. 역할 명확히 분리
- 강의 전략: "FSM 진입 = 방 청소, 1초 갱신 = 칠판 일부만 지우고 새로 쓰기" 비유

**4. 부분 갱신 좌표 계산 오류**
- 막힘: `ILI9341_Set_Address_Window(x, y, x+w-1, y+h-1)` 에서 off-by-one 오류 빈발
- 해소: 실수 정상화 — "이 오류는 처음 배울 때 모두 겪는 것" 명시, LCD에서 선이 어긋나는 현상으로 바로 확인 가능
- 강의 전략: 의도적으로 틀린 코드를 보여주고 어긋난 화면 사진을 함께 제시

**5. DMA 완료 후 콜백 타이밍 — 화면 갱신 중 다음 전송 시작**
- 막힘: DMA 전송 중에 UI_Refresh가 또 호출되어 데이터 오염
- 해소: `is_dma_busy` 플래그 패턴 — Ch12에서 SPI_Wait_Complete 패턴을 이미 배웠음을 상기
- 강의 전략: "Ch12의 SPI_Wait_Complete 기억하시나요? Ch13에서는 이를 플래그로 업그레이드합니다"

### GFX 레이어 추상화 개념 설명 전략

**핵심 비유**: 레스토랑 비유
- **LCD Driver 레이어** = 주방 (재료 직접 다룸, 특정 장비에 종속)
- **GFX Service 레이어** = 요리사 (도구 추상화 — 어느 주방에서도 동일한 레시피)
- **UI App 레이어** = 웨이터 (손님에게 의미 있는 단위로 제공 — "오늘의 메뉴: 디지털 시계")

**설명 순서**:
1. "Ch12 코드만 있을 때 문제": main.c에 ILI9341_xxx 호출 난무 → 유지보수 어려움
2. "GFX Service 추가 후 변화": main.c는 UI_Refresh()만 호출 → 로직 분리
3. "나중에 LCD를 다른 모델로 교체할 때": gfx_service.c 내부만 바꾸면 UI 코드는 불변

---

## 통합 기획 결론

### 절 구성 최종 확정

| 절 | 제목 | 분량 목표 | 포함 요소 |
|----|------|---------|---------|
| 도입부 | 아키텍처 위치 / 인터페이스 / FSM / 시퀀스 | - | SVG 3개 + API 목록 |
| 1절 | GFX 레이어 아키텍처 설계 원칙 | 2500자 | SVG 1개, 비유, FAQ |
| 2절 | 폰트 렌더링 구현 | 3000자 | SVG 1개, 코드, 강사팁 |
| 3절 | 화면 레이아웃 설계 및 구현 | 3500자 | SVG 1개, 코드, 면접포인트 |
| 4절 | DISPLAY FSM 구현 | 3500자 | 코드, 메타인지, FAQ |
| 5절 | 프레임버퍼 DMA 최적화 | 3000자 | SVG 1개, 코드, 실무팁 |
| 6절 | 전체 아키텍처 다이어그램 갱신 | 2000자 | SVG 1개 (완성본) |
| 실습 | 3-화면 전환 시스템 완성 | - | Step 3까지, 로그 예시 |
| 핵심 정리 | - | - | 아키텍처 업데이트 SVG |
| 연습문제 | 4개 (기억/이해/적용/분석) | - | 블룸 4수준 |

### 핵심 감정 곡선 설계
- **1절 도입**: "Ch12에서 만든 LCD가 이제 진짜 UI로 변신한다" → 호기심
- **2절 폰트**: 비트맵 구조가 낯설 → 약간의 불안 → "A 한 글자 직접 분석" 실습으로 이해
- **3절 레이아웃**: 3개 화면 좌표계 → 구체적 설계도로 안심
- **4절 FSM**: "Ch08 스톱워치 FSM 기억?" 복습으로 자신감
- **5절 DMA 최적화**: 심화 내용 → "기본 구현 먼저, 최적화는 선택" 안전 출구 제공
- **실습 완료**: 디지털 시계 + 온습도 + 스톱워치가 LCD에 표시 → 큰 성취감

### 리뷰 관심 포인트
- 기술 리뷰어: `GFX_FillRect` 내부에서 직접 픽셀 채우기 vs ILI9341 명령어 사용 성능 차이 명시 필요
- 독자: 비트맵 폰트 데이터 배열 해석 설명이 충분한지
- 교육심리: 5절 DMA 최적화가 과부하 유발하지 않도록 "선택 사항" 명시
- 강사: 4시간 강의에서 1~4절이 핵심, 5~6절은 심화 옵션으로 분리 가능하도록 구조화
