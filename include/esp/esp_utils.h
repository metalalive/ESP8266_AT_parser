#ifndef __ESP_UTILS_H
#define __ESP_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif


#define ESP_AT_CMD_BLOCKING      1 

#define ESP_AT_CMD_NONBLOCKING   0 

#define ESP_HW_RST_ASSERT        0

#define ESP_HW_RST_DEASSERT      1

// following 2 macros determine whether current set value should be saved to ESP device flash as 
// default value, the set value will be applied immediately after the ESP is reset next time. 
#define ESP_SETVALUE_NOT_SAVE    0
#define ESP_SETVALUE_SAVE        1


// Align `x` value to specific number of bytes, provided by  ESP_CFG_MEM_ALIGNMENT configuration
#define ESP_MEM_ALIGN(x)       ((x + (ESP_CFG_MEM_ALIGNMENT - 1)) & ~(ESP_CFG_MEM_ALIGNMENT - 1))

#define ESP_MIN(x, y)          ((x) < (y) ? (x) : (y))

#define ESP_MAX(x, y)          ((x) > (y) ? (x) : (y))

#define ESP_ARRAYSIZE(x)       (sizeof(x) / sizeof((x)[0]))

#define ESP_UNUSED(x)          ((void)(x))

// following macro only checks for decimal number
#define ESP_CHARISNUM(x)       ((x) >= '0' && (x) <= '9')

#define ESP_CHARTONUM(x)       ((x) - '0')

#define ESP_NUMTOCHAR(x)       ((x) + '0')

#define ESP_CHARISHEXNUM(x)    (((x) >= '0' && (x) <= '9') || ((x) >= 'a' && (x) <= 'f') || ((x) >= 'A' && (x) <= 'F'))

#define ESP_CHARHEXTONUM(x)   \
  ( \
     ((x) >= '0' && (x) <= '9') ? ((x) - '0') : \
        ( \
            ((x) >= 'a' && (x) <= 'f') ? ((x) - 'a' + 10) :  \
        ( \
            ((x) >= 'A' && (x) <= 'F') ? ((x) - 'A' + 10) : 0 \
        ) \
        ) \
  )

#define ESP_CHARTOLOWERCASE(x)    ((x) + 0x20)

#define ESP_CHARTOUPPERCASE(x)    ((x) - 0x20)


#define ESP_ISVALIDASCII(x)   (((x) >= 32 && (x) <= 126) || (x) == '\r' || (x) == '\n')

#define ESP_ASCII_CR              0xd
#define ESP_CHR_CR                "\r"
#define ESP_ASCII_LF              0xa
#define ESP_CHR_LF                "\n"
#define ESP_ASCII_DOUBLE_QUOTE    0x22
#define ESP_CHR_DOUBLE_QUOTE      "\""
#define ESP_ASCII_COMMA           0x2C
#define ESP_CHR_COMMA             ","
#define ESP_ASCII_DOT             0x2E
#define ESP_CHR_DOT               "."
#define ESP_ASCII_COLON           0x3A
#define ESP_CHR_COLON             ":"
#define ESP_ASCII_EQUAL           0x3D

#define ESP_ASCII_CRLF_LEN         2


#define ESP_SET_IP(ip_ptr, ip1, ip2, ip3, ip4)  \
{ \
    (ip_ptr)->ip[0] = (ip1); \
    (ip_ptr)->ip[1] = (ip2); \
    (ip_ptr)->ip[2] = (ip3); \
    (ip_ptr)->ip[3] = (ip4); \
}


int   iESPparseFirstNumFromStr( uint8_t **out_chr_pp, espDigitBase_t base_option );
 
uint32_t  uiESPcvtNumToStr( uint8_t *out_chr_p, int num, espDigitBase_t base_option );

uint16_t uESPparseStrUntilToken( char* _des, const char* _src, uint16_t _des_len, uint8_t token);



#ifdef __cplusplus
}
#endif
#endif // __ESP_UTILS_H

