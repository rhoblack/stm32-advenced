# Ch12 기술 검토 보고서 (Technical Review)
**검토자**: 기술 리뷰어 | **날짜**: 2026-03-19 | **대상**: v1.2 LCD 초기화 및 SPI DMA

---

## 검토 요약
| 항목 | 평가 |
|------|------|
| **LCD 초기화 명령어 정확성** | ✅ 검증 완료 |
| **D/C 핀 타이밍** | ✅ 올바름 |
| **SPI DMA 시퀀싱** | ✅ HAL 표준 준수 |
| **RGB565 포맷** | ✅ 정확함 |
| **Bresenham 알고리즘** | ✅ 표준 구현 |
| **전체 기술 평가** | ⭐⭐⭐⭐ (4/5) |

---

## 1. LCD 초기화 명령어 검증 (§12.4)

### 📋 15개 명령어 완전성 검사
코드 파일 `ch12_02_ili9341_init.c` (라인 163-254)에서 다음을 확인했습니다:

| # | 명령어 | 코드 | 파라미터 수 | 상태 |
|---|--------|------|-----------|------|
| 1 | Power Control A (0xCB) | ✅ | 5바이트 | 정확 |
| 2 | Power Control B (0xCF) | ✅ | 3바이트 | 정확 |
| 3 | Power Control 1 (0xC0) | ✅ | 3바이트 | 정확 |
| 4 | Power Control 2 (0xC1) | ✅ | 1바이트 | 정확 |
| 5 | VCOMH/VCML (0xC5) | ✅ | 2바이트 | 정확 |
| 6 | VCOMH Regulator (0xC7) | ✅ | 1바이트 | 정확 |
| 7 | Memory Access Control (0x36) | ✅ | 1바이트 (0x48=BGR) | 정확 |
| 8 | Pixel Format Set (0x3A) | ✅ | 1바이트 (0x55=RGB565) | 정확 |
| 9 | Frame Rate Control (0xB1) | ✅ | 2바이트 | 정확 |
| 10 | Display Function Control (0xB6) | ✅ | 4바이트 | 정확 |
| 11 | Gamma Set (0x26) | ✅ | 1바이트 | 정확 |
| 12 | Positive Gamma (0xE0) | ✅ | 15바이트 | 정확 |
| 13 | Negative Gamma (0xE1) | ✅ | 15바이트 | 정확 |
| 14 | Sleep Out (0x11) | ✅ | 파라미터 없음 | 정확 |
| 15 | Display On (0x29) | ✅ | 파라미터 없음 | 정확 |

**결론**: ILI9341 공식 데이터시트 표준 초기화 시퀀스와 정확히 일치합니다.

### 🔴 Critical Issue: 명령어 개수 표기 부정확
- **문제**: 원고(§12.4)에서 "약 25~35개" 명령어 언급했으나, 실제 코드는 15개만 구현
- **영향도**: 높음 - 학생이 "왜 내 코드는 25개가 아닌가?" 혼란 가능
- **해결책**: 원고의 표기를 다음으로 수정 필요:
  ```
  "ILI9341을 초기화하려면 약 10~20개의 명령어를 전송합니다.
  우리의 표준 초기화 시퀀스는 15개 명령어로 구성되며,
  이를 통해 전압, 색상, 감마 보정을 설정합니다."
  ```

### 🟠 Major Issue: Sleep Out 딜레이 타이밍
- **문제**: `Sleep Out (0x11)` 후 120ms 딜레이 필수이지만, 원고에서 충분히 강조되지 않음
- **코드 상태**: ✅ 구현 정확함 (Delay_Ms(120) 라인 248)
- **원고 상태**: ⚠️ 설명 부족 - "최소 120ms 대기" 표기만 있고, "이 시간 동안 LCD가 내부적으로 안정화됨" 설명 부족
- **해결책**: §12.4에 다음 추가:
  ```
  "Sleep Out 후 최소 120ms는 LCD 내부의 전압 안정화 시간입니다.
  이 시간을 단축하면 화면이 제대로 나타나지 않을 수 있습니다."
  ```

### 🟡 Minor Issue: Reset 타이밍 정밀도
- **문제**: Delay_Ms()는 반복문 기반으로 정확성이 ±20% 오차 범위
- **현재 상태**: 루프 기반 딜레이 (10,000 * ms) - 최적화 레벨에 따라 가변
- **권장사항**: 정확한 타이밍이 필요하면 TIM 타이머 사용 (이미 원고에 언급됨 - OK)

