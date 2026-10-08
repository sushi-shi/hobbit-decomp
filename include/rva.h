#ifndef HOBBIT_RVA_H
#define HOBBIT_RVA_H

// Ported from Gruntz include/rva.h at 7d4bd55b99e32f084834d991badf7609889481f4.
// Addresses are RVAs in the hash-pinned PC reference executable.
#if defined(__clang__) && defined(HOBBIT_EMIT_META)
#define RVA(addr, size) __attribute__((annotate("rva:" #addr " size:" #size), used))
#define DATA(addr) __attribute__((annotate("data:" #addr)))
#define OVERRIDE override
#else
#define RVA(addr, size)
#define DATA(addr)
#define OVERRIDE
#endif

#define RVA_COMPGEN(addr, size, symbol)
#define RVA_DYNINIT(addr, size, owner)
#define DATA_COMPGEN(addr, value) value

#endif
