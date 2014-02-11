#include "mbed.h"
#include "USBSerial.h"

#include "nunchuck.h"
#include "utils.h"

#define USB_VID         0x1D50
#define USB_PID         0x60A0
#define USB_REV         0x00
#define VERSION         0x0001

USBSerial usbdevice(USB_VID, USB_PID, USB_REV); // vid pid rev
Serial uart(P0_19, P0_18); // tx, rx
SPISlave spidevice(P0_9, P0_8, P0_10, P0_2); // mosi, miso, sck, ssel

BusInOut lcd_d(P1_0, P1_1, P1_2, P1_3, P1_4, P1_5, P1_6, P1_7);
DigitalOut lcd_rs(P0_17);
DigitalOut lcd_rw(P0_10);
DigitalOut lcd_en(P0_20);
DigitalOut lcd_psb(P1_28);
DigitalOut lcd_rst(P1_27);

SPI spimaster(P1_22, P1_21, P1_20); // mosi, miso, sck
DigitalOut spi_ssel(P1_23); // spi port ssel
DigitalOut sd_ssel(P0_7); // sd card ssel
DigitalIn sd_cd(P1_18); // sd card detect
DigitalIn sd_wp(P1_19); // sd card write protect

I2C i2c(P0_5, P0_4); // sda, scl

int nunchuck_rate;

bool encoder_running = false;
int16_t encoder_count = 0;
uint8_t encoder_a_pin;
uint8_t encoder_b_pin;
bool encoder_a_wing;
bool encoder_b_wing;

