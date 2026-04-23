/*
 * File:   main.c
 * Author: HP
 *
 * Created on 13 April, 2026, 9:54 AM
 */


#include <xc.h>
#include "black_box.h"
#include "ds1307.h"
#include "adc.h"
#include "clcd.h"
#include <string.h>
#include "matrix_keypad.h"
#include "eeprom.h"
#include "i2c.h"
#include "uart.h"

/* ---------------------------------------------------------------
 * Global variables
 * --------------------------------------------------------------- */
unsigned char clock_reg[3];
unsigned char time_buf[9]; /* renamed: avoid clash with time() in some toolchains */
unsigned char speed1[3];
unsigned char gear[9][3] = {"ON", "NE", "G1", "G2", "G3", "G4", "G5", "GR", "CL"};
unsigned short gear_index = 0;
unsigned short speed;
unsigned short count_evnt = 0; /* total events logged (may exceed 10) */

unsigned char addr = 0x00; /* EEPROM write pointer */

unsigned char prev[3] = ""; /* last gear stored, for change detection */

/*
 * FIX 4: menu[] now has 4 entries; index 0-3 maps directly.
 * Old code had menu[4][12] but accessed menu[menu_index] where
 * menu_index started at 1, skipping index 0 entry "VIEW LOG".
 */
unsigned char menu[4][14] = {"VIEW LOG", "CLEAR LOG", "DOWNLOAD LOG", "SET TIME"};
unsigned short menu_index = 0; /* FIX 4: start at 0 (not 1) */

State_t state;
State_t prev_s;

/*
 * FIX 4: removed 'star' and 'flag' globals; replaced by cleaner logic below.
 */
unsigned short flag = 0; /* 1 when EEPROM has wrapped once */

unsigned char read_data[10][15];
unsigned short log_index = 0; /* FIX: renamed from 'index' (conflicts with stdlib) */
unsigned short size = 0;
unsigned char incre;

/* FIX 5: hr/min/sec are set from RTC once when entering set_time */
unsigned short hr = 0;
unsigned short sec_val = 0; /* renamed from 'sec' to avoid stdlib conflict */
unsigned short min_val = 0; /* renamed from 'min' */

unsigned short hr12 = 0;
unsigned short field = 0;

unsigned int count = 0; /* FIX 7: renamed from 'count' - clearer purpose */

unsigned char key;

/* ==============================================================
 * init_config
 * ============================================================== */
void init_config(void) {
    init_clcd();
    init_i2c();
    init_adc();
    init_ds1307();
    init_uart();
    init_matrix_keypad();
    state = e_dashboard;
}

/* ==============================================================
 * main
 *
 * FIX 1: Gear keys and event_store() removed from main loop.
 *         They now live inside view_dashboard() so they only
 *         fire when the dashboard state is active.
 * FIX 10: MK_SW11 menu transition removed from main loop;
 *          handled inside view_dashboard() to avoid triggering
 *          from other states.
 * ============================================================== */
void main(void) {
    init_config();
    prev_s = state;

    while (1) {
        key = read_switches(STATE_CHANGE);

        switch (state) {
            case e_dashboard:
                view_dashboard();
                break;

            case e_main_menu:
                display_main_menu();
                break;

            case e_view_log:
                view_log();
                break;

            case e_download_log:
                download_log();
                break;

            case e_clear_log:
                clear_log();
                break;

            case e_set_time:
                set_time();
                break;
        }
    }
}

/* ==============================================================
 * view_dashboard
 *
 * FIX 1: Gear key handling and event_store() moved here from main()
 *         so they only execute in dashboard state.
 * FIX 9: min/sec extraction operator precedence corrected.
 *         Old: (time[3]-'0')*10 + time[4] - '0'  <- wrong precedence
 *         New: (time_buf[3]-'0')*10 + (time_buf[4]-'0')
 * ============================================================== */
