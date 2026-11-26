// 릴레이 순환 시퀀서(Counter) 에뮬레이션 - Arduino Mega 2560
//
// 회로의 동작 규칙(릴레이 접점 논리)을 그대로 옮긴 것:
//   K(n) 코일은 "앞 단 K(n-1)이 ON 이고, 그 앞 단 K(n-2)가 OFF" 일 때 켜진다.
//   (인덱스는 N단 링으로 순환: K1의 앞 단은 K_N)
// 릴레이 코일의 동작 지연(PULL_IN_MS)과 캐패시터 유지시간(RELEASE_MS)도 흉내 낸다.
//
// 최소 3단부터 동작한다. 2단은 논리상 순환이 성립하지 않는다.
//
// + Program RAM : 16비트 데이터 / 10비트 주소 (ram_model.h,      초기 데이터는 ram_data.h)
// + Data RAM    :  8비트 데이터 / 10비트 주소 (data_ram_model.h, 초기 데이터는 data_ram_data.h)
//   시리얼 모니터(115200bps)에서 명령으로 읽기/쓰기/덤프 가능 (주소/데이터는 16진수):
//     첫 글자가 메모리 선택:  p = Program RAM,  d = Data RAM
//     둘째 글자가 동작:       r = 읽기, w = 쓰기, d = 덤프
//       pr 100          Program RAM 0x100 읽기
//       pw 100 BEEF     Program RAM 0x100 에 0xBEEF 쓰기
//       pd 0 20         Program RAM 0x000 부터 0x20개 덤프
//       dr 10           Data RAM 0x010 읽기
//       dw 10 5A        Data RAM 0x010 에 0x5A 쓰기
//       dd 0 20         Data RAM 0x000 부터 0x20개 덤프
//     i                 RAM 상태 정보

#include <Arduino.h>
#include "prog_ram_model.h"
#include "data_ram_model.h"

const uint8_t  N            = 4;      // 순환 단수 (3 ~ MAX_STAGES)
const uint8_t  MAX_STAGES   = 16;

// 각 단의 출력 핀 (회로의 J6 출력 = 릴레이 ON일 때 HIGH)
// 핀 22~41은 확장용으로 비워 두었다.
const uint8_t  OUT_PIN[MAX_STAGES] = {42, 43, 44, 45, 46, 47, 48, 49,
                                      50, 51, 52, 53, 14, 15, 16, 17};

// 시작 버튼 (회로의 J5 'Loop Chain Start'에 해당). 누르는 동안 LOW
const uint8_t  START_PIN    = 2;

// 릴레이 타이밍 (실제 회로에서 측정한 값으로 조정)
const uint16_t PULL_IN_MS   = 5;      // 코일 인가 후 접점이 붙기까지
const uint16_t RELEASE_MS   = 300;    // 코일 전원 소실 후 떨어지기까지 (캐패시터 유지시간 포함)

// 전원 투입 시 자동으로 시작 펄스를 줄지
const bool     AUTO_START     = true;
const uint16_t START_PULSE_MS = 50;

Ram1kx16 progRam;     // Program RAM (16비트)
Ram1kx8  dataRam;     // Data RAM (8비트)

bool     on[MAX_STAGES];              // 릴레이 접점 상태
bool     coil[MAX_STAGES];            // 코일 전원 상태
uint32_t coilChangedAt[MAX_STAGES];   // 코일 상태가 바뀐 시각

bool startActive(uint32_t now) {
  if (digitalRead(START_PIN) == LOW) return true;
  return AUTO_START && now < START_PULSE_MS;
}

// ---------------- RAM 시리얼 명령 ----------------
static void printHex(uint16_t v, uint8_t digits) {
  for (int8_t sh = (digits - 1) * 4; sh >= 0; sh -= 4) Serial.print((v >> sh) & 0xF, HEX);
}

static bool parseHex(char *&p, uint16_t &out) {
  char *end;
  unsigned long v = strtoul(p, &end, 16);
  if (end == p) return false;
  out = (uint16_t)v;
  p = end;
  return true;
}

static void dump(bool prog, uint16_t a, uint16_t n) {
  uint8_t perLine = prog ? 8 : 16;
  for (uint16_t i = 0; i < n; i++) {
    uint16_t ad = (a + i) & RAM_ADDR_MASK;
    if (i % perLine == 0) { if (i) Serial.println(); printHex(ad, 3); Serial.print(F(":")); }
    Serial.print(' ');
    if (prog) printHex(progRam.read(ad), 4); else printHex(dataRam.read(ad), 2);
  }
  Serial.println();
}

