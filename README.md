# Roz's official system development toolkit
### The set is made for LINUX, you can use it on Windows using WSL
[Windows Setup](#windows)
 
[Linux Setup](#linux) 
# Setup

## <a name="windows"><a>if you have Windows start here:
## On windows install wsl
```bash
wsl --install
```
### A restart will be required
### After restart enter in cmd:
```bash
wsl
```
### Fill in the login and password fields
### Next
# <a name="linux"><a>If you have windows with wsl or linux continue:

### Enter in wsl linux or linux command:
```bash
git clone https://github.com/ROZcloud/RozOS-system-creator-set.git
```
### Go to the downloaded folder with the command:
```bash
cd RozOS-system-creator-set
cd RozOS
```
### Next edit kernel.c
### Run 
```bash
make install
```
### once to install the required packages
 
### After editing, to get the iso, execute the command:
```bash
make build
```
### An iso file will be created, to run it, execute:
```bash
make run-build
```
### To run without building an iso do:
```bash
make run
```
### this will run ```kernel.c```
