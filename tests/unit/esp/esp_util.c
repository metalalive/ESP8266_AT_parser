#include "esp/esp.h"
#include "unity.h"
#include "unity_fixture.h"
#include <string.h> // Required for memset

TEST_GROUP(parseNumFromStr);
TEST_GROUP(cvtNumToStr); // New test group for conversion functions
TEST_GROUP(parseStrUntilToken);

TEST_SETUP(parseNumFromStr) {}

TEST_TEAR_DOWN(parseNumFromStr) {}

TEST_SETUP(cvtNumToStr) {} // Setup for new test group

TEST_TEAR_DOWN(cvtNumToStr) {} // Teardown for new test group

TEST_SETUP(parseStrUntilToken) {}

TEST_TEAR_DOWN(parseStrUntilToken) {}

TEST(parseNumFromStr, DecimalBasic) {
    uint8_t *test_str_ptr;
    int      parsed_val;

    // Test case 1: Positive number at the beginning of the string
    uint8_t test_data1[] = "123abc";
    test_str_ptr = test_data1;
    parsed_val = iESPparseFirstNumFromStr(&test_str_ptr, ESP_DIGIT_BASE_DECIMAL);
    TEST_ASSERT_EQUAL_INT(123, parsed_val);
    TEST_ASSERT_EQUAL_PTR(test_data1 + 3, test_str_ptr); // Should point to 'a'

    // Test case 2: Negative number at the beginning of the string
    uint8_t test_data2[] = "-456xyz";
    test_str_ptr = test_data2;
    parsed_val = iESPparseFirstNumFromStr(&test_str_ptr, ESP_DIGIT_BASE_DECIMAL);
    TEST_ASSERT_EQUAL_INT(-456, parsed_val);
    TEST_ASSERT_EQUAL_PTR(test_data2 + 4, test_str_ptr); // Should point to 'x'

    // Test case 3: Positive number with leading non-digit characters
    uint8_t test_data3[] = "abc789def";
    test_str_ptr = test_data3;
    parsed_val = iESPparseFirstNumFromStr(&test_str_ptr, ESP_DIGIT_BASE_DECIMAL);
    TEST_ASSERT_EQUAL_INT(789, parsed_val);
    TEST_ASSERT_EQUAL_PTR(test_data3 + 6, test_str_ptr); // Should point to 'd'

    // Test case 4: Negative number with leading non-digit characters and a minus sign
    uint8_t test_data4[] = "prefix-100suffix";
    test_str_ptr = test_data4;
    parsed_val = iESPparseFirstNumFromStr(&test_str_ptr, ESP_DIGIT_BASE_DECIMAL);
    TEST_ASSERT_EQUAL_INT(-100, parsed_val);
    TEST_ASSERT_EQUAL_PTR(test_data4 + 10, test_str_ptr); // Should point to 's'

    // Test case 5: Number at the very end of the string
    uint8_t test_data5[] = "end123";
    test_str_ptr = test_data5;
    parsed_val = iESPparseFirstNumFromStr(&test_str_ptr, ESP_DIGIT_BASE_DECIMAL);
    TEST_ASSERT_EQUAL_INT(123, parsed_val);
    TEST_ASSERT_EQUAL_PTR(test_data5 + 6, test_str_ptr); // Should point to null terminator

    // Test case 6: String with only a number
    uint8_t test_data6[] = "987";
    test_str_ptr = test_data6;
    parsed_val = iESPparseFirstNumFromStr(&test_str_ptr, ESP_DIGIT_BASE_DECIMAL);
    TEST_ASSERT_EQUAL_INT(987, parsed_val);
    TEST_ASSERT_EQUAL_PTR(test_data6 + 3, test_str_ptr); // Should point to null terminator

    // Test case 7: String with only a negative number
    uint8_t test_data7[] = "-50";
    test_str_ptr = test_data7;
    parsed_val = iESPparseFirstNumFromStr(&test_str_ptr, ESP_DIGIT_BASE_DECIMAL);
    TEST_ASSERT_EQUAL_INT(-50, parsed_val);
    TEST_ASSERT_EQUAL_PTR(test_data7 + 3, test_str_ptr); // Should point to null terminator

    // Test case 8: String with multiple minus signs before a number (only the last one should count
    // if followed by digit)
    uint8_t test_data8[] = "--123abc";
    test_str_ptr = test_data8;
    parsed_val = iESPparseFirstNumFromStr(&test_str_ptr, ESP_DIGIT_BASE_DECIMAL);
    TEST_ASSERT_EQUAL_INT(-123, parsed_val);
    TEST_ASSERT_EQUAL_PTR(test_data8 + 5, test_str_ptr); // Should point to 'a'
}

