import re

# 1. 명령어별 4비트 Opcode 매핑 정의 (나중에 수정 가능)
OPCODES = {
    'NOP':   0x0,  # 0000
    'LOAD':  0x1,  # 0001
    'STORE': 0x2,  # 0010
    'ADD':   0x3,  # 0011
    'ADC':   0x4,  # 0100
    'SUB':   0x5,  # 0101
    'SBC':   0x6,  # 0110
    'OR':    0x7,  # 0111
    'AND':   0x8,  # 1000
    'XOR':   0x9,  # 1001
    'JMP':   0xA,  # 1010
    'JZ':    0xB,  # 1011
    'JC':    0xC,  # 1100
    'JS':    0xD,  # 1101
    'CALL':  0xE,  # 1110
    'RET':   0xF   # 1111
}

def parse_operand(op_str):
    """문자열로 된 오퍼랜드를 12비트 정수로 변환 (10진수 및 16진수 지원)"""
    op_str = op_str.strip()
    try:
        if op_str.lower().startswith('0x'):
            return int(op_str, 16)
        # FF0, FFF 처럼 0x가 없어도 16진수 문자가 포함되어 있으면 16진수로 처리
        if any(c in op_str.upper() for c in ['A', 'B', 'C', 'D', 'E', 'F']):
            return int(op_str, 16)
        return int(op_str, 10)
    except ValueError:
        # 혹시 몰라 16진수 파싱 재시도 (숫자로만 된 100번지 같은 경우 대비)
        try:
            return int(op_str, 16)
        except ValueError:
            return None

def assemble(asm_text):
    """어셈블리 코드를 입력받아 16진수(Hex) 기계어 리스트를 반환"""
    hex_output = []
    lines = asm_text.strip().split('\n')
    
    print("=== [번역 시작] ===")
    for idx, line in enumerate(lines, 1):
        # 1. 주석 제거 (; 기호 이후 제거) 및 공백 제거
        clean_line = line.split(';')[0].strip()
        if not clean_line:
            continue  # 빈 줄이거나 주석만 있는 줄은 패스
            
        # 2. 명령어와 오퍼랜드 분리
        tokens = re.split(r'[\s,]+', clean_line)
        instr = tokens[0].upper()
        
        if instr not in OPCODES:
            print(f"❌ 에러 [Line {idx}]: 알 수 없는 명령어 -> '{instr}'")
            return None
            
        opcode = OPCODES[instr]
        operand = 0
        
        # 3. 오퍼랜드가 필요한 명령어 처리 (NOP, RET 제외)
        if instr not in ['NOP', 'RET']:
            if len(tokens) < 2:
                print(f"❌ 에러 [Line {idx}]: {instr} 명령어에 오퍼랜드(주소/값)가 없습니다.")
                return None
                
            op_str = tokens[1]
            parsed_op = parse_operand(op_str)
            
            if parsed_op is None:
                print(f"❌ 에러 [Line {idx}]: 잘못된 오퍼랜드 형식 -> '{op_str}'")
                return None
                
            if parsed_op < 0 or parsed_op > 0xFFF:
                print(f"❌ 에러 [Line {idx}]: 오퍼랜드가 12비트 범위(0x000~0xFFF)를 벗어났습니다 -> '{op_str}'")
                return None
                
            operand = parsed_op
            
        # 4. 4비트 Opcode와 12비트 Operand를 결합하여 16비트 기계어 생성
        # 예: Opcode가 0x1(LOAD), Operand가 0xFF0 이면 -> (0x1 << 12) | 0xFF0 = 0x1FF0
        machine_code = (opcode << 12) | (operand & 0xFFF)
        
        # 4자리 16진수 대문자 포맷으로 저장 (예: 1FF0)
        hex_str = f"{machine_code:04X}"
        hex_output.append(hex_str)
        print(f"Line {idx:02d}: {clean_line.ljust(20)} -> {hex_str} (이진수: {machine_code:016b})")
        
    print("=== [번역 완료] ===")
    return hex_output

# ==========================================
# 2. 테스트용 예시 어셈블리 프로그램 코드
# ==========================================
program_code = """
LOAD  0x010    ; 0x010 번지에서 데이터를 가져와 T 레지스터에 로드
ADD   0x011    ; 0x011 번지 전압을 RAM에서 읽어 T와 더한 후 A에 저장
JZ    0x0A0    ; 연산 결과가 0(Zero)이면 0x0A0 주소로 점프
STORE 0xFF0    ; 결과가 0이 아니면 출력 포트(FF0)에 저장
NOP            ; 아무것도 하지 않음
RET            ; 서브루틴 종료
"""

# 어셈블러 가동
hex_file_content = assemble(program_code)

if hex_file_content:
    print("\n[최종 생성된 Intel HEX / 롬 라이터용 데이터]")
    print("\n".join(hex_file_content))