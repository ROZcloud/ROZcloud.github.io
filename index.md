---
layout: home
title: Strona główna
---

# Zestaw do tworzenia systemów w kernel c
Ten zestaw wspiera interface TUI i autorską bibliotekę RozOS TUI Framework
![alt text](image.png)
I prosty System Shell
No Image File!
Opis w AI:
# RozOS (by ROZcloud)

Autorski system operacyjny pisany w języku C.

## Przegląd techniczny
System obsługuje niskopoziomowe operacje sprzętowe oraz posiada własny framework GUI.

### Główne funkcjonalności:
* **Zarządzanie I/O**: Bezpośredni dostęp do portów (`inb`, `outb`).
* **Zarządzanie energią**: Obsługa tablic ACPI (FADT).
* **GUI Framework**: Własne środowisko TUI z systemem okienkowym.
* **Interpreter**: Wbudowany `run_hex` do wykonywania kodu maszynowego.

## Przykład kodu (Komunikacja z portem)

Poniżej znajduje się implementacja odczytu słowa (16-bit) z portu:

```c
static inline uint16_t inw(unsigned short port) {
    uint16_t ret;
    asm volatile ( "inw %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}
```