int processcommand(int c, int d) {
    int r;
    uint32_t t;
//    analogin_t adc;

    switch (c) {
        // System
        case 0x00: // nop
            r = 0x0000;
            break;

        case 0x01: // read usb vid
            r = USB_VID;
            break;
        case 0x02: // read usb pid
            r = USB_PID;
            break;
        case 0x03: // read usb revision
            r = USB_REV;
            break;
        case 0x04: // read version
            r = VERSION;
            break;

        // LCD
        case 0x10: // write lcd data byte
            lcd_d.output();
            lcd_d = d;
            lcd_rs = 1; // 0 cmd, 1 dat
            lcd_rw = 0; // 0 write, 1 read
            lcd_en = 0;
            wait(0.01);
            lcd_en = 1;
            wait(0.01);
            lcd_en = 0;
            r = 0x1000 + d;
            break;
        case 0x11: // read lcd data byte
            lcd_d.input();
            lcd_rs = 1; // 0 cmd, 1 dat
            lcd_rw = 1; // 0 write, 1 read
            lcd_en = 0;
            wait(0.01);
            lcd_en = 1;
            wait(0.01);
            r = 0x1100 + lcd_d;
            lcd_en = 0;
            break;
        case 0x12: // write lcd command byte
            lcd_d.output();
            lcd_d = d;
            lcd_rs = 0; // 0 cmd, 1 dat
            lcd_rw = 0; // 0 write, 1 read
            lcd_en = 0;
            wait(0.01);
            lcd_en = 1;
            wait(0.01);
            lcd_en = 0;
            r = 0x1200 + d;
            break;
        case 0x13: // read lcd command byte
            lcd_d.input();
            lcd_rs = 0; // 0 cmd, 1 dat
            lcd_rw = 1; // 0 write, 1 read
            lcd_en = 0;
            wait(0.01);
            lcd_en = 1;
            wait(0.01);
            r = 0x1300 + lcd_d;
            lcd_en = 0;
            break;
        case 0x14: // reset lcd
            lcd_rst = 1;
            wait(0.01);
            lcd_rst = 0;
            r = 0x1400 + d;
            break;

        // LCD Contrast
        case 0x18: // write lcd contrast dac
            r = 0x1800 + d;
            break;
        case 0x19: // read lcd contrast dac
            r = 0x1900 + d;
            break;

        // I2C
        case 0x20: // write i2c
            r = 0x2000 + i2c.write(d);
            break;
        case 0x21: // read i2c
            r = 0x2100 + i2c.read(d);
            break;
        case 0x22: // start i2c + address
            i2c.start();
            r = 0x2200 + i2c.write(d);
            break;
        case 0x23: // stop i2c
            i2c.stop();
            r = 0x2300;
            break;

        // SPI
        case 0x28: // write spi
            r = 0x2800 + spimaster.write(d);

        // Wing1
        case 0x40: // write wing1
            t = LPC_GPIO->PIN[0];
            t &= 0xFF3E07FF;
            t |= (d & 0x3F) << 11;
            t |= (d & 0xC0) << 16;
            LPC_GPIO->PIN[0] = t;
            r = 0x4000 + d;
            break;
        case 0x41: // read wing1
            t = LPC_GPIO->PIN[0];
            r = 0x4100 + ((t & 0x0001F800) >> 11) + ((t & 0x00C00000) >> 16);
            break;
        case 0x42: // set wing1
            LPC_GPIO->SET[0] = ((d & 0x3F) << 11) + ((d & 0xC0) << 16);
            r = 0x4200 + d;
            break;
        case 0x43: // clear wing1
            LPC_GPIO->CLR[0] = ((d & 0x3F) << 11) + ((d & 0xC0) << 16);
            r = 0x4300 + d;
            break;
        case 0x44: // not wing1
            LPC_GPIO->NOT[0] = ((d & 0x3F) << 11) + ((d & 0xC0) << 16);
            r = 0x4400 + d;
            break;
        case 0x45: // dir wing1
            t = LPC_GPIO->DIR[0];
            t &= 0xFF3E07FF;
            t |= (d & 0x3F) << 11;
            t |= (d & 0xC0) << 16;
            LPC_GPIO->DIR[0] = t;
            r = 0x4500 + d;
            break;
        case 0x46: // masked write wing1
            r = 0x4600 + d;
            break;
        case 0x47: // mask wing1
            r = 0x4700 + d;
            break;
        case 0x48: // configure wing1 bit0 P0_11
            r = write_iocon(P0_11, d);
            break;
        case 0x49: // configure wing1 bit1 P0_12
            r = write_iocon(P0_12, d);
            break;
        case 0x4A: // configure wing1 bit2 P0_13
            r = write_iocon(P0_13, d);
            break;
        case 0x4B: // configure wing1 bit3 P0_14
            r = write_iocon(P0_14, d);
            break;
        case 0x4C: // configure wing1 bit4 P0_15
            r = write_iocon(P0_15, d);
            break;
        case 0x4D: // configure wing1 bit5 P0_16
            r = write_iocon(P0_16, d);
            break;
        case 0x4E: // configure wing1 bit6 P0_22
            r = write_iocon(P0_22, d);
            break;
        case 0x4F: // configure wing1 bit7 P0_23
            r = write_iocon(P0_23, d);
            break;

        // Wing2
        case 0x50: // write wing2
            t = LPC_GPIO->PIN[1];
            t &= 0xFFFF00FF;
            t |= (d & 0xFF) << 8;
            LPC_GPIO->PIN[1] = t;
            r = 0x5000 + d;
            break;
        case 0x51: // read wing2
            r = 0x5100 + ((LPC_GPIO->PIN[1] & 0x0000FF00) >> 8);
            break;
        case 0x52: // set wing2
            LPC_GPIO->SET[1] = (d & 0xFF) << 8;
            r = 0x5200 + d;
            break;
        case 0x53: // clear wing2
            LPC_GPIO->CLR[1] = (d & 0xFF) << 8;
            r = 0x5300 + d;
            break;
        case 0x54: // not wing2
            LPC_GPIO->NOT[1] = (d & 0xFF) << 8;
            r = 0x5400 + d;
            break;
        case 0x55: // dir wing2
            t = LPC_GPIO->DIR[1];
            t &= 0xFFFF00FF;
            t |= (d & 0xFF) << 8;
            LPC_GPIO->DIR[1] = t;
            r = 0x5500 + d;
            break;
        case 0x56: // masked write wing2
            r = 0x5600 + d;
            break;
        case 0x57: // mask wing2
            r = 0x5700 + d;
            break;
        case 0x58: // configure wing2 bit0 P1_8
            r = write_iocon(P1_8, d);
            break;
        case 0x59: // configure wing2 bit1 P1_9
            r = write_iocon(P1_9, d);
            break;
        case 0x5A: // configure wing2 bit2 P1_10
            r = write_iocon(P1_10, d);
            break;
        case 0x5B: // configure wing2 bit3 P1_11
            r = write_iocon(P1_11, d);
            break;
        case 0x5C: // configure wing2 bit4 P1_12
            r = write_iocon(P1_12, d);
            break;
        case 0x5D: // configure wing2 bit5 P1_13
            r = write_iocon(P1_13, d);
            break;
        case 0x5E: // configure wing2 bit6 P1_14
            r = write_iocon(P1_14, d);
            break;
        case 0x5F: // configure wing2 bit7 P1_15
            r = write_iocon(P1_15, d);
            break;

        // Ext Pins
        case 0x60: // write ext
            LPC_GPIO->W0[1] = d & 0x80;
            LPC_GPIO->W0[7] = d & 0x10;
            LPC_GPIO->W0[21] = d & 0x40;
            LPC_GPIO->W1[23] = d & 0x20;
            t = LPC_GPIO->PIN[1];
            t &= 0xFFF0FFFF;
            t |= (d & 0x0F) << 16;
            LPC_GPIO->PIN[1] = t;
            r = 0x6000 + d;
            break;
        case 0x61: // read ext
            t = LPC_GPIO->B0[1] << 7;
            t |= LPC_GPIO->B0[7] << 4;
            t |= LPC_GPIO->B0[21] << 6;
            t |= LPC_GPIO->B1[23] << 5;
            t |= (LPC_GPIO->PIN[1] & 0x000F0000) >> 16;
            r = 0x6100 + t;
            break;
        case 0x62: // set ext
            t = (d & 0x80) >> 6;
            t |= (d & 0x10) << 3;
            t |= (d & 0x40) << 15;
            LPC_GPIO->SET[0] = t;
            t = (d & 0x0F) << 16;
            t |= (d & 0x20) << 18;
            LPC_GPIO->SET[1] = t;
            r = 0x6200 + d;
            break;
        case 0x63: // clear ext
            t = (d & 0x80) >> 6;
            t |= (d & 0x10) << 3;
            t |= (d & 0x40) << 15;
            LPC_GPIO->CLR[0] = t;
            t = (d & 0x0F) << 16;
            t |= (d & 0x20) << 18;
            LPC_GPIO->CLR[1] = t;
            r = 0x6300 + d;
            break;
        case 0x64: // not ext
            t = (d & 0x80) >> 6;
            t |= (d & 0x10) << 3;
            t |= (d & 0x40) << 15;
            LPC_GPIO->NOT[0] = t;
            t = (d & 0x0F) << 16;
            t |= (d & 0x20) << 18;
            LPC_GPIO->NOT[1] = t;
            r = 0x6400 + d;
            break;
        case 0x65: // dir ext
            t = LPC_GPIO->DIR[0] & 0xFFDFFF7D;
            t |= (d & 0x80) >> 6;
            t |= (d & 0x10) << 3;
            t |= (d & 0x40) << 15;
            LPC_GPIO->DIR[0] = t;
            t = LPC_GPIO->DIR[1] & 0xFF70FFFF;
            t |= (d & 0x0F) << 16;
            t |= (d & 0x20) << 18;
            LPC_GPIO->DIR[1] = t;
            r = 0x6500 + d;
            break;
        case 0x66: // masked write ext
            r = 0x6600 + d;
            break;
        case 0x67: // mask ext
            r = 0x6700 + d;
            break;
        case 0x6C: // configure ext bit4 P1_16 DAC_SDA
            r = write_iocon(P1_16, d);
            break;
        case 0x6D: // configure ext bit5 P1_17 DAC_SCL
            r = write_iocon(P1_17, d);
            break;
        case 0x69: // configure ext bit1 P1_18 SD_CD
            r = write_iocon(P1_18, d);
            break;
        case 0x6A: // configure ext bit2 P1_19 SD_WP
            r = write_iocon(P1_19, d);
            break;
        case 0x68: // configure ext bit0 P0_7  SD_SSEL
            r = write_iocon(P0_7, d);
            break;
        case 0x6B: // configure ext bit3 P1_23 SPI_SSEL
            r = write_iocon(P1_23, d);
            break;
        case 0x6E: // configure ext bit6 P0_21 I2C_INT
            r = write_iocon(P0_21, d);
            break;
        case 0x6F: // configure ext bit7 P0_1  ISP
            r = write_iocon(P0_1, d);
            break;

        // ADC (available on Wing1 pins by setting 0x48 - 0x4F bits 2:0 to 0x2)
        case 0x70: // sample + read adc0
            r = read_adc(P0_11);
            break;
        case 0x71: // sample + read adc0
            r = read_adc(P0_12);
            break;
        case 0x72: // sample + read adc0
            r = read_adc(P0_13);
            break;
        case 0x73: // sample + read adc0
            r = read_adc(P0_14);
            break;
        case 0x74: // sample + read adc0
            r = read_adc(P0_15);
            break;
        case 0x75: // sample + read adc0
            r = read_adc(P0_16);
            break;
        case 0x76: // sample + read adc0
            r = read_adc(P0_22);
            break;
        case 0x77: // sample + read adc0
            r = read_adc(P0_23);
            break;

        // PWM (available on pins: W2_5 W2_6 W2_7 W1_6 LCD_R LCD_G LCD_B)
        case 0x78: // write pwm control reg
            r = 0x7800 + pwm_init(d);
            break;
        case 0x79: // write pwm1 / W2_5 / P1_13
            r = 0x7900 + pwm_write(0, d);
            break;
        case 0x7A: // write pwm2 / W2_6 / P1_14
            r = 0x7A00 + pwm_write(1, d);
            break;
        case 0x7B: // write pwm3 / W2_7 / P1_15
            r = 0x7B00 + pwm_write(2, d);
            break;
        case 0x7C: // write pwm5 / W1_6 / P0_22
            r = 0x7C00 + pwm_write(3, d);
            break;
        case 0x7D: // write pwm6 / LCD_R / P1_24
            r = 0x7D00 + pwm_write(4, d);
            break;
        case 0x7E: // write pwm7 / LCD_G / P1_25
            r = 0x7E00 + pwm_write(5, d);
            break;
        case 0x7F: // write pwm8 / LCD_B / P1_26
            r = 0x7F00 + pwm_write(6, d);
            break;

        // other functions

        // nunchuck
        case 0x80: // init nunchuck (write 0 to sample manually, write >0 to set auto sampling speed)
            nunchuck_poll(&i2c);
            nunchuck_rate = (nunchuck_type >= 0) ? d : 0;
            r = nunchuck_type;
            break;
        case 0x81: // read nunchuck device type
            r = nunchuck_type;
            break;
        case 0x82: // read nunchuck byte 1 & 2
            r = (nunchuck_data[0] << 8) + nunchuck_data[1];
            break;
        case 0x83: // read nunchuck byte 3 & 4
            r = (nunchuck_data[2] << 8) + nunchuck_data[3];
            break;
        case 0x84: // read nunchuck byte 5 & 6
            r = (nunchuck_data[4] << 8) + nunchuck_data[5];
            break;
        case 0x85: // sample nunchuck
            r = 0x8500 + nunchuck_read(&i2c);
            break;

        // encoder
        case 0x86: // init encoder (data bits: 7 channel a wing, 6:4 channel a pin, 3 channel b wing, 2:0 channel b pin)
            r = 0x8600;
            encoder_a_wing = (d>>7)&0x01;
            encoder_a_pin = (d>>4)&0x07;
            encoder_b_wing = (d>>3)&0x01;
            encoder_b_pin = d&0x07;
            encoder_running = true;
            break;
        case 0x87: // read encoder (data: 1 to reset counter, 0 to not reset counter)
            r = encoder_count;
            if (d == 1) encoder_count = 0;
            break;

        // 0xDEAD
        default: // unrecognized command
            r = 0xDEAD;
            break;
    }
    return r;
}

