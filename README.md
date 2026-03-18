# Favorit


## 서비스 개요

- 딥 블랙 배경과 네온 시안(Neon Cyan) 액센트를 활용한 HUD 감성의 데스크톱 위젯입니다.
- 전역 단축키를 통해 게임 중이거나 전체화면 작업 중에도 위젯을 호출하거나 다이렉트로 프로그램을 실행할 수 있습니다.
-
- **모든 항목 지원:** `.exe` 실행 파일, 문서, 폴더, `http/https` 웹 링크 지원
-t
- C++
- Win32 API
- CMake
- NSIS
d -G "MinGW Makefiles"
cmake --build build
```

또는:

```powershell
npm run build
```

## 실행

```powershell
.\build\favorite_widget.exe
```

## 배포 자산

- 설치 파일: `docs/assets/downloads/FavoriteWidget-0.2.0-win64.exe`
- 스크린샷: `docs/assets/screenshots/`

## 다음 단계

- Windows 위젯 감성의 UI 리뉴얼
- 드래그 정렬, 검색, 그룹화
- 전역 단축키와 시작 프로그램 등록
- 설정 백업/복원