TEST(parseNumFromStr, DecimalNoNum) {
    uint8_t *test_str_ptr;
    int      parsed_val;

    // Test case 1: Empty string
    uint8_t test_data1[] = "";
    test_str_ptr = test_data1;
    parsed_val = iESPparseFirstNumFromStr(&test_str_ptr, ESP_DIGIT_BASE_DECIMAL);
    TEST_ASSERT_EQUAL_INT(0, parsed_val);
    TEST_ASSERT_EQUAL_PTR(test_data1, test_str_ptr); // Should remain at the start (or end if empty)

    // Test case 2: String with no digits
    uint8_t test_data2[] = "abcdefg";
    test_str_ptr = test_data2;
    parsed_val = iESPparseFirstNumFromStr(&test_str_ptr, ESP_DIGIT_BASE_DECIMAL);
    TEST_ASSERT_EQUAL_INT(0, parsed_val);
    TEST_ASSERT_EQUAL_PTR(test_data2 + 7, test_str_ptr); // Should point to null terminator

    // Test case 3: String with only a minus sign
    uint8_t test_data3[] = "-";
    test_str_ptr = test_data3;
    parsed_val = iESPparseFirstNumFromStr(&test_str_ptr, ESP_DIGIT_BASE_DECIMAL);
    TEST_ASSERT_EQUAL_INT(0, parsed_val);
    TEST_ASSERT_EQUAL_PTR(test_data3 + 1, test_str_ptr); // Should point to null terminator

    // Test case 4: String with non-digits and a lone minus sign not followed by a digit
    uint8_t test_data4[] = "abc-def";
    test_str_ptr = test_data4;
    parsed_val = iESPparseFirstNumFromStr(&test_str_ptr, ESP_DIGIT_BASE_DECIMAL);
    TEST_ASSERT_EQUAL_INT(0, parsed_val);
    TEST_ASSERT_EQUAL_PTR(test_data4 + 7, test_str_ptr); // Should point to null terminator

    // Test case 5: String with only non-decimal characters
    uint8_t test_data5[] = "xABC"; // a hex number, but not a decimal number if 'x' is not a digit
    test_str_ptr = test_data5;
    parsed_val = iESPparseFirstNumFromStr(&test_str_ptr, ESP_DIGIT_BASE_DECIMAL);
    TEST_ASSERT_EQUAL_INT(0, parsed_val); // Should not parse "0xABC" as a decimal number
    TEST_ASSERT_EQUAL_PTR(test_data5 + 4, test_str_ptr); // Should point to null terminator
}

