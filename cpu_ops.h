#pragma once

#include <stdbool.h>

#define GEN_ENUM(ENUM) ENUM,
#define GEN_STRING(STRING) #STRING,

#define FOREACH_MNEMONIC(MNEMONIC) \
    MNEMONIC(ADC) \
    MNEMONIC(ADS) \
    MNEMONIC(AND) \
    MNEMONIC(ASL) \
    MNEMONIC(BCC) \
    MNEMONIC(BCS) \
    MNEMONIC(BEQ) \
    MNEMONIC(BIT) \
    MNEMONIC(BMI) \
    MNEMONIC(BNE) \
    MNEMONIC(BPL) \
    MNEMONIC(BRK) \
    MNEMONIC(BVC) \
    MNEMONIC(BVS) \
    MNEMONIC(CLC) \
    MNEMONIC(CLD) \
    MNEMONIC(CLI) \
    MNEMONIC(CLV) \
    MNEMONIC(CMP) \
    MNEMONIC(CPX) \
    MNEMONIC(CPY) \
    MNEMONIC(DEC) \
    MNEMONIC(DEX) \
    MNEMONIC(DEY) \
    MNEMONIC(EOR) \
    MNEMONIC(INC) \
    MNEMONIC(INX) \
    MNEMONIC(INY) \
    MNEMONIC(JMP) \
    MNEMONIC(JSR) \
    MNEMONIC(LDA) \
    MNEMONIC(LDX) \
    MNEMONIC(LDY) \
    MNEMONIC(LSR) \
    MNEMONIC(NOP) \
    MNEMONIC(ORA) \
    MNEMONIC(PHA) \
    MNEMONIC(PHP) \
    MNEMONIC(PLA) \
    MNEMONIC(PLP) \
    MNEMONIC(ROL) \
    MNEMONIC(ROR) \
    MNEMONIC(RTI) \
    MNEMONIC(RTS) \
    MNEMONIC(SBC) \
    MNEMONIC(SEC) \
    MNEMONIC(SED) \
    MNEMONIC(SEI) \
    MNEMONIC(STA) \
    MNEMONIC(STX) \
    MNEMONIC(STY) \
    MNEMONIC(TAX) \
    MNEMONIC(TAY) \
    MNEMONIC(TSX) \
    MNEMONIC(TXA) \
    MNEMONIC(TXS) \
    MNEMONIC(TYA) \

#define FOREACH_UNDOCUMENTED_MNEMONIC(MNEMONIC) \
    MNEMONIC(ANC) \
    MNEMONIC(ANE) \
    MNEMONIC(ASR) \
    MNEMONIC(ARR) \
    MNEMONIC(RLA) \
    MNEMONIC(RRA) \
    MNEMONIC(SAX) \
    MNEMONIC(SHA) \
    MNEMONIC(SHY) \
    MNEMONIC(SHX) \
    MNEMONIC(SHS) \
    MNEMONIC(SLO) \
    MNEMONIC(SRE) \
    MNEMONIC(LAX) \
    MNEMONIC(LXA) \
    MNEMONIC(LAS) \
    MNEMONIC(DCP) \
    MNEMONIC(SBX) \
    MNEMONIC(ISB) \


enum mnemonics {
    FOREACH_MNEMONIC(GEN_ENUM)
    FOREACH_UNDOCUMENTED_MNEMONIC(GEN_ENUM)
    _U_,
};

static const char *mnemonics_strings[] = {
    FOREACH_MNEMONIC(GEN_STRING)
    FOREACH_UNDOCUMENTED_MNEMONIC(GEN_STRING)
    "_U_",
};

typedef enum {
    Absolute,
    Absolute_Y,
    Absolute_X,
    Accumulator,
    Immediate,
    Implied,
    Indirect,
    Indirect_X,
    Indirect_Y,
    Relative,
    Zeropage,
    Zeropage_X,
    Zeropage_Y,
    Undefined,
} addressing_modes;

struct operation {
    enum mnemonics   mnem;
    addressing_modes mode;
    uint8_t          cycles;
    bool             undocumented;
};

