// ram_model.h  -  16비트 데이터 / 10비트 주소 RAM 모사 (1024 x 16bit = 2KB)
//
// 시작할 때 ram_data.h 의 초기 데이터를 SRAM(2KB)으로 복사하고,
// 이후 읽기/쓰기는 모두 SRAM에서 처리한다. (재부팅하면 초기 데이터로 복귀)

#pragma once
#include <Arduino.h>
#include <avr/pgmspace.h>
#include "prog_ram_data.h"

static const uint16_t RAM_ADDR_BITS   = 10;
static const uint16_t RAM_WORDS       = 1024;          // 2^10
static const uint16_t RAM_ADDR_MASK   = 0x03FF;
static const uint16_t RAM_IMAGE_WORDS = sizeof(RAM_IMAGE) / sizeof(RAM_IMAGE[0]);

static_assert(sizeof(RAM_IMAGE) / sizeof(RAM_IMAGE[0]) <= 1024,
              "RAM_IMAGE 가 1024워드(10비트 주소)를 넘었습니다");

class Ram1kx16 {
public:
  // 초기 데이터 복사, 적지 않은 나머지는 0
  void begin() {
    for (uint16_t a = 0; a < RAM_WORDS; a++)
      mem_[a] = (a < RAM_IMAGE_WORDS) ? pgm_read_word(&RAM_IMAGE[a]) : 0;
  }

  // 주소는 하위 10비트만 사용 (칩의 주소선 10개처럼 상위 비트는 무시)
  uint16_t read(uint16_t addr) const { return mem_[addr & RAM_ADDR_MASK]; }
  void     write(uint16_t addr, uint16_t data) { mem_[addr & RAM_ADDR_MASK] = data; }

  // 칩 신호 단위 모사: we=1 이면 쓰기, oe=1 이면 읽은 값을 반환 (oe=0 이면 0)
  uint16_t cycle(uint16_t addr, uint16_t din, bool we, bool oe) {
    if (we) write(addr, din);
    return oe ? read(addr) : 0;
  }

private:
  uint16_t mem_[RAM_WORDS];
};
