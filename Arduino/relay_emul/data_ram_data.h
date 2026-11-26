// data_ram_data.h  -  Data RAM 초기 데이터 (사용자가 작성하는 파일)
//
// 8비트 데이터 / 10비트 주소(0x000 ~ 0x3FF, 총 1024바이트)
//
// 작성 방법 (ram_data.h 와 같은 방식)
//  - DATA_RAM_IMAGE[] 에 주소 0x000 부터 순서대로 8비트 값을 적습니다.
//  - 적지 않은 뒷부분은 모두 0x00 으로 시작합니다. (데이터가 없으면 0x00 하나만 남겨 두세요)
//  - 중간에 빈 구간이 필요하면 DZ8 / DZ16 / DZ64 / DZ256 매크로(0을 8/16/64/256개)를 쓰세요.
//  - 배열이 1024바이트를 넘으면 컴파일 오류가 납니다.
//
// 이 데이터는 플래시(PROGMEM)에 저장되고, 시작할 때 SRAM(Data RAM 1KB)으로 복사됩니다.

#pragma once
#include <stdint.h>
#include <avr/pgmspace.h>

#define DZ4    0,0,0,0
#define DZ8    DZ4,DZ4
#define DZ16   DZ8,DZ8
#define DZ64   DZ16,DZ16,DZ16,DZ16
#define DZ256  DZ64,DZ64,DZ64,DZ64

static const uint8_t DATA_RAM_IMAGE[] PROGMEM = {
  // ---- 0x000 ----
  0x00, 0x11, 0x22, 0x33,   // 0x000 ~ 0x003
  0x44, 0x55, 0x66, 0x77,   // 0x004 ~ 0x007

  // ---- 0x008 부터 0x00F 까지 비워 두기 ----
  DZ8                       // 0x008 ~ 0x00F
};