static const struct operation operations[256] = {
/* 00 - 0f */
{ .mnem = BRK, .cycles = 7, .mode = Implied,    },
{ .mnem = ORA, .cycles = 6, .mode = Indirect_X, },
{ .mnem = _U_, .cycles = 0, .mode = Undefined,  },
{ .mnem = SLO, .cycles = 1, .mode = Indirect_X, .undocumented = true, },
{ .mnem = NOP, .cycles = 2, .mode = Zeropage,   .undocumented = true, },
{ .mnem = ORA, .cycles = 3, .mode = Zeropage,   },
{ .mnem = ASL, .cycles = 5, .mode = Zeropage,   },
{ .mnem = SLO, .cycles = 1, .mode = Zeropage,   .undocumented = true, },

{ .mnem = PHP, .cycles = 3, .mode = Implied,    },
{ .mnem = ORA, .cycles = 2, .mode = Immediate,  },
{ .mnem = ASL, .cycles = 2, .mode = Accumulator,},
{ .mnem = ANC, .cycles = 1, .mode = Immediate,  .undocumented = true, },
{ .mnem = NOP, .cycles = 1, .mode = Absolute,   .undocumented = true, },
{ .mnem = ORA, .cycles = 4, .mode = Absolute,   },
{ .mnem = ASL, .cycles = 6, .mode = Absolute,   },
{ .mnem = SLO, .cycles = 1, .mode = Absolute,   .undocumented = true, },

/* 10 - 1f */
{ .mnem = BPL, .cycles = 2, .mode = Relative,   },
{ .mnem = ORA, .cycles = 5, .mode = Indirect_Y, },
{ .mnem = _U_, .cycles = 0, .mode = Undefined,  },
{ .mnem = SLO, .cycles = 5, .mode = Indirect_Y, .undocumented = true },
{ .mnem = NOP, .cycles = 4, .mode = Zeropage_X, .undocumented = true,  },
{ .mnem = ORA, .cycles = 4, .mode = Zeropage_X, },
{ .mnem = ASL, .cycles = 6, .mode = Zeropage_X, },
{ .mnem = SLO, .cycles = 6, .mode = Zeropage_X, .undocumented = true, },

{ .mnem = CLC, .cycles = 2, .mode = Implied,    },
{ .mnem = ORA, .cycles = 4, .mode = Absolute_Y, },
{ .mnem = NOP, .cycles = 2, .mode = Implied,    .undocumented = true, },
{ .mnem = SLO, .cycles = 4, .mode = Absolute_Y, .undocumented = true,    },
{ .mnem = NOP, .cycles = 4, .mode = Absolute_X, .undocumented = true,  },
{ .mnem = ORA, .cycles = 4, .mode = Absolute_X, },
{ .mnem = ASL, .cycles = 7, .mode = Absolute_X, },
{ .mnem = SLO, .cycles = 4, .mode = Absolute_X, .undocumented = true,  },

/* 20 - 2f */
{ .mnem = JSR, .cycles = 0, .mode = Implied,    },
{ .mnem = AND, .cycles = 0, .mode = Indirect_X, },
{ .mnem = _U_, .cycles = 0, .mode = Undefined,  },
{ .mnem = RLA, .cycles = 0, .mode = Indirect_X, },
{ .mnem = BIT, .cycles = 0, .mode = Zeropage,   },
{ .mnem = AND, .cycles = 0, .mode = Zeropage,   },
{ .mnem = ROL, .cycles = 0, .mode = Zeropage,   },
{ .mnem = RLA, .cycles = 0, .mode = Zeropage,   },

{ .mnem = PLP, .cycles = 0, .mode = Implied,    },
{ .mnem = AND, .cycles = 0, .mode = Immediate,  },
{ .mnem = ROL, .cycles = 0, .mode = Accumulator,},
{ .mnem = ANC, .cycles = 0, .mode = Immediate,  },
{ .mnem = BIT, .cycles = 0, .mode = Absolute,   },
{ .mnem = AND, .cycles = 0, .mode = Absolute,   },
{ .mnem = ROL, .cycles = 0, .mode = Absolute,   },
{ .mnem = RLA, .cycles = 0, .mode = Absolute,   },

/* 30 - 3f */
{ .mnem = BMI, .cycles = 0, .mode = Relative,   },
{ .mnem = AND, .cycles = 0, .mode = Indirect_Y, },
{ .mnem = _U_, .cycles = 0, .mode = Undefined,  },
{ .mnem = RLA, .cycles = 0, .mode = Indirect_Y, },
{ .mnem = NOP, .cycles = 0, .mode = Zeropage_X, },
{ .mnem = AND, .cycles = 0, .mode = Zeropage_X, },
{ .mnem = ROL, .cycles = 0, .mode = Zeropage_X, },
{ .mnem = RLA, .cycles = 0, .mode = Zeropage_X, },

{ .mnem = SEC, .cycles = 0, .mode = Implied,    },
{ .mnem = AND, .cycles = 0, .mode = Absolute_Y, },
{ .mnem = NOP, .cycles = 0, .mode = Implied,    },
{ .mnem = RLA, .cycles = 0, .mode = Absolute_Y, },
{ .mnem = NOP, .cycles = 0, .mode = Absolute_X, },
{ .mnem = AND, .cycles = 0, .mode = Absolute_X, },
{ .mnem = ROL, .cycles = 0, .mode = Absolute_X, },
{ .mnem = RLA, .cycles = 0, .mode = Absolute_X, },

/* 40 - 4f */
{ .mnem = RTI, .cycles = 0, .mode = Implied,    },
{ .mnem = EOR, .cycles = 0, .mode = Indirect_X, },
{ .mnem = _U_, .cycles = 0, .mode = Undefined,  },
{ .mnem = SRE, .cycles = 0, .mode = Indirect_X, },
{ .mnem = NOP, .cycles = 0, .mode = Zeropage,   },
{ .mnem = EOR, .cycles = 0, .mode = Zeropage,   },
{ .mnem = LSR, .cycles = 0, .mode = Zeropage,   },
{ .mnem = SRE, .cycles = 0, .mode = Zeropage,   },

{ .mnem = PHA, .cycles = 0, .mode = Implied,    },
{ .mnem = EOR, .cycles = 0, .mode = Immediate,  },
{ .mnem = LSR, .cycles = 0, .mode = Accumulator,},
{ .mnem = ASR, .cycles = 0, .mode = Immediate,  },
{ .mnem = JMP, .cycles = 0, .mode = Absolute,   },
{ .mnem = EOR, .cycles = 0, .mode = Absolute,   },
{ .mnem = LSR, .cycles = 0, .mode = Absolute,   },
{ .mnem = SRE, .cycles = 0, .mode = Absolute,   },

/* 50 - 5f */
{ .mnem = BVC, .cycles = 0, .mode = Relative,   },
{ .mnem = EOR, .cycles = 0, .mode = Indirect_Y, },
{ .mnem = _U_, .cycles = 0, .mode = Undefined,  },
{ .mnem = SRE, .cycles = 0, .mode = Indirect_Y, },
{ .mnem = NOP, .cycles = 0, .mode = Zeropage_X, },
{ .mnem = EOR, .cycles = 0, .mode = Zeropage_X, },
{ .mnem = LSR, .cycles = 0, .mode = Zeropage_X, },
{ .mnem = SRE, .cycles = 0, .mode = Zeropage_X, },

{ .mnem = CLI, .cycles = 0, .mode = Implied,    },
{ .mnem = EOR, .cycles = 0, .mode = Absolute_Y, },
{ .mnem = NOP, .cycles = 0, .mode = Implied,    },
{ .mnem = SRE, .cycles = 0, .mode = Absolute_Y, },
{ .mnem = NOP, .cycles = 0, .mode = Absolute_X, },
{ .mnem = EOR, .cycles = 0, .mode = Absolute_X, },
{ .mnem = LSR, .cycles = 0, .mode = Absolute_X, },
{ .mnem = SRE, .cycles = 0, .mode = Absolute_X, },

/* 60 - 6f */
{ .mnem = RTS, .cycles = 0, .mode = Implied,    },
{ .mnem = ADC, .cycles = 0, .mode = Indirect_X, },
{ .mnem = _U_, .cycles = 0, .mode = Undefined,  },
{ .mnem = RRA, .cycles = 0, .mode = Indirect_X, },
{ .mnem = NOP, .cycles = 0, .mode = Zeropage,   },
{ .mnem = ADC, .cycles = 0, .mode = Zeropage,   },
{ .mnem = ROR, .cycles = 0, .mode = Zeropage,   },
{ .mnem = RRA, .cycles = 0, .mode = Zeropage,   },

{ .mnem = PLA, .cycles = 0, .mode = Implied,    },
{ .mnem = ADC, .cycles = 0, .mode = Immediate,  },
{ .mnem = ROR, .cycles = 0, .mode = Accumulator,},
{ .mnem = ARR, .cycles = 0, .mode = Immediate,  },
{ .mnem = JMP, .cycles = 0, .mode = Indirect,   },
{ .mnem = ADC, .cycles = 0, .mode = Absolute,   },
{ .mnem = ROR, .cycles = 0, .mode = Absolute,   },
{ .mnem = RRA, .cycles = 0, .mode = Absolute,   },

/* 70 - 7f */
{ .mnem = BVS, .cycles = 0, .mode = Relative,   },
{ .mnem = ADC, .cycles = 0, .mode = Indirect_Y, },
{ .mnem = _U_, .cycles = 0, .mode = Undefined,  },
{ .mnem = RRA, .cycles = 0, .mode = Indirect_Y, },
{ .mnem = NOP, .cycles = 0, .mode = Zeropage_X, },
{ .mnem = ADC, .cycles = 0, .mode = Zeropage_X, },
{ .mnem = ROR, .cycles = 0, .mode = Zeropage_X, },
{ .mnem = RRA, .cycles = 0, .mode = Zeropage_X, },

{ .mnem = SEI, .cycles = 0, .mode = Implied,    },
{ .mnem = ADC, .cycles = 0, .mode = Absolute_Y, },
{ .mnem = NOP, .cycles = 0, .mode = Implied,    },
{ .mnem = RRA, .cycles = 0, .mode = Absolute_Y, },
{ .mnem = NOP, .cycles = 0, .mode = Absolute_X, },
{ .mnem = ADC, .cycles = 0, .mode = Absolute_X, },
{ .mnem = ROR, .cycles = 0, .mode = Absolute_X, },
{ .mnem = RRA, .cycles = 0, .mode = Absolute_X, },

/* 80 - 8f */
{ .mnem = NOP, .cycles = 0, .mode = Implied,    },
{ .mnem = STA, .cycles = 0, .mode = Indirect_X, },
{ .mnem = NOP, .cycles = 0, .mode = Undefined,  },
{ .mnem = SAX, .cycles = 0, .mode = Indirect_X, },
{ .mnem = STY, .cycles = 0, .mode = Zeropage,   },
{ .mnem = STA, .cycles = 0, .mode = Zeropage,   },
{ .mnem = STX, .cycles = 0, .mode = Zeropage,   },
{ .mnem = SAX, .cycles = 0, .mode = Zeropage,   },

{ .mnem = DEY, .cycles = 0, .mode = Implied,    },
{ .mnem = NOP, .cycles = 0, .mode = Immediate,  },
{ .mnem = TXA, .cycles = 0, .mode = Implied,    },
{ .mnem = ANE, .cycles = 0, .mode = Immediate,  },
{ .mnem = STY, .cycles = 0, .mode = Absolute,   },
{ .mnem = STA, .cycles = 0, .mode = Absolute,   },
{ .mnem = STX, .cycles = 0, .mode = Absolute,   },
{ .mnem = SAX, .cycles = 0, .mode = Absolute,   },

/* 90 - 9f */
{ .mnem = BCC, .cycles = 0, .mode = Relative,   },
{ .mnem = STA, .cycles = 0, .mode = Indirect_Y, },
{ .mnem = _U_, .cycles = 0, .mode = Undefined,  },
{ .mnem = SHA, .cycles = 0, .mode = Indirect_Y, },
{ .mnem = STY, .cycles = 0, .mode = Zeropage_X, },
{ .mnem = STA, .cycles = 0, .mode = Zeropage_X, },
{ .mnem = STX, .cycles = 0, .mode = Zeropage_X, },
{ .mnem = SAX, .cycles = 0, .mode = Zeropage_X, },

{ .mnem = TYA, .cycles = 0, .mode = Implied,    },
{ .mnem = STA, .cycles = 0, .mode = Absolute_Y, },
{ .mnem = TXS, .cycles = 0, .mode = Implied,    },
{ .mnem = SHS, .cycles = 0, .mode = Absolute_Y, },
{ .mnem = SHY, .cycles = 0, .mode = Absolute_X, },
{ .mnem = STA, .cycles = 0, .mode = Absolute_X, },
{ .mnem = SHX, .cycles = 0, .mode = Absolute_X, },
{ .mnem = SHA, .cycles = 0, .mode = Absolute_X, },

/* a0 - af */
{ .mnem = LDY, .cycles = 0, .mode = Immediate,  },
{ .mnem = LDA, .cycles = 0, .mode = Indirect_X, },
{ .mnem = LDX, .cycles = 0, .mode = Immediate,  },
{ .mnem = LAX, .cycles = 0, .mode = Indirect_X, },
{ .mnem = LDY, .cycles = 0, .mode = Zeropage,   },
{ .mnem = LDA, .cycles = 0, .mode = Zeropage,   },
{ .mnem = LDX, .cycles = 0, .mode = Zeropage,   },
{ .mnem = LAX, .cycles = 0, .mode = Zeropage,   },

{ .mnem = TAY, .cycles = 0, .mode = Implied,    },
{ .mnem = LDA, .cycles = 0, .mode = Immediate,  },
{ .mnem = TAX, .cycles = 0, .mode = Implied,    },
{ .mnem = LXA, .cycles = 0, .mode = Immediate,  },
{ .mnem = LDY, .cycles = 0, .mode = Absolute,   },
{ .mnem = LDA, .cycles = 0, .mode = Absolute,   },
{ .mnem = LDX, .cycles = 0, .mode = Absolute,   },
{ .mnem = LAX, .cycles = 0, .mode = Absolute,   },

/* b0 - bf */
{ .mnem = BCS, .cycles = 0, .mode = Relative,   },
{ .mnem = LDA, .cycles = 0, .mode = Indirect_Y, },
{ .mnem = _U_, .cycles = 0, .mode = Undefined,  },
{ .mnem = LAX, .cycles = 0, .mode = Indirect_Y, },
{ .mnem = LDY, .cycles = 0, .mode = Zeropage_X, },
{ .mnem = LDA, .cycles = 0, .mode = Zeropage_X, },
{ .mnem = LDX, .cycles = 0, .mode = Zeropage_Y, },
{ .mnem = LAX, .cycles = 0, .mode = Zeropage_Y, },

{ .mnem = CLV, .cycles = 0, .mode = Implied,    },
{ .mnem = LDA, .cycles = 0, .mode = Absolute_Y, },
{ .mnem = TSX, .cycles = 0, .mode = Implied,    },
{ .mnem = LAS, .cycles = 0, .mode = Absolute_Y, },
{ .mnem = LDY, .cycles = 0, .mode = Absolute_X, },
{ .mnem = LDA, .cycles = 0, .mode = Absolute_X, },
{ .mnem = LDX, .cycles = 0, .mode = Absolute_Y, },
{ .mnem = LAX, .cycles = 0, .mode = Absolute_Y, },

/* c0 - cf */
{ .mnem = CPY, .cycles = 0, .mode = Immediate,  },
{ .mnem = CMP, .cycles = 0, .mode = Indirect_X, },
{ .mnem = NOP, .cycles = 0, .mode = Undefined,  },
{ .mnem = DCP, .cycles = 0, .mode = Indirect_X, },
{ .mnem = CPY, .cycles = 0, .mode = Zeropage,   },
{ .mnem = CMP, .cycles = 0, .mode = Zeropage,   },
{ .mnem = DEC, .cycles = 0, .mode = Zeropage,   },
{ .mnem = DCP, .cycles = 0, .mode = Zeropage,   },

{ .mnem = INY, .cycles = 0, .mode = Implied,    },
{ .mnem = CMP, .cycles = 0, .mode = Immediate,  },
{ .mnem = DEX, .cycles = 0, .mode = Implied,    },
{ .mnem = SBX, .cycles = 0, .mode = Immediate,  },
{ .mnem = CPY, .cycles = 0, .mode = Absolute,   },
{ .mnem = CMP, .cycles = 0, .mode = Absolute,   },
{ .mnem = DEC, .cycles = 0, .mode = Absolute,   },
{ .mnem = DCP, .cycles = 0, .mode = Absolute,   },

/* d0 - df */
{ .mnem = BNE, .cycles = 0, .mode = Relative,   },
{ .mnem = CMP, .cycles = 0, .mode = Indirect_Y, },
{ .mnem = _U_, .cycles = 0, .mode = Undefined,  },
{ .mnem = DCP, .cycles = 0, .mode = Indirect_Y, },
{ .mnem = NOP, .cycles = 0, .mode = Zeropage_X, },
{ .mnem = CMP, .cycles = 0, .mode = Zeropage_X, },
{ .mnem = DEC, .cycles = 0, .mode = Zeropage_X, },
{ .mnem = DCP, .cycles = 0, .mode = Zeropage_X, },

{ .mnem = CLD, .cycles = 0, .mode = Implied,    },
{ .mnem = CMP, .cycles = 0, .mode = Absolute_Y, },
{ .mnem = NOP, .cycles = 0, .mode = Implied,    },
{ .mnem = DCP, .cycles = 0, .mode = Absolute_Y, },
{ .mnem = NOP, .cycles = 0, .mode = Absolute_X, },
{ .mnem = CMP, .cycles = 0, .mode = Absolute_X, },
{ .mnem = DEC, .cycles = 0, .mode = Absolute_X, },
{ .mnem = DCP, .cycles = 0, .mode = Absolute_X, },

/* e0 - ef */
{ .mnem = CPX, .cycles = 0, .mode = Immediate,  },
{ .mnem = SBC, .cycles = 0, .mode = Indirect_X, },
{ .mnem = NOP, .cycles = 0, .mode = Undefined,  },
{ .mnem = ISB, .cycles = 0, .mode = Indirect_X, },
{ .mnem = CPX, .cycles = 0, .mode = Zeropage,   },
{ .mnem = SBC, .cycles = 0, .mode = Zeropage,   },
{ .mnem = INC, .cycles = 0, .mode = Zeropage,   },
{ .mnem = ISB, .cycles = 0, .mode = Zeropage,   },

{ .mnem = INX, .cycles = 0, .mode = Implied,    },
{ .mnem = SBC, .cycles = 0, .mode = Immediate,  },
{ .mnem = NOP, .cycles = 0, .mode = Accumulator,},
{ .mnem = SBC, .cycles = 0, .mode = Immediate,  },
{ .mnem = CPX, .cycles = 0, .mode = Absolute,   },
{ .mnem = SBC, .cycles = 0, .mode = Absolute,   },
{ .mnem = INC, .cycles = 0, .mode = Absolute,   },
{ .mnem = ISB, .cycles = 0, .mode = Absolute,   },

/* f0 - ff */
{ .mnem = BEQ, .cycles = 0, .mode = Relative,   },
{ .mnem = SBC, .cycles = 0, .mode = Indirect_Y, },
{ .mnem = _U_, .cycles = 0, .mode = Undefined,  },
{ .mnem = ISB, .cycles = 0, .mode = Indirect_Y, },
{ .mnem = NOP, .cycles = 0, .mode = Zeropage_X, },
{ .mnem = SBC, .cycles = 0, .mode = Zeropage_X, },
{ .mnem = INC, .cycles = 0, .mode = Zeropage_X, },
{ .mnem = ISB, .cycles = 0, .mode = Zeropage_X, },

{ .mnem = SED, .cycles = 0, .mode = Implied,    },
{ .mnem = SBC, .cycles = 0, .mode = Absolute_Y, },
{ .mnem = NOP, .cycles = 0, .mode = Implied,    },
{ .mnem = ISB, .cycles = 0, .mode = Absolute_Y, },
{ .mnem = NOP, .cycles = 0, .mode = Absolute_X, },
{ .mnem = SBC, .cycles = 0, .mode = Absolute_X, },
{ .mnem = INC, .cycles = 0, .mode = Absolute_X, },
{ .mnem = ISB, .cycles = 0, .mode = Absolute_X, },
};
