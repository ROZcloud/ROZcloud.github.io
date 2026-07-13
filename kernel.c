// MIT License
// 
// Copyright (c) 2026 ROZcloud
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <stdint.h>
#include <stddef.h>
// RozOS main library
//
// Made by:
// Szymon Wolak (github:ROZcloud)
// Note:
// None
char adres_hex[100];
static inline uint16_t inw(unsigned short port) {
    uint16_t ret;
    asm volatile ( "inw %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

#define CHAR_TL 218 // ┌ - Górny lewy róg
#define CHAR_TR 191 // ┐ - Górny prawy róg
#define CHAR_BL 192 // └ - Dolny lewy róg
#define CHAR_BR 217 // ┘ - Dolny prawy róg
#define CHAR_HL 196 // ─ - Linia pozioma
#define CHAR_VL 179 // │ - Linia pionowa


// Funkcje komunikacji z portami I/O (dla klawiatury)
static inline void outb(unsigned short port, unsigned char val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}
static inline void outw(unsigned short port, unsigned short val) {
    asm volatile ( "outw %0, %1" : : "a"(val), "Nd"(port) );
}
static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}


// Multiboot
struct multiboot_info {
    unsigned int flags;
    unsigned int mem_lower;
    unsigned int mem_upper;
    unsigned int boot_device;
    unsigned int cmdline;
};
static int shift_pressed = 0;
int str_contains(const char* str, const char* sub) {
    if (!str || !sub) return 0;
    while (*str) {
        const char* h = str;
        const char* n = sub;
        while (*h && *n && *h == *n) {
            h++;
            n++;
        }
        if (!*n) return 1;
        str++;
    }
    return 0;
}



// Czyszczenie ekranu (szare litery na czarnym tle)
void clear() {
    volatile char* video_memory = (char*) 0xB8000;
    for(int i = 0; i < 80 * 25 * 2; i += 2) {
        video_memory[i] = ' ';
        video_memory[i+1] = 0x07;
    }
}

// Wypisywanie tekstu w konkretnym miejscu
void print(const char* str, int line, int col, char color) {
    volatile char* video_memory = (char*) 0xB8000;
    int offset = (line * 80 + col) * 2;
    for(int i = 0; str[i] != '\0'; i++) {
        video_memory[offset + i*2] = str[i];
        video_memory[offset + i*2 + 1] = color;
    }
}

// Pełna tablica skankodów klawiatury (małe litery)
char get_ascii(unsigned char scancode) {
    static const char kbd_map[] = {
        0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
        '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
    };
    if (scancode < 58) return kbd_map[scancode];
    return 0;
}
void reboot() {
    outb(0xCF9, 0x02);
    outb(0xCF9, 0x06);
    unsigned char good = 0x02;
    for (int i = 0; i < 1000; i++) { 
        good = inb(0x64);
        if ((good & 0x02) == 0) break;
    }
    outb(0x64, 0xFE);
    volatile uint16_t idt_empty[3] = {0, 0, 0};
    asm volatile ("lidt (%0)" : : "r" (idt_empty));
    asm volatile ("int $3");
    asm volatile ("cli");
    while(1) { asm volatile ("hlt"); }
}

void input(char* buffer, int line, int col, int limit) { // Dodaliśmy parametr limit
    int i = 0;
    while(1) {
        if(inb(0x64) & 1) {
            unsigned char scancode = inb(0x60);
            if(scancode & 0x80) continue;

            char c = get_ascii(scancode);
            
            if(c == '\n') {
                buffer[i] = '\0';
                break;
            } 
            else if(c == '\b' && i > 0) {
                i--;
                buffer[i] = '\0';
                print(" ", line, col + i, 0x07);
            }
            // Teraz sprawdzamy przekazany limit zamiast sztywnej piętnastki
            else if(c != 0 && i < limit) { 
                buffer[i] = c;
                char tmp[2] = {c, '\0'};
                print(tmp, line, col + i, 0x0F);
                i++;
            }
        }
    }
}
// Porównywanie tekstów
int strcmp(const char* s1, const char* s2) {
    while(*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}
void run_hex(char* input) {
    static unsigned char bin[128];
    int p = 0;
    int i = 0;

    while (input[i] != '\0' && p < 127) {
        if (input[i] == ' ') {
            i++;
            continue;
        }

        unsigned char byte = 0;
        for (int j = 0; j < 2; j++) {
            char c = input[i + j];
            byte <<= 4;
            if (c >= '0' && c <= '9') byte += (c - '0');
            else if (c >= 'a' && c <= 'f') byte += (c - 'a' + 10);
        }
        
        bin[p++] = byte;
        i += 2;
    }

    if (p > 0 && bin[p - 1] != 0xc3) {
        bin[p++] = 0xc3;
    }

    ((void (*)())bin)();
}

void delay(int count) {
    for(volatile int i = 0; i < count * 1000000; i++);
}
char* random() {
    static char buf[2];
    unsigned int lo;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo)); 
    buf[0] = (lo % 10) + '0'; 
    buf[1] = '\0';
    
    return buf;
}
// RozOS Main
// Made by:
// Szymon Wolak (github:ROZcloud, email:aza756903@gmail.com)
//
// Note:
// [X] Popraw delay z 3000 na 1000
// [NOTE] Pamiętaj delay 3000 nie działa i jest ignorowane

struct RSDPDescriptor {
    char Signature[8];
    uint8_t Checksum;
    char OEMID[6];
    uint8_t Revision;
    uint32_t RsdtAddress;
} __attribute__((packed));

struct ACPISDTHeader {
    char Signature[4];
    uint32_t Length;
    uint8_t Revision;
    uint8_t Checksum;
    char OEMID[6];
    char OEMTableID[8];
    uint32_t OEMRevision;
    uint32_t CreatorID;
    uint32_t CreatorRevision;
};

struct FADT {
    struct ACPISDTHeader h;
    uint32_t FirmwareCtrl;
    uint32_t Dsdt;
    uint8_t  Reserved;
    uint8_t  PreferredPowerManagementProfile;
    uint16_t SciInterrupt;
    uint32_t SMI_CommandPort;
    uint8_t  AcpiEnable;
    uint8_t  AcpiDisable;
    uint8_t  S4Bios_Req;
    uint8_t  PSTATE_Control;
    uint32_t PM1a_Event_Block;
    uint32_t PM1b_Event_Block;
    uint32_t PM1a_Control_Block;
    uint32_t PM1b_Control_Block;
} __attribute__((packed));

// 2. Funkcja pomocnicza: porównywanie fragmentów pamięci
int memcmp_local(const void* s1, const void* s2, int n) {
    const unsigned char *p1 = s1, *p2 = s2;
    while(n--) if(*p1++ != *p2++) return 1;
    return 0;
}

struct FADT* find_fadt() {
    for (uint32_t addr = 0xE0000; addr < 0x100000; addr += 16) {
        if (memcmp_local((void*)addr, "RSD PTR ", 8) == 0) {
            struct RSDPDescriptor* rsdp = (struct RSDPDescriptor*)addr;
            struct ACPISDTHeader* rsdt = (struct ACPISDTHeader*)rsdp->RsdtAddress;
            int entries = (rsdt->Length - sizeof(struct ACPISDTHeader)) / 4;
            uint32_t* table_ptr = (uint32_t*)(rsdp->RsdtAddress + sizeof(struct ACPISDTHeader));
            
            for (int i = 0; i < entries; i++) {
                struct ACPISDTHeader* h = (struct ACPISDTHeader*)table_ptr[i];
                if (memcmp_local(h->Signature, "FACP", 4) == 0) {
                    return (struct FADT*)h;
                }
            }
        }
    }
    return 0;
}

// 4. Funkcja przejmująca kontrolę (OS Owned)
void acpi_enable(struct FADT* fadt) {
    if (fadt && fadt->SMI_CommandPort != 0) {
        outb(fadt->SMI_CommandPort, fadt->AcpiEnable);
        while ((inw(fadt->PM1a_Control_Block) & 1) == 0);
    }
}

int wait_for_power(struct FADT* fadt) {
    if (!fadt) return 0;
    uint16_t status = inw(fadt->PM1a_Event_Block);

    if (status & (1 << 8)) {
        outw(fadt->PM1a_Event_Block, (1 << 8));
        return 1;
    }

    return 0;
}
struct FADT* fadt = 0;
void dev() {
    while(1) {

        clear();
        int i = 1;
        while(i) {
        if (wait_for_power(fadt)) {
            print("Press developer key", 0, 0, 0x0F);
            i=0;
        }
        }
        clear();
        print("Developer shell", 0, 0, 0x0C);
        print("Dev>>", 1, 0, 0x0F);
        char dshell[16] = {0};
        input(dshell, 1, 6, 0x0F);
        if(strcmp(dshell, "acpi") == 0) {
            struct FADT* fadt = find_fadt();
        if(fadt) {
            acpi_enable(fadt);
        }
        }
    }
}

int boot_param(struct multiboot_info* mbi, const char* param_name) {
    if (mbi && (mbi->flags & (1 << 2))) {
        const char* kernel_arguments = (const char*)mbi->cmdline;
        if (str_contains(kernel_arguments, param_name)) {
            return 1;
        }
    }
    return 0;
}

void acpi_disable(struct FADT* fadt) {
    if (fadt && fadt->SMI_CommandPort != 0) {
        outb(fadt->SMI_CommandPort, fadt->AcpiDisable);
        while ((inw(fadt->PM1a_Control_Block) & 1) != 0);
    }
}


// RozOS GUI framework
/* RozOS GUI Framework
Version: 7.0
*/
#define SCR_W 80
#define SCR_H 25
#define MAX_COMP 16
#define MAX_WIN 10
#define MAX_TERMINAL_CMDS 12

#define MAX_MENU_ITEMS 5
#define MAX_SUB_ITEMS 6

#define TERM_MAX_LINES 32
#define TERM_LINE_WIDTH 50

/* Klasyczna Retro Paleta Blue (Norton Commander Style) */
#define COL_DESKTOP     0x1F
#define COL_MENU_BAR    0x70  /* Czarny tekst na szarym tle */
#define COL_MENU_SEL    0x4F  /* Biały tekst na czerwonym tle (aktywne menu) */
#define COL_WIN_ACT     0x1F  
#define COL_WIN_PAS     0x70  
#define COL_HDR_ACT     0x3F  
#define COL_HDR_PAS     0x78  
#define COL_COMP_FOCUS  0x4F  
#define COL_INPUT_BOX   0x0F  
#define COL_TERM_BG     0x07  
#define COL_SHADOW      0x08  

struct Component;
struct Window;

