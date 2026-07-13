---
layout: default
title: O RozOS
---

# RozOS - System Operacyjny w C
<!-- BEGIN AADS AD UNIT 2447857 -->

<div id="frame" style="width: 100%;margin: auto;position: relative; z-index: 99998;">
  <iframe data-aa='2447857' src='//acceptable.a-ads.com/2447857/?size=Adaptive'
                    style='border:0; padding:0; width:70%; height:auto; overflow:hidde
                display: block;margin: auto'></iframe>
</div>

<!-- END AADS AD UNIT 2447857 -->
**RozOS** to autorski projekt systemu operacyjnego tworzonego od podstaw przez **ROZcloud**. Projekt powstał z chęci zrozumienia niskopoziomowej komunikacji ze sprzętem, zarządzania pamięcią oraz budowania własnego środowiska graficznego w trybie tekstowym (TUI).

## Co wyróżnia RozOS?

W przeciwieństwie do wielu edukacyjnych projektów typu "kernel", RozOS stawia na własny framework GUI, który zapewnia:
* **Płynność**: Dzięki zastosowaniu techniki *shadow matrix*, odświeżanie okienek odbywa się bez efektu migotania.
* **Elastyczność**: Wbudowany interpreter komend HEX pozwala na wstrzykiwanie kodu bezpośrednio do pamięci systemu w czasie rzeczywistym.
* **Kontrolę**: Pełna obsługa ACPI pozwala na zarządzanie energią, co jest rzadkością w projektach tego typu.

## Architektura systemu


Projekt jest rozwijany w języku **C** z użyciem wstawek asemblerowych, co zapewnia maksymalną wydajność przy bezpośrednim dostępie do portów I/O oraz pamięci wideo VGA.

## Roadmapa projektu
- [x] Podstawowy kernel i obsługa przerwań
- [x] Framework TUI i obsługa klawiatury
- [ ] Implementacja prostego systemu plików
- [ ] Zarządzanie pamięcią dynamiczną (malloc)

## Wesprzyj projekt
Jeśli podoba Ci się kierunek, w którym zmierza RozOS, możesz śledzić postępy w repozytorium na GitHubie. RozOS jest udostępniony na licencji **MIT**, więc każdy może analizować i modyfikować kod.

> "Wszystko, co techniczne, powinno być dobrze udokumentowane." – ROZcloud
