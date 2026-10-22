# ESP32 온습도 수집 서버

ESP32-S3-DevKitC-1과 DFRobot SEN0137 DHT22로 온습도를 측정하고 Wi-Fi Modbus TCP 서버로 제공한다. 실제 센서 값은 CIMON을 거쳐 FX 환경 판정과 LS 공정 시작 허가에 사용했다.

## 개발 환경과 구성

| 항목 | 기록된 버전 또는 설정 |
|---|---|
| 프레임워크 | ESP-IDF 6.0.1 / target `esp32s3` |
| DHT 드라이버 | `esp-idf-lib/dht` 1.2.0 |
| 보조 컴포넌트 | `esp-idf-lib/esp_idf_lib_helpers` 1.4.0 |
| Modbus | `espressif/esp-modbus` 2.1.3 |
| 신호 | GPIO4 |
| 측정 주기 | 2500 ms |
| 정상 측정 미갱신 기준 | 6초 |

시험에서 사용한 배선은 SEN0137 VCC=3V3, 공통 GND, S 신호=GPIO4다. 이는 해당 시제품의 관측 조건이며 센서 전원 사양 적합성이나 장기 신뢰성을 검증한 결과는 아니다.

## 모듈

| 파일 | 역할 |
|---|---|
| `main/main.c` | 센서 서비스, Wi-Fi, Modbus 초기화 순서 |
| `main/dht22_sensor.c/.h` | DHT 읽기, 물리 범위 검사, 100배 정수 변환 |
| `main/sensor_state.c/.h` | 정상 순번, 상태, AGE와 원시 스냅샷 |
| `main/sensor_boot.c/.h` | NVS 부팅 ID |
| `main/sensor_service.c/.h` | 읽기·감시 task와 공유 상태 접근 |
| `main/sensor_wifi.c/.h` | Wi-Fi STA와 재접속 |
| `main/sensor_modbus.c/.h` | Modbus TCP 서버와 공개 레지스터 |
| `main/sensor_frame_pack.c/.h` | 동일 발행번호를 포함한 확장 프레임 |
| `main/idf_component.yml`, `dependencies.lock` | 컴포넌트 의존성 |
| `main/Kconfig.projbuild`, `sdkconfig.defaults` | 네트워크 메뉴와 공개 기본 설정 |

## 빌드와 실행

ESP-IDF 6.0.1 환경에서 프로젝트를 열고 target이 `esp32s3`인지 확인한다. ESP-IDF 터미널에서는 다음을 사용한다.

```sh
idf.py menuconfig
idf.py build
```

`menuconfig`의 **Sensor network**에서 사용할 2.4 GHz Wi-Fi를 로컬 설정한다. SSID 기본값이 빈 상태에서는 센서 읽기만 실행하고 Wi-Fi와 Modbus를 시작하지 않는다. 연결할 보드의 포트를 선택해 Flash와 Monitor를 실행한다. 모니터에서 받은 실제 IP를 CIMON 대상 주소에 적용한다. IP는 다른 네트워크에서도 동일하다고 가정하지 않는다.

`sdkconfig`, `sdkconfig.old`, 빌드 결과에는 로컬 접속 정보가 포함될 수 있다. 실제 자격 정보나 해당 바이너리를 공개하지 않는다. 공개 소스는 빈 네트워크 기본값을 제공하고 각 환경에서 재생성한다. NVS 저장 실패 시 초기화를 중단하며 펌웨어가 NVS를 자동 삭제하지 않는다.

## Modbus 공개 영역

**TCP 502 / Unit ID 1 / FC04 / offset 0-23, 총 24 words**다. 프로토콜 offset 0은 CIMON 드라이버의 Input Register 30001 표기에 대응한다.

| offset | 원시값 | 자료형과 의미 |
|---:|---|---|
| 0 | MAP_VERSION | U16, 1 |
| 1 | BOOT_ID | U16, 재부팅 식별 번호 |
| 2 | SAMPLE_SEQ | U16, 정상 측정 때만 증가 |
| 3 | TEMP_X100 | I16, 온도 ×100 |
| 4 | RH_X100 | U16, 습도 ×100 |
| 5 | SENSOR_VALID | U16, 0/1 |
| 6 | SENSOR_STATUS | U16, 상태 0-5 |
| 7 | SAMPLE_AGE_100MS | U16, 마지막 정상 측정 후 경과시간, 100 ms 단위 |

예를 들어 온도 25.3 °C와 습도 48.7 %RH는 2530과 4870이다. 음수 온도는 signed 16-bit로 복원한다. 원시 측정 묶음은 offset 0부터 8 words를 함께 읽는다.

offset 8-23은 전달 검증용 DWORD 8개다. 각 값은 다음 구조를 사용한다.

```text
DWORD = 발행번호 × 65536 + 원시 16-bit 워드
물리 워드 순서 = [발행번호, 원시값]
```

발행번호는 약 100 ms마다 전진하며 1-32000에서 순환한다. 정상 측정 순번 SAMPLE_SEQ와 별개다. CIMON은 8개 DWORD의 같은 발행번호와 형식을 확인한 뒤 FX의 준비, 미리보기, 적용, ACK 순서로 전달한다. 통신 DWORD 순서는 실제 CIMON 드라이버 설정과 검증 결과에 맞추며 단순한 숫자 변화만으로 허가를 판단하지 않는다.

## 측정 상태

| 코드 | 의미 |
|---:|---|
| 0 | 정상 측정 |
| 1 | 첫 정상 측정 대기 |
| 2 | 센서 읽기 오류 |
| 3 | 체크섬 오류 |
| 4 | 물리 범위 오류 |
| 5 | 정상 측정 이력은 있으나 6초 이상 미갱신 |

정상 읽기만 측정값, SEQ와 마지막 정상 시각을 갱신한다. 오류 때는 마지막 측정값을 보존하면서 VALID를 0으로 바꾼다. Wi-Fi 재접속이나 PC의 반복 읽기로 SEQ와 측정 AGE를 초기화하지 않는다. BOOT, SEQ와 발행번호는 서로 다른 사건을 구분한다.

## 통합 검증 범위

실제 센서 측정, Wi-Fi 및 Modbus 읽기, CIMON의 확장 프레임 검증, FX 적용 ACK, 환경 판정과 LS 허가 연동까지 통합 시험에 포함했다. ESP 전원 차단 후 데이터 차단과 수동 복구는 AE08에서 기록했다. 실제 실행 조건과 전체 25건의 결과는 [시험 자료](../06_TEST/README.md)와 [완료보고서](../06_TEST/06_FINAL_PDF/SIM_COMPLETION_REPORT_R0_ko.pdf)를 참조한다.

센서 측정 정확도의 교정 성적서, 장기 통신 안정성, 실제 PLC·제어반 및 현장 안전 검증은 이번 범위에 포함하지 않는다.
