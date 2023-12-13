#include <stdint.h>
#include "mcu.h"

/* Values exported by the linker */
extern uint32_t _eflash;
extern uint32_t _sdtext;
extern uint32_t _edtext;
extern uint32_t _sbss;
extern uint32_t _ebss;
extern uint32_t __main_stack_end__;
extern uint32_t __ram_start__;
extern uint32_t __ram_size__;
extern uint32_t __seed_start__;
extern uint32_t __seed_size__;

/* Pointer to the Cortex vector table (located at offset 0) */
extern uint32_t *_vectors;

/* A place for the vector table to live while in RAM */
static uint32_t ram_vectors[64] __attribute__ ((aligned (1024)));

__attribute__ ((section(".startup")))
void memcpy32(uint32_t *src, uint32_t *dest, uint32_t count) {
  count /= sizeof(*src);
  while (count--)
    *dest++ = *src++;
}

__attribute__ ((section(".startup"), noinline))
void __attribute__ ((noinline)) trng_gen_seed256(uint32_t *seed, uint32_t *ram, uint32_t ram_size) {
  uint32_t size = ram_size / sizeof(*ram);
  unsigned int word = 0;
  unsigned int nwords = 8;
  while (size--){
    seed[word] ^= *ram++;
    word = (word+1)%nwords;
  }
}

__attribute__ ((section(".startup")))
static void init_crt(void) {
  //generate seed from uninitialized RAM
  uint32_t *seed = &__seed_start__;
  uint32_t *ram = &__ram_start__;
  uint32_t ram_size = (uint32_t)&__ram_size__;
  trng_gen_seed256(seed, ram, ram_size);
  
  /* Relocate data and text sections to RAM */
  memcpy32(&_eflash, &_sdtext, (uint32_t)&_edtext - (uint32_t)&_sdtext);

  /* Clear BSS */
  uint32_t *dest = &_sbss;
  while (dest < &_ebss) *dest++ = 0;

  /* Copy IVT to RAM */
  uint32_t *src = (uint32_t *) &_vectors;
  dest = &ram_vectors[0];
  while (dest <= &ram_vectors[63])
    *dest++ = *src++;

  /* Switch to IVT now located in RAM */
  SCB->VTOR = (uint32_t) &ram_vectors[0];
}

__attribute__((weak))
void __early_init(void) {
  return;
}

__attribute__ ((noreturn, weak))
void bootloader_main(void) {
  while(1);
}

__attribute__ ((section(".startup"), noreturn))
void Reset_Handler(void) {

  init_crt();
  __early_init();
  bootloader_main();
}