---

## 2. D/C 핀 타이밍 검증 (§12.2, §12.4)

### 🟢 검증 결과: 타이밍 정확
코드 전반에서 D/C 신호 제어 검사:

```c
/* 명령어 전송 (ch12_02_ili9341_init.c 라인 679-689) */
static void LCD_Send_Command(uint8_t cmd)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);  // D/C = 0
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);  // CS = 0
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    while (hspi1.State != HAL_SPI_STATE_READY) __NOP();
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);    // CS = 1
}

/* 데이터 전송 (라인 692-702) */
static void LCD_Send_Data(uint8_t *pData, uint16_t Size)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);    // D/C = 1
    // ... 나머지 동일
}
```

**평가**:
- ✅ D/C 신호는 SPI 전송 전에 설정됨
- ✅ CS 신호와 함께 제어됨
- ✅ SPI 완료 후 상태 확인 (while 루프)

---

## 3. SPI DMA 시퀀싱 검증

### 🟢 HAL 표준 준수
`ch12_01_spi_dma_init.c` 검토:

**✅ 정확한 점**:
1. DMA 콜백에서 CS HIGH 처리 (라인 141-151)
2. DMA 버퍼 validation (NULL, Size=0 검사)
3. 에러 콜백 구현 (라인 158-166)
4. 타임아웃 카운터 (SPI_Wait_Complete)

**🟡 미세한 개선 사항**:
```c
/* 현재 코드 (라인 119-130) */
void SPI_Wait_Complete(void)
{
    uint32_t timeout = 1000000;
    while (hspi1.State != HAL_SPI_STATE_READY && timeout-- > 0) {
        __NOP();
    }
    if (timeout == 0) {
        LOG_W("SPI DMA 전송 타임아웃!");
    }
}
```
**개선**: timeout 값 (1,000,000 NOP)이 시스템 클럭 속도에 따라 가변적. 권장은 HAL_Delay() 기반:
```c
uint32_t start = HAL_GetTick();
while (hspi1.State != HAL_SPI_STATE_READY) {
    if (HAL_GetTick() - start > 1000) { /* 1초 타임아웃 */
        LOG_E("Timeout!");
        break;
    }
}
```
하지만 현재 구현도 실제 사용에는 무방함.

---

## 4. RGB565 포맷 정확성

### 🟢 완벽함
`ch12_03_ili9341_pixel_write.c` (라인 24-47) 검토:

```c
#define RGB565(r, g, b) (((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))
```

**검증**:
- R 성분: 8비트 → 5비트 (r >> 3) ✅
- G 성분: 8비트 → 6비트 (g >> 2) ✅
- B 성분: 8비트 → 5비트 (b >> 3) ✅
- 비트 시프트 위치:
  - R: 11비트 시프트 (Bit 15~11) ✅
  - G: 5비트 시프트 (Bit 10~5) ✅
  - B: 0비트 (Bit 4~0) ✅

**색상 팔레트 검증** (원고 §12.5 테이블):
| 색상 | RGB(8-bit) | RGB565 | 코드값 | 검증 |
|------|-----------|--------|--------|------|
| 빨강 | (255, 0, 0) | 0xF800 | (31 << 11) ✅ | 정확 |
| 초록 | (0, 255, 0) | 0x07E0 | (63 << 5) ✅ | 정확 |
| 파랑 | (0, 0, 255) | 0x001F | 31 ✅ | 정확 |
| 흰색 | (255, 255, 255) | 0xFFFF | 모두 1 ✅ | 정확 |

---

## 5. 픽셀 쓰기 시퀀스 (§12.5)

### 🟢 정확함
`ch12_03_ili9341_pixel_write.c`:

```c
/* Column 설정: 0x2A 명령어 → 4바이트 데이터 */
data[0] = (x0 >> 8) & 0xFF;  /* START_HIGH */
data[1] = x0 & 0xFF;         /* START_LOW */
data[2] = (x1 >> 8) & 0xFF;  /* END_HIGH */
data[3] = x1 & 0xFF;         /* END_LOW */
```

**검증**: ✅ Big Endian 형식 정확, 320×240 범위 체크 구현됨

---

## 6. Bresenham 직선 알고리즘 (§12.6)

### 🟢 표준 구현
원고의 알고리즘 설명 (§12.6):

