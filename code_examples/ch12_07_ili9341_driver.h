/**
 * ch12_07_ili9341_driver.h
 * ILI9341 LCD 드라이버 공개 인터페이스
 *
 * 역할:
 * - ILI9341 초기화 및 기본 드로잉 함수 제공
 * - SPI Driver 위에 구축 (계층 구조)
 * - Service 레이어가 사용할 최소한의 API만 노출
 */

#ifndef ILI9341_DRIVER_H
#define ILI9341_DRIVER_H

#include "main.h"

/* ===== 해상도 및 색상 정의 ===== */

#define LCD_WIDTH   320
#define LCD_HEIGHT  240

/* RGB565 색상 매크로 */
#define RGB565(r, g, b) (((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))

/* 기본 색상 팔레트 */
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F

/* ===== 초기화 함수 ===== */

/**
 * @brief ILI9341 LCD 초기화
 *
 * 동작:
 * 1. Reset 신호 전송 (LOW → HIGH)
 * 2. 공식 초기화 명령어 시퀀스 (25~35개 명령어)
 * 3. 화면 ON
 *
 * 전제 조건:
 * - SPI_Driver_Init() 호출 완료
 * - SPI DMA 설정 완료
 *
 * 소요 시간: 약 500ms
 *
 * 예제:
 *   SPI_Driver_Init();
 *   ILI9341_Init();  // 이제 LCD 사용 가능
 */
void ILI9341_Init(void);

/* ===== 색상 설정 함수 ===== */

/**
 * @brief 전체 화면을 특정 색상으로 채우기
 *
 * @param color: RGB565 형식의 색상값
 *
 * 동작:
 * - 320×240 = 76,800 픽셀을 같은 색상으로 채움
 * - 대용량 데이터(153,600 바이트)는 버퍼로 분할 전송
 * - DMA 사용하여 CPU 부하 최소화
 *
 * 소요 시간: 약 100ms (DMA 전송)
 *
 * 예제:
 *   ILI9341_Fill_Color(COLOR_BLACK);    // 화면 검정
 *   ILI9341_Fill_Color(COLOR_BLUE);     // 화면 파랑
 */
void ILI9341_Fill_Color(uint16_t color);

/**
 * @brief 지정 범위를 특정 색상으로 채우기
 *
 * @param x, y: 왼쪽 위 좌표
 * @param width: 너비 (픽셀)
 * @param height: 높이 (픽셀)
 * @param color: RGB565 색상값
 *
 * 사용:
 * - 부분 화면 업데이트 (성능 최적화)
 * - 사각형 배경 채우기
 *
 * 예제:
 *   ILI9341_Fill_Rectangle(50, 50, 100, 100, COLOR_GREEN);
 */
void ILI9341_Fill_Rectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);

/* ===== 픽셀 드로잉 함수 ===== */

/**
 * @brief 개별 픽셀 그리기
 *
 * @param x, y: 픽셀 좌표 (0~319, 0~239)
 * @param color: RGB565 색상값
 *
 * 주의:
 * - 매우 느림 (픽셀마다 SPI 통신)
 * - 소수 픽셀만 그릴 때만 사용
 * - 많은 픽셀 그리기는 도형 함수 사용 권장
 *
 * 예제:
 *   ILI9341_Draw_Pixel(160, 120, COLOR_WHITE);
 */
void ILI9341_Draw_Pixel(uint16_t x, uint16_t y, uint16_t color);

/* ===== 도형 그리기 함수 ===== */

/**
 * @brief 직선 그리기 (Bresenham 알고리즘)
 *
 * @param x0, y0: 시작점
 * @param x1, y1: 끝점
 * @param color: RGB565 색상값
 *
 * 예제:
 *   // 화면 우상단에서 좌하단으로 대각선
 *   ILI9341_Draw_Line(0, 0, 319, 239, COLOR_WHITE);
 */
void ILI9341_Draw_Line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);

/**
 * @brief 수평선 그리기 (최적화)
 *
 * @param x0, x1: X 좌표 범위
 * @param y: Y 좌표 (상수)
 * @param color: RGB565 색상값
 *
 * Bresenham보다 빠릅니다.
 */
void ILI9341_Draw_Horizontal_Line(uint16_t x0, uint16_t x1, uint16_t y, uint16_t color);

/**
 * @brief 수직선 그리기 (최적화)
 *
 * @param x: X 좌표 (상수)
 * @param y0, y1: Y 좌표 범위
 * @param color: RGB565 색상값
 */
void ILI9341_Draw_Vertical_Line(uint16_t x, uint16_t y0, uint16_t y1, uint16_t color);

/**
 * @brief 사각형 윤곽선 그리기
 *
 * @param x, y: 왼쪽 위 좌표
 * @param width: 너비
 * @param height: 높이
 * @param color: RGB565 색상값
 *
 * 예제:
 *   ILI9341_Draw_Rectangle(50, 50, 100, 100, COLOR_BLUE);
 */