TEST(parseNumFromStr, HexBasic) {
    uint8_t *test_str_ptr;
    int      parsed_val;
    // Note only the hex string without leading zero will be parsed
    // Test case 1: Positive hex number at the beginning
    uint8_t test_data1[] = "1A2Bxyz";
    test_str_ptr = test_data1;
    parsed_val = iESPparseFirstNumFromStr(&test_str_ptr, ESP_DIGIT_BASE_HEX);
    TEST_ASSERT_EQUAL_INT(0x1A2B, parsed_val);
    TEST_ASSERT_EQUAL_PTR(test_data1 + 4, test_str_ptr); // Should point to 'x'

    // Test case 2: Hex number with lowercase letters
    uint8_t test_data2[] = "Xdeadbeef_suffix";
    test_str_ptr = test_data2;
    parsed_val = iESPparseFirstNumFromStr(&test_str_ptr, ESP_DIGIT_BASE_HEX);
    TEST_ASSERT_EQUAL_INT((int)0xdeadbeef, parsed_val);
    // Should point to '_' (after 'X' and 'deadbeef')
    TEST_ASSERT_EQUAL_PTR(test_data2 + 9, test_str_ptr);

    // Test case 3: Hex number with mixed case letters and leading non-hex chars
    uint8_t test_data3[] = "sup_C0FFEE_end";
    test_str_ptr = test_data3;
    parsed_val = iESPparseFirstNumFromStr(&test_str_ptr, ESP_DIGIT_BASE_HEX);
    TEST_ASSERT_EQUAL_INT((int)0xC0FFEE, parsed_val);
    TEST_ASSERT_EQUAL_PTR(test_data3 + 10, test_str_ptr); // Should point to '_'

    // Test case 4: Hex number at the very end
    uint8_t test_data4[] = "urban_lifewild";
    test_str_ptr = test_data4;
    parsed_val = iESPparseFirstNumFromStr(&test_str_ptr, ESP_DIGIT_BASE_HEX);
    TEST_ASSERT_EQUAL_INT(0xba, parsed_val);
    TEST_ASSERT_EQUAL_PTR(test_data4 + 4, test_str_ptr); // Should point to null terminator

    // Test case 5: String with only a hex number
    uint8_t test_data5[] = "ABC";
    test_str_ptr = test_data5;
    parsed_val = iESPparseFirstNumFromStr(&test_str_ptr, ESP_DIGIT_BASE_HEX);
    TEST_ASSERT_EQUAL_INT(0xABC, parsed_val);
    TEST_ASSERT_EQUAL_PTR(test_data5 + 3, test_str_ptr); // Should point to null terminator

    // Test case 6: Hex number starting with '0'
    uint8_t test_data6[] = "0123GHI";
    test_str_ptr = test_data6;
    parsed_val = iESPparseFirstNumFromStr(&test_str_ptr, ESP_DIGIT_BASE_HEX);
    TEST_ASSERT_EQUAL_INT(0x123, parsed_val);
    TEST_ASSERT_EQUAL_PTR(test_data6 + 4, test_str_ptr); // Should point to 'G'
}

TEST(parseNumFromStr, HexNoNum) {
    uint8_t *test_str_ptr;
    int      parsed_val;

    // Test case 1: Empty string
    uint8_t test_data1[] = "";
    test_str_ptr = test_data1;
    parsed_val = iESPparseFirstNumFromStr(&test_str_ptr, ESP_DIGIT_BASE_HEX);
    TEST_ASSERT_EQUAL_INT(0, parsed_val);
    TEST_ASSERT_EQUAL_PTR(test_data1, test_str_ptr); // Should remain at the start (or end if empty)

    // Test case 2: String with no hex digits
    uint8_t test_data2[] = "ghijklm"; // 'g' is not a hex digit
    test_str_ptr = test_data2;
    parsed_val = iESPparseFirstNumFromStr(&test_str_ptr, ESP_DIGIT_BASE_HEX);
    TEST_ASSERT_EQUAL_INT(0, parsed_val);
    TEST_ASSERT_EQUAL_PTR(test_data2 + 7, test_str_ptr); // Should point to null terminator

    // Test case 3: String with only non-hex characters
    uint8_t test_data3[] = "!@#$%^&*";
    test_str_ptr = test_data3;
    parsed_val = iESPparseFirstNumFromStr(&test_str_ptr, ESP_DIGIT_BASE_HEX);
    TEST_ASSERT_EQUAL_INT(0, parsed_val);
    TEST_ASSERT_EQUAL_PTR(test_data3 + 8, test_str_ptr); // Should point to null terminator

    // Test case 4: String with characters that are not hex digits
    uint8_t test_data4[] = "GHIJKL"; // 'G' is not a hex digit
    test_str_ptr = test_data4;
    parsed_val = iESPparseFirstNumFromStr(&test_str_ptr, ESP_DIGIT_BASE_HEX);
    TEST_ASSERT_EQUAL_INT(0, parsed_val);
    TEST_ASSERT_EQUAL_PTR(test_data4 + 6, test_str_ptr); // Should point to null terminator

    // Test case 5: String with a minus sign (hex numbers are typically unsigned or handled
    // differently for sign)
    uint8_t test_data5[] = "-12a";
    test_str_ptr = test_data5;
    parsed_val = iESPparseFirstNumFromStr(&test_str_ptr, ESP_DIGIT_BASE_HEX);
    TEST_ASSERT_EQUAL_INT(-0x12a, parsed_val);           // Should parse '123' as hex, ignoring '-'
    TEST_ASSERT_EQUAL_PTR(test_data5 + 4, test_str_ptr); // Should point to null terminator
}

