/*
 * File:   utils.c
 * Author: Ted Salmon <tass2001@gmail.com>
 * Description:
 *     Helper utils that may be useful in more than one place
 */
#include "utils.h"

/* Hold a pin to register map for all programmable output pins */
static uint16_t *ROPR_PINS[] = {
    GET_RPOR(0),
    GET_RPOR(0),
    GET_RPOR(1),
    GET_RPOR(1),
    GET_RPOR(2),
    GET_RPOR(2),
    GET_RPOR(3),
    GET_RPOR(3),
    GET_RPOR(4),
    GET_RPOR(4),
    GET_RPOR(5),
    GET_RPOR(5),
    GET_RPOR(6),
    GET_RPOR(6),
    GET_RPOR(7),
    GET_RPOR(7),
    GET_RPOR(8),
    GET_RPOR(8),
    GET_RPOR(9),
    GET_RPOR(9),
    GET_RPOR(10),
    GET_RPOR(10),
    GET_RPOR(11),
    GET_RPOR(11),
    GET_RPOR(12),
    GET_RPOR(12),
    GET_RPOR(13),
    GET_RPOR(13),
    GET_RPOR(14),
    GET_RPOR(14),
    GET_RPOR(15),
    GET_RPOR(15),
    GET_RPOR(16),
    GET_RPOR(16),
    GET_RPOR(17),
    GET_RPOR(17),
    GET_RPOR(18),
    GET_RPOR(18)
};

UtilsAbstractDisplayValue_t UtilsDisplayValueInit(char *text, uint8_t status)
{
    UtilsAbstractDisplayValue_t value;
    strncpy(value.text, text, UTILS_DISPLAY_TEXT_SIZE - 1);
    value.index = 0;
    value.timeout = 0;
    value.status = status;
    value.length = strlen(text);
    return value;
}

/**
 * UtilsNormalizeText()
 *     Description:
 *         Unescape characters and convert them from UTF-8 to their Unicode
 *         bytes. This is to support extended ASCII.
 *     Params:
 *         char *string - The subject
 *         const char *input - The string to copy from
 *     Returns:
 *         void
 */
void UtilsNormalizeText(char *string, const char *input)
{
    uint16_t idx;
    uint16_t strIdx = 0;
    uint8_t bytesInChar = 0;
    uint32_t unicodeChar;

    uint8_t transIdx;
    uint8_t transStrLength;

    uint16_t strLength = strlen(input);

    for (idx = 0; idx < strLength; idx++) {
        uint8_t c = (uint8_t) input[idx];

        if (c == 0x5C) {
            if (idx + 2 <= strLength) {
                char buf[] = { (uint8_t) input[idx + 1], (uint8_t) input[idx + 2], '\0' };
                c = UtilsStrToHex(buf);

                idx += 2;
            } else {
                idx = strLength;
                continue;
            }
        }

        if (bytesInChar == 0) {
            unicodeChar = 0 | c;

            // Identify number of bytes to read from the first negative byte
            if (c >> 3 == 30) { // 11110xxx
                bytesInChar = 3;
                continue;
            } else if (c >> 4 == 14) { // 1110xxxx
                bytesInChar = 2;
                continue;
            } else if (c >> 5 == 6) { // 110xxxx
                bytesInChar = 1;
                continue;
            }
        }

        if (bytesInChar > 0) {
            unicodeChar = unicodeChar << 8 | c;
            bytesInChar--;
        }

        if (bytesInChar != 0) {
            continue;
        }

        if (unicodeChar <= 0x7F) {
            string[strIdx++] = (char) unicodeChar;
        } else if (unicodeChar >= 0xC2A1 && unicodeChar <= 0xC2BF) {
            // Convert UTF-8 byte to Unicode then check if it falls within
            // the range of extended ASCII
            uint32_t extendedChar = (unicodeChar & 0xFF) + ((unicodeChar >> 8) - 0xC2) * 64;
            if (extendedChar < 0xFF) {
                string[strIdx++] = (char) extendedChar;
            }
        } else if (unicodeChar > 0xC2BF) {
            char * transStr = UtilsTransliterateUnicodeToASCII(unicodeChar);
            transStrLength = strlen(transStr);
            for (transIdx = 0; transIdx < transStrLength; transIdx++) {
                string[strIdx++] = (char)transStr[transIdx];
            }
        }
    }
    string[strIdx] = '\0';
}

/**
 * UtilsRemoveSubstring()
 *     Description:
 *         Remove the given substring from the given subject
 *     Params:
 *         char *string - The subject
 *         const char *trash - The substring to remove
 *     Returns:
 *         void
 */