char usb_buf[16];
int usb_pos = 0;
void usbdevice_onloop() {
    if(usbdevice.available()) {
        usb_buf[usb_pos] = usbdevice.getc();
        if (usb_buf[usb_pos] == '\n') {
            usb_buf[usb_pos] = '\0';
            usb_pos = 0;
            char * command = strtok(usb_buf, " \t\r\n");
            char * data = strtok(NULL, " \t\r\n");
            if(command != NULL && data != NULL) {
                int c = atoi(command);
                int d = atoi(data);
                int r = processcommand(c, d);
                usbdevice.printf("%d %d\r\n", c, d);
                usbdevice.printf("0x%x 0x%x\r\n", (r & 0xFF00) >> 8, r & 0x00FF);
            }
        }
        else if (usb_pos < 15) {
            usb_pos++;
        }
    }
}

char uart_buf[16];
int uart_pos = 0;
void uartdevice_onloop() {
    if(uart.readable()) {
        uart_buf[uart_pos] = uart.getc();
        if(uart_buf[uart_pos] == '\n') {
            uart_buf[uart_pos] = '\0';
            uart_pos = 0;
            char * command = strtok(uart_buf, " \t\r\n");
            char * data = strtok(NULL, " \t\r\n");
            if(command != NULL && data != NULL) {
                int c = atoi(command);
                int d = atoi(data);
                int r = processcommand(c, d);
                uart.printf("%d %d\r\n", c, d);
                uart.printf("0x%x 0x%x\r\n", (r & 0xFF00) >> 8, r & 0x00FF);
//                    uart.putc((r & 0xFF00) >> 8);
//                    uart.putc(r & 0x00FF);
            }
        }
        else if(uart_pos < 15) {
            uart_pos++;
        }
    }
}