TEST(cvtNumToStr, DecimalBasic) {
    uint8_t  out_buffer[20]; // Sufficiently large buffer
    uint32_t num_chars;

    // Test case 1: Positive number
    int num1 = 123;
    memset(out_buffer, 0, sizeof(out_buffer)); // Clear buffer
    num_chars = uiESPcvtNumToStr(out_buffer, num1, ESP_DIGIT_BASE_DECIMAL);
    TEST_ASSERT_EQUAL_UINT(3, num_chars);
    out_buffer[num_chars] = '\0'; // Null-terminate for string comparison
    TEST_ASSERT_EQUAL_STRING("123", (char *)out_buffer);

    // Test case 2: Zero
    int num2 = 0;
    memset(out_buffer, 0, sizeof(out_buffer)); // Clear buffer
    num_chars = uiESPcvtNumToStr(out_buffer, num2, ESP_DIGIT_BASE_DECIMAL);
    TEST_ASSERT_EQUAL_UINT(1, num_chars);
    out_buffer[num_chars] = '\0'; // Null-terminate for string comparison
    TEST_ASSERT_EQUAL_STRING("0", (char *)out_buffer);

    // Test case 3: Another positive number
    int num3 = 45678;
    memset(out_buffer, 0, sizeof(out_buffer)); // Clear buffer
    num_chars = uiESPcvtNumToStr(out_buffer, num3, ESP_DIGIT_BASE_DECIMAL);
    TEST_ASSERT_EQUAL_UINT(5, num_chars);
    out_buffer[num_chars] = '\0'; // Null-terminate for string comparison
    TEST_ASSERT_EQUAL_STRING("45678", (char *)out_buffer);

    // Test case 4: Max int value (or close to it)
    int num4 = 2147483647;                     // INT_MAX
    memset(out_buffer, 0, sizeof(out_buffer)); // Clear buffer
    num_chars = uiESPcvtNumToStr(out_buffer, num4, ESP_DIGIT_BASE_DECIMAL);
    TEST_ASSERT_EQUAL_UINT(10, num_chars);
    out_buffer[num_chars] = '\0'; // Null-terminate for string comparison
    TEST_ASSERT_EQUAL_STRING("2147483647", (char *)out_buffer);

    // Test case 5: Concatenating numbers into the same buffer
    memset(out_buffer, 0, sizeof(out_buffer)); // Clear buffer for this sub-case
    num_chars = uiESPcvtNumToStr(out_buffer, 567, ESP_DIGIT_BASE_DECIMAL);
    TEST_ASSERT_EQUAL_UINT(3, num_chars);
    num_chars = uiESPcvtNumToStr(&out_buffer[3], 148, ESP_DIGIT_BASE_DECIMAL);
    TEST_ASSERT_EQUAL_UINT(3, num_chars);
    out_buffer[6] = '\0'; // Null-terminate the combined string
    TEST_ASSERT_EQUAL_STRING("567148", (char *)out_buffer);
}

