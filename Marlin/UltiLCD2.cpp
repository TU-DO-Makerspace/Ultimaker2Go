#include "configuration.h"
#ifdef ENABLE_ULTILCD2
#include "UltiLCD2.h"
#include "UltiLCD2_hi_lib.h"
#include "UltiLCD2_gfx.h"
#include "UltiLCD2_menu_material.h"
#include "UltiLCD2_menu_print.h"
#include "UltiLCD2_menu_first_run.h"
#include "cardreader.h"
#include "ConfigurationStore.h"
#include "temperature.h"
#include "pins.h"

#define SPECIAL_STARTUP

static void lcd_menu_startup();
static void lcd_menu_special_startup();

void lcd_menu_main();
void lcd_menu_material();
static void lcd_menu_maintenance();
static void lcd_menu_maintenance_first_run_select();
static void lcd_menu_maintenance_advanced();
void lcd_menu_maintenance_advanced_heatup();
void lcd_menu_maintenance_advanced_bed_heatup();
static void lcd_menu_advanced_factory_reset();
static void lcd_menu_breakout();
static void lcd_menu_TODO();

void lcd_init()
{
    lcd_lib_init();
    currentMenu = lcd_menu_startup;
}

void lcd_update()
{
    if (!lcd_lib_update_ready()) return;
    lcd_lib_buttons_update();
    
    if (led_glow_dir)
    {
        led_glow-=2;
        if (led_glow == 0) led_glow_dir = 0;
    }else{
        led_glow+=2;
        if (led_glow == 128) led_glow_dir = 1;
    }
    
    if (IsStopped())
    {
        lcd_lib_clear();
        lcd_lib_draw_stringP(15, 10, PSTR("ERROR - STOPPED"));
        switch(StoppedReason())
        {
        case STOP_REASON_MAXTEMP:
        case STOP_REASON_MINTEMP:
            lcd_lib_draw_stringP(15, 20, PSTR("Temp sensor"));
            break;
        case STOP_REASON_MAXTEMP_BED:
            lcd_lib_draw_stringP(15, 20, PSTR("Temp sensor BED"));
            break;
        case STOP_REASON_SAFETY_TRIGGER:
            lcd_lib_draw_stringP(15, 20, PSTR("Safety circuit"));
            break;
        }
        lcd_lib_draw_stringP(1, 40, PSTR("Contact:"));
        lcd_lib_draw_stringP(1, 50, PSTR("support@ultimaker.com"));
        lcd_lib_led_color(led_glow,0,0);
        lcd_lib_update_screen();
    }else{
        currentMenu();
    }
}

void lcd_menu_startup()
{
    lcd_lib_encoder_pos = ENCODER_NO_SELECTION;
    
    lcd_lib_led_color(32,32,40);
    lcd_lib_clear();
    
    if (led_glow < 84)
    {
        lcd_lib_draw_gfx(0, 22, ultimakerTextGfx);
        for(uint8_t n=0;n<10;n++)
        {
            if (led_glow*2 >= n + 20)
                lcd_lib_clear(0, 22+n*2, led_glow*2-n-20, 23+n*2);
            if (led_glow*2 >= n)
                lcd_lib_clear(led_glow*2 - n, 22+n*2, 127, 23+n*2);
            else
                lcd_lib_clear(0, 22+n*2, 127, 23+n*2);
        }
    /*
    }else if (led_glow < 86) {
        led_glow--;
        //lcd_lib_set();
        //lcd_lib_clear_gfx(0, 22, ultimakerTextGfx);
        lcd_lib_draw_gfx(0, 22, ultimakerTextGfx);
    */
    }else{
        led_glow--;
        //lcd_lib_draw_gfx(80, 0, ultimakerRobotGfx);
        //lcd_lib_clear_gfx(0, 22, ultimakerTextOutlineGfx);
        lcd_lib_draw_gfx(0, 22, ultimakerTextGfx);
    }
    lcd_lib_update_screen();

    if (led_glow_dir || lcd_lib_button_pressed)
    {
        led_glow = led_glow_dir = 0;
        if (lcd_lib_button_pressed)
            lcd_lib_beep();

#ifdef SPECIAL_STARTUP
        currentMenu = lcd_menu_special_startup;
#else        
        if (!IS_FIRST_RUN_DONE())
        {
            currentMenu = lcd_menu_first_run_init;
        }else{
            currentMenu = lcd_menu_main;
        }
#endif
    }
}

