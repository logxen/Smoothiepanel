#define LPC_IOCON0_BASE (LPC_IOCON_BASE)
#define LPC_IOCON1_BASE (LPC_IOCON_BASE + 0x60)
__IO uint32_t *get_reg(PinName pin) {
    if (pin == (PinName)NC) return NULL;
    uint32_t pin_number = (uint32_t)pin;

    return (pin_number < 32) ?
            (__IO uint32_t*)(LPC_IOCON0_BASE + 4 * pin_number) :
            (__IO uint32_t*)(LPC_IOCON1_BASE + 4 * (pin_number - 32));
}

uint16_t write_iocon(PinName pin, uint32_t d) {
    if (pin == (PinName)NC) return 0;
    __IO uint32_t *reg = get_reg(pin);

    uint32_t t = *reg & 0xFFFFFF00;
    t |= d & 0xFF;
    *reg = t;
    return (uint16_t)t;
}

uint16_t read_adc(PinName pin) {
    if (pin == (PinName)NC) return 0;
    analogin_t adc;
    __IO uint32_t *reg = get_reg(pin);

    uint32_t t = *reg;
    analogin_init(&adc, P0_11);
    uint16_t r = analogin_read_u16(&adc);
    *reg = t;
    return r;
}

uint8_t pwm_state = 0x00;
PwmOut *pwm_pins[8];
uint16_t pwm_init(uint8_t reg) {
    for(int i=0;i<7;i++) {
        if( ((reg>>i)&0x01)==0x01 && ((pwm_state>>i)&0x01)==0x00 ) {
            pwm_state |= 0x01<<i;
            PwmOut *pin;
            switch(i) {
            case 0:
                pin = new PwmOut(P1_13);
                break;
            case 1:
                pin = new PwmOut(P1_14);
                break;
            case 2:
                pin = new PwmOut(P1_15);
                break;
            case 3:
                pin = new PwmOut(P0_22);
                break;
            case 4:
                pin = new PwmOut(P1_24);
                break;
            case 5:
                pin = new PwmOut(P1_25);
                break;
            case 6:
                pin = new PwmOut(P1_26);
                break;
            default:
                pin = NULL;
                break;
            }
            pin->period_us(100);
            pin->pulsewidth_us(0);
            pwm_pins[i] = pin;
        }
    }
    return pwm_state;
}

uint16_t pwm_write(int pwm, uint8_t d) {
    if((pwm_state>>pwm)&0x01) {
        *pwm_pins[pwm] = (float)d / 255;
    }
    return d;
}

