/*
 * libk/udivdi3.c
 *
 * Implementation of the __udivdi3 function for 64-bit unsigned division.
 * This is required by GCC when performing 64-bit division operations
 * in freestanding environments where the full libgcc runtime is not available.
 *
 * This implementation is based on the algorithm described in
 * "Hacker's Delight" by Henry S. Warren.
 */

// Ensure this file is compiled as part of the kernel build.
// It should be added to the Makefile's source list.

// Function prototype for the 64-bit unsigned division helper.
// GCC will call this for operations like (uint64_t a) / (uint64_t b)
unsigned long long __udivdi3(unsigned long long dividend, unsigned long long divisor);

// Function prototype for the 64-bit unsigned modulo helper.
// GCC will call this for operations like (uint64_t a) % (uint64_t b)
unsigned long long __umoddi3(unsigned long long dividend, unsigned long long divisor);

// Helper function for unsigned 64-bit division and modulo.
// This performs both operations in one go for efficiency.
static void udiv_mod_di(unsigned long long dividend, unsigned long long divisor,
                        unsigned long long *quotient, unsigned long long *remainder) {
    // Handle division by zero
    if (divisor == 0) {
        // In standard environments, this would raise SIGFPE.
        // In a kernel, we should probably panic or handle it gracefully.
        // For now, we return 0 for both quotient and remainder.
        // A more robust kernel might have a dedicated division by zero handler.
        if (quotient) *quotient = 0;
        if (remainder) *remainder = 0;
        return;
    }

    // Handle cases where dividend is zero
    if (dividend == 0) {
        if (quotient) *quotient = 0;
        if (remainder) *remainder = 0;
        return;
    }

    // Handle cases where divisor is 1
    if (divisor == 1) {
        if (quotient) *quotient = dividend;
        if (remainder) *remainder = 0;
        return;
    }

    // Handle cases where dividend is less than divisor
    if (dividend < divisor) {
        if (quotient) *quotient = 0;
        if (remainder) *remainder = dividend;
        return;
    }

    // Handle cases where dividend equals divisor
    if (dividend == divisor) {
        if (quotient) *quotient = 1;
        if (remainder) *remainder = 0;
        return;
    }

    // --- Long division algorithm ---
    // This is a bit-by-bit long division algorithm.
    unsigned long long q = 0; // Quotient
    unsigned long long r = 0; // Remainder
    unsigned long long bit = 1; // Bit to set in quotient

    // Left-align the divisor for the long division process
    // Find the highest bit set in dividend and align divisor accordingly
    // This is a simple way to find the number of leading zeros and shift.
    // A more optimized version might use built-in functions like __builtin_clzll
    // if available and efficient on the target architecture.
    unsigned long long d = divisor;
    while (d < dividend) {
        d <<= 1;
        bit <<= 1;
    }

    // Perform the long division
    while (bit > 0) {
        if (dividend >= d) {
            dividend -= d;
            q |= bit;
        }
        d >>= 1;
        bit >>= 1;
    }

    r = dividend; // The final dividend is the remainder

    if (quotient) *quotient = q;
    if (remainder) *remainder = r;
}

// GCC entry point for 64-bit unsigned division
unsigned long long __udivdi3(unsigned long long dividend, unsigned long long divisor) {
    unsigned long long quotient;
    udiv_mod_di(dividend, divisor, &quotient, (unsigned long long *)0);
    return quotient;
}

// GCC entry point for 64-bit unsigned modulo
unsigned long long __umoddi3(unsigned long long dividend, unsigned long long divisor) {
    unsigned long long remainder;
    udiv_mod_di(dividend, divisor, (unsigned long long *)0, &remainder);
    return remainder;
}