# 챕터 집필 워크플로우 — STM32 고급 실무교육

## 챕터 HTML 필수 구조

```html
<!DOCTYPE html>
<html lang="ko">
<head>
  <meta charset="UTF-8">
  <title>Ch[NN]. [챕터 제목] — STM32 고급 실무교육</title>
  <link rel="stylesheet" href="../../templates/book_style.css">
</head>
<body>

<header class="chapter-header">
  <div class="chapter-meta">Part [N] · Chapter [NN] · 프로젝트 v[X.X]</div>
  <h1>Ch[NN]. [챕터 제목]</h1>
  <div class="chapter-summary">[한 줄 챕터 요약]</div>
</header>

<!-- Ch04 이상: 아키텍처 위치 섹션 필수 -->
<section class="architecture-position">
  <h2>이 챕터의 아키텍처 위치</h2>
  <!-- SVG: 전체 레이어 다이어그램에서 이 챕터 컴포넌트 강조 -->
  <img src="../../figures/ch[NN]_sec00_architecture.svg" alt="아키텍처 위치">

  <h3>인터페이스 설계</h3>
  <pre><code class="language-c">
/* 상위 레이어에 노출하는 API */
  </code></pre>

  <h3>FSM 설계</h3> <!-- 해당하는 경우 -->
  <img src="../../figures/ch[NN]_sec00_fsm.svg" alt="FSM 다이어그램">

  <h3>시퀀스 다이어그램</h3> <!-- 해당하는 경우 -->
  <img src="../../figures/ch[NN]_sec00_sequence.svg" alt="시퀀스 다이어그램">
</section>

<section class="learning-objectives">
  <h2>학습 목표</h2>
  <ul>
    <li>~할 수 있다 (블룸: 적용)</li>
    <li>~할 수 있다 (블룸: 분석)</li>
    <li>~할 수 있다 (블룸: 이해)</li>
  </ul>
  <div class="project-version">
    이 챕터 완료 시 프로젝트가 <strong>v[X.X]</strong>로 업그레이드됩니다.
    <br>추가 기능: [누적 기능 설명]
  </div>
</section>

<!-- 본문 섹션 반복 -->
<section>
  <h2>[절 제목]</h2>
  <!-- 도입 → 개념 → 비유 → 예시 → 코드 → 실습 → 정리 흐름 -->

  <aside class="tip">💡 <strong>실무 팁</strong><br>[내용]</aside>
  <aside class="faq">❓ <strong>수강생 단골 질문</strong><br>[내용]</aside>
  <aside class="interview">🎯 <strong>면접 포인트</strong><br>[내용]</aside>
  <aside class="metacognition">🔍 <strong>스스로 점검</strong><br>[내용]</aside>
  <aside class="instructor-tip">📌 <strong>강사 꿀팁</strong><br>[내용]</aside>
</section>

<section class="chapter-summary-section">
  <h2>핵심 정리</h2>
  <!-- 챕터 핵심 내용 3~5개 -->
</section>

<section class="exercises">
  <h2>연습문제</h2>
  <!-- 블룸 분류 최소 3수준 (기억/이해/적용) -->
</section>

<section class="next-chapter">
  <h2>다음 챕터 예고</h2>
  <!-- 다음 챕터와의 연결 + 현재 프로젝트 상태 요약 -->
</section>

</body>
</html>
```

---

## 집필 체크리스트 (절 작성 시)

- [ ] 절 분량 2000~4000자
- [ ] 새 개념 첫 등장 시 비유/실생활 예시 포함
- [ ] 기술 용어 첫 등장 시 한글 설명 병기 예: `DMA(직접 메모리 접근, Direct Memory Access)`
- [ ] 이전 챕터 코드를 기반으로 기능 추가하는 방식
- [ ] 모든 HAL 코드에 `LOG_D` / `LOG_I` / `LOG_W` / `LOG_E` 적용
- [ ] 한 절에 새 개념 3개 이하
- [ ] 코드 블록 30줄 이하
- [ ] SVG 다이어그램 사용 (ASCII art 금지)

## 코드 예제 파일 네이밍
```
code_examples/ch[NN]_[기능명].c
code_examples/ch[NN]_[기능명].h
```

## SVG 파일 네이밍
```
figures/ch[NN]_sec[NN]_[설명].svg
```
색상 팔레트:
- `#1A73E8` 파랑 — HAL 레이어
- `#34A853` 초록 — Driver 레이어
- `#FBBC04` 노랑 — Service 레이어
- `#EA4335` 빨강 — App 레이어
