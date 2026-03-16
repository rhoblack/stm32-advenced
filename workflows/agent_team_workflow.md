# Agent Teams 집필 워크플로우 — STM32 고급 실무교육

## 10단계 전체 워크플로우

```
STEP 1   기획 회의        편집장 + 저자 + 강사
STEP 2   초안 작성        기술 저자 단독
STEP 3   기술 리뷰        기술 리뷰어
STEP 4   독자 리뷰        독자 에이전트
STEP 5   교육 설계 리뷰   교육 설계자
STEP 6   심리 리뷰        교육심리전문가
STEP 7   강의 적합성 리뷰 교육전문강사
STEP 8   리뷰 종합 회의   전체 7명
STEP 9   수정             기술 저자
STEP 10  최종 승인        편집장
```

Agent Teams 사용 시 10단계를 **4 Phase로 압축하여 병렬 실행**합니다.

---

## Phase 1: 기획 회의

**참여**: 저자 + 교육설계자 + 강사 (3 teammates 병렬)
**출력**: `review_logs/chapterNN_plan.md`

```
Chapter [N] 기획 회의를 위한 에이전트 팀을 만들어줘.

세 명의 teammate를 병렬로 생성해줘:

1. 기술 저자 teammate:
   - agents/technical_author.md 파일을 읽어 역할을 숙지해줘
   - TABLE_OF_CONTENTS.md에서 Chapter [N]의 내용을 분석해줘
   - 각 절의 핵심 메시지, HAL 코드 예제 목록, SVG 다이어그램 목록을 작성해줘
   - 아키텍처 위치 섹션(레이어/인터페이스/FSM/시퀀스)을 기획해줘

2. 교육 설계자 teammate:
   - agents/instructional_designer.md 파일을 읽어 역할을 숙지해줘
   - Chapter [N]의 학습 목표 3~5개를 "~할 수 있다" 형태로 정의해줘
   - 블룸 분류체계 기준으로 인지 부하 설계 계획을 작성해줘
   - 이전 챕터와의 누적 성장 연결점을 명시해줘

3. 교육전문강사 teammate:
   - agents/expert_instructor.md 파일을 읽어 역할을 숙지해줘
   - Chapter [N]에서 수강생이 막힐 것으로 예상되는 포인트 Top 5를 작성해줘
   - 각 막힘 포인트에 대한 사전 해소 방법을 제안해줘

세 teammate가 각자 작업 완료 후 서로의 의견을 검토하고 통합해서
review_logs/chapter[NN]_plan.md를 최종 완성한 다음 팀 리드에게 보고해줘.
완료 후 팀을 정리해줘.
```

---

## Phase 2: 초안 작성

**참여**: 기술 저자 (단독)
**출력**: `manuscripts/partN/chapterNN.html` + SVG + 코드

```
기술 저자 teammate를 생성해줘.

agents/technical_author.md 파일을 읽어 역할을 숙지한 후:

1. review_logs/chapter[NN]_plan.md를 읽어 기획 내용을 파악해줘
2. CLAUDE.md의 아키텍처 적용 원칙을 확인해줘
3. templates/chapter_template.html 구조를 따라
   manuscripts/part[N]/chapter[NN].html을 HTML로 집필해줘
   - Ch04 이상: 도입부에 아키텍처 위치 / 인터페이스 설계 / FSM / 시퀀스 다이어그램 섹션 포함
   - 모든 코드에 LOG_D/I/W/E 적용 (Ch02 표준)
4. 다이어그램은 figures/ 디렉터리에 SVG로 생성해줘
5. 코드 예제는 code_examples/ 디렉터리에 별도 파일로 저장해줘

집필 중 각 절 완료 시 팀 리드에게 진행 상황을 보내줘.
전체 완료 후 팀 리드에게 보고해줘.
```

---

## Phase 3: 병렬 리뷰

**참여**: 리뷰어 + 독자 + 심리 + 강사 (4 teammates 동시)
**출력**: 4개 리뷰 파일

```
Chapter [N] 초안(manuscripts/part[N]/chapter[NN].html)에 대한
병렬 리뷰 팀을 만들어줘. 네 명의 teammate를 동시에 생성해줘:

1. 기술 리뷰어 teammate:
   - agents/technical_reviewer.md 역할
   - STM32 HAL 코드 정확성, NUCLEO-F411RE 핀 충돌, 실무 적합성 검토
   - 아키텍처 위치 섹션의 레이어 배치 적절성 검토
   - review_logs/chapter[NN]_tech_review.md에 [Critical/Major/Minor] 분류로 저장

2. 독자 teammate:
   - agents/beginner_reader.md 역할
   - 중급 개발자 관점 이해도 5점 척도 평가, 설명 없이 등장하는 용어 목록
   - review_logs/chapter[NN]_beginner_review.md에 저장

3. 교육심리전문가 teammate:
   - agents/educational_psychologist.md 역할
   - 학습 불안 지점, 자기효능감 위기 포인트, 감정 곡선 분석
   - review_logs/chapter[NN]_psych_review.md에 저장

4. 교육전문강사 teammate:
   - agents/expert_instructor.md 역할
   - 설명·비유 품질, 수강생 단골 질문 추가 제안, 4시간 강의 흐름 평가
   - review_logs/chapter[NN]_instructor_review.md에 저장

각 teammate가 리뷰 완료 후 서로의 리뷰 파일을 읽고 보완 의견을 교환한 다음
팀 리드에게 보고해줘. 완료 후 팀을 정리해줘.
```

---

## Phase 4: 종합 회의 및 수정

**참여**: 편집장 종합 → 기술 저자 수정 → 편집장 승인
**출력**: `review_logs/chapterNN_meeting.md` + 수정된 HTML

```
review_logs/chapter[NN]_*.md 파일을 모두 읽고 종합 회의록을 작성해줘.

회의록 형식 (review_logs/chapter[NN]_meeting.md):
- 각 리뷰어 피드백 요약
- Critical/Major 이슈 목록
- 반영할 피드백 vs 보류 피드백 (우선순위: 정확성 > 심리적 안전 > 이해도 > 분량)
- Action Items

그런 다음 기술 저자 teammate를 생성해서:
- agents/technical_author.md 역할
- Critical/Major 피드백을 우선 반영하여 chapter[NN].html을 수정
- 수정 완료 후 팀 리드에게 보고

수정 완료 후, 팀 리드(편집장)가 다음 기준으로 최종 승인 여부를 판단해줘:
- Critical 이슈 0개, Major 이슈 0개
- 독자 이해도 ⭐⭐⭐ 이상
- 교육 설계 / 심리적 안전성 / 강의 적합도 각 ⭐⭐⭐ 이상
```

---

## 단일 Teammate 빠른 검토

```
# 기술 검토만
기술 리뷰어 teammate를 하나 생성해줘. agents/technical_reviewer.md 역할로
manuscripts/part[N]/chapter[NN].html의 STM32 HAL 코드를 검토해줘.

# 독자 관점만
독자 teammate를 하나 생성해줘. agents/beginner_reader.md 역할로
manuscripts/part[N]/chapter[NN].html을 읽고 이해도 피드백을 해줘.

# 아키텍처 검토만
기술 리뷰어 teammate를 하나 생성해줘. agents/technical_reviewer.md 역할로
manuscripts/part[N]/chapter[NN].html의 아키텍처 위치 섹션이
CLAUDE.md의 레이어드 아키텍처 원칙에 부합하는지 검토해줘.
```
