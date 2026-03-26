# Favorit Wiki

## 개요

Favorite Widget은 작은 실행 위젯에 집중한 Windows 런처입니다.

- 메인 위젯은 Win32 기반으로 유지합니다.
- 설정 창은 WinUI 3로 분리합니다.
- 저장 포맷은 기존 `favorite-widget.ini`를 그대로 사용합니다.

## 현재 정리 방향

- 메인 위젯은 240x160 밀도에 맞춰 버튼, 아이콘, 라벨 간격을 다시 맞춥니다.
- Pages는 실제 프로그램 화면이 먼저 보이도록 hero와 스크린샷 구성을 설계합니다.
- 문서 톤은 짧고 직접적인 설명 위주로 통일합니다.

## 빠른 링크

- [README.md](../README.md)
- [GitHub Pages](https://sheryloe.github.io/Favorit/)
- [Service-Roadmap.md](./Service-Roadmap.md)

## 동작 기준

- 위젯은 설정 앱이 없어도 INI를 읽어 동작합니다.
- 설정 앱에서 저장한 변경 사항은 위젯에 다시 반영됩니다.
- INI 포맷, 키, 저장 경로는 기존 형식을 유지합니다.

## 운영 메모

- UI 조정 후에는 스크린샷도 같이 갱신합니다.
- Pages 변경 후에는 Docker + Playwright 감사로 레이아웃을 확인합니다.
- 문서 변경은 `README`, `wiki`, `docs`를 같은 기준으로 맞춥니다.