static void lcd_menu_special_startup()
{
    LED_GLOW();
    
    lcd_lib_clear();
    lcd_lib_draw_gfx(7, 12, specialStartupGfx);
    lcd_lib_draw_stringP(3, 2, PSTR("Welcome"));
    lcd_lib_draw_string_centerP(47, PSTR("To the Ultimaker2"));
    lcd_lib_draw_string_centerP(55, PSTR("experiance!"));
    lcd_lib_update_screen();

    if (lcd_lib_button_pressed)
    {
        if (!IS_FIRST_RUN_DONE())
        {
            lcd_change_to_menu(lcd_menu_first_run_init);
        }else{
            lcd_change_to_menu(lcd_menu_main);
        }
    }
}

void doCooldown()
{
    for(uint8_t n=0; n<EXTRUDERS; n++)
        setTargetHotend(0, n);
    setTargetBed(0);
    fanSpeed = 0;
    
    //quickStop();         //Abort all moves already in the planner
}

void lcd_menu_main()
{
    lcd_tripple_menu(PSTR("PRINT"), PSTR("MATERIAL"), PSTR("MAINTENANCE"));

    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED(0))
        {
            lcd_clear_cache();
            lcd_change_to_menu(lcd_menu_print_select, MENU_ITEM_POS(0));
        }
        else if (IS_SELECTED(1))
            lcd_change_to_menu(lcd_menu_material);
        else if (IS_SELECTED(2))
            lcd_change_to_menu(lcd_menu_maintenance);
    }
    if (lcd_lib_button_down && lcd_lib_encoder_pos == ENCODER_NO_SELECTION)
    {
        led_glow_dir = 0;
        if (led_glow > 200)
            lcd_change_to_menu(lcd_menu_breakout);
    }else{
        led_glow = led_glow_dir = 0;
    }

    lcd_lib_update_screen();
}

void lcd_menu_maintenance()
{
    lcd_tripple_menu(PSTR("CALIBRATE"), PSTR("ADVANCED"), PSTR("RETURN"));

    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED(0))
            lcd_change_to_menu(lcd_menu_maintenance_first_run_select);
        else if (IS_SELECTED(1))
            lcd_change_to_menu(lcd_menu_maintenance_advanced);
        else if (IS_SELECTED(2))
            lcd_change_to_menu(lcd_menu_main);
    }

    lcd_lib_update_screen();
}

static void lcd_menu_maintenance_first_run_select()
{
    lcd_tripple_menu(PSTR("BED"), PSTR("..."), PSTR("RETURN"));

    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED(0))
        {
            lcd_change_to_menu(lcd_menu_first_run_start_bed_leveling);
        }else if (IS_SELECTED(1))
            lcd_change_to_menu(lcd_menu_TODO);
        else if (IS_SELECTED(2))
            lcd_change_to_menu(lcd_menu_maintenance);
    }

    lcd_lib_update_screen();
}

static char* lcd_advanced_item(uint8_t nr)
{
    if (nr == 0)
        strcpy_P(card.longFilename, PSTR("<- RETURN"));
    else if (nr == 1)
        strcpy_P(card.longFilename, PSTR("Heatup head"));
    else if (nr == 2)
        strcpy_P(card.longFilename, PSTR("Heatup bed"));
    else if (nr == 3)
        strcpy_P(card.longFilename, PSTR("Home head"));
    else if (nr == 4)
        strcpy_P(card.longFilename, PSTR("Home bed"));
    else if (nr == 5)
        strcpy_P(card.longFilename, PSTR("Raise bed"));
    else if (nr == 6)
        strcpy_P(card.longFilename, PSTR("Factory reset"));
    else
        strcpy_P(card.longFilename, PSTR("???"));
    return card.longFilename;
}

static void lcd_advanced_details(uint8_t nr)
{
    if (nr == 0)
    {
        
    }else if(nr == 1)
    {
        lcd_lib_draw_stringP(5, 53, PSTR("Heatup the head"));
    }else if(nr == 2)
    {
        lcd_lib_draw_stringP(5, 53, PSTR("Heatup the bed"));
    }else if(nr == 3)
    {
        
    }else if(nr == 4)
    {
        
    }else if(nr == 5)
    {
        
    }else if(nr == 6)
    {
        lcd_lib_draw_stringP(5, 53, PSTR("Clear all settings"));
    }
}

