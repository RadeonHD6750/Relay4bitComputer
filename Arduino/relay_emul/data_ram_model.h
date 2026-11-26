// data_ram_model.h  -  Data RAM 모사: 8비트 데이터 / 10비트 주소 (1024 x 8bit = 1KB)
//
// 시작할 때 data_ram_data.h 의 초기 데이터를 SRAM으로 복사하고,
// 이후 읽기/쓰기는 모두 SRAM에서 처리한다. (재부팅하면 초기 데이터로 복귀)

#pragma once
#include <Arduino.h>
#include <avr/pgmspace.h>
#include "data_ram_data.h"

static const uint16_t DATA_RAM_WORDS       = 1024;          // 2^10
static const uint16_t DATA_RAM_ADDR_MASK   = 0x03FF;
static const uint16_t DATA_RAM_IMAGE_BYTES = sizeof(DATA_RAM_IMAGE) / sizeof(DATA_RAM_IMAGE[0]);

static_assert(sizeof(DATA_RAM_IMAGE) / sizeof(DATA_RAM_IMAGE[0]) <= 1024,
              "DATA_RAM_IMAGE 가 1024바이트(10비트 주소)를 넘었습니다");

class Ram1kx8 {
public:
  void begin() {
    for (uint16_t a = 0; a < DATA_RAM_WORDS; a++)
      mem_[a] = (a < DATA_RAM_IMAGE_BYTES) ? pgm_read_byte(&DATA_RAM_IMAGE[a]) : 0;
  }

  // 주소는 하위 10비트만 사용
  uint8_t read(uint16_t addr) const { return mem_[addr & DATA_RAM_ADDR_MASK]; }
  void    write(uint16_t addr, uint8_t data) { mem_[addr & DATA_RAM_ADDR_MASK] = data; }

  // 칩 신호 단위 모사: we=1 이면 쓰기, oe=1 이면 읽은 값을 반환 (oe=0 이면 0)
  uint8_t cycle(uint16_t addr, uint8_t din, bool we, bool oe) {
    if (we) write(addr, din);
    return oe ? read(addr) : 0;
  }

private:
  uint8_t mem_[DATA_RAM_WORDS];
};
