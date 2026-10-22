# CIMON 운전 화면과 통신 중계

CIMON SCADA에서 공정 조작, 알람, 검사 이력과 환경 데이터를 표시한다. 실제 ESP32의 Modbus TCP 데이터를 FX에 전달하고, FX 환경 판정 워드를 LS에 중계한다. LabVIEW에는 검사 요청·결과 교환용 Modbus TCP 서버를 제공한다.

## 시작 파일

**CIMON 3.91.10**에서 `SEMICON_SORTER_HMI.prj`를 연다. 프로젝트 DB·페이지·스크립트·I/O 디바이스 파일을 같은 폴더 구조로 보존한다. `.prj` 하나만 복사하면 전체 프로젝트가 되는 것은 아니다.

| 파일 | 용도 |
|---|---|
| `SEMICON_SORTER_HMI.prj` | CIMON 프로젝트 |
| `CimonDbm.mdb` 및 프로젝트 동반 파일 | 태그와 프로젝트 데이터 |
| `SEMICON_SORTER_HMI.scx` | 프로젝트 스크립트 |
| `*.pgx` | 화면 페이지 |
| `ESP.dvx`, `FX-OPC.dvx`, `LABVIEW.dvx`, `LS XGB ENET.dvx` | 통신 디바이스 구성 |

## 화면

| 페이지 | 역할 |
|---|---|
| `MAIN.pgx` | 공정 상태, 운전 조건, 장치 명령·응답, 수량, START/STOP/RESET/ACK |
| `ALARM.pgx` | 현재 알람과 과거 이력 |
| `INSPECTION.pgx` | CSV 기반 최근 검사 결과 조회 |
| `ENVIRONMENT.pgx` | FX-LS 환경 판정 전달과 운전 허가 |
| `SENSOR.pgx` | ESP 원시값, 준비·적용 ACK 및 센서 중계 상태 |
| `SAFETY.pgx` | 비상정지·문·고장 등 원시 모사 입력 조작 |

`SYSTAGSCALE.pgx`와 `SYSTAGSELECT.pgx`는 프로젝트에 함께 있는 보조 페이지다. 3D 공정 애니메이션, 개별 Jog와 사용자 권한 로그인은 구현 범위에 포함하지 않았다.

## 통신 경로

```text
ESP32 ── Modbus TCP 502 / FC04 ──→ CIMON
                                      │ 센서 프레임 검증과 전달
                                      ↓
                             MX OPC DA ↔ GX Simulator
                                      │ FX 환경 판정 워드
                                      ↓
LS XG-SIM ←── XGT Fast Ethernet ─── CIMON

LabVIEW ↔ CIMON Modbus TCP 서버 1502 / Unit ID 1
```

태그 이름과 그룹은 현재 DB를 기준으로 한다. 예전 안내의 별칭을 실제 페이지명이나 DB 태그명으로 자동 치환하지 않는다. LS 시뮬레이션 M 영역은 읽기 주소와 쓰기 주소를 구분하며 최종 논리 상태를 가상 조작 태그로 덮어쓰지 않는다.

## 조작과 종료

1. LS와 FX 시뮬레이터, MX OPC 및 ESP 접속을 준비한다.
2. CIMON을 실행해 각 통신 대상의 현재값과 갱신을 확인한다.
3. 환경 중계를 시작하고 FX·LS 자료 정상을 확인한 다음 센서 중계를 시작한다. 적용 ACK와 환경 허가 상태를 확인한다.
4. 원시 안전 입력을 정상으로 복구하고 안전 복귀와 공정 RESET을 구분해 조작한다.
5. LabVIEW의 ATE 준비 상태를 확인하고 새 START를 누른다.
6. 종료할 때는 공정을 대기 상태로 만들고 LabVIEW, 센서 중계, 환경 중계를 정지한 뒤 CIMON을 정상 종료한다.

개별 시험의 정확한 입력 조건과 종료 순서는 [실행기록과 완료보고서](../06_TEST/README.md)를 따른다. PC를 바꿔 실행할 때는 IP, OPC 대상, CSV 저장 경로와 읽기·쓰기 매핑을 실제 구성에 맞춘다. 가상 안전 팝업은 실제 안전회로를 대체하지 않는다.
