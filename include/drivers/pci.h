#ifndef PCI_H
#define PCI_H

#include <libc/stdint.h>

void pci_init();
const char* pci_class_subclass_to_string(kuint8_t class_code, kuint8_t subclass);

#endif //PCI_H
