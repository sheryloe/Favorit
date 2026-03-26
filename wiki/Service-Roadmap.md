# Favorit Service Roadmap

## 방향

로드맵은 세 가지에 집중합니다.

- 메인 위젯 실행 경험 정리
- WinUI 3 설정 분리 완성
- 문서와 배포 흐름 자동 검증

## Priority 1

- Win32 메인 위젯의 밀도, 아이콘 배치, 라벨 정렬을 240x160 기준으로 고정
- WinUI 3 설정 창에서 목록, 폼, 액션 버튼 흐름 정리
- `favorite-widget.ini` 변경 후 위젯 자동 갱신 안정화
- GitHub Pages hero와 스크린샷이 실제 프로그램 화면을 정확히 보여주도록 유지

## Priority 2

- 검색과 필터 흐름 정리
- 항목 추가, 수정, 삭제 후 반영 테스트 자동화 확대
- 설정 실행 경로 해석 실패 시 오류 메시지와 fallback 정리

## Ops

- `npm run test:crop`으로 스크린샷 규격 검증
- `npm run test:pages:docker`로 Pages 렌더링과 asset 상태 감사
- 릴리스 전 `docs`, `README`, `wiki` 동기화 확인

## 시각 기준

- 메인 위젯은 가장 먼저 식별되어야 합니다.
- 설정 화면은 보조 정보로 배치합니다.
- 설명 문구는 짧게 유지하고, 기능보다 화면 인지성을 우선합니다.
