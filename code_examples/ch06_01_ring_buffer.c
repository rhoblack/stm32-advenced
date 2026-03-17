/* ch06_01_ring_buffer.c — 범용 링 버퍼 모듈 구현
 *
 * 레이어: 유틸리티
 * 위치: 프로젝트 아키텍처 v0.6
 *
 * SPSC(단일 생산자-단일 소비자) lock-free 링 버퍼.
 * head는 생산자(ISR), tail은 소비자(메인 루프)만 수정.
 * volatile로 컴파일러 최적화 방지.
 *
 * 빈 상태: head == tail
 * 가득 참: (head + 1) % size == tail
 * 실 저장 가능: size - 1 바이트
 */

#include "ch06_01_ring_buffer.h"
#include "log.h"

/* ===== 초기화 ===== */

void ring_buf_init(ring_buffer_t *rb, uint8_t *buf, uint16_t size)
{
    rb->buf  = buf;
    rb->size = size;
    rb->head = 0;
    rb->tail = 0;
    LOG_D("ring_buf 초기화: size=%d, 실저장=%d", size, size - 1);
}

/* ===== 1바이트 쓰기 ===== */

bool ring_buf_write(ring_buffer_t *rb, uint8_t byte)
{
    uint16_t next = (rb->head + 1) % rb->size;

    if (next == rb->tail) {
        /* 가득 참 — 데이터 유실 방지를 위해 쓰지 않음 */
        LOG_W("ring_buf 오버플로: head=%d, tail=%d",
              rb->head, rb->tail);
        return false;
    }

    rb->buf[rb->head] = byte;
    rb->head = next;
    return true;
}

/* ===== 1바이트 읽기 ===== */

bool ring_buf_read(ring_buffer_t *rb, uint8_t *byte)
{
    if (rb->head == rb->tail) {
        return false;  /* 비어있음 */
    }

    *byte = rb->buf[rb->tail];
    rb->tail = (rb->tail + 1) % rb->size;
    return true;
}

/* ===== 읽을 수 있는 바이트 수 ===== */

uint16_t ring_buf_available(const ring_buffer_t *rb)
{
    return (rb->head - rb->tail + rb->size) % rb->size;
}

/* ===== 링 버퍼 비우기 ===== */

void ring_buf_flush(ring_buffer_t *rb)
{
    rb->head = 0;
    rb->tail = 0;
    LOG_D("ring_buf 플러시 완료");
}