static void lcd_menu_maintenance_advanced()
{
    lcd_scroll_menu(PSTR("ADVANCED"), 7, lcd_advanced_item, lcd_advanced_details);
    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED(0))
            lcd_change_to_menu(lcd_menu_maintenance);
        else if (IS_SELECTED(1))
            lcd_change_to_menu(lcd_menu_maintenance_advanced_heatup, 0);
        else if (IS_SELECTED(2))
            lcd_change_to_menu(lcd_menu_maintenance_advanced_bed_heatup, 0);
        else if (IS_SELECTED(3))
        {
            lcd_lib_beep();
            enquecommand_P(PSTR("G28 X0 Y0"));
        }
        else if (IS_SELECTED(4))
        {
            lcd_lib_beep();
            enquecommand_P(PSTR("G28 Z0"));
        }
        else if (IS_SELECTED(5))
        {
            lcd_lib_beep();
            enquecommand_P(PSTR("G28 Z0"));
            enquecommand_P(PSTR("G1 Z40"));
        }
        else if (IS_SELECTED(6))
            lcd_change_to_menu(lcd_menu_advanced_factory_reset, MENU_ITEM_POS(1));
    }
}

void lcd_menu_maintenance_advanced_heatup()
{
    if (lcd_lib_encoder_pos / ENCODER_TICKS_PER_MENU_ITEM != 0)
    {
        target_temperature[0] = int(target_temperature[0]) + (lcd_lib_encoder_pos / ENCODER_TICKS_PER_MENU_ITEM);
        if (target_temperature[0] < 0)
            target_temperature[0] = 0;
        if (target_temperature[0] > HEATER_0_MAXTEMP - 15)
            target_temperature[0] = HEATER_0_MAXTEMP - 15;
        lcd_lib_encoder_pos = 0;
    }
    if (lcd_lib_button_pressed)
        lcd_change_to_menu(previousMenu, previousEncoderPos);
    
    lcd_lib_clear();
    lcd_lib_draw_string_centerP(20, PSTR("Head temperature:"));
    lcd_lib_draw_string_centerP(53, PSTR("Click to return"));
    char buffer[16];
    int_to_string(int(current_temperature[0]), buffer, PSTR("C/"));
    int_to_string(int(target_temperature[0]), buffer+strlen(buffer), PSTR("C"));
    lcd_lib_draw_string_center(30, buffer);
    lcd_lib_update_screen();
}

void lcd_menu_maintenance_advanced_bed_heatup()
{
    if (lcd_lib_encoder_pos / ENCODER_TICKS_PER_MENU_ITEM != 0)
    {
        target_temperature_bed = int(target_temperature_bed) + (lcd_lib_encoder_pos / ENCODER_TICKS_PER_MENU_ITEM);
        if (target_temperature_bed < 0)
            target_temperature_bed = 0;
        if (target_temperature_bed > BED_MAXTEMP - 15)
            target_temperature_bed = BED_MAXTEMP - 15;
        lcd_lib_encoder_pos = 0;
    }
    if (lcd_lib_button_pressed)
        lcd_change_to_menu(previousMenu, previousEncoderPos);
    
    lcd_lib_clear();
    lcd_lib_draw_string_centerP(20, PSTR("Bed temperature:"));
    lcd_lib_draw_string_centerP(53, PSTR("Click to return"));
    char buffer[16];
    int_to_string(int(current_temperature_bed), buffer, PSTR("C/"));
    int_to_string(int(target_temperature_bed), buffer+strlen(buffer), PSTR("C"));
    lcd_lib_draw_string_center(30, buffer);
    lcd_lib_update_screen();
}

static void doFactoryReset()
{
    //Clear the EEPROM settings so they get read from default.
    eeprom_write_byte((uint8_t*)100, 0);
    eeprom_write_byte((uint8_t*)101, 0);
    eeprom_write_byte((uint8_t*)102, 0);
    eeprom_write_byte((uint8_t*)EEPROM_FIRST_RUN_DONE_OFFSET, 0);
    eeprom_write_byte(EEPROM_MATERIAL_COUNT_OFFSET(), 0);
    
    cli();
    //NOTE: Jumping to address 0 is not a fully proper way to reset.
    // Letting the watchdog timeout is a better reset, but the bootloader does not continue on a watchdog timeout.
    // So we disable interrupts and hope for the best!
    //Jump to address 0x0000
#ifdef __AVR__
    asm volatile(
            "clr	r30		\n\t"
            "clr	r31		\n\t"
            "ijmp	\n\t"
            );
#else
    currentMenu = lcd_menu_startup;
#endif
}

