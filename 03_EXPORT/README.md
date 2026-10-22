# 도면 표 원본

최종 도면의 주소표, 설정표, 동작 정책 및 검증 요약에 사용한 XLSX 표 원본 27개를 보관한다. 파일명 앞의 3자리 번호는 대응하는 도면 번호다. 한 도면에 여러 표가 사용되어 도면 22장과 표 27개의 개수는 서로 다르다.

## 찾는 방법

1. [PDF 도면](../02_PDF/README.md)에서 해당 기능과 도면 번호를 확인한다.
2. [최종 표 폴더](02_FINAL/)에서 같은 번호로 시작하는 파일을 찾는다.
3. 구현 값을 바꾸면 표와 DWG, PDF 및 관련 시험 근거를 함께 대조한다.

대표 표는 다음과 같다.

- [001_DRAWING_INDEX](02_FINAL/001_DRAWING_INDEX.xlsx)
- [008_VIRTUAL_ATE_REGISTER_MAP](02_FINAL/008_VIRTUAL_ATE_REGISTER_MAP.xlsx)
- [011_FX_SENSOR_INTERFACE_REGISTER_MAP](02_FINAL/011_FX_SENSOR_INTERFACE_REGISTER_MAP.xlsx)
- [017_VERIFICATION_AND_EVIDENCE_STATUS](02_FINAL/017_VERIFICATION_AND_EVIDENCE_STATUS.xlsx)

## 자료의 역할

이 폴더의 XLSX는 도면 표의 편집 원본이며 PLC가 실행하는 레지스터 설정 파일이나 CIMON 데이터베이스가 아니다. 실제 시험 결과와 설정 기록은 [시험 원장](../06_TEST/README.md)을 사용한다.

`01_DRAFT`의 표는 이전 설계 이력이다. 현재 도면 검토에는 `02_FINAL`을 사용한다. Excel 표를 수정했다는 사실만으로 DWG와 PDF가 갱신됐다고 보지 않고 실제 출력 결과를 다시 확인한다.

[프로젝트 소개로 돌아가기](../README.md)