TEST(cvtNumToStr, HexBasic) {
    uint8_t  out_buffer[20]; // Sufficiently large buffer
    uint32_t num_chars;

    // Test case 1: Zero
    int num1 = 0;
    memset(out_buffer, 0, sizeof(out_buffer)); // Clear buffer
    num_chars = uiESPcvtNumToStr(out_buffer, num1, ESP_DIGIT_BASE_HEX);
    TEST_ASSERT_EQUAL_UINT(1, num_chars);
    out_buffer[num_chars] = '\0'; // Null-terminate for string comparison
    TEST_ASSERT_EQUAL_STRING("0", (char *)out_buffer);

    // Test case 2: Single digit hex
    int num2 = 0xA;                            // 10
    memset(out_buffer, 0, sizeof(out_buffer)); // Clear buffer
    num_chars = uiESPcvtNumToStr(out_buffer, num2, ESP_DIGIT_BASE_HEX);
    TEST_ASSERT_EQUAL_UINT(1, num_chars);
    out_buffer[num_chars] = '\0'; // Null-terminate for string comparison
    TEST_ASSERT_EQUAL_STRING("a", (char *)out_buffer);

    // Test case 3: Two digit hex
    int num3 = 0x2F;                           // 47
    memset(out_buffer, 0, sizeof(out_buffer)); // Clear buffer
    num_chars = uiESPcvtNumToStr(out_buffer, num3, ESP_DIGIT_BASE_HEX);
    TEST_ASSERT_EQUAL_UINT(2, num_chars);
    out_buffer[num_chars] = '\0'; // Null-terminate for string comparison
    TEST_ASSERT_EQUAL_STRING("2f", (char *)out_buffer);

    // Test case 4: Multiple digit hex with mixed numbers and letters
    int num4 = 0xaBCd12;                       // 11258002
    memset(out_buffer, 0, sizeof(out_buffer)); // Clear buffer
    num_chars = uiESPcvtNumToStr(out_buffer, num4, ESP_DIGIT_BASE_HEX);
    TEST_ASSERT_EQUAL_UINT(6, num_chars);
    out_buffer[num_chars] = '\0'; // Null-terminate for string comparison
    TEST_ASSERT_EQUAL_STRING("abcd12", (char *)out_buffer);

    // Test case 5: Max positive int value in hex (assuming int is 32-bit)
    int num5 = 0x7FFFFFFF;                     // INT_MAX
    memset(out_buffer, 0, sizeof(out_buffer)); // Clear buffer
    num_chars = uiESPcvtNumToStr(out_buffer, num5, ESP_DIGIT_BASE_HEX);
    TEST_ASSERT_EQUAL_UINT(8, num_chars);
    out_buffer[num_chars] = '\0'; // Null-terminate for string comparison
    TEST_ASSERT_EQUAL_STRING("7fffffff", (char *)out_buffer);

    // Test case 6: Concatenating hex numbers into the same buffer
    memset(out_buffer, 0, sizeof(out_buffer)); // Clear buffer for this sub-case
    num_chars = uiESPcvtNumToStr(out_buffer, 0x1A, ESP_DIGIT_BASE_HEX);
    TEST_ASSERT_EQUAL_UINT(2, num_chars);
    num_chars = uiESPcvtNumToStr(&out_buffer[2], 0x2E, ESP_DIGIT_BASE_HEX);
    TEST_ASSERT_EQUAL_UINT(2, num_chars);
    out_buffer[4] = '\0'; // Null-terminate the combined string
    TEST_ASSERT_EQUAL_STRING("1a2e", (char *)out_buffer);
}

TEST(parseStrUntilToken, BasicCases) {
    char  out_buffer[20]; // Sufficiently large buffer for tests
    short chars_copied = 0;

    // sub-case: Token in the middle
    const char *src1 = "hello,world";
    memset(out_buffer, 0, sizeof(out_buffer));
    chars_copied = uESPparseStrUntilToken(out_buffer, src1, sizeof(out_buffer), ESP_ASCII_COMMA);
    TEST_ASSERT_EQUAL_UINT(5, chars_copied);
    out_buffer[chars_copied] = '\0'; // Null-terminate for comparison
    TEST_ASSERT_EQUAL_STRING("hello", out_buffer);

    // sub-case: Token at the beginning
    const char *src2 = ",world";
    memset(out_buffer, 0, sizeof(out_buffer));
    chars_copied = uESPparseStrUntilToken(out_buffer, src2, sizeof(out_buffer), ESP_ASCII_COMMA);
    TEST_ASSERT_EQUAL_UINT(0, chars_copied);
    out_buffer[chars_copied] = '\0'; // Null-terminate for comparison
    TEST_ASSERT_EQUAL_STRING("", out_buffer);

    // sub-case: Token at the end
    const char *src3 = "hello,";
    memset(out_buffer, 0, sizeof(out_buffer));
    chars_copied = uESPparseStrUntilToken(out_buffer, src3, sizeof(out_buffer), ESP_ASCII_COMMA);
    TEST_ASSERT_EQUAL_UINT(5, chars_copied);
    out_buffer[chars_copied] = '\0'; // Null-terminate for comparison
    TEST_ASSERT_EQUAL_STRING("hello", out_buffer);

    // sub-case: Token not found
    const char *src4 = "helloworld";
    memset(out_buffer, 0, sizeof(out_buffer));
    chars_copied = uESPparseStrUntilToken(out_buffer, src4, sizeof(out_buffer), ESP_ASCII_COMMA);
    TEST_ASSERT_LESS_THAN(0, chars_copied);
    // token not found, content copied to `out_buffer` is useless
    // TODO, improve the function

    // sub-case: all blank chars string
    const char *src5 = "            ";
    memset(out_buffer, 0, sizeof(out_buffer));
    chars_copied = uESPparseStrUntilToken(out_buffer, src5, sizeof(out_buffer), ESP_ASCII_COMMA);
    TEST_ASSERT_LESS_THAN(0, chars_copied);
}