static void lcd_menu_advanced_factory_reset()
{
    lcd_question_screen(NULL, doFactoryReset, PSTR("YES"), previousMenu, NULL, PSTR("NO"));
    
    lcd_lib_draw_string_centerP(10, PSTR("Reset everything"));
    lcd_lib_draw_string_centerP(20, PSTR("to default?"));
    lcd_lib_update_screen();
}

static void lcd_menu_TODO()
{
    lcd_info_screen(previousMenu, NULL, PSTR("OK"));
    
    lcd_lib_draw_string_centerP(20, PSTR("UNIMPLEMENTED"));

    lcd_lib_update_screen();
}

#define BREAKOUT_PADDLE_WIDTH 21
#define ball_x (*(int16_t*)&lcd_cache[3*5])
#define ball_y (*(int16_t*)&lcd_cache[3*5+2])
#define ball_dx (*(int16_t*)&lcd_cache[3*5+4])
#define ball_dy (*(int16_t*)&lcd_cache[3*5+6])
static void lcd_menu_breakout()
{
    if (lcd_lib_encoder_pos == ENCODER_NO_SELECTION)
    {
        lcd_lib_encoder_pos = (128 - BREAKOUT_PADDLE_WIDTH) / 2 / 2;
        for(uint8_t y=0; y<3;y++)
            for(uint8_t x=0; x<5;x++)
                lcd_cache[x+y*5] = 3;
        ball_x = 0;
        ball_y = 57 << 8;
        ball_dx = 0;
        ball_dy = 0;
    }
    
    if (lcd_lib_encoder_pos < 0) lcd_lib_encoder_pos = 0;
    if (lcd_lib_encoder_pos * 2 > 128 - BREAKOUT_PADDLE_WIDTH - 1) lcd_lib_encoder_pos = (128 - BREAKOUT_PADDLE_WIDTH - 1) / 2;
    ball_x += ball_dx;
    ball_y += ball_dy;
    if (ball_x < 1 << 8) ball_dx = abs(ball_dx);
    if (ball_x > 124 << 8) ball_dx = -abs(ball_dx);
    if (ball_y < (1 << 8)) ball_dy = abs(ball_dy);
    if (ball_y < (3 * 10) << 8)
    {
        uint8_t x = (ball_x >> 8) / 25;
        uint8_t y = (ball_y >> 8) / 10;
        if (lcd_cache[x+y*5])
        {
            lcd_cache[x+y*5]--;
            ball_dy = abs(ball_dy);
        }
    }
    if (ball_y > (58 << 8))
    {
        if (ball_x < (lcd_lib_encoder_pos * 2 - 2) << 8 || ball_x > (lcd_lib_encoder_pos * 2 + BREAKOUT_PADDLE_WIDTH) << 8)
            lcd_change_to_menu(lcd_menu_main);
        ball_dx += (ball_x - ((lcd_lib_encoder_pos * 2 + BREAKOUT_PADDLE_WIDTH / 2) * 256)) / 64;
        ball_dy = -512 + abs(ball_dx);
    }
    if (ball_dy == 0)
    {
        ball_y = 57 << 8;
        ball_x = (lcd_lib_encoder_pos * 2 + BREAKOUT_PADDLE_WIDTH / 2) << 8;
        if (lcd_lib_button_pressed)
        {
            ball_dx = -256 + lcd_lib_encoder_pos * 8;
            ball_dy = -512 + abs(ball_dx);
        }
    }
    
    lcd_lib_clear();
    
    for(uint8_t y=0; y<3;y++)
        for(uint8_t x=0; x<5;x++)
        {
            if (lcd_cache[x+y*5])
                lcd_lib_draw_box(3 + x*25, 2 + y * 10, 23 + x*25, 10 + y * 10);
            if (lcd_cache[x+y*5] == 2)
                lcd_lib_draw_shade(4 + x*25, 3 + y * 10, 22 + x*25, 9 + y * 10);
            if (lcd_cache[x+y*5] == 3)
                lcd_lib_set(4 + x*25, 3 + y * 10, 22 + x*25, 9 + y * 10);
        }
    
    lcd_lib_draw_box(ball_x >> 8, ball_y >> 8, (ball_x >> 8) + 2, (ball_y >> 8) + 2);
    lcd_lib_draw_box(lcd_lib_encoder_pos * 2, 60, lcd_lib_encoder_pos * 2 + BREAKOUT_PADDLE_WIDTH, 63);
    lcd_lib_update_screen();
}

/* Warning: This function is called from interrupt context */
void lcd_buttons_update()
{
    lcd_lib_buttons_update_interrupt();
}

#endif//ENABLE_ULTILCD2