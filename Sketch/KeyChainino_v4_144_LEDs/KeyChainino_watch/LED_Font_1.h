/**
 * @file    LED_Font_1.h
 * @brief   Font 1 for the SparkFun 8x7 Charlieplex LED Array
 * @author  Shawn Hymel (SparkFun Electronics)
 *
 * @copyright   This code is public domain but you buy me a beer if you use
 * this and we meet someday (Beerware license).
 *
 * This font file supports the 8x7 Charlieplex LED array by defining the LEDs
 * that need to turn off and on for each character.
 */
 
#ifndef LED_Font_1_H
#define LED_Font_1_H

/* Variable size character widths */
/* Format: { size in bytes of char, byte 1, byte 2, ... } */
static const unsigned char c_sp[] PROGMEM = {1, 0x00};
static const unsigned char c_ex[] PROGMEM = {1, 0x4F};
static const unsigned char c_qt[] PROGMEM = {3, 0x07, 0x00, 0x07};
static const unsigned char c_ha[] PROGMEM = {5, 0x14, 0x7F, 0x14, 0x7F, 0x14};
static const unsigned char c_dl[] PROGMEM = {5, 0x24, 0x2A, 0x7F, 0x2A, 0x12};
static const unsigned char c_pc[] PROGMEM = {5, 0x23, 0x13, 0x08, 0x64, 0x62};
static const unsigned char c_am[] PROGMEM = {5, 0x36, 0x49, 0x55, 0x22, 0x50};
static const unsigned char c_ap[] PROGMEM = {2, 0x05, 0x03};
static const unsigned char c_op[] PROGMEM = {3, 0x1C, 0x22, 0x41};
static const unsigned char c_cp[] PROGMEM = {3, 0x41, 0x22, 0x1C};
static const unsigned char c_as[] PROGMEM = {5, 0x14, 0x08, 0x3E, 0x08, 0x14};
static const unsigned char c_pl[] PROGMEM = {5, 0x08, 0x08, 0x3E, 0x08, 0x08};
static const unsigned char c_cm[] PROGMEM = {2, 0x50, 0x30};
static const unsigned char c_da[] PROGMEM = {5, 0x08, 0x08, 0x08, 0x08, 0x08};
static const unsigned char c_pr[] PROGMEM = {2, 0x60, 0x60};
static const unsigned char c_fs[] PROGMEM = {5, 0x20, 0x10, 0x08, 0x04, 0x02};
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
static const unsigned char c_co[] PROGMEM = {2, 0x36, 0x36};
static const unsigned char c_sc[] PROGMEM = {2, 0x56, 0x36};
static const unsigned char c_lt[] PROGMEM = {4, 0x08, 0x14, 0x22, 0x41};
static const unsigned char c_eq[] PROGMEM = {4, 0x14, 0x14, 0x14, 0x14};
static const unsigned char c_gt[] PROGMEM = {4, 0x41, 0x22, 0x14, 0x08};
static const unsigned char c_qm[] PROGMEM = {4, 0x02, 0x51, 0x09, 0x06};
static const unsigned char c_at[] PROGMEM = {5, 0x32, 0x49, 0x79, 0x41, 0x3E};
static const unsigned char c_A[] PROGMEM =  {4, 0x7E, 0x11, 0x11, 0x7E};
static const unsigned char c_B[] PROGMEM =  {4, 0x7F, 0x49, 0x49, 0x36};
static const unsigned char c_C[] PROGMEM =  {4, 0x3E, 0x41, 0x41, 0x22};
static const unsigned char c_D[] PROGMEM =  {4, 0x7F, 0x41, 0x22, 0x1C};
static const unsigned char c_E[] PROGMEM =  {4, 0x7F, 0x49, 0x49, 0x41};
static const unsigned char c_F[] PROGMEM =  {4, 0x7F, 0x09, 0x09, 0x01};
static const unsigned char c_G[] PROGMEM =  {4, 0x3E, 0x41, 0x49, 0x7A};
static const unsigned char c_H[] PROGMEM =  {4, 0x7F, 0x08, 0x08, 0x7F};
static const unsigned char c_I[] PROGMEM =  {3, 0x41, 0x7F, 0x41};
static const unsigned char c_J[] PROGMEM =  {5, 0x20, 0x40, 0x41, 0x3F, 0x01};
static const unsigned char c_K[] PROGMEM =  {5, 0x7F, 0x08, 0x14, 0x22, 0x41};
static const unsigned char c_L[] PROGMEM =  {4, 0x7F, 0x40, 0x40, 0x40};
static const unsigned char c_M[] PROGMEM =  {5, 0x7F, 0x02, 0x0C, 0x02, 0x7F};
static const unsigned char c_N[] PROGMEM =  {5, 0x7F, 0x04, 0x08, 0x10, 0x7F};
static const unsigned char c_O[] PROGMEM =  {5, 0x3E, 0x41, 0x41, 0x41, 0x3E};
static const unsigned char c_P[] PROGMEM =  {4, 0x7F, 0x09, 0x09, 0x06};
static const unsigned char c_Q[] PROGMEM =  {5, 0x3E, 0x41, 0x51, 0x21, 0x5E};
static const unsigned char c_R[] PROGMEM =  {4, 0x7F, 0x19, 0x29, 0x46};
static const unsigned char c_S[] PROGMEM =  {4, 0x46, 0x49, 0x49, 0x31};
static const unsigned char c_T[] PROGMEM =  {5, 0x01, 0x01, 0x7F, 0x01, 0x01};
static const unsigned char c_U[] PROGMEM =  {5, 0x3F, 0x40, 0x40, 0x40, 0x3F};
static const unsigned char c_V[] PROGMEM =  {5, 0x1F, 0x20, 0x40, 0x20, 0x1F};
static const unsigned char c_W[] PROGMEM =  {5, 0x3F, 0x40, 0x30, 0x40, 0x3F};
static const unsigned char c_X[] PROGMEM =  {5, 0x63, 0x14, 0x08, 0x14, 0x63};
static const unsigned char c_Y[] PROGMEM =  {5, 0x07, 0x08, 0x70, 0x08, 0x07};
static const unsigned char c_Z[] PROGMEM =  {5, 0x61, 0x51, 0x49, 0x45, 0x43};
static const unsigned char c_ob[] PROGMEM = {3, 0x7F, 0x41, 0x41};
static const unsigned char c_bs[] PROGMEM = {5, 0x02, 0x04, 0x08, 0x10, 0x20};
static const unsigned char c_cb[] PROGMEM = {3, 0x41, 0x41, 0x7F};
static const unsigned char c_ct[] PROGMEM = {5, 0x04, 0x02, 0x01, 0x02, 0x04};
static const unsigned char c_us[] PROGMEM = {5, 0x40, 0x40, 0x40, 0x40, 0x40};
static const unsigned char c_ga[] PROGMEM = {3, 0x01, 0x02, 0x04};
static const unsigned char c_a[] PROGMEM =  {4, 0x20, 0x54, 0x54, 0x78};
static const unsigned char c_b[] PROGMEM =  {4, 0x7F, 0x48, 0x48, 0x30};
static const unsigned char c_c[] PROGMEM =  {3, 0x30, 0x48, 0x48};
static const unsigned char c_d[] PROGMEM =  {4, 0x30, 0x48, 0x48, 0x7F};
static const unsigned char c_e[] PROGMEM =  {4, 0x38, 0x54, 0x54, 0x58};
static const unsigned char c_f[] PROGMEM =  {4, 0x08, 0x7E, 0x09, 0x02};
static const unsigned char c_g[] PROGMEM =  {4, 0x48, 0x54, 0x54, 0x3C};
static const unsigned char c_h[] PROGMEM =  {4, 0x7F, 0x08, 0x08, 0x70};
static const unsigned char c_i[] PROGMEM =  {1, 0x7A};
static const unsigned char c_j[] PROGMEM =  {4, 0x20, 0x40, 0x40, 0x3D};
static const unsigned char c_k[] PROGMEM =  {4, 0x7F, 0x10, 0x28, 0x44};
static const unsigned char c_l[] PROGMEM =  {3, 0x41, 0x7F, 0x40};
static const unsigned char c_m[] PROGMEM =  {5, 0x78, 0x08, 0x30, 0x08, 0x78};
static const unsigned char c_n[] PROGMEM =  {4, 0x78, 0x08, 0x08, 0x70};
static const unsigned char c_o[] PROGMEM =  {4, 0x30, 0x48, 0x48, 0x30};
static const unsigned char c_p[] PROGMEM =  {4, 0x7C, 0x14, 0x14, 0x08};
static const unsigned char c_q[] PROGMEM =  {4, 0x08, 0x14, 0x14, 0x7C};
static const unsigned char c_r[] PROGMEM =  {4, 0x78, 0x10, 0x08, 0x10};
static const unsigned char c_s[] PROGMEM =  {4, 0x48, 0x54, 0x54, 0x24};
static const unsigned char c_t[] PROGMEM =  {5, 0x04, 0x04, 0x3F, 0x44, 0x24};
static const unsigned char c_u[] PROGMEM =  {4, 0x38, 0x40, 0x40, 0x38};
static const unsigned char c_v[] PROGMEM =  {5, 0x18, 0x20, 0x40, 0x20, 0x18};
static const unsigned char c_w[] PROGMEM =  {5, 0x38, 0x40, 0x30, 0x40, 0x38};
static const unsigned char c_x[] PROGMEM =  {5, 0x44, 0x28, 0x10, 0x28, 0x44};
static const unsigned char c_y[] PROGMEM =  {5, 0x04, 0x48, 0x30, 0x08, 0x04};
static const unsigned char c_z[] PROGMEM =  {5, 0x44, 0x64, 0x54, 0x4C, 0x44};
static const unsigned char c_oc[] PROGMEM = {4, 0x08, 0x36, 0x41, 0x41};
static const unsigned char c_vb[] PROGMEM = {1, 0x77};
static const unsigned char c_cc[] PROGMEM = {4, 0x41, 0x41, 0x36, 0x08};
static const unsigned char c_tl[] PROGMEM = {5, 0x10, 0x08, 0x08, 0x08, 0x04};