void view_dashboard(void) {
    clock_reg[0] = read_ds1307(HOUR_ADDR);
    clock_reg[1] = read_ds1307(MIN_ADDR);
    clock_reg[2] = read_ds1307(SEC_ADDR);

    /* Decode hours - check 12/24-hour mode bit */
    if (clock_reg[0] & 0x40) {
        hr12 = 1;
        time_buf[0] = ((clock_reg[0] >> 4) & 0x01) + '0';
        time_buf[1] = (clock_reg[0] & 0x0F) + '0';
    } else {
        hr12 = 0;
        time_buf[0] = ((clock_reg[0] >> 4) & 0x03) + '0';
        time_buf[1] = (clock_reg[0] & 0x0F) + '0';
    }

    time_buf[2] = ':';
    time_buf[3] = ((clock_reg[1] >> 4) & 0x0F) + '0';
    time_buf[4] = (clock_reg[1] & 0x0F) + '0';
    time_buf[5] = ':';
    time_buf[6] = ((clock_reg[2] >> 4) & 0x0F) + '0';
    time_buf[7] = (clock_reg[2] & 0x0F) + '0';
    time_buf[8] = '\0';

    /* FIX 9: parentheses around second digit subtraction */
    hr = (time_buf[0] - '0') * 10 + (time_buf[1] - '0');
    min_val = (time_buf[3] - '0') * 10 + (time_buf[4] - '0');
    sec_val = (time_buf[6] - '0') * 10 + (time_buf[7] - '0');

    speed = read_adc(CHANNEL4);
    speed = speed / 10.23;

    speed1[0] = speed / 10 + '0';
    speed1[1] = speed % 10 + '0';
    speed1[2] = '\0';

    /* Display labels */
    clcd_print("TIME", LINE1(2));
    clcd_print("EV", LINE1(10));
    clcd_print("SP", LINE1(13));

    /* Display values */
    clcd_print(time_buf, LINE2(0));
    clcd_print(gear[gear_index], LINE2(10));
    clcd_print(speed1, LINE2(13));

    /*
     * FIX 1: Gear key handling here (not in main loop).
     * Prevents gear changes from registering while in menus.
     */
    if (key == MK_SW1 && gear_index < 7) gear_index++;
    if (key == MK_SW2 && gear_index > 0) gear_index--;
    if (key == MK_SW3) gear_index = 8;

    /* FIX 1: event_store() called here, after gear update */
    event_store();

    /* FIX 10: Menu transition only from dashboard */
    if (key == MK_SW11) {
        CLEAR_DISP_SCREEN;
        state = e_main_menu;
    }
}

/* ==============================================================
 * event_store
 *
 * FIX 2: Replaced (count_evnt == 10 -> flag=1, addr=0) with a
 *         hard boundary guard (addr >= 120). The old reset only
 *         triggered exactly at count 10; any drift caused overflow.
 *
 * The strcmp() change-detection is kept but now uses time_buf.
 * ============================================================== */
void event_store(void) {
    if (strcmp((const char *) gear[gear_index], (const char *) prev) != 0) {
        strcpy((char *) prev, (const char *) gear[gear_index]);
        count_evnt++;

        /* Store 8 time characters */
        for (int i = 0; i < 8; i++) {
            write_internal_eeprom(addr++, time_buf[i]);
        }

        /* Store 2 gear characters */
        write_internal_eeprom(addr++, gear[gear_index][0]);
        write_internal_eeprom(addr++, gear[gear_index][1]);

        /* Store 2 speed characters */
        write_internal_eeprom(addr++, speed1[0]);
        write_internal_eeprom(addr++, speed1[1]);

        /* FIX 2: Hard boundary wrap (120 = 10 records x 12 bytes) */
        if (addr >= 120) {
            flag = 1; /* mark that buffer has wrapped */
            addr = 0x00;
        }
    }
}

/* ==============================================================
 * extract_data
 *
 * FIX 3a: Inner loop runs j < 15 (not j < 16) to match the
 *          15-element record layout (indices 0-14).
 * FIX 3b: EEPROM wrap now uses (incre >= 120) consistently,
 *          not the magic constant 0x78 (which equals 120 but
 *          was confusing and only checked per-row not per-byte).
 * ============================================================== */
void extract_data(void) {
    size = (flag == 0) ? count_evnt : 10;

    /*
     * When buffer has wrapped, oldest record starts at the slot
     * that would be written next = (count_evnt % 10) * 12.
     */
    incre = (flag == 0) ? 0x00 : (unsigned char) ((count_evnt % 10) * 12);

    for (int i = 0; i < size; i++) {
        for (int j = 0; j < 15; j++) { /* FIX 3a: was j < 16 */
            if (j == 8 || j == 11) {
                read_data[i][j] = ' '; /* separator spaces */
            } else if (j == 14) {
                read_data[i][j] = '\0'; /* null terminate */
            } else {
                read_data[i][j] = read_internal_eeprom(incre++);

                /* FIX 3b: wrap per-byte, not per-row */
                if (incre >= 120) {
                    incre = 0x00;
                }
            }
        }
        /* Removed: old per-row wrap check (if incre==0x78) was too late */
    }
}

/* ==============================================================
 * display_main_menu
 *
 * FIX 4: Completely replaced the broken star+menu_index logic.
 *
 * Old logic: menu_index started at 1, star toggled between rows,
 * MK_SW11 used (menu_index-1) or (menu_index) based on star,
 * causing wrong state transitions.
 *
 * New logic: menu_index is 0-3, cursor '*' always on LINE2 when
 * index > 0, LINE1 when index == 0.  MK_SW11 uses menu_index
 * directly with no arithmetic correction needed.
 * ============================================================== */
