# Favorit

Favorite Widget은 앱, 폴더, URL을 빠르게 실행하는 Windows 위젯입니다.  
메인 위젯은 Win32로 유지하고, 설정 경험은 WinUI 3로 분리하는 방향으로 정리하고 있습니다.

## 핵심 구성

- 메인 위젯: 240x160 기준의 컴팩트 실행 카드
- 설정 UI: WinUI 3 기반 편집 창
- 데이터 저장: 기존 `favorite-widget.ini` 경로와 키 유지
- 문서/배포: `docs` 기반 GitHub Pages 운영

## 실행

```powershell
cmake -S . -B build-vs -G "Visual Studio 17 2022"
cmake --build build-vs --config Release
.\build-vs\Release\favorite_widget.exe
```

## 문서 미리보기

```powershell
python -m http.server 4280 --directory docs
```

브라우저에서 `http://127.0.0.1:4280/index.html`로 확인합니다.

## 테스트

이미지 크롭 검증:

```powershell
npm run test:crop
```

Docker + Playwright Pages 감사:

```powershell
npm run test:pages:docker
```

감사 결과 산출물:

- 스크린샷: `build/playwright/desktop-home.png`
- 스크린샷: `build/playwright/mobile-home.png`
- 리포트: `build/playwright/report.json`

## 배포 자산

- 설치 파일: `docs/assets/downloads/FavoriteWidget-0.2.0-win64.exe`
- 스크린샷: `docs/assets/screenshots/`
- Pages 진입점: `docs/index.html`

## 운영 원칙

- 위젯 실사용 화면이 먼저 보이도록 문서와 스크린샷을 구성합니다.
- UI를 수정하면 `docs`, `README`, `wiki`를 함께 갱신합니다.
- 기존 사용자 데이터 호환성을 깨지 않는 방향을 우선합니다.