void UtilsRemoveSubstring(char *string, const char *trash)
{
    uint16_t removeLength = strlen(trash);
    while ((string = strstr(string, trash))) {
        memmove(string, string + removeLength, 1 + strlen(string + removeLength));
    }
}

/**
 * UtilsReset()
 *     Description:
 *         Reset the MCU
 *     Params:
 *         void
 *     Returns:
 *         void
 */
void UtilsReset()
{
    __asm__ volatile ("RESET");
}

/**
 * UtilsSetRPORMode()
 *     Description:
 *         Set the mode of a programmable output pin
 *     Params:
 *         uint8_t pin - The pin to set
 *         uint8_t mode - The mode to set the given pin to
 *     Returns:
 *         void
 */
void UtilsSetRPORMode(uint8_t pin, uint16_t mode)
{
    if ((pin % 2) == 0) {
        uint16_t msb = *ROPR_PINS[pin] >> 8;
        // Set the least significant bits for the even pin number
        *ROPR_PINS[pin] = (msb << 8) + mode;
    } else {
        uint16_t lsb = *ROPR_PINS[pin] & 0xFF;
        // Set the least significant bits of the register for the odd pin number
        *ROPR_PINS[pin] = (mode << 8) + lsb;
    }
}

/**
 * UtilsStrToHex()
 *     Description:
 *         Convert a string to a octal
 *     Params:
 *         char *string - The subject
 *     Returns:
 *         uint8_t The unsigned char
 */
unsigned char UtilsStrToHex(char *string)
{
    char *ptr;
    return (unsigned char) strtol(string, &ptr, 16);
}


/**
 * UtilsStrToInt()
 *     Description:
 *         Convert a string to an integer
 *     Params:
 *         char *string - The subject
 *     Returns:
 *         uint8_t The Unsigned 8-bit integer representation
 */
uint8_t UtilsStrToInt(char *string)
{
    char *ptr;
    return (uint8_t) strtol(string, &ptr, 10);
}

/**
 * UtilsStricmp()
 *     Description:
 *         Case-Insensitive string comparison 
 *     Params:
 *         const char *string - The subject
 *         const char *compare - The string to compare the subject against
 *     Returns:
 *         int8_t -
 *             Negative 1 when string is less than compare
 *             Zero when string matches compare
 *             Positive 1 when string is greater than compare
 */
int8_t UtilsStricmp(const char *string, const char *compare)
{
    int8_t result;
    while(!(result = toupper(*string) - toupper(*compare)) && *string) {
        string++;
        compare++;
    }
    return result;
}

/**
 * UtilsTransliterateUnicodeToExtendedASCII()
 *     Description:
 *         Transliterates Unicode character to the corresponding ASCII string
 *     Params:
 *         uint32_t input - Representation of the Unicode character
 *     Returns:
 *         char * - Corresponding Extended ASCII characters
 */