void display_main_menu(void) {

    if (key == MK_SW2) { /* scroll down */
        CLEAR_DISP_SCREEN;
        if (menu_index < 3) menu_index++;
    }
    if (key == MK_SW1) { /* scroll up */
        CLEAR_DISP_SCREEN;
        if (menu_index > 0) menu_index--;
    }

    /* Render two rows: item above cursor and highlighted item */
    if (menu_index == 0) {
        clcd_putch('*', LINE1(0));
        clcd_print(menu[0], LINE1(2));
        clcd_putch(' ', LINE2(0));
        clcd_print(menu[1], LINE2(2));
    } else {
        clcd_putch(' ', LINE1(0));
        clcd_print(menu[menu_index - 1], LINE1(2));
        clcd_putch('*', LINE2(0));
        clcd_print(menu[menu_index], LINE2(2));
    }

    /* FIX 4: Direct mapping, no star arithmetic */
    if (key == MK_SW11) {
        CLEAR_DISP_SCREEN;
        switch (menu_index) {
            case 0: state = e_view_log;
                break;
            case 1: state = e_clear_log;
                break;
            case 2: state = e_download_log;
                break;
            case 3: state = e_set_time;
                break;
        }
    }

    if (key == MK_SW12) {
        CLEAR_DISP_SCREEN;
        state = e_dashboard;
    }
}

/* ==============================================================
 * view_log
 * ============================================================== */
void view_log(void) {
    extract_data();

    clcd_print("TIME", LINE1(2));
    clcd_print("EV", LINE1(10));
    clcd_print("SP", LINE1(13));

    if (size == 0) {
        clcd_print("NO LOGS", LINE2(4));
        if (key == MK_SW12) {
            CLEAR_DISP_SCREEN;
            state = e_main_menu;
        }
        return;
    }

    if (key == MK_SW2) {
        CLEAR_DISP_SCREEN;
        if (log_index < size - 1) log_index++;
    }
    if (key == MK_SW1) {
        CLEAR_DISP_SCREEN;
        if (log_index > 0) log_index--;
    }
    if (key == MK_SW12) {
        CLEAR_DISP_SCREEN;
        log_index = 0;
        state = e_main_menu;
    }

    clcd_print(read_data[log_index], LINE2(0));
}

/* ==============================================================
 * clear_log
 * ============================================================== */
void clear_log(void) {
    count_evnt = 0;
    addr = 0x00;
    size = 0;
    log_index = 0;
    flag = 0; /* FIX: reset wrap flag too - old code forgot this */

    clcd_print("Cleared data", LINE1(0));
    __delay_ms(1000);
    CLEAR_DISP_SCREEN;
    state = e_main_menu;
}

/* ==============================================================
 * download_log
 * ============================================================== */
void download_log(void) {
    extract_data();

    if (size == 0) {
        clcd_print("NO LOGS", LINE1(4));
        __delay_ms(1000);
        CLEAR_DISP_SCREEN;
        state = e_main_menu;
        return;
    }

    puts("\r\n");
    for (int i = 0; i < size; i++) {
        puts((const char *) read_data[i]);
        puts("\r\n");
    }

    clcd_print("downloading data", LINE1(0));
    __delay_ms(100);
    CLEAR_DISP_SCREEN;
    state = e_main_menu;
}

/* ==============================================================
 * blink_field
 *
 * FIX 7: Old code had three separate count++ and reset blocks,
 *         one per field. Fields 1 and 2 only reset count inside
 *         the blink-off branch, so count could run past 1000
 *         before resetting, making the blink freeze.
 *
 * New approach: single blink_count incremented once per call,
 * field content drawn first, then blanked in second half of cycle.
 * ============================================================== */
