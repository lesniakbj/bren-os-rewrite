#include <libc/strings.h>

generic_ptr memset(generic_ptr bufptr, int value, size_t size) {
    kuint8_t* buf = (kuint8_t*) bufptr;
    for (size_t i = 0; i < size; i++) {
        buf[i] = (kuint8_t) value;
    }
    return bufptr;
}

size_t strlen(const char *str) {
    size_t len = 0;
    while(str[len] != '\0') {
        len++;
    }
    return len;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    // If n is 0, the strings are considered equal
    if (n == 0) {
        return 0;
    }

    // Loop through the characters until n characters are compared
    // or a null terminator is found in either string
    do {
        // If characters differ, return the difference
        if (*s1 != *s2) {
            return *(unsigned char *)s1 - *(unsigned char *)s2;
        }

        // If a null terminator is found in s1, and characters matched so far,
        // it means s1 is a prefix of s2 (or they are equal up to the null)
        if (*s1 == '\0') {
            break; // s1 ends here, and no difference found yet
        }

        // Move to the next characters
        s1++;
        s2++;
        n--;
    } while (n > 0); // Continue as long as n is greater than 0

    // If the loop finishes without finding a difference,
    // the strings are equal up to n characters or a null terminator
    return 0;
}