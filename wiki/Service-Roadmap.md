# Favorit Service Roadmap

## Scope

로드맵은 WinUI 3 설정 분리 완성, 패키징 안정성, 운영 운영성 강화에 초점을 둡니다.

## Priority 1

- 설정 UI가 list + form + command bar 구조의 WinUI 3로 정리됐습니다.
- WinUI에서 INI 읽기/쓰기 서비스를 기존 경로/섹션 그대로 사용하도록 유지했습니다.
- `favorite-widget.ini` 변경 시 위젯 자동 갱신 동작을 보장합니다.
- GitHub Pages, README, Wiki 업데이트를 동기화 체크리스트에 포함했습니다.

## Priority 2

- 검색/필터 성능 개선과 오류 상태 메시지 일관성 작업
- 테스트 스냅샷(항목 추가/수정/삭제 후 반영) 자동화 보강
- 설정 실행 경로 해석 실패 시 크래시 없이 안전하게 fallback 처리

## Ops

- 위젯/설정 패키지 동시 릴리스 체크리스트를 문서화합니다.
- WinUI 설정 앱은 MSIX 기준으로 정식 배포 산출물에 포함하고, NSIS와 ZIP도 병행 제공합니다.
- WinUI 툴체인 미설치 환경에서의 우회 동작과 트러블슈팅 문서를 확장합니다.

## Visual Refine Note

- 아이콘 배치와 카드 간격을 기준 샷 기준(컴팩트 240x160) 느낌으로 재설계했습니다.
- 이 변경은 기능 동작에는 영향이 없으며 GitHub Pages 이미지 갱신 포인트로 분류합니다.
