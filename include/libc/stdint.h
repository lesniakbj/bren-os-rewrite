// Kernel Types
#ifndef KERNEL_STD_INT_H
#define KERNEL_STD_INT_H

// Unsigned
typedef unsigned long  kuint64_t;
typedef unsigned int   kuint32_t;
typedef unsigned short kuint16_t;
typedef unsigned char  kuint8_t;

// Signed
typedef long kint64_t;
typedef int   kint32_t;
typedef short kint16_t;
typedef char  kint8_t;

// Size and Offset (pointer difference)
typedef unsigned int size_t;
typedef int ptrdiff_t;

// Bools
typedef int bool;
#define true 1
#define false 0

// Addresses
typedef kuint32_t physical_addr_t;
typedef kuint32_t virtual_addr_t;

// Null pointer
#define NULL ((void*)0)

#endif