/* Pointers to characters */
const unsigned char * const char_table[] PROGMEM = { 
    c_sp, c_ex, c_qt, c_ha, c_dl, c_pc, c_am, c_ap, c_op, c_cp, c_as, c_pl, 
    c_cm, c_da, c_pr, c_fs, c_0,  c_1,  c_2,  c_3,  c_4,  c_5,  c_6,  c_7,  
    c_8,  c_9,  c_co, c_sc, c_lt, c_eq, c_gt, c_qm, c_at, c_A,  c_B,  c_C,  
    c_D,  c_E,  c_F,  c_G,  c_H,  c_I,  c_J,  c_K,  c_L,  c_M,  c_N,  c_O, 
    c_P,  c_Q,  c_R,  c_S,  c_T,  c_U,  c_V,  c_W,  c_X,  c_Y,  c_Z,  c_ob,
    c_bs, c_cb, c_ct, c_us, c_ga, c_a,  c_b,  c_c,  c_d,  c_e,  c_f,  c_g,
    c_h,  c_i,  c_j,  c_k,  c_l,  c_m,  c_n,  c_o,  c_p,  c_q,  c_r,  c_s,
    c_t,  c_u,  c_v,  c_w,  c_x,  c_y,  c_z,  c_oc, c_vb, c_cc, c_tl 
};


#endif // LED_Font_1_H