# Favorite Widget

`Favorite Widget`은 Windows 바탕화면에서 자주 쓰는 파일과 URL을 바로 실행할 수 있게 정리하는 데스크톱 런처 도구입니다.

- 저장소: `https://github.com/sheryloe/Favorit`
- GitHub Pages: `https://sheryloe.github.io/Favorit/`

## 서비스 개요

- 실행 파일, 문서, URL을 즐겨찾기 형태로 등록합니다.
- 위젯처럼 빠르게 실행하는 경험을 목표로 합니다.
- 설치 파일과 공개 소개 페이지까지 포함한 Windows 도구 프로젝트입니다.

## 핵심 기능

- `.exe`, 문서/파일, `http/https` 링크 등록
- 실행 파일의 기본 아이콘 표시
- 별도 설정 창에서 추가/수정/삭제
- NSIS 기반 설치 파일 생성

## 스택

- C++
- Win32 API
- CMake
- NSIS

## 빌드

```powershell
cmake -S . -B build -G "MinGW Makefiles"
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