void spidevice_onloop() {
    if(spidevice.receive()) {
        int s = spidevice.read();
        int c = (s & 0xFF00) >> 8;
        int d = s & 0x00FF;
        int r = processcommand(c, d);
        spidevice.reply(r);
    }
}

void nunchuck_onloop() {
    if(nunchuck_rate > 0) {
        if(LPC_GPIO->B0[21]) {
            if(nunchuck_state < 10) {
                nunchuck_reset();
                nunchuck_state++;
            }
            else if(nunchuck_state == 10) {
                nunchuck_poll(&i2c);
                if(nunchuck_type >= 0) {
                    nunchuck_state = 11;
                }
            }
            else if(nunchuck_state < (nunchuck_rate*1000)) {
                nunchuck_state++;
            }
            else {
                nunchuck_state = 11;
                nunchuck_read(&i2c);
                //uart.printf("polling nunchuck... %x %x %x %x %x %x\r\n", nunchuck_data[0], nunchuck_data[1], nunchuck_data[2], nunchuck_data[3], nunchuck_data[4], nunchuck_data[5]);
            }
        }
        else {
            nunchuck_reset();
            nunchuck_state = 0;
        }
    }
}

void encoder_onloop() {
	static int8_t enc_states[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
	static uint8_t old_AB = 0;
    if(encoder_running) {
	    old_AB <<= 2;                   //remember previous state
        old_AB |= (processcommand(encoder_a_wing == 0 ? 0x41 : 0x51, 0x00)>>encoder_a_pin)&0x01;
        old_AB |= ((processcommand(encoder_b_wing == 0 ? 0x41 : 0x51, 0x00)>>encoder_b_pin)&0x01)<<1;
        encoder_count += enc_states[(old_AB&0x0f)];
    }
}

void init_gpio() {
    LPC_IOCON->TDI_PIO0_11 = 0x81;
    LPC_IOCON->TMS_PIO0_12 = 0x81;
    LPC_IOCON->TDO_PIO0_13 = 0x81;
    LPC_IOCON->TRST_PIO0_14 = 0x81;
    LPC_IOCON->SWDIO_PIO0_15 = 0x81;
    LPC_IOCON->PIO0_16 = 0x80;
    LPC_IOCON->PIO0_22 = 0x80;
    LPC_IOCON->PIO0_23 = 0x80;

    LPC_IOCON->PIO1_8 = 0x80;
    LPC_IOCON->PIO1_9 = 0x80;
    LPC_IOCON->PIO1_10 = 0x80;
    LPC_IOCON->PIO1_11 = 0x80;
    LPC_IOCON->PIO1_12 = 0x80;
    LPC_IOCON->PIO1_13 = 0x80;
    LPC_IOCON->PIO1_14 = 0x80;
    LPC_IOCON->PIO1_15 = 0x80;

    LPC_IOCON->PIO1_16 = 0x90;
    LPC_IOCON->PIO1_17 = 0x90;
    LPC_IOCON->PIO1_18 = 0x90;
    LPC_IOCON->PIO1_19 = 0x90;
    LPC_IOCON->PIO0_7 = 0x80;
    LPC_IOCON->PIO1_23 = 0x80;
    LPC_IOCON->PIO0_21 = 0x88;
    LPC_IOCON->PIO0_1 = 0x80;

    LPC_GPIO->DIR[0] = 0x00000080;
    LPC_GPIO->PIN[0] = 0x00000080;
    LPC_GPIO->DIR[1] = 0x00800000;
    LPC_GPIO->PIN[1] = 0x00800000;
}

int main(void) {
//    usbdevice.printf("Smoothiepanel Prototype 3\r\n");
//    usbdevice.printf("firmware version: alpha 0\r\n");

    uart.baud(115200);
    uart.format(8, SerialBase::None, 1);
    uart.printf("Smoothiepanel Prototype 3\r\n");
    uart.printf("firmware version: alpha 0\r\n");

    spidevice.format(16, 0);
    spidevice.frequency(1000000);
    spidevice.reply(0x0000); // preload spi slave with null reply

    init_gpio();

    lcd_d.input();
    lcd_rs = 0; // 0 cmd, 1 dat
    lcd_rw = 0; // 0 write, 1 read
    lcd_en = 0;
    lcd_psb = 1; // 0 serial, 1 parallel
    lcd_rst = 0;
    wait(0.01);
    lcd_rst = 1;
    wait(0.01);
    lcd_rst = 0;

    spimaster.format(8,0);
    spimaster.frequency(1000000);
    spi_ssel = 1;
    sd_ssel = 1;

    i2c.frequency(10000);

    while (true) {
        usbdevice_onloop();
        uartdevice_onloop();
        spidevice_onloop();
        nunchuck_onloop();
        encoder_onloop();
    }
}

