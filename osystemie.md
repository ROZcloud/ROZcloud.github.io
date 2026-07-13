---
layout: default
title: O RozOS
---

# RozOS - System Operacyjny w C

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
<div style="position: absolute; z-index: 99999">
    <input autocomplete="off" type="checkbox" id="aadsstickymrjelpt8" hidden />
    <div style="padding-top: auto; padding-bottom: 0;">
        <div style="width:100%;height:auto;position:fixed;text-align:center;font-size:0;top:0;left:0;right:0;margin:auto">
            <label for="aadsstickymrjelpt8" style="top: 50%;transform: translateY(-50%);right:24px; position: absolute;border-radius: 4px; background: rgba(248, 248, 249, 0.70); padding: 4px;z-index: 99999;cursor:pointer">
                <svg fill="#000000" height="16px" width="16px" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 490 490">
                    <polygon points="456.851,0 245,212.564 33.149,0 0.708,32.337 212.669,245.004 0.708,457.678 33.149,490 245,277.443 456.851,490 489.292,457.678 277.331,245.004 489.292,32.337 "/>
                </svg>
            </label>
            <div id="frame" style="width: 100%;margin: auto;position: relative; z-index: 99998;"><iframe data-aa=2447857 src=//acceptable.a-ads.com/2447857/?size=Adaptive style='border:0; padding:0; width:70%; height:auto; overflow:hidden; margin: auto'></iframe></div>
        </div>
        <style>
            #aadsstickymrjelpt8:checked + div { display: none; }
        </style>
    </div>
</div>