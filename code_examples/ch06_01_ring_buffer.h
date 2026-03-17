/* ch06_01_ring_buffer.h — 범용 링 버퍼 모듈 공개 인터페이스
 *
 * 레이어: 유틸리티 (Driver/Service 레이어에서 공용)
 * 위치: 프로젝트 아키텍처 v0.6
 *
 * 단일 생산자-단일 소비자(SPSC) 구조로 설계.
 * ISR(생산자)과 메인 루프(소비자) 간 lock-free 사용 가능.
 *
 * 사용법:
 *   uint8_t mem[256];
 *   ring_buffer_t rb;
 *   ring_buf_init(&rb, mem, 256);
 *   ring_buf_write(&rb, 0x41);
 */

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

/* ===== 링 버퍼 구조체 ===== */
typedef struct {
    uint8_t  *buf;           /* 외부에서 제공하는 배열 포인터 */
    uint16_t  size;          /* 배열 크기 (실 저장량 = size - 1) */
    volatile uint16_t head;  /* 쓰기 인덱스 (생산자) */
    volatile uint16_t tail;  /* 읽기 인덱스 (소비자) */
} ring_buffer_t;

/* ===== 초기화 ===== */

/**
 * @brief 링 버퍼 초기화
 * @param rb   링 버퍼 핸들
 * @param buf  외부 배열 (호출자가 할당)
 * @param size 배열 크기 (실 저장 가능 = size - 1)
 */
void ring_buf_init(ring_buffer_t *rb, uint8_t *buf, uint16_t size);

/* ===== 쓰기/읽기 ===== */

/**
 * @brief 1바이트 쓰기
 * @param rb   링 버퍼 핸들
 * @param byte 쓸 데이터
 * @return true 성공, false 가득 참
 */
bool ring_buf_write(ring_buffer_t *rb, uint8_t byte);

/**
 * @brief 1바이트 읽기
 * @param rb   링 버퍼 핸들
 * @param byte 읽은 데이터 저장 포인터
 * @return true 성공, false 비어있음
 */
bool ring_buf_read(ring_buffer_t *rb, uint8_t *byte);

/* ===== 상태 조회 ===== */

/**
 * @brief 읽을 수 있는 바이트 수
 * @param rb 링 버퍼 핸들
 * @return 저장된 바이트 수
 */
uint16_t ring_buf_available(const ring_buffer_t *rb);

/**
 * @brief 링 버퍼 비우기 (head = tail = 0)
 * @param rb 링 버퍼 핸들
 */
void ring_buf_flush(ring_buffer_t *rb);

#endif /* RING_BUFFER_H */
