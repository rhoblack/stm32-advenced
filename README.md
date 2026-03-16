# STM32 고급 실무교육 — 교재 집필 프로젝트

## 프로젝트 개요

STM32 NUCLEO-F411RE 기반 **스마트 시계 & 환경 모니터링 시스템**을 누적 성장형으로 완성하는 교재입니다.
Claude Code Agent Team을 활용하여 7명의 전문가가 협업하여 집필합니다.

- **대상**: 중급 임베디드 개발자 (C언어 숙련, HAL 기본 경험)
- **구성**: 7 Part / 19 Chapter / 76시간
- **메인 프로젝트**: 스텝 모터 아날로그 시계 + 스톱워치 + 알람 + 온습도 + TFT LCD + PC 대시보드

## 개발 환경
- **OS**: Windows
- **문서 형식**: HTML (브라우저에서 바로 확인)
- **다이어그램**: SVG 전용 (ASCII art 금지)
- **코드**: STM32CubeIDE, HAL 라이브러리, NUCLEO-F411RE

## 에이전트 팀 구성 (7명)

| 에이전트 | 역할 | 파일 |
|---------|------|------|
| 📋 총괄 편집장 | 프로젝트 관리, 최종 품질 결정 | agents/editor_in_chief.md |
| ✍️ 기술 저자 | HAL 코드·SVG·본문 집필 | agents/technical_author.md |
| 🔍 기술 리뷰어 | STM32 코드 정확성, 실무 적합성 | agents/technical_reviewer.md |
| 🎓 독자 | 중급 개발자 관점 이해도 평가 | agents/beginner_reader.md |
| 📐 교육 설계자 | 학습 흐름, 누적 성장 구조 | agents/instructional_designer.md |
| 🧠 교육심리전문가 | 학습 동기, 불안 관리 | agents/educational_psychologist.md |
| 🎤 교육전문강사 | 강의 적합성, 비유 검증 | agents/expert_instructor.md |

## 집필 워크플로우

```
Phase 1: 기획 회의  (저자 + 교육설계자 + 강사 병렬)
Phase 2: 초안 작성  (기술 저자 단독)
Phase 3: 병렬 리뷰  (리뷰어 + 독자 + 심리 + 강사 동시)
Phase 4: 종합 수정  (회의록 작성 → 수정 → 편집장 승인)
```

상세: `workflows/agent_team_workflow.md` 참조

## 누적 성장 로드맵

```
Part 0  Ch01~03   기반 구축 (GPIO/로그/아키텍처)
Part 1  Ch04~06   통신 기초 + DMA
Part 2  Ch07~10   타이머 + RTC + 스텝 모터     ← 마일스톤 ① v1.0
Part 3  Ch11~14   센서 + 통신 심화
Part 4  Ch15~16   CLI + PC 대시보드            ← 마일스톤 ② v1.6
Part 5  Ch17      아키텍처 심화 & 통합
Part 6  Ch18~19   디버깅 + 리팩토링             ← 마일스톤 ③ v2.2
```

## 디렉터리 구조

```
stm32-advanced/
├── CLAUDE.md                   Claude Code 프로젝트 지침
├── README.md
├── TABLE_OF_CONTENTS.md        전체 목차
├── .claude/settings.json       Agent Teams 활성화
├── agents/                     에이전트 정의 7명
├── templates/                  HTML 템플릿 + CSS
├── workflows/                  워크플로우 정의
├── manuscripts/                원고 HTML (partN/chapterNN.html)
├── code_examples/              HAL 코드 예제
├── figures/                    SVG 다이어그램
├── review_logs/                리뷰 기록
└── output/                     최종 산출물 (ppt/workbook/docx)
```

## 사용법

### Claude Code에서 실행
```bash
cd stm32-advanced
claude
```

### 주요 명령어
```
"Chapter 1 기획 회의를 시작해줘"
"Chapter 1 초안을 작성해줘"
"Chapter 1 초안에 대해 에이전트 팀 리뷰를 진행해줘"
"Chapter 1 종합 회의 및 수정을 진행해줘"
"Part 0 전체 품질 체크리스트를 실행해줘"
```

## 원고 미리보기
브라우저에서 `manuscripts/partN/chapterNN.html` 파일을 열어 확인합니다.