```c
int16_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
int16_t dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
int16_t sx = (x0 < x1) ? 1 : -1;
int16_t sy = (y0 < y1) ? 1 : -1;
int16_t err = dx - dy;

while (1) {
    ILI9341_Draw_Pixel(x, y, color);
    if (x == x1 && y == y1) break;
    int16_t e2 = 2 * err;
    if (e2 > -dy) { err -= dy; x += sx; }
    if (e2 < dx) { err += dx; y += sy; }
}
```

**검증**:
- ✅ 정수 연산만 사용 (부동소수 없음)
- ✅ 대칭성 처리 (sx, sy 방향 제어)
- ✅ 오차 계산 (err = dx - dy, e2 = 2*err)

**🟡 경향성 오류**: 픽셀별 호출이므로 각 선분마다 최대 max(dx, dy) 픽셀 호출 → 성능 문제. 원고의 "단순 픽셀 드로잉보다 빠름" 표기는 "정확함"이 맞음.

---

## 7. 비트맵 폰트 렌더링 (§12.7)

### 🟢 구현 정확
`ch12_05_ili9341_text.c`:

```c
for (uint8_t row = 0; row < 7; row++) {
    uint8_t byte = font_data[row];
    for (uint8_t col = 0; col < 5; col++) {
        if (byte & (0x80 >> col)) {  /* MSB부터 검사 */
            ILI9341_Draw_Pixel(px, py, color);
        }
    }
}
```

**검증**:
- ✅ 5×7 비트맵 정확한 처리
- ✅ 비트마스킹 올바름 (0x80 >> col)
- ✅ ASCII 범위 검사 (ch < 32 || ch > 127)

---

## 8. 하드웨어 핀 배치 (§12.2)

### 🟢 정확함
원고 표 12-2 vs 코드:
| 신호 | STM32 핀 | 원고 | 코드 | 검증 |
|------|---------|------|------|------|
| SCLK | PB3 | ✅ | AF 설정 ✅ | 정확 |
| MOSI | PB5 | ✅ | AF 설정 ✅ | 정확 |
| D/C | PA4 | ✅ | OUTPUT ✅ | 정확 |
| CS | PA3 | ✅ | OUTPUT ✅ | 정확 |
| RESET | PA5 | ✅ | OUTPUT ✅ | 정확 |

---

## 9. I2C vs SPI 비교 (§12.0)

### 🟢 정확함
원고 표 및 코드:
- 속도: SPI 1~42Mbps vs I2C 100~400kbps ✅
- 핀 수: SPI 4+ vs I2C 2 ✅
- 용도: 올바른 설명 ✅

**표현**: "직업 전화 vs 우편 배송" 비유는 효과적이지만, I2C의 "확인신호" 개념을 더 명확히 할 수 있음.

---

## 최종 판정

| 카테고리 | 평가 |
|---------|------|
| **LCD 초기화** | ⭐⭐⭐⭐ (15개 명령어 완전, 타이밍 정확) |
| **SPI DMA** | ⭐⭐⭐⭐ (HAL 표준 준수) |
| **알고리즘** | ⭐⭐⭐⭐ (Bresenham, 폰트 정확) |
| **핀 배치** | ⭐⭐⭐⭐⭐ (모두 정확) |
| **RGB565** | ⭐⭐⭐⭐⭐ (완벽) |
| **코드 품질** | ⭐⭐⭐⭐ (로그 + 에러 처리 + 주석) |

---

## 🔴 필수 수정 (Critical)

1. **명령어 개수 표기 수정**: §12.4 "약 25~35개" → "약 10~20개" (현재 15개 구현)
2. 원고 라인 ~603, ~715 수정 필요

---

## 🟠 권장 수정 (Major)

1. **Sleep Out 딜레이 설명 강화**: 120ms 의미를 원고에 추가
2. **타임아웃 정밀도**: SPI_Wait_Complete()의 1000000 NOP 값을 시간 기반으로 개선 (선택사항)

---

## 최종 평가

### ⭐⭐⭐⭐ **4/5 Stars**

**승인 조건**:
- ✅ Critical Issue (명령어 개수) 수정 후 재검토 필요
- ✅ Major Issue (Sleep Out) 원고 강화 권장

**강점**:
- LCD 초기화 데이터시트 정확성 우수
- SPI DMA 구현 안정성 높음
- 알고리즘 정확도 탁월

**약점**:
- 원고와 코드의 명령어 개수 불일치 (설명 부족)
- 일부 타이밍 정밀도 개선 여지

---

## 서명
**기술 리뷰어** | **검토 완료**: 2026-03-19