typedef void (*TUI_EventHandler)(struct Window* win, struct Component* comp);
typedef void (*TUI_ConfirmHandler)(int result_yes);
typedef void (*TUI_CmdHandler)(int win_id, int term_idx);
typedef void (*TUI_MenuHandler)(void); 
typedef void (*TUI_FKeyHandler)(void);

typedef enum { COMP_TEXT, COMP_BUTTON, COMP_PROGRESS, COMP_INPUT, COMP_TERMINAL_APP } CompType;

struct Component {
    CompType type;
    const char* text;
    int rel_x, rel_y, w, h, value;
    char* buffer;         
    int buffer_len, curr_chars;    
    char term_history[TERM_MAX_LINES][TERM_LINE_WIDTH];
    int term_head;
    const char* term_prompt;
    TUI_EventHandler on_action;
};

struct Window {
    int id, x, y, w, h;
    const char* title;
    const char* help_text;
    int is_active, current_focus_comp, scroll_y, virtual_h, child_count, next_item_y, is_modal;
    int is_permanent; 
    TUI_ConfirmHandler confirm_callback;

    TUI_FKeyHandler f_handlers[13];

    struct Component children[MAX_COMP]; 
};

struct TerminalCommand { const char* name; TUI_CmdHandler handler; };

struct SubMenuItem {
    const char* name;
    TUI_MenuHandler handler;
};

struct MainMenuItem {
    const char* name;
    int x_pos;
    struct SubMenuItem sub_items[MAX_SUB_ITEMS];
    int sub_count;
};

static unsigned short vga_shadow_matrix[SCR_W * SCR_H];
static unsigned short* vga_hardware_mem = (unsigned short*)0xB8000;
static struct Window windows_pool[MAX_WIN];
static struct Window* rendering_layers[MAX_WIN];
static int total_windows = 0;
static struct TerminalCommand registered_cmds[MAX_TERMINAL_CMDS];
static int total_registered_cmds = 0;

static struct MainMenuItem menu_bar[MAX_MENU_ITEMS];
static int total_menu_items = 0;
static int menu_active = 0;         
static int menu_selected_main = 0;  
static int menu_dropdown_open = 0;  
static int menu_selected_sub = 0;   

static const char* global_custom_help_text = " F1 Pomoc  F10 Menu  Alt+X Zamknij  Alt+WSAD Ruch Okna  Alt+TAB Przelacz";

/* Zmienne globalne wymagane przez demonstracyjne kernel_main */
static int win_glowny;
static int win_statystyki;
static char command_input[16];

/* --- PODSTAWOWE FUNKCJE POMOCNICZE C --- */
static int core_strlen(const char* s) { int l = 0; while(s[l]) l++; return l; }
static void core_poke(int x, int y, char attr, unsigned char ch) { if (x >= 0 && x < SCR_W && y >= 0 && y < SCR_H) vga_shadow_matrix[y * SCR_W + x] = (attr << 8) | ch; }
static int core_strcmp(const char* s1, const char* s2) { while (*s1 && (*s1 == *s2)) { s1++; s2++; } return *(unsigned char*)s1 - *(unsigned char*)s2; }

static void core_layer_bring_to_front(int idx) {
    if (idx < 0 || idx >= total_windows || (rendering_layers[total_windows-1]->is_modal && total_windows > 0)) return;
    struct Window* target = rendering_layers[idx];
    for (int i = idx; i < total_windows - 1; i++) rendering_layers[i] = rendering_layers[i + 1];
    rendering_layers[total_windows - 1] = target;
    for (int i = 0; i < total_windows; i++) rendering_layers[i]->is_active = (i == total_windows - 1);
}

/* --- RENDERER SYSTEMOWY --- */

void tui_refresh_screen() {
    // =========================================================================
    // KROK 1: KROKOWANE TŁO EKRANU (PULPIT)
    // =========================================================================
    for (int i = 0; i < SCR_W * SCR_H; i++) {
        vga_shadow_matrix[i] = (COL_DESKTOP << 8) | 32; // Kropki = 176 Spacja = 32
    }
    
    // =========================================================================
    // KROK 2: GÓRNY PASEK MENU (MENU BAR)
    // =========================================================================
    for (int c = 0; c < SCR_W; c++) {
        vga_shadow_matrix[0 * SCR_W + c] = (COL_MENU_BAR << 8) | ' ';
    }
    for (int i = 0; i < total_menu_items; i++) {
        char attr = (menu_active && menu_selected_main == i) ? COL_MENU_SEL : COL_MENU_BAR;
        int xp = menu_bar[i].x_pos;
        
        vga_shadow_matrix[0 * SCR_W + xp] = (attr << 8) | ' ';
        int j = 0;
        for (; menu_bar[i].name[j]; j++) {
            vga_shadow_matrix[0 * SCR_W + (xp + 1 + j)] = (attr << 8) | menu_bar[i].name[j];
        }
        vga_shadow_matrix[0 * SCR_W + (xp + 1 + j)] = (attr << 8) | ' ';
    }

    // =========================================================================
    // KROK 3: DOLNY PASEK STATUSU / POMOCY
    // =========================================================================
    const char* help_to_display = global_custom_help_text;
    if (total_windows > 0) {
        struct Window* active_win = rendering_layers[total_windows - 1];
        if (active_win->help_text != 0) {
            help_to_display = active_win->help_text;
        }
    }

    int row_offset = (SCR_H - 1) * SCR_W;
    for (int c = 0; c < SCR_W; c++) {
        vga_shadow_matrix[row_offset + c] = (COL_MENU_BAR << 8) | ' ';
    }
    for (int i = 0; help_to_display[i] != 0 && i < SCR_W; i++) {
        vga_shadow_matrix[row_offset + i] = (COL_MENU_BAR << 8) | help_to_display[i];
    }

    // =========================================================================
    // KROK 4: RYSOWANIE OKIEN I ELEMENTÓW
    // =========================================================================
    for (int z = 0; z < total_windows; z++) {
        struct Window* win = rendering_layers[z];
        char w_col = win->is_active ? COL_WIN_ACT : COL_WIN_PAS;
        char h_col = win->is_active ? COL_HDR_ACT : COL_HDR_PAS;

        for (int l = 1; l <= win->h; l++) {
            int sx1 = win->x + win->w, sx2 = win->x + win->w + 1, sy = win->y + l;
            if (sy < SCR_H - 1) {
                if (sx1 < SCR_W) vga_shadow_matrix[sy * SCR_W + sx1] = (COL_SHADOW << 8) | (vga_shadow_matrix[sy * SCR_W + sx1] & 0x00FF);
                if (sx2 < SCR_W) vga_shadow_matrix[sy * SCR_W + sx2] = (COL_SHADOW << 8) | (vga_shadow_matrix[sy * SCR_W + sx2] & 0x00FF);
            }
        }
        for (int c = 2; c < win->w + 2; c++) {
            int sx = win->x + c, sy = win->y + win->h;
            if (sx < SCR_W && sy < SCR_H - 1) vga_shadow_matrix[sy * SCR_W + sx] = (COL_SHADOW << 8) | (vga_shadow_matrix[sy * SCR_W + sx] & 0x00FF);
        }

        for (int l = 0; l < win->h; l++) {
            for (int c = 0; c < win->w; c++) {
                unsigned char ch = ' ';
                if (l == 0 || l == win->h - 1) ch = 205; 
                else if (c == 0 || c == win->w - 1) ch = 186;
                if (l == 0 && c == 0) ch = 201; 
                if (l == 0 && c == win->w - 1) ch = 187;
                if (l == win->h - 1 && c == 0) ch = 200; 
                if (l == win->h - 1 && c == win->w - 1) ch = 188;
                
                int target_x = win->x + c;
                int target_y = win->y + l;
                if (target_x >= 0 && target_x < SCR_W && target_y >= 0 && target_y < SCR_H) {
                    vga_shadow_matrix[target_y * SCR_W + target_x] = (w_col << 8) | ch;
                }
            }
        }
        
        for (int c = 1; c < win->w - 1; c++) {
            vga_shadow_matrix[(win->y + 1) * SCR_W + (win->x + c)] = (h_col << 8) | ' ';
        }
        
        vga_shadow_matrix[(win->y + 1) * SCR_W + (win->x + 2)] = (h_col << 8) | '[';
        if (win->is_permanent) {
            vga_shadow_matrix[(win->y + 1) * SCR_W + (win->x + 3)] = (h_col << 8) | '-';
        } else {
            vga_shadow_matrix[(win->y + 1) * SCR_W + (win->x + 3)] = (0x4F << 8) | 'X';
        }
        vga_shadow_matrix[(win->y + 1) * SCR_W + (win->x + 4)] = (h_col << 8) | ']';

        int tl = core_strlen(win->title); 
        int t_start = win->x + ((win->w - tl) / 2);
        for (int i = 0; i < tl && (t_start + i) < (win->x + win->w - 2); i++) {
            vga_shadow_matrix[(win->y + 1) * SCR_W + (t_start + i)] = (h_col << 8) | win->title[i];
        }

        for (int i = 0; i < win->child_count; i++) {
            struct Component* comp = &win->children[i];
            int scrolled_rel_y = comp->rel_y - win->scroll_y;
            if (scrolled_rel_y < 2 || scrolled_rel_y >= win->h - 1) continue;
            
            int cx = win->x + comp->rel_x; 
            int cy = win->y + scrolled_rel_y;
            int has_focus = (!menu_active && win->is_active && win->current_focus_comp == i);
            char dynamic_attr = has_focus ? COL_COMP_FOCUS : w_col;

            if (cx < 0 || cx >= SCR_W || cy < 0 || cy >= SCR_H) continue;

            if (comp->type == COMP_TEXT) {
                for (int j = 0; comp->text[j] && (cx + j) < SCR_W; j++) {
                    vga_shadow_matrix[cy * SCR_W + (cx + j)] = (w_col << 8) | comp->text[j];
                }
            } else if (comp->type == COMP_BUTTON) {
                int len = 0;
                while(comp->text[len] != '\0') len++;
                
                vga_shadow_matrix[cy * SCR_W + cx] = (dynamic_attr << 8) | CHAR_TL;
                for(int j = 0; j < len + 2; j++) {
                    vga_shadow_matrix[cy * SCR_W + (cx + 1 + j)] = (dynamic_attr << 8) | CHAR_HL;
                }
                vga_shadow_matrix[cy * SCR_W + (cx + len + 3)] = (dynamic_attr << 8) | CHAR_TR;
                
                vga_shadow_matrix[(cy + 1) * SCR_W + cx] = (dynamic_attr << 8) | CHAR_VL;
                for(int j = 0; j < len; j++) {
                    vga_shadow_matrix[(cy + 1) * SCR_W + (cx + 2 + j)] = (dynamic_attr << 8) | comp->text[j];
                }
                vga_shadow_matrix[(cy + 1) * SCR_W + (cx + len + 3)] = (dynamic_attr << 8) | CHAR_VL;
                
                vga_shadow_matrix[(cy + 2) * SCR_W + cx] = (dynamic_attr << 8) | CHAR_BL;
                for(int j = 0; j < len + 2; j++) {
                    vga_shadow_matrix[(cy + 2) * SCR_W + (cx + 1 + j)] = (dynamic_attr << 8) | CHAR_HL;
                }
                vga_shadow_matrix[(cy + 2) * SCR_W + (cx + len + 3)] = (dynamic_attr << 8) | CHAR_BR;
                
            } else if (comp->type == COMP_INPUT) {
                int lbl_l = core_strlen(comp->text);
                // Rysowanie etykiety
                for (int j = 0; j < lbl_l && (cx + j) < SCR_W; j++) {
                    vga_shadow_matrix[cy * SCR_W + (cx + j)] = (w_col << 8) | comp->text[j];
                }

                char input_attr = has_focus ? 0x4F : COL_INPUT_BOX;

                // Rysowanie pola tekstowego (używa teraz zaktualizowanego comp->w)
                for (int b = 0; b < comp->w && (cx + lbl_l + b) < SCR_W; b++) {
                    // Używamy bufora tylko do długości comp->buffer_len
                    unsigned char ch = (comp->buffer[b] != '\0') ? comp->buffer[b] : ' ';
                    vga_shadow_matrix[cy * SCR_W + (cx + lbl_l + b)] = (input_attr << 8) | ch;
                }
            } else if (comp->type == COMP_TERMINAL_APP) {
                for (int th = 0; th < comp->h && (cy + th) < SCR_H; th++) {
                    for (int tw = 0; tw < comp->w && (cx + tw) < SCR_W; tw++) {
                        vga_shadow_matrix[(cy + th) * SCR_W + (cx + tw)] = (COL_TERM_BG << 8) | ' ';
                    }
                }
                int print_row = 0;
                for (int line_idx = 0; line_idx < comp->term_head && print_row < comp->h; line_idx++) {
                    char* line_text = comp->term_history[line_idx];
                    for (int char_idx = 0; line_text[char_idx] != '\0' && (cx + char_idx) < SCR_W; char_idx++) {
                        if ((cy + print_row) < SCR_H) {
                            vga_shadow_matrix[(cy + print_row) * SCR_W + (cx + char_idx)] = (0x0A << 8) | line_text[char_idx];
                        }
                    }
                    print_row++;
                }
            }
        }
    }

    if (menu_active && menu_dropdown_open) {
        struct MainMenuItem* main_m = &menu_bar[menu_selected_main];
        int m_x = main_m->x_pos;
        int m_w = 18; 
        int m_h = main_m->sub_count + 2;

        for (int l = 0; l < m_h; l++) {
            for (int c = 0; c < m_w; c++) {
                char attr = COL_MENU_BAR;
                unsigned char ch = ' ';
                if (l == 0 || l == m_h - 1) ch = 196; else if (c == 0 || c == m_w - 1) ch = 179;
                if (l == 0 && c == 0) { ch = 218; } 
                if (l == 0 && c == m_w - 1) { ch = 191; }
                if (l == m_h - 1 && c == 0) { ch = 192; } 
                if (l == m_h - 1 && c == m_w - 1) { ch = 193; }
                
                if ((m_x + c) < SCR_W && (1 + l) < SCR_H) {
                    vga_shadow_matrix[(1 + l) * SCR_W + (m_x + c)] = (attr << 8) | ch;
                }
            }
        }
        for (int i = 0; i < main_m->sub_count; i++) {
            char item_attr = (menu_selected_sub == i) ? COL_MENU_SEL : COL_MENU_BAR;
            for (int c = 1; c < m_w - 1; c++) {
                if ((m_x + c) < SCR_W && (2 + i) < SCR_H) {
                    vga_shadow_matrix[(2 + i) * SCR_W + (m_x + c)] = (item_attr << 8) | ' ';
                }
            }
            const char* name = main_m->sub_items[i].name;
            for (int j = 0; name[j] && j < (m_w - 3); j++) {
                if ((m_x + 2 + j) < SCR_W && (2 + i) < SCR_H) {
                    vga_shadow_matrix[(2 + i) * SCR_W + (m_x + 2 + j)] = (item_attr << 8) | name[j];
                }
            }
        }
    }

    while ((inb(0x3DA) & 0x08));
    while (!(inb(0x3DA) & 0x08));

    for (int i = 0; i < SCR_W * SCR_H; i++) {
        vga_hardware_mem[i] = vga_shadow_matrix[i];
    }
}

