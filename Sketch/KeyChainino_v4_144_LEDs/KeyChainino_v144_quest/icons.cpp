#include "icons.h"

/* The treasure: three solid triangles, apex up, hollow center. */
const uint16_t ICON_TREASURE[12] PROGMEM = {
  0x000, 0x000, 0x000,
  0x060,  // ......##....   apex of the top triangle
  0x0F0,  // .....####...
  0x1F8,  // ....######..
  0x30C,  // ..##....##..   two lower apexes
  0x79E,  // .####..####.
  0xFFF,  // ############
  0x000, 0x000, 0x000
};

/* KeyChainino faces (same drawings as the multigame firmware) */
const uint16_t KeyChaininoFace[12] PROGMEM = {
  0x000, 0x000, 0x000, 0x000, 0x060, 0x000,
  0x108, 0x0F0, 0x000, 0x000, 0x000, 0x000
};
const uint16_t KeyChaininoSadFace[12] PROGMEM = {
  0x000, 0x000, 0x000, 0x000, 0x060, 0x000,
  0x000, 0x0F0, 0x108, 0x000, 0x000, 0x000
};
