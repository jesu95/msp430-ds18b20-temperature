#ifndef DELAY_H_
#define DELAY_H_

#define CYCLES_PER_US 8L // depends on the CPU speed
#define CYCLES_PER_MS (CYCLES_PER_US * 8000L)

#define DELAY_US(x) __delay_cycles((x * CYCLES_PER_US))
#define DELAY_MS(x) __delay_cycles((x * CYCLES_PER_MS))

#endif /* DELAY_H_ */