TEST(parseStrUntilToken, BufferLimitCases) {
    char  out_buffer[10]; // Small buffer for testing limits
    short chars_copied = 0;

    // Test 1: Token found exactly at the buffer limit (or just beyond what can be copied)
    // "longstring,token" -> token is at index 10. If buffer size is 10, it copies "longstring" (10
    // chars)
    const char *src1 = "longstring,token";
    memset(out_buffer, 0, sizeof(out_buffer));
    chars_copied = uESPparseStrUntilToken(out_buffer, src1, sizeof(out_buffer), ESP_ASCII_COMMA);
    // Should copy up to 10 chars, but did not reach the token
    TEST_ASSERT_LESS_THAN(0, chars_copied);

    // Test 2: Token not found, source string longer than buffer limit
    const char *src2 = "verylongstringwithouttoken";
    memset(out_buffer, 0, sizeof(out_buffer));
    chars_copied = uESPparseStrUntilToken(out_buffer, src2, sizeof(out_buffer), ESP_ASCII_COMMA);
    TEST_ASSERT_LESS_THAN(0, chars_copied);
}

TEST(parseStrUntilToken, NullDestinationBuffer) {
    short chars_copied = 0;

    // Test 1: Token in the middle, _des is NULL
    const char *src1 = "hello,world";
    chars_copied = uESPparseStrUntilToken(NULL, src1, 100, ESP_ASCII_COMMA);
    TEST_ASSERT_LESS_THAN(0, chars_copied);

    // Test 2: Token not found, _des is NULL
    const char *src2 = "helloworld";
    chars_copied = uESPparseStrUntilToken(NULL, src2, 100, ESP_ASCII_COMMA);
    TEST_ASSERT_LESS_THAN(0, chars_copied);

    // Test 3: Empty source string, _des is NULL
    const char *src3 = "";
    chars_copied = uESPparseStrUntilToken(NULL, src3, 100, ESP_ASCII_COMMA);
    TEST_ASSERT_LESS_THAN(0, chars_copied);

    // Test 4: Buffer limit reached, _des is NULL
    const char *src4 = "verylongstringwithouttoken";
    chars_copied = uESPparseStrUntilToken(NULL, src4, 5, ESP_ASCII_COMMA);
    TEST_ASSERT_LESS_THAN(0, chars_copied);
}

TEST_GROUP_RUNNER(EspUtility) {
    RUN_TEST_CASE(parseNumFromStr, DecimalBasic);
    RUN_TEST_CASE(parseNumFromStr, DecimalNoNum);
    RUN_TEST_CASE(parseNumFromStr, HexBasic);
    RUN_TEST_CASE(parseNumFromStr, HexNoNum);
    RUN_TEST_CASE(cvtNumToStr, DecimalBasic);
    RUN_TEST_CASE(cvtNumToStr, HexBasic);
    RUN_TEST_CASE(parseStrUntilToken, BasicCases);
    RUN_TEST_CASE(parseStrUntilToken, BufferLimitCases);
    RUN_TEST_CASE(parseStrUntilToken, NullDestinationBuffer);
}