/* --- MENU API --- */
int ui_menu_add_category(const char* name) {
    if (total_menu_items >= MAX_MENU_ITEMS) return -1;
    int prev_x = (total_menu_items == 0) ? 2 : menu_bar[total_menu_items - 1].x_pos + core_strlen(menu_bar[total_menu_items - 1].name) + 4;
    menu_bar[total_menu_items].name = name;
    menu_bar[total_menu_items].x_pos = prev_x;
    menu_bar[total_menu_items].sub_count = 0;
    return total_menu_items++;
}

void ui_menu_add_item(int cat_id, const char* name, TUI_MenuHandler handler) {
    if (cat_id < 0 || cat_id >= total_menu_items) return;
    struct MainMenuItem* m = &menu_bar[cat_id];
    if (m->sub_count < MAX_SUB_ITEMS) {
        m->sub_items[m->sub_count].name = name;
        m->sub_items[m->sub_count].handler = handler;
        m->sub_count++;
    }
}

void ui_set_bottom_help(const char* text) {
    global_custom_help_text = text;
}

/* --- INTERFACE API --- */
void tui_init_framework() { total_windows = 0; total_registered_cmds = 0; total_menu_items = 0; menu_active = 0; }
int tui_create_window(const char* title, int x, int y, int w, int h) {
    if (total_windows >= MAX_WIN) return -1;
    struct Window* win = &windows_pool[total_windows];
    win->id = total_windows; win->title = title; win->x = x; win->y = y; win->w = w; win->h = h;
    win->current_focus_comp = 0; win->scroll_y = 0; win->virtual_h = h; win->next_item_y = 3; win->child_count = 0; win->is_modal = 0; win->confirm_callback = 0;
    win->is_permanent = 0;

    for (int f = 1; f <= 12; f++) {
        win->f_handlers[f] = 0;
    } 

    rendering_layers[total_windows] = win; total_windows++; core_layer_bring_to_front(total_windows - 1);
    return win->id;
}

void ui_window_set_permanent(int win_id, int status) {
    if (win_id >= 0 && win_id < MAX_WIN) {
        windows_pool[win_id].is_permanent = status;
    }
}

int ui_register_f_key(int f_number, TUI_FKeyHandler handler) {
    if (f_number == 10 || f_number < 1 || f_number > 12) return 0; 
    if (total_windows == 0) return 0;
    struct Window* active_win = rendering_layers[total_windows - 1];
    active_win->f_handlers[f_number] = handler;
    return 1;
}
void ui_unregister_f_key(int f_number) {
    if (f_number == 10 || f_number < 1 || f_number > 12) return;
    if (total_windows == 0) return;

    struct Window* active_win = rendering_layers[total_windows - 1];
    active_win->f_handlers[f_number] = 0;
}

void ui_add_text(int win_id, const char* text) {
    struct Window* win = &windows_pool[win_id]; struct Component* c = &win->children[win->child_count++];
    c->type = COMP_TEXT; c->text = text; c->rel_x = 3; c->rel_y = win->next_item_y++;
}


int ui_add_button(int win_id, const char* label, TUI_EventHandler handler) {
    struct Window* win = &windows_pool[win_id]; 
    int idx = win->child_count; 
    struct Component* c = &win->children[win->child_count++];
    c->type = COMP_BUTTON; 
    c->text = label; 
    c->rel_x = 3; 
    c->rel_y = win->next_item_y; 
    c->on_action = handler;
    int len = 0;
    while(label[len] != '\0') len++;
    c->w = len + 4; 
    c->h = 3;
    win->next_item_y += 4;
    return idx;
}

int ui_add_progress(int win_id, int init_val) {
    struct Window* win = &windows_pool[win_id]; int idx = win->child_count; struct Component* c = &win->children[win->child_count++];
    c->type = COMP_PROGRESS; c->rel_x = 3; c->rel_y = win->next_item_y; c->w = win->w - 7; c->value = init_val;
    win->next_item_y += 2; return idx;
}

int ui_add_input(int win_id, const char* label, char* buf, int max_len) {
    struct Window* win = &windows_pool[win_id]; 
    int idx = win->child_count; 
    struct Component* c = &win->children[win->child_count++];
    
    c->type = COMP_INPUT; 
    c->text = label; 
    c->rel_x = 3; 
    c->rel_y = win->next_item_y;
    c->w = (max_len > 40) ? 40 : max_len; 
    
    c->buffer = buf; 
    c->buffer_len = max_len; 
    c->curr_chars = 0; 
    buf[0] = '\0';
    
    win->next_item_y += 2; 
    return idx;
}

int ui_add_terminal(int win_id, int height_lines) {
    struct Window* win = &windows_pool[win_id]; int idx = win->child_count; struct Component* c = &win->children[win->child_count++];
    c->type = COMP_TERMINAL_APP; c->rel_x = 3; c->rel_y = win->next_item_y; c->w = win->w - 7; c->h = height_lines; c->term_head = 0; c->term_prompt = "OS> ";
    win->next_item_y += (height_lines + 1); return idx;
}

void tui_append_terminal(int win_id, int comp_idx, const char* msg) {
    struct Component* c = &windows_pool[win_id].children[comp_idx]; if (c->type != COMP_TERMINAL_APP) return;
    int head = c->term_head; if (head >= TERM_MAX_LINES) { for (int i = 1; i < TERM_MAX_LINES; i++) { for (int j = 0; j < TERM_LINE_WIDTH; j++) c->term_history[i - 1][j] = c->term_history[i][j]; } head = TERM_MAX_LINES - 1; }
    int j = 0; while (msg[j] != '\0' && j < (TERM_LINE_WIDTH - 1)) { c->term_history[head][j] = msg[j]; j++; } c->term_history[head][j] = '\0'; c->term_head = head + 1;
}

void ui_terminal_set_prompt(int win_id, int term_idx, const char* prompt) { if (windows_pool[win_id].children[term_idx].type == COMP_TERMINAL_APP) windows_pool[win_id].children[term_idx].term_prompt = prompt; }
void ui_terminal_register_cmd(const char* name, TUI_CmdHandler handler) { if (total_registered_cmds < MAX_TERMINAL_CMDS) { registered_cmds[total_registered_cmds].name = name; registered_cmds[total_registered_cmds].handler = handler; total_registered_cmds++; } }

