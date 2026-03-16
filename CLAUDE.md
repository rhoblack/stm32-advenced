# CLAUDE.md

This file provides guidance to Claude Code when working with this repository.

## 프로젝트 정보
- **제목**: STM32 고급 실무교육
- **대상 독자**: 중급 임베디드 개발자 (C언어 숙련, HAL 기본 사용 경험, DMA/고급TIM/RTC 미경험)
- **목표**: STM32 NUCLEO-F411RE 기반 스마트 시계 & 환경 모니터링 시스템을 누적 성장형으로 완성
- **톤**: 격식체, 한국어 중심, 영어 기술 용어 병기 (예: DMA(직접 메모리 접근))

## 개발 환경
- **OS**: Windows
- **MCU**: STM32F411RE (NUCLEO-F411RE)
- **IDE**: STM32CubeIDE
- **라이브러리**: HAL
- **주변장치**: ULN2003+28BYJ-48 스텝모터, SHT31 온습도센서, ILI9341 TFT LCD
- **프로젝트 루트**: stm32-advanced/

---

## Claude Code Agent Teams 운영

이 프로젝트는 **Claude Code Agent Teams** 기능을 사용하여 7명의 전문가 팀을 운영합니다.

### 에이전트 팀 구성

| 역할 | 담당 | 정의 파일 |
|------|------|----------|
| 📋 Team Lead (총괄 편집장) | 프로젝트 관리, 최종 승인 | agents/editor_in_chief.md |
| ✍️ Teammate: 기술 저자 | 콘텐츠 집필, HAL 코드·SVG 작성 | agents/technical_author.md |
| 🔍 Teammate: 기술 리뷰어 | STM32 HAL 정확성, 실무 적합성 | agents/technical_reviewer.md |
| 🎓 Teammate: 독자 | 중급 개발자 관점 이해도 평가 | agents/beginner_reader.md |
| 📐 Teammate: 교육 설계자 | 학습 흐름, 누적 성장 구조 | agents/instructional_designer.md |
| 🧠 Teammate: 교육심리전문가 | 학습 동기, 불안 관리 | agents/educational_psychologist.md |
| 🎤 Teammate: 교육전문강사 | 강의 적합성, 비유 검증 | agents/expert_instructor.md |

### 챕터 집필 실행 순서
```
Phase 1: 기획 회의   — 저자 + 교육설계자 + 강사 (3 teammates 병렬)
Phase 2: 초안 작성   — 기술 저자 단독
Phase 3: 병렬 리뷰   — 리뷰어 + 독자 + 심리 + 강사 (4 teammates 동시)
Phase 4: 종합 회의   — 전체 종합 → 기술 저자 수정 → 편집장 승인
```
상세 프롬프트: `workflows/agent_team_workflow.md` 참조

### Agent Team 운영 규칙
1. Team Lead = 편집장 역할로 팀 조율
2. 각 Teammate 생성 시 `agents/*.md` 파일 내용을 spawn prompt에 명시
3. 독립적 리뷰 작업은 반드시 병렬 실행
4. **파일 충돌 방지**: 기술 저자만 HTML 수정, 리뷰어는 review_logs만 작성
5. 작업 완료 후 반드시 팀 정리

---

## 프로젝트 아키텍처

### 디렉터리 구조
```
stm32-advanced/
├── CLAUDE.md
├── README.md
├── TABLE_OF_CONTENTS.md
├── .claude/settings.json
├── agents/                         에이전트 정의 7명
├── templates/                      HTML 템플릿 + CSS
│   ├── chapter_template.html
│   └── book_style.css
├── workflows/                      워크플로우 정의
│   ├── agent_team_workflow.md
│   ├── chapter_writing.md
│   ├── review_meeting.md
│   └── quality_checklist.md
├── manuscripts/                    원고 HTML
│   ├── part0/
│   ├── part1/
│   ├── part2/
│   ├── part3/
│   ├── part4/
│   ├── part5/
│   └── part6/
├── code_examples/                  HAL 코드 예제 (.c/.h)
├── figures/                        SVG 다이어그램
├── review_logs/                    리뷰 기록
└── output/                         최종 산출물
    ├── ppt/
    ├── workbook/
    └── docx/
```

### 원고 파일 규칙
- 위치: `manuscripts/partN/chapterNN.html`
- CSS: `../../templates/book_style.css`
- SVG: `../../figures/chNN_secNN_설명.svg`
- 코드: `<pre><code class="language-c">`

---

## 소프트웨어 아키텍처 적용 원칙 (전 챕터 공통)

**Ch04부터 모든 챕터 도입부에 다음 4개 섹션 포함:**

1. **아키텍처 위치** — 전체 레이어 다이어그램(SVG)에서 이 챕터 컴포넌트 위치
   - 레이어: `HAL → Driver → Service → App`
2. **인터페이스 설계** — 상위 레이어에 노출하는 API 함수 시그니처
3. **FSM / 시퀀스 다이어그램** — 해당하는 경우 SVG로 작성
4. **아키텍처 업데이트** — 챕터 완료 후 전체 아키텍처 다이어그램 갱신

**Ch17(소프트웨어 아키텍처 심화)**: Ch03 초안 → 최종 아키텍처 진화 전 과정 통합 리뷰

---

## HTML 원고 필수 구조

`templates/chapter_template.html` 참조

### aside 박스 종류
```html
<aside class="tip">          💡 실무 팁
<aside class="faq">          ❓ 수강생 단골 질문
<aside class="interview">    🎯 면접 포인트
<aside class="metacognition"> 🔍 스스로 점검
<aside class="instructor-tip"> 📌 강사 꿀팁
```

---

## 다이어그램 규칙
- **SVG만 사용** (ASCII art 절대 금지)
- 네이밍: `figures/chNN_secNN_설명.svg`
- 색상 팔레트: `#1A73E8`(파랑/HAL), `#34A853`(초록/Driver), `#FBBC04`(노랑/Service), `#EA4335`(빨강/App)
- 폰트: Noto Sans KR, 14px 기준

## 코드 품질 기준
- STM32 HAL 표준 준수 (NUCLEO-F411RE 기준)
- 들여쓰기 4칸, 스네이크 케이스(`uart_send_data`), **한국어 주석**
- 모든 코드 예제에 `LOG_D` / `LOG_I` / `LOG_W` / `LOG_E` 적용 (Ch02 표준)
- 코드 블록 30줄 이하 권장

## 콘텐츠 품질 기준
- 각 절 2000~4000자
- 새 개념 시 비유/실생활 예시 필수
- 기술 용어 첫 등장 시 괄호 안 한글 설명
- 이전 챕터 지식만으로 현재 챕터 이해 가능
- 누적 성장: 이전 챕터 코드에 기능 추가하는 방식
- 감정 곡선: 호기심 → 약간의 불안 → 이해 → 성취감

## 전체 목차
`TABLE_OF_CONTENTS.md` 참조 — 총 7 Part / 19 Chapter / 76시간