char * UtilsTransliterateUnicodeToASCII(uint32_t input)
{
    switch (input) {
        case UTILS_CHAR_LATIN_CAPITAL_A_WITH_GRAVE:
        case UTILS_CHAR_LATIN_CAPITAL_A_WITH_ACUTE:
        case UTILS_CHAR_LATIN_CAPITAL_A_WITH_CIRCUMFLEX:
        case UTILS_CHAR_LATIN_CAPITAL_A_WITH_TILDE:
        case UTILS_CHAR_LATIN_CAPITAL_A_WITH_DIAERESIS:
        case UTILS_CHAR_LATIN_CAPITAL_A_WITH_RING_ABOVE:
            return "A";
            break;
        case UTILS_CHAR_LATIN_CAPITAL_AE:
            return "Ae";
            break;
        case UTILS_CHAR_LATIN_CAPITAL_C_WITH_CEDILLA:
            return "C";
            break;
        case UTILS_CHAR_LATIN_CAPITAL_E_WITH_GRAVE:
        case UTILS_CHAR_LATIN_CAPITAL_E_WITH_ACUTE:
        case UTILS_CHAR_LATIN_CAPITAL_E_WITH_CIRCUMFLEX:
        case UTILS_CHAR_LATIN_CAPITAL_E_WITH_DIAERESIS:
            return "E";
            break;
        case UTILS_CHAR_LATIN_CAPITAL_I_WITH_GRAVE:
        case UTILS_CHAR_LATIN_CAPITAL_I_WITH_ACUTE:
        case UTILS_CHAR_LATIN_CAPITAL_I_WITH_CIRCUMFLEX:
        case UTILS_CHAR_LATIN_CAPITAL_I_WITH_DIAERESIS:
            return "I";
            break;
        case UTILS_CHAR_LATIN_CAPITAL_ETH:
            return "Eth";
            break;
        case UTILS_CHAR_LATIN_CAPITAL_N_WITH_TILDE:
            return "N";
            break;
        case UTILS_CHAR_LATIN_CAPITAL_O_WITH_GRAVE:
        case UTILS_CHAR_LATIN_CAPITAL_O_WITH_ACUTE:
        case UTILS_CHAR_LATIN_CAPITAL_O_WITH_CIRCUMFLEX:
        case UTILS_CHAR_LATIN_CAPITAL_O_WITH_TILDE:
        case UTILS_CHAR_LATIN_CAPITAL_O_WITH_DIAERESIS:
        case UTILS_CHAR_LATIN_CAPITAL_O_WITH_STROKE:
            return "O";
            break;
        case UTILS_CHAR_MULTIPLICATION_SIGN:
            return "x";
            break;
        case UTILS_CHAR_LATIN_CAPITAL_U_WITH_GRAVE:
        case UTILS_CHAR_LATIN_CAPITAL_U_WITH_ACUTE:
        case UTILS_CHAR_LATIN_CAPITAL_U_WITH_CIRCUMFLEX:
        case UTILS_CHAR_LATIN_CAPITAL_U_WITH_DIAERESIS:
            return "U";
            break;
        case UTILS_CHAR_LATIN_CAPITAL_Y_WITH_ACUTE:
            return "Y";
            break;
        case UTILS_CHAR_LATIN_CAPITAL_THORN:
            return "Th";
            break;
        case UTILS_CHAR_LATIN_SMALL_SHARP_S:
            return "ss";
            break;
        case UTILS_CHAR_LATIN_SMALL_A_WITH_GRAVE:
        case UTILS_CHAR_LATIN_SMALL_A_WITH_ACUTE:
        case UTILS_CHAR_LATIN_SMALL_A_WITH_CIRCUMFLEX:
        case UTILS_CHAR_LATIN_SMALL_A_WITH_TILDE:
        case UTILS_CHAR_LATIN_SMALL_A_WITH_DIAERESIS:
        case UTILS_CHAR_LATIN_SMALL_A_WITH_RING_ABOVE:
            return "a";
            break;
        case UTILS_CHAR_LATIN_SMALL_AE:
            return "ae";
            break;
        case UTILS_CHAR_LATIN_SMALL_C_WITH_CEDILLA:
            return "c";
            break;
        case UTILS_CHAR_LATIN_SMALL_E_WITH_GRAVE:
        case UTILS_CHAR_LATIN_SMALL_E_WITH_ACUTE:
        case UTILS_CHAR_LATIN_SMALL_E_WITH_CIRCUMFLEX:
        case UTILS_CHAR_LATIN_SMALL_E_WITH_DIAERESIS:
            return "e";
            break;
        case UTILS_CHAR_LATIN_SMALL_I_WITH_GRAVE:
        case UTILS_CHAR_LATIN_SMALL_I_WITH_ACUTE:
        case UTILS_CHAR_LATIN_SMALL_I_WITH_CIRCUMFLEX:
        case UTILS_CHAR_LATIN_SMALL_I_WITH_DIAERESIS:
            return "i";
            break;
        case UTILS_CHAR_LATIN_SMALL_ETH:
            return "eth";
            break;
        case UTILS_CHAR_LATIN_SMALL_N_WITH_TILDE:
            return "n";
            break;
        case UTILS_CHAR_LATIN_SMALL_O_WITH_GRAVE:
        case UTILS_CHAR_LATIN_SMALL_O_WITH_ACUTE:
        case UTILS_CHAR_LATIN_SMALL_O_WITH_CIRCUMFLEX:
        case UTILS_CHAR_LATIN_SMALL_O_WITH_TILDE:
        case UTILS_CHAR_LATIN_SMALL_O_WITH_DIAERESIS:
        case UTILS_CHAR_LATIN_SMALL_O_WITH_STROKE:
            return "o";
            break;
        case UTILS_CHAR_DIVISION_SIGN:
            return "%";
            break;
        case UTILS_CHAR_LATIN_SMALL_U_WITH_GRAVE:
        case UTILS_CHAR_LATIN_SMALL_U_WITH_ACUTE:
        case UTILS_CHAR_LATIN_SMALL_U_WITH_CIRCUMFLEX:
        case UTILS_CHAR_LATIN_SMALL_U_WITH_DIAERESIS:
            return "u";
            break;
        case UTILS_CHAR_LATIN_SMALL_Y_WITH_ACUTE:
        case UTILS_CHAR_LATIN_SMALL_Y_WITH_DIAERESIS:
            return "y";
            break;
        case UTILS_CHAR_LATIN_SMALL_THORN:
            return "th";
            break;
        case UTILS_CHAR_LATIN_SMALL_CAPITAL_R:
            return "R";
            break;
        case UTILS_CHAR_CYRILLIC_BY_UA_CAPITAL_I:
        case UTILS_CHAR_CYRILLIC_CAPITAL_YI: {
            char s[] = { 'I', '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_A: {
            char s[] = { 192, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_BE: {
            char s[] = { 193, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_VE: {
            char s[] = { 194, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_GHE: {
            char s[] = { 195, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_DE: {
            char s[] = { 196, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_IO:
        case UTILS_CHAR_CYRILLIC_UA_CAPITAL_IE:
        case UTILS_CHAR_CYRILLIC_CAPITAL_YE: {
            char s[] = { 197, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_ZHE: {
            char s[] = { 198, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_ZE: {
            char s[] = { 199, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_I: {
            char s[] = { 200, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_SHORT_I: {
            char s[] = { 201, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_KA: {
            char s[] = { 202, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_EL: {
            char s[] = { 203, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_EM: {
            char s[] = { 204, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_EN: {
            char s[] = { 205, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_O: {
            char s[] = { 206, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_PE: {
            char s[] = { 207, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_ER: {
            char s[] = { 208, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_ES: {
            char s[] = { 209, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_TE: {
            char s[] = { 210, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_U:
        case UTILS_CHAR_CYRILLIC_CAPITAL_SHORT_U: {
            char s[] = { 211, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_EF: {
            char s[] = { 212, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_HA: {
            char s[] = { 213, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_TSE: {
            char s[] = { 214, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_CHE: {
            char s[] = { 215, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_SHA: {
            char s[] = { 216, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_SCHA: {
            char s[] = { 217, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_HARD_SIGN: {
            char s[] = { 218, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_YERU: {
            char s[] = { 219, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_SOFT_SIGN: {
            char s[] = { 220, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_E: {
            char s[] = { 221, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_YU: {
            char s[] = { 222, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_CAPITAL_YA: {
            char s[] = { 223, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_A: {
            char s[] = { 224, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_BE: {
            char s[] = { 225, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_VE: {
            char s[] = { 226, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_GHE: {
            char s[] = { 227, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_DE: {
            char s[] = { 228, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_IE:
        case UTILS_CHAR_CYRILLIC_SMALL_IO:
        case UTILS_CHAR_CYRILLIC_UA_SMALL_IE: {
            char s[] = { 229, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_ZHE: {
            char s[] = { 230, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_ZE: {
            char s[] = { 231, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_I: {
            char s[] = { 232, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_SHORT_I: {
            char s[] = { 233, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_KA: {
            char s[] = { 234, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_EL: {
            char s[] = { 235, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_EM: {
            char s[] = { 236, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_EN: {
            char s[] = { 237, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_O: {
            char s[] = { 238, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_PE: {
            char s[] = { 239, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_ER: {
            char s[] = { 240, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_ES: {
            char s[] = { 241, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_TE: {
            char s[] = { 242, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_U:
        case UTILS_CHAR_CYRILLIC_SMALL_SHORT_U: {
            char s[] = { 243, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_EF: {
            char s[] = { 244, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_HA: {
            char s[] = { 245, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_TSE: {
            char s[] = { 246, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_CHE: {
            char s[] = { 247, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_SHA: {
            char s[] = { 248, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_SHCHA: {
            char s[] = { 249, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_LEFT_HARD_SIGN: {
            char s[] = { 250, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_YERU: {
            char s[] = { 251, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_SOFT_SIGN: {
            char s[] = { 252, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_E: {
            char s[] = { 253, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_YU: {
            char s[] = { 254, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_SMALL_YA: {
            char s[] = { 255, '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_CYRILLIC_BY_UA_SMALL_I:
        case UTILS_CHAR_CYRILLIC_SMALL_YI: {
            char s[] = { 'i', '\0' };
            return s;
        }
            break;
        case UTILS_CHAR_LEFT_SINGLE_QUOTATION_MARK:
            return "'";
            break;
        case UTILS_CHAR_RIGHT_SINGLE_QUOTATION_MARK:
            return "'";
            break;
        default:
            return "";
            break;
    }
}
