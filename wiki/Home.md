# Favorit Wiki

## Project Summary

Favorite Widget은 기존 INI 경로와 호환성을 유지한 상태로,
위젯은 Win32로 동작하고 설정 UI는 WinUI 3로 분리했습니다.
등록 항목(앱/폴더/URL), 창 위치, 메타데이터는 기존 `favorite-widget.ini`에 저장됩니다.

## Quick Links

- [README.md](../README.md)
- [GitHub Pages](../docs/index.html)
- [Service-Roadmap.md](./Service-Roadmap.md)

## What is changed

- 메인 위젯의 설정 버튼은 WinUI 설정 앱 실행 경로를 호출합니다.
- WinUI 설정 exe가 발견되면 자동으로 직접 실행합니다.
- `favorite-widget.ini`가 변경되면 위젯이 자동 새로고침 됩니다.
- INI 포맷/키/저장 경로는 기존 형식을 그대로 유지합니다.

## UI 정리 상태

- 메인 그리드 타일/아이콘/라벨 간격을 240x160 비율 감각에 맞게 정렬했습니다.
- 빈 상태/리스트 항목/명령 버튼 배치의 폰트·패딩·아이콘 위치를 통일했습니다.
- 설정 화면은 리스트 + 폼 + 액션 버튼 배치로 밀집되지 않게 정돈했습니다.

## Notes

- 기존 사용자는 INI 이동 없이 바로 사용할 수 있어 마이그레이션 부담이 낮습니다.
- UI 변경 시 docs/wiki/README를 함께 갱신합니다.