static void runCommand(char *s) {
  if ((s[0] | 0x20) == 'i' && s[1] == 0) {
    Serial.print(F("Program RAM 1024 x 16bit, 초기 데이터 ")); Serial.print((unsigned long)RAM_IMAGE_WORDS);
    Serial.println(F("워드 (나머지는 0)"));
    Serial.print(F("Data RAM    1024 x  8bit, 초기 데이터 ")); Serial.print((unsigned long)DATA_RAM_IMAGE_BYTES);
    Serial.println(F("바이트 (나머지는 0)"));
    return;
  }

  char mem = s[0] | 0x20;                 // 'p' 또는 'd'
  char op  = s[0] ? (s[1] | 0x20) : 0;    // 'r', 'w', 'd'
  bool prog = (mem == 'p');
  bool valid = (mem == 'p' || mem == 'd') && s[0] && s[1];
  char *p = valid ? s + 2 : s;
  uint16_t a = 0, v = 0;

  if (valid && op == 'r' && parseHex(p, a)) {
    a &= RAM_ADDR_MASK;
    Serial.print(prog ? F("PROG[") : F("DATA[")); printHex(a, 3); Serial.print(F("] = "));
    if (prog) printHex(progRam.read(a), 4); else printHex(dataRam.read(a), 2);
    Serial.println();
  } else if (valid && op == 'w' && parseHex(p, a) && parseHex(p, v)) {
    a &= RAM_ADDR_MASK;
    Serial.print(prog ? F("PROG[") : F("DATA[")); printHex(a, 3); Serial.print(F("] <- "));
    if (prog) { progRam.write(a, v);          printHex(v, 4); }
    else      { dataRam.write(a, (uint8_t)v); printHex(v & 0xFF, 2); }
    Serial.println();
  } else if (valid && op == 'd' && parseHex(p, a)) {
    uint16_t n = 16;
    parseHex(p, n);
    dump(prog, a, n);
  } else {
    Serial.println(F("명령: p|d + r|w|d  예) pr 100 / pw 100 BEEF / pd 0 20 / dr 10 / dw 10 5A / dd 0 20 / i  (16진수)"));
  }
}

static void handleSerial() {
  static char line[32];
  static uint8_t len = 0;
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\r' || ch == '\n') {
      if (len) { line[len] = 0; runCommand(line); len = 0; }
    } else if (len < sizeof(line) - 1) {
      line[len++] = ch;
    }
  }
}

void setup() {
  Serial.begin(115200);
  progRam.begin();
  dataRam.begin();
  pinMode(START_PIN, INPUT_PULLUP);
  for (uint8_t k = 0; k < MAX_STAGES; k++) {
    pinMode(OUT_PIN[k], OUTPUT);
    digitalWrite(OUT_PIN[k], LOW);
  }
  if (N < 3 || N > MAX_STAGES) {
    Serial.println(F("N must be 3..MAX_STAGES"));
    while (true) {}
  }
}

void loop() {
  uint32_t now = millis();
  handleSerial();

  // 1) 현재 접점 상태로부터 각 코일에 전원이 들어오는지 계산
  bool next[MAX_STAGES];
  for (uint8_t k = 0; k < N; k++) {
    uint8_t p = (k + N - 1) % N;      // 앞 단
    uint8_t q = (k + N - 2) % N;      // 앞의 앞 단
    next[k] = on[p] && !on[q];
  }
  // 시작 펄스: K1의 pin13(= 마지막 단이 OFF일 때 +12V)을 K1 코일+에 연결
  if (startActive(now) && !on[N - 1]) next[0] = true;

  // 2) 릴레이 동작 지연 반영
  for (uint8_t k = 0; k < N; k++) {
    if (next[k] != coil[k]) { coil[k] = next[k]; coilChangedAt[k] = now; }
    if (coil[k]  && !on[k] && now - coilChangedAt[k] >= PULL_IN_MS) on[k] = true;
    if (!coil[k] &&  on[k] && now - coilChangedAt[k] >= RELEASE_MS) on[k] = false;
  }

  // 3) 출력 갱신 및 변화 시 시리얼 출력
  static uint16_t lastMask = 0xFFFF;
  uint16_t mask = 0;
  for (uint8_t k = 0; k < N; k++) {
    digitalWrite(OUT_PIN[k], on[k] ? HIGH : LOW);
    if (on[k]) mask |= (1u << k);
  }
  if (mask != lastMask) {
    lastMask = mask;
    Serial.print(now); Serial.print(F(" ms  "));
    for (uint8_t k = 0; k < N; k++) Serial.print(on[k] ? '1' : '.');
    Serial.println();
  }
}
