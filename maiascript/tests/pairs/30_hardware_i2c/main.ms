import maia.hardware.i2c;

i32 g_closed = 0;

void closeBus(i2c.Bus* b) {
    b->close();
    g_closed = 1;
}

i32 main() {
    i2c.Bus* bus = i2c.open(0)?;
    defer closeBus(bus);

    i32 ver = bus->readRegisterByte(0x6D, 0x01);

    if (g_closed == 1) {
        delete bus;
        return ver; // Retorna 42
    }
    delete bus;
    return ver;
}