/* CONFIRM BOX */
static void confirm_yes_click(struct Window* win, struct Component* comp) { TUI_ConfirmHandler cb = win->confirm_callback; total_windows--; rendering_layers[total_windows-1]->is_modal = 0; if (cb) cb(1); tui_refresh_screen(); }
static void confirm_no_click(struct Window* win, struct Component* comp) { TUI_ConfirmHandler cb = win->confirm_callback; total_windows--; rendering_layers[total_windows-1]->is_modal = 0; if (cb) cb(0); tui_refresh_screen(); }

void ui_show_confirm(const char* title, const char* question, TUI_ConfirmHandler callback) {
    int cf = tui_create_window(title, 22, 9, 36, 8); windows_pool[cf].is_modal = 1; windows_pool[cf].confirm_callback = callback;
    ui_add_text(cf, question);
    struct Window* win = &windows_pool[cf];
    struct Component* b1 = &win->children[win->child_count++]; b1->type = COMP_BUTTON; b1->text = "TAK"; b1->rel_x = 6; b1->rel_y = 5; b1->on_action = confirm_yes_click;
    struct Component* b2 = &win->children[win->child_count++]; b2->type = COMP_BUTTON; b2->text = "NIE"; b2->rel_x = 20; b2->rel_y = 5; b2->on_action = confirm_no_click;
    tui_refresh_screen();
}

static char core_scancode_to_ascii(unsigned char scan, int shift) {
    static const char tui_kbd_map_low[] = {
        0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
        '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
    };

    static const char tui_kbd_map_high[] = {
        0, 27, '!', '@', '#', '$', '%', '^', '&', '(', ')', '_', '+', '\b',
        '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
        0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,
        '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' '
    };

    // Zwracamy indeks bezpośrednio (bez scan - 1)
    if (scan < 59) { 
        return shift ? tui_kbd_map_high[scan] : tui_kbd_map_low[scan];
    }
    return 0;
}

/* --- DISPATCHER KLAWIATURY --- */
void tui_dispatch_keyboard(unsigned char scancode) {
    static int alt_active = 0;
    if (scancode == 0x38) { alt_active = 1; return; } if (scancode == 0xB8) { alt_active = 0; return; }
    
    if (scancode == 0x44) {
        menu_active = !menu_active; menu_selected_main = 0; menu_dropdown_open = 0; menu_selected_sub = 0;
        tui_refresh_screen(); return;
    }

    if (scancode == 0x01 && menu_active) {
        menu_active = 0; menu_dropdown_open = 0; tui_refresh_screen(); return;
    }

    if (!menu_active && total_windows > 0) {
        struct Window* active_win = rendering_layers[total_windows - 1];
        
        if (scancode == 0x49) { // PAGE UP
            if (active_win->scroll_y > 0) {
                active_win->scroll_y--;
                tui_refresh_screen();
            }
            return;
        }
        
        if (scancode == 0x51) { // PAGE DOWN
            if (active_win->scroll_y < (active_win->virtual_h - active_win->h)) {
                active_win->scroll_y++;
                tui_refresh_screen();
            }
            return;
        }
    }

    if (menu_active) {
        if (scancode & 0x80) return;
        struct MainMenuItem* current = &menu_bar[menu_selected_main];

        if (!menu_dropdown_open) {
            if (scancode == 0x4B) { if (menu_selected_main > 0) menu_selected_main--; else menu_selected_main = total_menu_items - 1; tui_refresh_screen(); return; }
            if (scancode == 0x4D) { if (menu_selected_main < total_menu_items - 1) menu_selected_main++; else menu_selected_main = 0; tui_refresh_screen(); return; }
            if (scancode == 0x50 || scancode == 0x1C) { if (current->sub_count > 0) { menu_dropdown_open = 1; menu_selected_sub = 0; } tui_refresh_screen(); return; }
        } 
        else { 
            if (scancode == 0x48) { if (menu_selected_sub > 0) menu_selected_sub--; else menu_selected_sub = current->sub_count - 1; tui_refresh_screen(); return; }
            if (scancode == 0x50) { if (menu_selected_sub < current->sub_count - 1) menu_selected_sub++; else menu_selected_sub = 0; tui_refresh_screen(); return; }
            if (scancode == 0x4B) { menu_dropdown_open = 0; if (menu_selected_main > 0) menu_selected_main--; else menu_selected_main = total_menu_items - 1; tui_refresh_screen(); return; }
            if (scancode == 0x4D) { menu_dropdown_open = 0; if (menu_selected_main < total_menu_items - 1) menu_selected_main++; else menu_selected_main = 0; tui_refresh_screen(); return; }
            
            if (scancode == 0x1C) { 
                TUI_MenuHandler h = current->sub_items[menu_selected_sub].handler;
                menu_active = 0; menu_dropdown_open = 0; 
                if (h) h(); 
                tui_refresh_screen(); return;
            }
        }
        return;
    }

    if (alt_active && scancode == 0x2D) { 
        if (total_windows > 0 && !rendering_layers[total_windows - 1]->is_modal) { 
            if (!rendering_layers[total_windows - 1]->is_permanent) {
                total_windows--; 

                if (total_windows > 0) {
                    for (int i = 0; i < total_windows; i++) {
                        rendering_layers[i]->is_active = 0;
                    }
                    rendering_layers[total_windows - 1]->is_active = 1;
                }
            }
            tui_refresh_screen(); 
        } 
        return; 
    }
    
    if (scancode & 0x80) return;

    if (total_windows == 0) return; 
    struct Window* active_win = rendering_layers[total_windows - 1];

    int f_pressed = 0;
    if (scancode >= 0x3B && scancode <= 0x43) {
        f_pressed = scancode - 0x3B + 1;
    } else if (scancode == 0x57) {
        f_pressed = 11;
    } else if (scancode == 0x58) {
        f_pressed = 12;
    }

    if (f_pressed > 0 && f_pressed != 10) {
        if (active_win->f_handlers[f_pressed] != 0) {
            TUI_FKeyHandler handler = active_win->f_handlers[f_pressed];
            handler();
            tui_refresh_screen();
            return;
        }
    }

    if (alt_active) {
        if (scancode == 0x48) { if (active_win->y > 1) active_win->y--; tui_refresh_screen(); return; }
        if (scancode == 0x50) { if (active_win->y < 18) active_win->y++; tui_refresh_screen(); return; }
        if (scancode == 0x4B) { if (active_win->x > 0) active_win->x--; tui_refresh_screen(); return; }
        if (scancode == 0x4D) { if (active_win->x < 50) active_win->x++; tui_refresh_screen(); return; }
        if (!menu_active && total_windows > 0) {
            struct Window* active_win = rendering_layers[total_windows - 1];
            
            if (scancode == 0x49) { // PAGE UP
                if (active_win->scroll_y > 0) {
                    active_win->scroll_y--;
                    tui_refresh_screen();
                }
                return;
            }

            if (scancode == 0x51) { // PAGE DOWN
                if (active_win->scroll_y < (active_win->virtual_h - active_win->h)) {
                    active_win->scroll_y++;
                    tui_refresh_screen();
                }
                return;
            }
        }
    }

    if (alt_active && scancode == 0x0F) {
        if (total_windows > 1) {
            struct Window* first = rendering_layers[0];

            for (int i = 0; i < total_windows - 1; i++) {
                rendering_layers[i] = rendering_layers[i + 1];
            }

            rendering_layers[total_windows - 1] = first;

            for (int i = 0; i < total_windows; i++) {
                rendering_layers[i]->is_active = (i == total_windows - 1);
            }

            tui_refresh_screen();
            return;
        }
    }

    if (scancode == 0x48) { if (active_win->current_focus_comp > 0) active_win->current_focus_comp--; else active_win->current_focus_comp = active_win->child_count - 1; tui_refresh_screen(); return; }
    if (scancode == 0x50) { if (active_win->current_focus_comp < active_win->child_count - 1) active_win->current_focus_comp++; else active_win->current_focus_comp = 0; tui_refresh_screen(); return; }

    if (active_win->child_count > 0 && active_win->current_focus_comp >= 0) {
        struct Component* fc = &active_win->children[active_win->current_focus_comp];
        if (scancode == 0x1C) {
            if (fc->type == COMP_BUTTON && fc->on_action) { fc->on_action(active_win, fc); tui_refresh_screen(); return; }
            if (fc->type == COMP_INPUT && fc->buffer[0] != '\0') {
                for(int i=0; i<active_win->child_count; i++) {
                    if (active_win->children[i].type == COMP_TERMINAL_APP) {
                        char full_echo[64] = {0}; int p_len = core_strlen(active_win->children[i].term_prompt);
                        for(int j=0; j<p_len; j++) full_echo[j] = active_win->children[i].term_prompt[j];
                        for(int j=0; fc->buffer[j]; j++) full_echo[p_len + j] = fc->buffer[j];
                        tui_append_terminal(active_win->id, i, full_echo);

                        int found = 0;
                        for(int c=0; c<total_registered_cmds; c++) {
                            if (core_strcmp(fc->buffer, registered_cmds[c].name) == 0) { registered_cmds[c].handler(active_win->id, i); found = 1; break; }
                        }
                        if(!found) tui_append_terminal(active_win->id, i, "Unknown command.");
                    }
                }
                fc->buffer[0] = '\0'; fc->curr_chars = 0; tui_refresh_screen(); return;
            }
        }
        if (fc->type == COMP_INPUT) {
            if (scancode == 0x0E) { if (fc->curr_chars > 0) { fc->curr_chars--; fc->buffer[fc->curr_chars] = '\0'; } tui_refresh_screen(); return; }
            if (scancode == 0x2A || scancode == 0x36) { 
                shift_pressed = 1; 
                return; 
            } 
            if (scancode == 0xAA || scancode == 0xB6) { 
                shift_pressed = 0; 
                return; 
            }
            char ascii = core_scancode_to_ascii(scancode, shift_pressed);
            if (ascii != 0 && (fc->curr_chars < fc->buffer_len - 1)) {
                if (fc->curr_chars < fc->w) { fc->buffer[fc->curr_chars] = ascii; fc->curr_chars++; fc->buffer[fc->curr_chars] = '\0'; }
            }
            tui_refresh_screen(); return;
        }
    }
}

#define MAX_TABLES 5
#define DB_MAX_ROWS 50
#define DB_MAX_COLS 5
#define DB_MAX_STR 32

typedef struct {
    int is_active;
    char name[DB_MAX_STR];
    int row_count;
    char rows[DB_MAX_ROWS][DB_MAX_COLS][DB_MAX_STR];
} Table;

