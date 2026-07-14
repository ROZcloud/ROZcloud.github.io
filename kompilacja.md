---
layout: default
title: Kompilacja
---

# Kompilacja systemu KevosOS
Aby wykonać kompilacje KevosOS sklonuj repozytorium
```bash
git clone https://github.com/ROZcloud/KevosOS-system-creator-set.git && cd KevosOS-system-creator-set
```
Opcjonalnie: Edytuj dowolnie kernel.c, aby otworzyć edytor nano wpisz:
```
nano kernel.c
```
System jest gotowy wykonaj jednorazowo aby zainstalować wymagane pakiety:
```bash
make install-deps
```
Aby skompilować do .bin wpisz:
```bash
make
```
Plik .bin jest gotowy ale aby zrobić ISO z wbudowanym grub wpisz:
```bash
make create
```