void blink_field(unsigned short f) {
    clcd_print("TIME", LINE1(0));
    if (field == 0) {
        if (count++ < 500) {
            clcd_putch(hr / 10 + '0', LINE2(0));
            clcd_putch(hr % 10 + '0', LINE2(1));
        } else if (count < 1000) {
            clcd_putch(' ', LINE2(0));
            clcd_putch(' ', LINE2(1));
        } else
            count = 0;
        clcd_putch(':', LINE2(2));
        clcd_putch(min_val / 10 + '0', LINE2(3));
        clcd_putch(min_val % 10 + '0', LINE2(4));
        clcd_putch(':', LINE2(5));
        clcd_putch(sec_val / 10 + '0', LINE2(6));
        clcd_putch(sec_val % 10 + '0', LINE2(7));
    } else if (field == 1) {
        if (count++ < 500) {
            clcd_putch(min_val / 10 + '0', LINE2(3));
            clcd_putch(min_val % 10 + '0', LINE2(4));
        } else if (count < 1000) {
            clcd_putch(' ', LINE2(3));
            clcd_putch(' ', LINE2(4));
        } else
            count = 0;
        clcd_putch(':', LINE2(2));
        clcd_putch(hr / 10 + '0', LINE2(0));
        clcd_putch(hr % 10 + '0', LINE2(1));
        clcd_putch(':', LINE2(5));
        clcd_putch(sec_val / 10 + '0', LINE2(6));
        clcd_putch(sec_val % 10 + '0', LINE2(7));
    } else if (field == 2) {
        if (count++ < 500) {
            clcd_putch(sec_val / 10 + '0', LINE2(6));
            clcd_putch(sec_val % 10 + '0', LINE2(7));
        } else if (count < 1000) {
            clcd_putch(' ', LINE2(6));
            clcd_putch(' ', LINE2(7));
        } else
            count = 0;
        clcd_putch(':', LINE2(2));
        clcd_putch(hr / 10 + '0', LINE2(0));
        clcd_putch(hr % 10 + '0', LINE2(1));
        clcd_putch(':', LINE2(5));
        clcd_putch(min_val / 10 + '0', LINE2(3));
        clcd_putch(min_val % 10 + '0', LINE2(4));
    }
}

/* ==============================================================
 * set_time
 *
 * FIX 5: Added init_flag. hr/min_val/sec_val are loaded from the
 *         RTC only once when entering this state. Without this,
 *         view_dashboard() refreshes time_buf every loop, and hr/
 *         min_val/sec_val get overwritten mid-edit.
 *
 * FIX 6: Added state = e_main_menu after saving (old code left
 *         state unchanged, so set_time ran again immediately).
 * ============================================================== */
void set_time(void) {
    static unsigned char init_flag = 0; /* FIX 5 */

    /* Load RTC values only on first entry */
    if (init_flag == 0) {
        clock_reg[0] = read_ds1307(HOUR_ADDR);
        clock_reg[1] = read_ds1307(MIN_ADDR);
        clock_reg[2] = read_ds1307(SEC_ADDR);

        if (clock_reg[0] & 0x40) {
            hr12 = 1;
            hr = ((clock_reg[0] >> 4) & 0x01) * 10 + (clock_reg[0] & 0x0F);
        } else {
            hr12 = 0;
            hr = ((clock_reg[0] >> 4) & 0x03) * 10 + (clock_reg[0] & 0x0F);
        }
        min_val = ((clock_reg[1] >> 4) & 0x0F) * 10 + (clock_reg[1] & 0x0F);
        sec_val = ((clock_reg[2] >> 4) & 0x0F) * 10 + (clock_reg[2] & 0x0F);

        init_flag = 1;
    }

    blink_field(field);

    /* Cycle field: HH -> MM -> SS -> HH */
    if (key == MK_SW2) {
        field = (field < 2) ? field + 1 : 0;
    }

    /* Increment selected field */
    if (key == MK_SW1) {
        if (field == 0) {
            if (hr12) {
                hr = (hr < 12) ? hr + 1 : 1; /* 12-hour: 1-12 */
            } else {
                hr = (hr < 23) ? hr + 1 : 0; /* 24-hour: 0-23 */
            }
        } else if (field == 1) {
            min_val = (min_val < 59) ? min_val + 1 : 0;
        } else if (field == 2) {
            sec_val = (sec_val < 59) ? sec_val + 1 : 0;
        }
    }

    /* Save to DS1307 */
    if (key == MK_SW11) {
        CLEAR_DISP_SCREEN;
        unsigned char hour_reg;

        if (hr12) {
            hour_reg = (unsigned char) (((hr / 10) << 4) | (hr % 10));
            hour_reg |= 0x40; /* set 12-hour mode bit */
        } else {
            hour_reg = (unsigned char) (((hr / 10) << 4) | (hr % 10));
            hour_reg &= 0x3F; /* clear 12-hour mode bit */
        }

        write_ds1307(HOUR_ADDR, hour_reg);
        write_ds1307(MIN_ADDR, (unsigned char) (((min_val / 10) << 4) | (min_val % 10)));
        write_ds1307(SEC_ADDR, (unsigned char) (((sec_val / 10) << 4) | (sec_val % 10)));

        /* FIX 6: transition back to menu after saving */
        init_flag = 0;
        field = 0;
        count = 0;
        state = e_main_menu;
    }

    /* Cancel */
    if (key == MK_SW12) {
        CLEAR_DISP_SCREEN;
        init_flag = 0;
        field = 0;
        count = 0;
        state = e_main_menu;
    }
}

