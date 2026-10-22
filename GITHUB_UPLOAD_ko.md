# GitHub 업로드 안내

이 안내는 공개할 사본과 로컬 작업 보관본을 구분하기 위한 것이다. 기존 프로그램, 기준선과 증거 원본을 삭제하는 작업이 아니다. GitHub에 실제로 게시하는 단계는 별도로 진행한다.

## 공개할 내용

- 루트와 각 폴더 README, 대표 캡처 3개
- 최종 독립 DWG 22장, 도면 PDF 22개, 표 XLSX 27개
- LS, FX, CIMON, LabVIEW 프로젝트와 ESP-IDF 소스 및 의존성 선언
- 시험 원장 XLSX, 완료보고서 DOCX, 배포 PDF
- 시험 캡처와 CSV, video/encoded 아래의 영상 압축 사본

기준선 BL-001과 원본 영상, 도면 및 표의 DRAFT는 로컬 보관한다. 원장에 남은 해당 경로는 원본 추적용이며 공개 저장소에서 파일이 보이지 않을 수 있다. 압축 영상은 열람용 사본으로 원장의 증거ID를 바꾸지 않는다.

## 실제 접속 정보가 있는 파일

공개 준비 과정에서 현재 ESP32 프로젝트의 sdkconfig, sdkconfig.old 및 build/config의 생성 설정 파일에 실제 Wi-Fi SSID와 비밀번호 설정이 존재하는 것을 확인했다. 값은 README나 감사 기록에 복사하지 않았다.

루트 .gitignore는 sdkconfig와 sdkconfig.old, build 폴더 전체를 제외한다. sdkconfig.defaults는 현재 Wi-Fi 두 필드가 빈 문자열인 기본값이므로 소스와 함께 유지한다. main/Kconfig.projbuild 및 dependencies.lock도 유지한다.

빌드 후 실행할 사람은 ESP-IDF menuconfig에서 자신의 접속 정보를 넣는다. 이 로컬 설정이나 접속 정보가 포함될 수 있는 펌웨어 바이너리는 공개 사본에 넣지 않는다.

## 큰 영상 파일

원본 영상 중 18개는 100 MiB를 넘었다. 현재 별도의 video/encoded 폴더에 있는 압축 사본을 배포 대상으로 구분한다. 압축이 진행 중이면 파일 개수와 크기는 최종 저장 후 다시 확인한다.

GitHub 일반 Git 저장소는 100 MiB 초과 파일을 차단하고, 브라우저 파일 업로드는 파일당 25 MiB 제한이 있다. 압축본 중에도 브라우저 제한을 넘는 파일이 있을 수 있으므로 기존에 사용하는 Git 도구나 편집기의 소스 제어에서 올리는 편이 적합하다. [GitHub 공식 파일 크기 안내](https://docs.github.com/en/repositories/working-with-files/managing-large-files/about-large-files-on-github)

추후 100 MiB를 넘는 공개 영상이 필요하면 파일을 더 줄이거나 Git LFS 또는 Release 배포 방식을 별도로 선택한다. 이 작업에서는 Git LFS를 설치하거나 설정하지 않았다.

## 업로드 목록 확인

.gitignore는 Git의 제외 규칙이다. 폴더를 웹 화면에 직접 끌어다 올리는 방식이 제외 규칙을 대신 적용해 주는 것으로 가정하지 않는다. 이미 Git에 추적된 파일도 .gitignore만으로 제거되지는 않는다. [GitHub 공식 제외 파일 안내](https://docs.github.com/en/get-started/git-basics/ignoring-files)

프로젝트 최상위 폴더의 터미널에서 다음을 확인한다. 아직 Git 저장소가 아니라면 먼저 기존에 사용하는 Git 도구에서 저장소로 초기화한다.

```text
git status --short --ignored
git check-ignore -v SEMICON_SORTER_SENSOR/sdkconfig
git check-ignore -v SEMICON_SORTER_SENSOR/sdkconfig.old
git check-ignore -v SEMICON_SORTER_SENSOR/build/config/sdkconfig.h
git add --dry-run .
```

접속 설정 파일과 build가 제외되고, README·소스·최종 도면·기록 문서 및 encoded 압축 사본이 추가 대상인지 확인한 뒤 실제 추가와 커밋을 진행한다. 이 안내에는 실제 접속값이나 원격 저장소 URL을 적지 않았다.

## 네이티브 파일과 링크

- CAD는 최종 DWG를 독립 파일로 공개한다. 기존 WDP/AEPX에는 DRAFT 도면도 등록되어 있어 공개 기본 목록에서 제외했다. 원본 Electrical 프로젝트는 로컬에 보관한다.
- LabVIEW의 주 진입점은 VirtualATE_V3.vi다. lvproj의 과거 상대 경로는 공개 사본을 실제 LabVIEW에서 열어 확인한다. 핵심 VI 파일은 같은 폴더에 있다.
- OPC와 CIMON의 독점 형식 설정 DB는 파일 존재와 역할을 확인했으며 로그인 정보가 없다고 검증한 상태는 아니다. 외부 접속 계정을 사용했다면 해당 도구에서 공개할 사본의 연결 설정을 확인한다.
- 외부 문서나 설치 프로그램은 새로 포함하지 않는다. 04_REFERENCE는 현재 README만 공개한다.

## 업로드 후 확인

GitHub의 루트 README에서 대표 화면 3개, 폴더 안내, 최종 도면과 보고서 링크를 열어 본다. 시연 영상 압축본의 재생 및 다운로드도 확인한다. 라이선스는 소유자가 별도로 선택하기 전까지 자동으로 추가하지 않는다.
