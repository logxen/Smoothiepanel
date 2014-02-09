#define DEVICE_NUNCHUCK             0x0000
#define DEVICE_CLASSIC              0x0101
#define DEVICE_INACT_WMP            0x0005
#define DEVICE_WMP                  0x0405
#define DEVICE_WMP_NUNCHUCK         0x0505
#define DEVICE_WMP_CLASSIC          0x0705
#define NUNCHUCK                    0xA4

int nunchuck_state = 0;
int nunchuck_type = -1;
char nunchuck_data[6];
//bool data_ready = false;

void nunchuck_init(I2C *i2c){
    char cmd[2];
//    data_ready = false;

    cmd[0] = 0xF0; // first two writes are a magic init sequence
    cmd[1] = 0x55;
    i2c->write(NUNCHUCK, cmd, 2);
    wait_ms(10);
    cmd[0] = 0xFB;
    cmd[1] = 0x00;
    i2c->write(NUNCHUCK, cmd, 2);
    wait_ms(10);
    cmd[0] = 0xFA; // read out the device type
    i2c->write(NUNCHUCK, cmd, 1, false);
    int res = i2c->read(NUNCHUCK, nunchuck_data, 6);
    cmd[0] = 0x00; // request first sensor readings
    i2c->write(NUNCHUCK, cmd, 1);
    if(res == 0 && nunchuck_data[2] == NUNCHUCK){
        nunchuck_type = (nunchuck_data[4] << 8) + nunchuck_data[5];
    }else{
        nunchuck_type = -1;
    }
}

void nunchuck_reset() {
    nunchuck_type = -1;
    for(int i=0;i<6;i++) {
        nunchuck_data[i] = 0x00;
    }
}

int nunchuck_read(I2C *i2c) {
    char cmd = 0x00;
    int res = i2c->read(NUNCHUCK, nunchuck_data, 6); // read sensors
//    uart.printf("%x %x %x %x %x %x\r\n", nunchuck_data[0], nunchuck_data[1], nunchuck_data[2], nunchuck_data[3], nunchuck_data[4], nunchuck_data[5]);
    i2c->write(NUNCHUCK, &cmd, 1); // request next sensor readings
    return res;
}

void nunchuck_poll(I2C *i2c) {
//    data_ready = false;
    if(nunchuck_type < 0) {
        nunchuck_init(i2c);
        wait_ms(100);
    }
    // if there is a connected device read it and if it responds right parse the data
    if(nunchuck_type >= 0 && nunchuck_read(i2c) == 0) {
//        data_ready = true;
//        switch(nunchuck_type) {
//        case DEVICE_NUNCHUCK:       parse_nunchuck(); break;
//        case DEVICE_CLASSIC:        parse_classic(); break;
//        case DEVICE_INACT_WMP:      init_wmp(); break;
//        case DEVICE_WMP:            parse_wmp(); break;
//        default:
//            break;
//        }
    }
}