void ILI9341_Draw_Rectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);

/**
 * @brief 채워진 사각형 그리기
 *
 * @param x, y: 왼쪽 위 좌표
 * @param width: 너비
 * @param height: 높이
 * @param color: RGB565 색상값
 *
 * 예제:
 *   ILI9341_Draw_Filled_Rectangle(100, 100, 50, 50, COLOR_RED);
 */
void ILI9341_Draw_Filled_Rectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);

/**
 * @brief 원 그리기 (Midpoint Circle Algorithm)
 *
 * @param cx, cy: 중심
 * @param radius: 반지름
 * @param color: RGB565 색상값
 *
 * 예제:
 *   ILI9341_Draw_Circle(160, 120, 50, COLOR_YELLOW);
 */
void ILI9341_Draw_Circle(uint16_t cx, uint16_t cy, uint16_t radius, uint16_t color);

/**
 * @brief 채워진 원 그리기
 *
 * @param cx, cy: 중심
 * @param radius: 반지름
 * @param color: RGB565 색상값
 *
 * 예제:
 *   ILI9341_Draw_Filled_Circle(160, 120, 50, COLOR_CYAN);
 */
void ILI9341_Draw_Filled_Circle(uint16_t cx, uint16_t cy, uint16_t radius, uint16_t color);

/**
 * @brief 삼각형 그리기
 *
 * @param x0, y0: 첫 번째 점
 * @param x1, y1: 두 번째 점
 * @param x2, y2: 세 번째 점
 * @param color: RGB565 색상값
 *
 * 예제:
 *   ILI9341_Draw_Triangle(100, 50, 150, 150, 50, 150, COLOR_WHITE);
 */
void ILI9341_Draw_Triangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

/* ===== 텍스트 출력 함수 ===== */

/**
 * @brief 개별 문자 출력 (5×7 비트맵 폰트)
 *
 * @param x, y: 왼쪽 위 좌표
 * @param ch: ASCII 문자 (32~126)
 * @param color: RGB565 전경색
 * @param bg_color: RGB565 배경색
 *
 * 지원: 영문(A-Z, a-z), 숫자(0-9), 기호(!@#$ 등)
 * 미지원: 한글, 한자
 *
 * 예제:
 *   ILI9341_Draw_Char(10, 10, 'A', COLOR_WHITE, COLOR_BLACK);
 */
void ILI9341_Draw_Char(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bg_color);

/**
 * @brief 문자열 출력
 *
 * @param x, y: 시작 좌표
 * @param str: C 문자열 (NULL 종료)
 * @param color: RGB565 전경색
 * @param bg_color: RGB565 배경색
 *
 * 지원:
 * - 일반 ASCII 문자
 * - '\n': 다음 줄
 * - '\r': 줄의 시작
 * - 자동 줄바꿈 (화면 끝)
 *
 * 예제:
 *   ILI9341_Draw_String(10, 10, "Hello STM32!", COLOR_WHITE, COLOR_BLACK);
 *   ILI9341_Draw_String(10, 30, "Temp: 25C\nHumi: 65%", COLOR_GREEN, COLOR_BLACK);
 */
void ILI9341_Draw_String(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg_color);

/**
 * @brief 정수 출력
 *
 * @param x, y: 좌표
 * @param value: 정수값
 * @param color: RGB565 색상값
 *
 * 예제:
 *   int temp = 25;
 *   ILI9341_Draw_Integer(10, 50, temp, COLOR_WHITE);  // "25" 출력
 */
void ILI9341_Draw_Integer(uint16_t x, uint16_t y, int32_t value, uint16_t color);

/**
 * @brief 실수 출력
 *
 * @param x, y: 좌표
 * @param value: 실수값
 * @param precision: 소수점 자리수
 * @param color: RGB565 색상값
 *
 * 예제:
 *   float temp = 23.45;
 *   ILI9341_Draw_Float(10, 50, temp, 2, COLOR_WHITE);  // "23.45" 출력
 */
void ILI9341_Draw_Float(uint16_t x, uint16_t y, float value, uint8_t precision, uint16_t color);

/* ===== 유틸리티 함수 ===== */

/**
 * @brief RGB888 → RGB565 색상 변환
 *
 * @param r: 빨강 (0~255)
 * @param g: 초록 (0~255)
 * @param b: 파랑 (0~255)
 *
 * @return RGB565 16비트 값
 *
 * 예제:
 *   uint16_t orange = ILI9341_Color_RGB565(255, 165, 0);
 *   ILI9341_Fill_Color(orange);
 */
uint16_t ILI9341_Color_RGB565(uint8_t r, uint8_t g, uint8_t b);

#endif /* ILI9341_DRIVER_H */