static Table tables[MAX_TABLES] = {0};
static char db_input_buffer[256];

void core_strcpy(char* dest, const char* src) {
    int i = 0;
    while(src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}


int db_tokenize(char* input, char tokens[][DB_MAX_STR], int max_tokens) {
    int count = 0;
    int i = 0, j = 0;
    
    while(input[i] != '\0' && count < max_tokens) {
        if(input[i] == ' ' || input[i] == ',' || input[i] == '(' || input[i] == ')' || input[i] == ';') {
            if (j > 0) {
                tokens[count][j] = '\0';
                count++;
                j = 0;
            }
        } else {
            if (j < DB_MAX_STR - 1) {
                tokens[count][j++] = input[i];
            }
        }
        i++;
    }
    if(j > 0 && count < max_tokens) {
        tokens[count][j] = '\0';
        count++;
    }
    return count;
}

int get_table_index(const char* name) {
    for(int i = 0; i < MAX_TABLES; i++) {
        if(tables[i].is_active && core_strcmp(tables[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

void db_execute_query(struct Window* win, struct Component* comp) {
    struct Component* input_comp = &win->children[1];
    char* query = input_comp->buffer;
    
    char echo_buf[256];
    core_strcpy(echo_buf, "SQL> ");
    int p = 5;
    for(int i = 0; query[i] != '\0' && p < 255; i++) {
        echo_buf[p++] = query[i];
    }
    echo_buf[p] = '\0';
    tui_append_terminal(win->id, 2, echo_buf);

    char tokens[30][DB_MAX_STR] = {0};
    int token_count = db_tokenize(query, tokens, 30);

    if (token_count > 0) {
        if (core_strcmp(tokens[0], "CREATE") == 0 && core_strcmp(tokens[1], "TABLE") == 0 && token_count > 2) {
            if (get_table_index(tokens[2]) != -1) {
                tui_append_terminal(win->id, 2, "ERR: TABLE ALREADY EXISTS");
            } else {
                int free_idx = -1;
                for(int i = 0; i < MAX_TABLES; i++) {
                    if(!tables[i].is_active) { free_idx = i; break; }
                }
                if (free_idx != -1) {
                    tables[free_idx].is_active = 1;
                    core_strcpy(tables[free_idx].name, tokens[2]);
                    tables[free_idx].row_count = 0;
                    tui_append_terminal(win->id, 2, "OK: TABLE CREATED");
                } else {
                    tui_append_terminal(win->id, 2, "ERR: MAX TABLES REACHED");
                }
            }
        }
        else if (core_strcmp(tokens[0], "DROP") == 0 && core_strcmp(tokens[1], "TABLE") == 0 && token_count > 2) {
            int t_idx = get_table_index(tokens[2]);
            if (t_idx != -1) {
                tables[t_idx].is_active = 0;
                tables[t_idx].row_count = 0;
                tui_append_terminal(win->id, 2, "OK: TABLE DROPPED");
            } else {
                tui_append_terminal(win->id, 2, "ERR: TABLE NOT FOUND");
            }
        }
        else if (core_strcmp(tokens[0], "INSERT") == 0 && core_strcmp(tokens[1], "INTO") == 0 && token_count > 3) {
            int t_idx = get_table_index(tokens[2]);
            if (t_idx == -1) {
                tui_append_terminal(win->id, 2, "ERR: TABLE NOT FOUND");
            } else if (tables[t_idx].row_count >= DB_MAX_ROWS) {
                tui_append_terminal(win->id, 2, "ERR: TABLE FULL");
            } else {
                int val_start = (core_strcmp(tokens[3], "VALUES") == 0) ? 4 : 3;
                int r = tables[t_idx].row_count;
                for (int c = 0; c < DB_MAX_COLS; c++) {
                    if (val_start + c < token_count) {
                        core_strcpy(tables[t_idx].rows[r][c], tokens[val_start + c]);
                    } else {
                        core_strcpy(tables[t_idx].rows[r][c], "NULL");
                    }
                }
                tables[t_idx].row_count++;
                tui_append_terminal(win->id, 2, "OK: 1 ROW INSERTED");
            }
        }
        else if (core_strcmp(tokens[0], "SELECT") == 0 && core_strcmp(tokens[1], "FROM") == 0 && token_count > 2) {
            int t_idx = get_table_index(tokens[2]);
            if (t_idx == -1) {
                tui_append_terminal(win->id, 2, "ERR: TABLE NOT FOUND");
            } else if (tables[t_idx].row_count == 0) {
                tui_append_terminal(win->id, 2, "EMPTY SET");
            } else {
                int use_where = 0;
                int w_col = 0;
                char w_val[DB_MAX_STR] = {0};

                if (token_count >= 6 && core_strcmp(tokens[3], "WHERE") == 0) {
                    use_where = 1;
                    w_col = tokens[4][0] - '0';
                    core_strcpy(w_val, tokens[5]);
                }

                int found = 0;
                for (int i = 0; i < tables[t_idx].row_count; i++) {
                    if (use_where) {
                        if (w_col < 0 || w_col >= DB_MAX_COLS) continue;
                        if (core_strcmp(tables[t_idx].rows[i][w_col], w_val) != 0) continue;
                    }
                    
                    found++;
                    char row_buf[256] = {0};
                    int pos = 0;
                    row_buf[pos++] = '['; row_buf[pos++] = '0' + i; row_buf[pos++] = ']'; row_buf[pos++] = ' ';
                    
                    for (int c = 0; c < DB_MAX_COLS; c++) {
                        for (int k = 0; tables[t_idx].rows[i][c][k] != '\0' && pos < 240; k++) {
                            row_buf[pos++] = tables[t_idx].rows[i][c][k];
                        }
                        if (c < DB_MAX_COLS - 1) { 
                            row_buf[pos++] = ' '; row_buf[pos++] = '|'; row_buf[pos++] = ' '; 
                        }
                    }
                    row_buf[pos] = '\0';
                    tui_append_terminal(win->id, 2, row_buf);
                }
                if (found == 0) tui_append_terminal(win->id, 2, "NO MATCHES");
            }
        }
        else if (core_strcmp(tokens[0], "DELETE") == 0 && core_strcmp(tokens[1], "FROM") == 0 && token_count > 2) {
            int t_idx = get_table_index(tokens[2]);
            if (t_idx == -1) {
                tui_append_terminal(win->id, 2, "ERR: TABLE NOT FOUND");
            } else if (token_count >= 6 && core_strcmp(tokens[3], "WHERE") == 0) {
                int w_col = tokens[4][0] - '0';
                char w_val[DB_MAX_STR];
                core_strcpy(w_val, tokens[5]);
                int del_count = 0;

                for (int i = tables[t_idx].row_count - 1; i >= 0; i--) {
                    if (w_col >= 0 && w_col < DB_MAX_COLS && core_strcmp(tables[t_idx].rows[i][w_col], w_val) == 0) {
                        for (int j = i; j < tables[t_idx].row_count - 1; j++) {
                            for (int c = 0; c < DB_MAX_COLS; c++) {
                                core_strcpy(tables[t_idx].rows[j][c], tables[t_idx].rows[j+1][c]);
                            }
                        }
                        tables[t_idx].row_count--;
                        del_count++;
                    }
                }
                if (del_count > 0) tui_append_terminal(win->id, 2, "OK: ROWS DELETED");
                else tui_append_terminal(win->id, 2, "NO MATCHES TO DELETE");
            }
        }
        else if (core_strcmp(tokens[0], "UPDATE") == 0 && token_count >= 8 && core_strcmp(tokens[2], "SET") == 0 && core_strcmp(tokens[5], "WHERE") == 0) {
            int t_idx = get_table_index(tokens[1]);
            if (t_idx == -1) {
                tui_append_terminal(win->id, 2, "ERR: TABLE NOT FOUND");
            } else {
                int set_col = tokens[3][0] - '0';
                char set_val[DB_MAX_STR];
                core_strcpy(set_val, tokens[4]);
                
                int w_col = tokens[6][0] - '0';
                char w_val[DB_MAX_STR];
                core_strcpy(w_val, tokens[7]);
                
                int up_count = 0;
                for (int i = 0; i < tables[t_idx].row_count; i++) {
                    if (w_col >= 0 && w_col < DB_MAX_COLS && core_strcmp(tables[t_idx].rows[i][w_col], w_val) == 0) {
                        if (set_col >= 0 && set_col < DB_MAX_COLS) {
                            core_strcpy(tables[t_idx].rows[i][set_col], set_val);
                            up_count++;
                        }
                    }
                }
                if (up_count > 0) tui_append_terminal(win->id, 2, "OK: ROWS UPDATED");
                else tui_append_terminal(win->id, 2, "NO MATCHES TO UPDATE");
            }
        }
        else {
            tui_append_terminal(win->id, 2, "ERR: SYNTAX ERROR");
        }
    }

    input_comp->buffer[0] = '\0';
    input_comp->curr_chars = 0;
    tui_refresh_screen();
}

void DBApp() {
    int db_win = tui_create_window("RozOS Relational DB", 5, 2, 74, 22);
    
    ui_add_text(db_win, "CREATE TABLE t | DROP TABLE t | SELECT FROM t");
    ui_add_input(db_win, "SQL> ", db_input_buffer, 200);
    ui_add_terminal(db_win, 14);
    ui_add_button(db_win, "EXECUTE", db_execute_query);
    
    tui_refresh_screen();
}

/* ========================================================================= */
/*       RozOS - SYSTEM PLIKÓW FAT32 (BEZPIECZNY, NIEZALEŻNY STEROWNIK)      */
/* ========================================================================= */

#define ATA_REG_DATA       0x1F0
#define ATA_REG_FEATURES   0x1F1
#define ATA_REG_SECCOUNT   0x1F2
#define ATA_REG_LBA_LO     0x1F3
#define ATA_REG_LBA_MID    0x1F4
#define ATA_REG_LBA_HI     0x1F5
#define ATA_REG_DRIVE      0x1F6
#define ATA_REG_STATUS     0x1F7
#define ATA_REG_COMMAND    0x1F7

#pragma pack(push, 1)
struct FAT32_BPB {
    uint8_t  bootjmp[3];
    char     oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sector_count;
    uint8_t  table_count;
    uint16_t root_entry_count;
    uint16_t total_sectors_16;
    uint8_t  media_type;
    uint16_t table_size_16;
    uint16_t sectors_per_track;
    uint16_t head_side_count;
    uint32_t hidden_sector_count;
    uint32_t total_sectors_32;
    uint32_t table_size_32;
    uint16_t extended_flags;
    uint16_t fat_version;
    uint32_t root_cluster;
    uint16_t fat_info;
    uint16_t backup_BS_sector;
    uint8_t  reserved[12];
    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_signature;
    uint32_t volume_id;
    char     volume_label[11];
    char     fat_type_label[8];
};

struct FAT32_DirEntry {
    uint8_t  name[11]; /* uint8_t zapobiega błędom rzutowania znaku ze znakiem */
    uint8_t  attr;
    uint8_t  NTRes;
    uint8_t  CRTTimeTenth;
    uint16_t CRTTime;
    uint16_t CRTDate;
    uint16_t LSTAccDate;
    uint16_t first_cluster_hi;
    uint16_t WRTTime;
    uint16_t WRTDate;
    uint16_t first_cluster_lo;
    uint32_t file_size;
};
#pragma pack(pop)

/* Skalarne zmienne konfiguracyjne systemu plików */
static uint32_t Partition_Start_LBA = 0;
static uint32_t Fat_Start_LBA = 0;
static uint32_t Data_Start_LBA = 0;
static uint32_t Sectors_Per_Cluster = 0;
static uint32_t Bytes_Per_Cluster = 0;
static uint32_t Root_Cluster = 0;
static uint32_t Sectors_Per_Fat = 0;

uint8_t cluster_scratchpad[8192];
/* ========================================================================= */
/* NISKIPOZIOMOWE OPERACJE ATA (POLLED IO DLA RozOS)                        */
/* ========================================================================= */

void ata_read_sector(uint32_t lba, uint8_t* buffer) {
    outb(0x3F6, 0x02); /* Wyłączenie przerwań sprzętowych dla stabilności */

    outb(ATA_REG_SECCOUNT, 1);
    outb(ATA_REG_LBA_LO, (uint8_t)lba);
    outb(ATA_REG_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_REG_LBA_HI, (uint8_t)(lba >> 16));
    outb(ATA_REG_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_REG_COMMAND, 0x20);

    while (!(inb(ATA_REG_STATUS) & 0x08));

    for (int i = 0; i < 256; i++) {
        uint16_t data = inw(ATA_REG_DATA);
        buffer[i * 2] = (uint8_t)data;
        buffer[i * 2 + 1] = (uint8_t)(data >> 8);
    }
}

void ata_write_sector(uint32_t lba, uint8_t* buffer) {
    outb(0x3F6, 0x02); /* Wyłączenie przerwań sprzętowych */

    outb(ATA_REG_SECCOUNT, 1);
    outb(ATA_REG_LBA_LO, (uint8_t)lba);
    outb(ATA_REG_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_REG_LBA_HI, (uint8_t)(lba >> 16));
    outb(ATA_REG_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_REG_COMMAND, 0x30);

    while (!(inb(ATA_REG_STATUS) & 0x08));

    for (int i = 0; i < 256; i++) {
        uint16_t data = buffer[i * 2] | (buffer[i * 2 + 1] << 8);
        outw(ATA_REG_DATA, data);
    }
}

/* ========================================================================= */
/* FUNKCJE POMOCNICZE OBSŁUGI STRUKTURY FAT32                                */
/* ========================================================================= */

static uint32_t fs_cluster_to_lba(uint32_t cluster) {
    if (cluster < 2) return Data_Start_LBA;
    return Data_Start_LBA + ((cluster - 2) * Sectors_Per_Cluster);
}

static void fs_read_cluster(uint32_t cluster, uint8_t* buffer) {
    uint32_t lba = fs_cluster_to_lba(cluster);
    for (uint32_t i = 0; i < Sectors_Per_Cluster; i++) {
        ata_read_sector(lba + i, buffer + (i * 512));
    }
}

static void fs_write_cluster(uint32_t cluster, uint8_t* buffer) {
    uint32_t lba = fs_cluster_to_lba(cluster);
    for (uint32_t i = 0; i < Sectors_Per_Cluster; i++) {
        ata_write_sector(lba + i, buffer + (i * 512));
    }
}

static void fs_to_fat_name(const char* src, char* dest) {
    for (int i = 0; i < 11; i++) dest[i] = ' ';
    int i = 0, d = 0;
    while (src[i] != '\0' && src[i] != '/' && src[i] != '.' && d < 8) {
        char c = src[i++];
        if (c >= 'a' && c <= 'z') c -= 32;
        dest[d++] = c;
    }
    if (src[i] == '.') {
        i++;
        d = 8;
        while (src[i] != '\0' && src[i] != '/' && d < 11) {
            char c = src[i++];
            if (c >= 'a' && c <= 'z') c -= 32;
            dest[d++] = c;
        }
    }
}

static uint32_t fs_get_next_cluster(uint32_t cluster) {
    if (cluster < 2) return 0x0FFFFFF8;
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = Fat_Start_LBA + (fat_offset / 512);
    uint32_t ent_offset = fat_offset % 512;

    uint8_t sector_buf[512];
    ata_read_sector(fat_sector, sector_buf);
    uint32_t next_cluster = *(uint32_t*)&sector_buf[ent_offset];
    return next_cluster & 0x0FFFFFFF;
}

static void fs_set_cluster_value(uint32_t cluster, uint32_t value) {
    if (cluster < 2) return;
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = Fat_Start_LBA + (fat_offset / 512);
    uint32_t ent_offset = fat_offset % 512;

    uint8_t sector_buf[512];
    ata_read_sector(fat_sector, sector_buf);
    *(uint32_t*)&sector_buf[ent_offset] = (value & 0x0FFFFFFF);
    ata_write_sector(fat_sector, sector_buf);
}

static uint32_t fs_find_free_cluster(void) {
    uint32_t total_fat_sectors = Sectors_Per_Fat;
    uint8_t sector_buf[512];
    
    for (uint32_t s = 0; s < total_fat_sectors; s++) {
        ata_read_sector(Fat_Start_LBA + s, sector_buf);
        uint32_t* entries = (uint32_t*)sector_buf;
        for (int e = 0; e < 128; e++) {
            uint32_t current_cl = (s * 128) + e;
            if (current_cl < 2) continue;
            
            if ((entries[e] & 0x0FFFFFFF) == 0) {
                return current_cl;
            }
        }
    }
    return 0;
}

static int fs_find_entry_in_cluster(uint32_t cluster, const char* fat_name, struct FAT32_DirEntry* out, uint32_t* out_sector, int* out_idx) {
    if (Bytes_Per_Cluster == 0 || Bytes_Per_Cluster > 8192) return 0;
    
    uint8_t cluster_buf[Bytes_Per_Cluster];
    fs_read_cluster(cluster, cluster_buf);
    
    struct FAT32_DirEntry* entries = (struct FAT32_DirEntry*)cluster_buf;
    int max_entries = Bytes_Per_Cluster / 32;

    for (int i = 0; i < max_entries; i++) {
        if (entries[i].name[0] == 0x00) return 0;
        if (entries[i].name[0] == 0xE5) continue;

        int match = 1;
        for (int k = 0; k < 11; k++) {
            if (entries[i].name[k] != (uint8_t)fat_name[k]) { match = 0; break; }
        }
        if (match) {
            if (out) *out = entries[i];
            if (out_sector) *out_sector = fs_cluster_to_lba(cluster) + (i / 16);
            if (out_idx) *out_idx = i % 16;
            return 1;
        }
    }
    return 0;
}

static int fs_resolve_path(const char* path, struct FAT32_DirEntry* out, uint32_t* out_sector, int* out_idx) {
    uint32_t current_cluster = Root_Cluster;
    int i = 0;
    if (path[0] == '/') i++;

    char fat_name[11];
    while (path[i] != '\0') {
        fs_to_fat_name(&path[i], fat_name);
        
        while (path[i] != '/' && path[i] != '\0') i++;
        if (path[i] == '/') i++;

        struct FAT32_DirEntry entry;
        uint32_t sec; int idx;
        
        uint32_t cl = current_cluster;
        int found = 0;
        while (cl < 0x0FFFFFF8) {
            if (fs_find_entry_in_cluster(cl, fat_name, &entry, &sec, &idx)) {
                found = 1;
                break;
            }
            cl = fs_get_next_cluster(cl);
        }

        if (!found) return 0;

        if (path[i] != '\0') {
            if (!(entry.attr & 0x10)) return 0;
            current_cluster = ((uint32_t)entry.first_cluster_hi << 16) | entry.first_cluster_lo;
        } else {
            if (out) *out = entry;
            if (out_sector) *out_sector = sec;
            if (out_idx) *out_idx = idx;
            return 1;
        }
    }
    return 0;
}

/* ========================================================================= */
/* POLA WYCOFANIA (ZGODNE Z INTERFEJSEM ARCHITEKTURY RozOS)                  */
/* ========================================================================= */

int fs_init(void) {
    uint8_t sector_buf[512];
    ata_read_sector(0, sector_buf);
    
    if (sector_buf[510] != 0x55 || sector_buf[511] != 0xAA) return 0;

    /* Unikanie Alignment Faults poprzez odczyt bajt po bajcie rekordu MBR */
    uint8_t* p_lba = &sector_buf[0x1BE + 8];
    Partition_Start_LBA = p_lba[0] | (p_lba[1] << 8) | (p_lba[2] << 16) | (p_lba[3] << 24);

    ata_read_sector(Partition_Start_LBA, sector_buf);
    struct FAT32_BPB* bpb = (struct FAT32_BPB*)sector_buf;

    if (bpb->bytes_per_sector != 512 || bpb->sectors_per_cluster == 0) return 0; 

    Sectors_Per_Cluster = bpb->sectors_per_cluster;
    Bytes_Per_Cluster   = Sectors_Per_Cluster * 512;
    Sectors_Per_Fat     = bpb->table_size_32;
    Root_Cluster        = bpb->root_cluster;

    Fat_Start_LBA  = Partition_Start_LBA + bpb->reserved_sector_count;
    Data_Start_LBA = Fat_Start_LBA + (bpb->table_count * Sectors_Per_Fat);

    return 1;
}

int exists(const char* path) {
    if (strcmp(path, "/") == 0) return 1;
    return fs_resolve_path(path, NULL, NULL, NULL);
}

void list(const char* dir_path, int win_id, int term_idx) {
    if (Bytes_Per_Cluster == 0 || Bytes_Per_Cluster > 8192) return;
    uint32_t cluster = Root_Cluster;
    if (strcmp(dir_path, "/") != 0) {
        struct FAT32_DirEntry entry;
        if (!fs_resolve_path(dir_path, &entry, NULL, NULL) || !(entry.attr & 0x10)) {
            tui_append_terminal(win_id, term_idx, "Katalog nie istnieje.");
            return;
        }
        cluster = ((uint32_t)entry.first_cluster_hi << 16) | entry.first_cluster_lo;
    }

    uint8_t cluster_buf[Bytes_Per_Cluster];
    char display_name[13];
    
    while (cluster < 0x0FFFFFF8) {
        fs_read_cluster(cluster, cluster_buf);
        struct FAT32_DirEntry* entries = (struct FAT32_DirEntry*)cluster_buf;
        int max_entries = Bytes_Per_Cluster / 32;

        for (int i = 0; i < max_entries; i++) {
            if (entries[i].name[0] == 0x00) return;
            if (entries[i].name[0] == 0xE5 || entries[i].attr == 0x0F) continue;

            int p = 0;
            for(int j=0; j<8; j++) if(entries[i].name[j] != ' ') display_name[p++] = entries[i].name[j];
            if(entries[i].name[8] != ' ' && !(entries[i].attr & 0x10)) {
                display_name[p++] = '.';
                for(int j=8; j<11; j++) if(entries[i].name[j] != ' ') display_name[p++] = entries[i].name[j];
            }
            if (entries[i].attr & 0x10) display_name[p++] = '/';
            display_name[p] = '\0';

            tui_append_terminal(win_id, term_idx, display_name);
        }
        cluster = fs_get_next_cluster(cluster);
    }
}

int open(const char* path, uint32_t* out_first_cluster, uint32_t* out_size) {
    struct FAT32_DirEntry entry;
    if (fs_resolve_path(path, &entry, NULL, NULL)) {
        if (entry.attr & 0x10) return 0;
        if (out_first_cluster) *out_first_cluster = ((uint32_t)entry.first_cluster_hi << 16) | entry.first_cluster_lo;
        if (out_size) *out_size = entry.file_size;
        return 1;
    }
    return 0;
}

int remove(const char* path) {
    uint32_t target_sector; int entry_idx;
    uint8_t sector_buf[512];
    if (!fs_resolve_path(path, NULL, &target_sector, &entry_idx)) return 0;

    ata_read_sector(target_sector, sector_buf);
    struct FAT32_DirEntry* entries = (struct FAT32_DirEntry*)sector_buf;
    
    uint32_t file_cluster = ((uint32_t)entries[entry_idx].first_cluster_hi << 16) | entries[entry_idx].first_cluster_lo;

    entries[entry_idx].name[0] = 0xE5;
    ata_write_sector(target_sector, sector_buf);

    while (file_cluster > 1 && file_cluster < 0x0FFFFFF8) {
        uint32_t next = fs_get_next_cluster(file_cluster);
        fs_set_cluster_value(file_cluster, 0x00000000);
        file_cluster = next;
    }
    return 1;
}

int write(const char* path, uint8_t* source_buffer, uint32_t data_bytes) {
    if (Bytes_Per_Cluster == 0 || Bytes_Per_Cluster > 8192) return 0;
    uint32_t target_sector; int entry_idx;
    uint8_t sector_buf[512];
    
    if (!fs_resolve_path(path, NULL, &target_sector, &entry_idx)) {
        char fat_name[11];
        const char* p_name = path[0] == '/' ? path + 1 : path;
        fs_to_fat_name(p_name, fat_name);

        uint32_t root_cl = Root_Cluster;
        int slot_found = 0;
        
        while (root_cl < 0x0FFFFFF8 && !slot_found) {
            uint32_t start_sec = fs_cluster_to_lba(root_cl);
            for (uint32_t s = 0; s < Sectors_Per_Cluster; s++) {
                ata_read_sector(start_sec + s, sector_buf);
                struct FAT32_DirEntry* entries = (struct FAT32_DirEntry*)sector_buf;
                for (int e = 0; e < 16; e++) {
                    if (entries[e].name[0] == 0x00 || entries[e].name[0] == 0xE5) {
                        target_sector = start_sec + s;
                        entry_idx = e;
                        
                        for(int k=0; k<11; k++) entries[e].name[k] = (uint8_t)fat_name[k];
                        entries[e].attr = 0x20;
                        entries[e].file_size = 0;
                        entries[e].first_cluster_hi = 0;
                        entries[e].first_cluster_lo = 0;
                        
                        ata_write_sector(target_sector, sector_buf);
                        slot_found = 1;
                        break;
                    }
                }
                if (slot_found) break;
            }
            if (!slot_found) root_cl = fs_get_next_cluster(root_cl);
        }
        if (!slot_found) return 0;
    }

    uint32_t needed_clusters = (data_bytes + Bytes_Per_Cluster - 1) / Bytes_Per_Cluster;
    if (needed_clusters == 0) needed_clusters = 1;
    uint32_t prev_cl = 0;
    uint32_t first_allocated_cl = 0;
    uint32_t bytes_written = 0;

    uint8_t cluster_buf[Bytes_Per_Cluster];

    for (uint32_t i = 0; i < needed_clusters; i++) {
        uint32_t new_cl = fs_find_free_cluster();
        if (!new_cl) return 0;

        if (i == 0) first_allocated_cl = new_cl;
        else fs_set_cluster_value(prev_cl, new_cl);

        fs_set_cluster_value(new_cl, 0x0FFFFFFF);

        uint32_t to_write = (data_bytes - bytes_written) > Bytes_Per_Cluster ? Bytes_Per_Cluster : (data_bytes - bytes_written);
        
        for (uint32_t b = 0; b < Bytes_Per_Cluster; b++) {
            cluster_buf[b] = (b < to_write) ? source_buffer[bytes_written + b] : 0x00;
        }

        fs_write_cluster(new_cl, cluster_buf);
        bytes_written += to_write;
        prev_cl = new_cl;
    }

    ata_read_sector(target_sector, sector_buf);
    struct FAT32_DirEntry* entries = (struct FAT32_DirEntry*)sector_buf;
    entries[entry_idx].file_size = data_bytes;
    entries[entry_idx].first_cluster_hi = (uint16_t)(first_allocated_cl >> 16);
    entries[entry_idx].first_cluster_lo = (uint16_t)(first_allocated_cl & 0xFFFF);
    ata_write_sector(target_sector, sector_buf);

    return 1;
}

int mkdir(const char* path) {
    if (Bytes_Per_Cluster == 0 || Bytes_Per_Cluster > 8192) return 0;
    char fat_name[11];
    fs_to_fat_name(path[0] == '/' ? path + 1 : path, fat_name);

    uint32_t target_sector; int entry_idx;
    uint8_t sector_buf[512];
    if (fs_resolve_path(path, NULL, &target_sector, &entry_idx)) return 0;

    uint32_t root_cl = Root_Cluster;
    uint8_t cluster_buf[Bytes_Per_Cluster];

    while (root_cl < 0x0FFFFFF8) {
        uint32_t start_sec = fs_cluster_to_lba(root_cl);
        for (uint32_t s = 0; s < Sectors_Per_Cluster; s++) {
            ata_read_sector(start_sec + s, sector_buf);
            struct FAT32_DirEntry* entries = (struct FAT32_DirEntry*)sector_buf;
            for (int e = 0; e < 16; e++) {
                if (entries[e].name[0] == 0x00 || entries[e].name[0] == 0xE5) {
                    uint32_t new_dir_cl = fs_find_free_cluster();
                    if (!new_dir_cl) return 0;

                    fs_set_cluster_value(new_dir_cl, 0x0FFFFFFF);

                    for (uint32_t b = 0; b < Bytes_Per_Cluster; b++) cluster_buf[b] = 0;
                    fs_write_cluster(new_dir_cl, cluster_buf);

                    for(int k=0; k<11; k++) entries[e].name[k] = (uint8_t)fat_name[k];
                    entries[e].attr = 0x10;
                    entries[e].file_size = 0;
                    entries[e].first_cluster_hi = (uint16_t)(new_dir_cl >> 16);
                    entries[e].first_cluster_lo = (uint16_t)(new_dir_cl & 0xFFFF);

                    ata_write_sector(start_sec + s, sector_buf);
                    return 1;
                }
            }
        }
        root_cl = fs_get_next_cluster(root_cl);
    }
    return 0;
}
void ui_window_set_help(const char* text) {
    if (total_windows <= 0) return;

    struct Window* active_win = rendering_layers[total_windows - 1];
    active_win->help_text = text;

    tui_refresh_screen();
}


void tui_close_active_window() {
    if (total_windows <= 0) return;
    total_windows--;
    if (total_windows > 0) {
        for (int i = 0; i < total_windows; i++) {
            rendering_layers[i]->is_active = 0;
        }
        rendering_layers[total_windows - 1]->is_active = 1;
    }
    tui_refresh_screen();
}



void ui_add_component(int win_id, int type, int rel_x, int rel_y, const char* text) {
    struct Window* win = &windows_pool[win_id];
    struct Component* comp = &win->children[win->child_count];
    
    comp->type = type;
    comp->rel_x = rel_x;
    comp->rel_y = rel_y;
    comp->text = text;
    
    win->child_count++;
}

static unsigned int hex_to_int(char* hex) {
    unsigned int val = 0;
    while (*hex) {
        val *= 16;
        if (*hex >= '0' && *hex <= '9') val += *hex - '0';
        else if (*hex >= 'A' && *hex <= 'F') val += *hex - 'A' + 10;
        else if (*hex >= 'a' && *hex <= 'f') val += *hex - 'a' + 10;
        hex++;
    }
    return val;
}

void show_msgbox(const char* title, const char* text) {
    int w = 30, h = 6;
    int x = (SCR_W - w) / 2;
    int y = (SCR_H - h) / 2;
    
    // Tworzysz tymczasowe okno modalne
    int win_id = tui_create_window(title, x, y, w, h);
    windows_pool[win_id].is_modal = 1;
    // Dodaj tekst jako komponent COMP_TEXT
    ui_add_component(win_id, COMP_TEXT, 2, 2, text);
}

void acpi_real_pc_shutdown(struct FADT* fadt) {
    if (!fadt || !fadt->Dsdt) return;

    struct ACPISDTHeader* dsdt_header = (struct ACPISDTHeader*)fadt->Dsdt;
    unsigned char* aml_code = (unsigned char*)fadt->Dsdt;
    unsigned int dsdt_length = dsdt_header->Length;

    uint16_t SLP_TYPa = 0xFF;
    uint16_t SLP_TYPb = 0xFF;

    for (unsigned int i = 0; i < dsdt_length - 4; i++) {
        if (aml_code[i] == '_' && aml_code[i+1] == 'S' && aml_code[i+2] == '5' && aml_code[i+3] == '_') {
            unsigned int ptr = i + 4;
            
            if (aml_code[ptr] == 0x12) {
                ptr++;
                
                if ((aml_code[ptr] & 0xC0) != 0) {
                    ptr += (aml_code[ptr] >> 6); 
                }
                ptr++; 
                
                ptr++; 

                if (aml_code[ptr] == 0x0A) {
                    ptr++;
                }
                SLP_TYPa = aml_code[ptr];
                ptr++;

                if (aml_code[ptr] == 0x0A) {
                    ptr++;
                }
                SLP_TYPb = aml_code[ptr];
                
                break;
            }
        }
    }

    if (SLP_TYPa == 0xFF) {
        SLP_TYPa = 5;
        SLP_TYPb = 5;
    }

    uint16_t pm1a_cmd = (SLP_TYPa << 10) | (1 << 13);
    uint16_t pm1b_cmd = (SLP_TYPb << 10) | (1 << 13);

    acpi_enable(fadt);

    if (fadt->PM1a_Control_Block != 0) {
        outw(fadt->PM1a_Control_Block, pm1a_cmd);
    }
    if (fadt->PM1b_Control_Block != 0) {
        outw(fadt->PM1b_Control_Block, pm1b_cmd);
    }

    outw(fadt->PM1a_Control_Block, 0x2000); 
}

void acpi_real_pc_shutdown_run() {
    acpi_real_pc_shutdown(fadt);
}

// 2. Wrapper, który pasuje do TUI_MenuHandler
void poweroff_msg() {
    acpi_disable(fadt);
    ui_set_bottom_help(" Press power to power off");
    int winh = tui_create_window("HALTED", 20, 8, 40, 6);
    ui_add_text(winh, "Halted press power button");
    tui_refresh_screen();
    asm volatile ("cli");
    while(1) {
        asm volatile ("hlt");
    }
}
void HEXRun(struct Window* win, struct Component* comp) {
    struct Component* input_comp = &win->children[2];
    char* hex_cmd = input_comp->buffer;
    run_hex(hex_cmd);
    tui_refresh_screen();
}
void HEXR() {
    int hex_win = tui_create_window("Hex Editor", 0, 0, 40, 16);
    ui_add_text(hex_win, "Enter HEX");
    ui_add_input(hex_win, "HEX", adres_hex, 100);
    ui_add_button(hex_win, "RUN", HEXRun);
    tui_refresh_screen();
}

struct RozOS_Framework {
    void (*refresh_screen)();
    int  (*create_window)(const char* title, int x, int y, int w, int h);
    void (*close_active_window)();
    void (*add_component)(int win_id, int type, int rel_x, int rel_y, const char* text);
    void (*show_msgbox)(const char* title, const char* text);
    void (*set_bottom_help)(const char* text);
    
    int  (*file_exists)(const char* path);
    void (*file_list)(const char* dir_path, int win_id, int term_idx);
    int  (*file_remove)(const char* path);
    int  (*file_write)(const char* path, uint8_t* source_buffer, uint32_t data_bytes);
    int  (*file_mkdir)(const char* path);

    void (*clear_screen)();
    void (*print_raw)(const char* str, int line, int col, char color);
    void (*reboot_system)();
    void (*poweroff_system)();
};

void exec(const char* path) {
    struct RozOS_Framework* os = (struct RozOS_Framework*)0x400000;
    
    os->refresh_screen       = tui_refresh_screen;
    os->create_window        = tui_create_window;
    os->close_active_window  = tui_close_active_window;
    os->add_component        = ui_add_component;
    os->show_msgbox          = show_msgbox;
    os->set_bottom_help      = ui_set_bottom_help;
    
    os->file_exists          = exists;
    os->file_list            = list;
    os->file_remove          = remove;
    os->file_write           = write;
    os->file_mkdir           = mkdir;
    
    os->clear_screen         = clear;
    os->print_raw            = print;
    os->reboot_system        = reboot;
    os->poweroff_system      = acpi_real_pc_shutdown_run;

    uint32_t first_cluster = 0;
    uint32_t file_size = 0;
    
    if (!open(path, &first_cluster, &file_size)) {
        show_msgbox("System", "Program error!");
        return;
    }

    if (file_size == 0) return;

    uint8_t* program_buffer = (uint8_t*)0x500000;
    uint32_t current_cluster = first_cluster;
    uint32_t bytes_left = file_size;
    uint32_t buffer_offset = 0;

    extern uint8_t cluster_scratchpad[8192]; 
    extern uint32_t Bytes_Per_Cluster;

    while (current_cluster > 0 && current_cluster < 0x0FFFFFF8 && bytes_left > 0) {
        fs_read_cluster(current_cluster, cluster_scratchpad);
        
        uint32_t bytes_to_copy = (bytes_left > Bytes_Per_Cluster) ? Bytes_Per_Cluster : bytes_left;
        
        for (uint32_t i = 0; i < bytes_to_copy; i++) {
            program_buffer[buffer_offset + i] = cluster_scratchpad[i];
        }
        
        buffer_offset += bytes_to_copy;
        bytes_left -= bytes_to_copy;
        
        current_cluster = fs_get_next_cluster(current_cluster);
    }

    void (*program_entry)() = (void (*)())program_buffer;
    program_entry();
    
    tui_refresh_screen();
}




void launcher() {
    int winl = tui_create_window("System", 0, 0, 40, 16);
    ui_add_text(winl, "Program launcher");
    ui_add_button(winl, "HEX Editor", HEXR);
    tui_refresh_screen();
}

void gui_init() {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
    tui_init_framework();
    ui_set_bottom_help(" F10 Menu Alt+X Close Alt+Arrow Window pos Alt+TAB Switch window");
    int cat_sys = ui_menu_add_category("RozOS");
    int cat_win = ui_menu_add_category("Window");
    ui_menu_add_item(cat_win, "Close", tui_close_active_window);
    ui_menu_add_item(cat_sys, "Launcher", launcher);
    ui_menu_add_item(cat_sys, "Halt", poweroff_msg);
    ui_menu_add_item(cat_sys, "Power off", acpi_real_pc_shutdown_run);
    ui_menu_add_item(cat_sys, "Reboot", reboot);
    int Run = ui_menu_add_category("Run Program");
    ui_menu_add_item(Run, "Database", DBApp);
    int win2 = tui_create_window("System", 0, 0, 40, 16);
    ui_add_text(win2, "Welcome to RozOS running on\n the ROZ work terminal");
    tui_refresh_screen();
    tui_refresh_screen();
}

void kernel_main(unsigned int magic, struct multiboot_info* mbi) {
    
    //if (fs_init()) {
    //    print("Mounted /", 0, 0, 0x0F);
    //} else {
    //    print("Error mounting /", 0, 0, 0x0F);
    //    return;
    //}
    delay(1000);
    clear();
    if (boot_param(mbi, "info_boot=1")) {
        clear();
	    print("RozOS 0.1.3", 0, 0, 0x0E);
        print("32 BIT MODE 16 CHAR BUFFER 1MB IMAGE", 1, 0, 0x0A);
        print("To exit press power button", 2, 0, 0x0F);
	    asm volatile (
	        "cli \n\t"
	        "1: \n\t"
	        "hlt \n\t"
	        "jmp 1b"
	    );
    }

    if (boot_param(mbi, "no_acpi=1")) {
        print("Acpi disabled", 0, 0, 0x0F);
        delay(1000);
    } else {
        fadt = find_fadt();
        if(fadt) {
	        acpi_enable(fadt);
        }
    }
    gui_init();
    tui_refresh_screen();
    while(1) {
        if (inb(0x64) & 1) {
            unsigned char scancode = inb(0x60);
            tui_dispatch_keyboard(scancode);
        }
    }
    while(1) {
        clear();
        print("RozOS 0.1.3", 0, 0, 0x0E);
        print("32 BIT MODE 16 CHAR BUFFER 1MB IMAGE", 1, 0, 0x0A);
        print("SLEEP MODE ACTIVE", 2, 0, 0x0A);
	    print("Type / to help", 3, 0, 0x0A);
        print("RozOS>> ", 4, 0, 0x0F);

        char buffer[16] = {0};
        input(buffer, 4, 8, 10);

        if(strcmp(buffer, "status") == 0) {
            print("23 BIT MODE 16 CHAR BUFFER 1MB IMAGE", 5, 0, 0x0A);
            print("SLEEP MODE DEACTIVED", 6, 0, 0x0F);
	        delay(1000);
        }
        else if(strcmp(buffer, "reboot") == 0) {
            print("Reboot actived!", 5, 0, 0x0F);
	        delay(1000);
	        reboot();
        }
        else if(strcmp(buffer, "clear") == 0) {
            clear();
        }
	    else if(strcmp(buffer, "version") == 0) {
	        print("RozOS 0.1.1 Home edition", 5, 0, 0x0A);
	        print("build: 000010", 6, 0, 0x0A);
	        print("add-ons: input fix, halt fix, ststus fix", 7, 0, 0x0A);
	        print("RozOS is Open Source", 8, 0, 0x0A);
	        print("Made by:Szymon Wolak (github:ROZcloud)", 9, 0, 0x0A);
	        print("Project github:github.com/ROZcloud/RozOShe", 10, 0, 0x0A);
	        delay(1000);
	    }
	    else if(strcmp(buffer, "hex") == 0) {
            clear();
	        print("hex>> ", 0, 0, 0x0F);
	        char hex_cmd[428];
	        input(hex_cmd, 0, 6, 100);
	        run_hex(hex_cmd);
	        delay(1000);
        }
	    else if(strcmp(buffer, "dev") == 0) {
	        dev();
	    }
	    else if(strcmp(buffer, "/") == 0) {
	        clear();
	        print("Commands:", 1, 0, 0x0F);
	        print("/ - help commands", 2, 0, 0x0F);
	        print("logout - logout from session", 3, 0, 0x0F);
	        print("status - system status", 4, 0, 0x0F);
	        print("reboot - reboot system", 5, 0, 0x0F);
	        print("version - show system version", 6, 0, 0x0F);
	        print("hex - enter and run hex", 7, 0, 0x0F);
	        print("lock - lock user", 8, 0, 0x0F);
            print("gui - start RozOS TUI", 8, 0, 0x0F);
	        delay(1000);
	    }
	    else if(strcmp(buffer, "logout") == 0) {
	        clear();
	        print("System halted press power button to poweroff", 0, 0, 0x04);
            acpi_disable(fadt);
	        asm volatile (
	            "cli \n\t"
	            "halt_loop: \n\t"
	            "hlt \n\t"
	            "jmp halt_loop"
	        );
	    }
        else if(strcmp(buffer, "gui") == 0) {
            gui_init();
            tui_refresh_screen();
            while(1) {
                if (inb(0x64) & 1) {
                    unsigned char scancode = inb(0x60);
                    tui_dispatch_keyboard(scancode);
                }
            }
        }
        else {
            print("error unknown command", 5, 0, 0x0C);
            delay(1000);
        }
    }
}
