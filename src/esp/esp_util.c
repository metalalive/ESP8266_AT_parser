#include "esp/esp.h"

int iESPparseFirstNumFromStr(uint8_t **out_chr_pp, espDigitBase_t base_option) {
    int     val = 0;
    uint8_t base, last_non_digit = 0, *p = *out_chr_pp;
    int8_t  minus = 1;
    switch (base_option) {
    case ESP_DIGIT_BASE_HEX:
        base = 16;
        while ((*p != '\0') && !ESP_CHARISHEXNUM(*p)) {
            p++;
        }
        break;
    case ESP_DIGIT_BASE_DECIMAL:
    default:
        base = 10;
        while ((*p != '\0') && !ESP_CHARISNUM(*p)) {
            p++;
        }
        break;
    }

    last_non_digit = *(p - 1);
    minus = (last_non_digit == '-' ? -1 : 1);
    switch (base_option) {
    case ESP_DIGIT_BASE_HEX:
        while ((*p != '\0') && ESP_CHARISHEXNUM(*p)) {
            val = val * base + ESP_CHARHEXTONUM(*p);
            p++;
        }
        break;
    case ESP_DIGIT_BASE_DECIMAL:
    default:
        while ((*p != '\0') && ESP_CHARISNUM(*p)) {
            val = val * base + ESP_CHARTONUM(*p);
            p++;
        }
        break;
    }
    val = val * minus;
    *out_chr_pp = p;
    return val;
} // end of iESPparseFirstNumFromStr

uint32_t uiESPcvtNumToStr(uint8_t *out_chr_p, int num, espDigitBase_t base_option) {
    uint32_t num_chr = 0;
    uint8_t  base = 0, digit = 0, idx = 0;
    if (num == 0) { // if input number is zero
        *(out_chr_p + num_chr) = ESP_NUMTOCHAR(digit);
        num_chr = 1;
    }
    switch (base_option) {
    case ESP_DIGIT_BASE_HEX:
        base = 16;
        break;
    case ESP_DIGIT_BASE_DECIMAL:
    default:
        base = 10;
        break;
    }
    while (num != 0) { // if input number is non-zero
        digit = num % base;
        num = (num - digit) / base;
        *(out_chr_p + num_chr) = ESP_NUMTOCHAR(digit);
        num_chr++;
    }
    // reserve the appended string
    for (idx = 0; idx < (num_chr >> 1); idx++) {
        digit = *(out_chr_p + idx);
        *(out_chr_p + idx) = *(out_chr_p + num_chr - 1 - idx);
        *(out_chr_p + num_chr - 1 - idx) = digit;
    }
    return num_chr;
} // end of uiESPcvtNumToStr

short uESPparseStrUntilToken(char *des, const char *src, uint16_t des_len, uint8_t token) {
#define TOKEN_NOT_FOUND -1
    uint16_t num_chrs_copied = 0;
    uint8_t  chr = 0, tok_found = 0;
    if (des == NULL) {
        return (short)TOKEN_NOT_FOUND;
    }
    while (num_chrs_copied < des_len) {
        chr = *(src + num_chrs_copied);
        if (chr == token) {
            tok_found = 1;
            break;
        }
        *(des + num_chrs_copied) = chr;
        num_chrs_copied++;
    }
    return (short)(tok_found ? num_chrs_copied : TOKEN_NOT_FOUND);
#undef TOKEN_NOT_FOUND
}
