/*
 * font_data.h — the ten digit glyphs (public-domain SparkFun "Font 1",
 * unchanged: the data is proven on this matrix). The counter shows
 * nothing but numbers, so the rest of the font is gone.
 *
 * Format per glyph: {width, col0, col1, ...}; each column byte holds 7
 * rows, bit 0 = top. digit_table[] indexes glyphs by digit value.
 *
 * IMPORTANT: include this file from text.cpp ONLY. The arrays have
 * internal linkage, so every translation unit that references them would
 * embed its own copy of the font in flash.
 */
#ifndef RETHINK_FONT_DATA_H
#define RETHINK_FONT_DATA_H

/* Variable size character widths */
/* Format: { size in bytes of char, byte 1, byte 2, ... } */
static const unsigned char c_0[] PROGMEM =  {5, 0x3E, 0x51, 0x49, 0x45, 0x3E};
static const unsigned char c_1[] PROGMEM =  {3, 0x42, 0x7F, 0x40};
static const unsigned char c_2[] PROGMEM =  {4, 0x62, 0x51, 0x49, 0x46};
static const unsigned char c_3[] PROGMEM =  {4, 0x22, 0x41, 0x49, 0x36};
static const unsigned char c_4[] PROGMEM =  {4, 0x1F, 0x10, 0x7C, 0x10};
static const unsigned char c_5[] PROGMEM =  {4, 0x27, 0x45, 0x45, 0x39};
static const unsigned char c_6[] PROGMEM =  {4, 0x3C, 0x4A, 0x49, 0x30};
static const unsigned char c_7[] PROGMEM =  {4, 0x71, 0x09, 0x05, 0x03};
static const unsigned char c_8[] PROGMEM =  {4, 0x36, 0x49, 0x49, 0x36};
static const unsigned char c_9[] PROGMEM =  {4, 0x06, 0x49, 0x29, 0x1E};

/* Pointers to characters, by digit value */
const unsigned char * const digit_table[10] PROGMEM = {
    c_0, c_1, c_2, c_3, c_4, c_5, c_6, c_7, c_8, c_9
};

#endif // RETHINK_FONT_DATA_H
