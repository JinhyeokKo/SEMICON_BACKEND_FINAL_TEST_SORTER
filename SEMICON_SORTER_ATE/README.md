# LabVIEW 가상 검사기

LS 공정의 검사 요청을 받아 가상 전압 또는 직접 선택한 결과로 PASS, FAIL, RETEST를 결정한다. CIMON의 Modbus TCP 서버를 통해 결과와 응답 ID를 전달하고, PLC 수락을 확인한 뒤 CSV에 한 행을 기록한다. 실제 DUT 전압을 측정하는 DAQ는 사용하지 않았다.

## 시작 파일

| 파일 | 용도 |
|---|---|
| `VirtualATE_V3.vi` | 최종 통합 시험의 가상 검사기 |
| `JudgeVoltage.vi` | 가상 전압의 하한·상한 판정 |
| `AppendInspectionRow.vi` | 수락 결과 CSV 기록 |
| `SEMICON_SORTER_ATE.lvproj` | LabVIEW 프로젝트 목록 |
| `ATE_CommCheck.vi`, `ATE_ManualReply.vi` | 초기 통신 확인용 VI |
| `VirtualATE.vi`, `VirtualATE_V2*.vi` | 이전 개발 단계 VI |

기록된 환경은 **LabVIEW 2026 Q1 32Bit**다. 최종 실행의 기준은 `VirtualATE_V3.vi`이며 파일명의 버전 숫자를 바꾸거나 이전 VI를 같은 시험에서 섞어 실행하지 않는다.

## 통신과 검사 흐름

```text
LabVIEW → CIMON Modbus TCP 서버 → LS 논리 레지스터
요청 읽기 → 요청 ID와 검사 조건 고정 → 약 1초 가상 검사
→ 결과·응답 ID 쓰기 → PLC echo 확인 → DONE 쓰기
→ PLC 수락 ID 확인 → CSV 1행 기록
```

| 항목 | 시험 설정 |
|---|---|
| 접속 | CIMON 서버 TCP 1502 / Unit ID 1 |
| 요청 읽기 | FC03, offset 0부터 14 words |
| 결과와 응답 ID | FC16, offset 3부터 2 words |
| 완료 표시 | PLC echo 확인 후 FC06, offset 5에 DONE=1 |
| 기본 가상 전압 | 3300 mV |
| 기본 합격 범위 | 3000-3600 mV, 경계 포함 |

`AutoVoltage` ON에서는 전압으로 결과를 판정한다. OFF에서는 검사 결과를 직접 선택한다. 이 검사기에서 말하는 수동 결과 선택과 LS의 Manual 운전 모드는 다른 기능이다.

## CSV

```text
captured_utc,request_id,mode,voltage_mV,low_mV,high_mV,result_code,accepted_utc,accepted_id
```

- `result_code`: 1 PASS / 2 FAIL / 3 RETEST.
- MANUAL 검사에서는 전압과 상·하한 칸을 비운다.
- CSV 시각은 UTC이며 시험 원장의 KST와 9시간 차이가 있다.
- PLC 수락 시점에 기록하므로 수락 후 공정이 중단되면 배출 완료 수량과 다를 수 있다.
- 시험별 실행을 분리했다. 서로 다른 CSV의 요청 ID가 반복된다는 이유로 중복 기록으로 합산하지 않는다.

## 열기와 실행

1. 프로젝트 또는 `VirtualATE_V3.vi`를 연다. 하위 VI 찾기 창이 나오면 이 폴더의 `JudgeVoltage.vi`와 `AppendInspectionRow.vi`를 지정한다.
2. CIMON 서버 주소와 포트, Unit ID를 현재 실행 환경에 맞춘다.
3. 존재하는 저장 폴더의 새 CSV 파일을 `LogFilePath`에 지정한다.
4. CIMON, LS 통신과 요청 대기 상태를 준비한 후 VI를 실행한다.
5. `PLC_LIVE_OK`, `ATE_READY` 및 처리 메시지를 확인하고 공정 START를 누른다.

현재 `.lvproj`의 VI 항목에는 `../파일명.vi` 경로가 남아 있고 실제 VI는 프로젝트와 같은 폴더에 있다. 다른 위치에 복사한 프로젝트는 LabVIEW에서 경로를 확인하고 필요하면 **작업 사본**을 재저장한다. 이식 후 경로 확인은 기존 기능 시험의 합격과 구분한다.

검증 범위와 실제 결과는 [완료보고서와 원장](../06_TEST/README.md), 요청·수락 계약은 [최종 도면 008-009](../02_PDF/02_FINAL/)을 참조한다.
