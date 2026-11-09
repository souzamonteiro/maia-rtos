#pragma once

namespace i2c {
    class Bus {
        i32 port;
        i32 is_open;

        void init(i32 p) {
            port = p;
            is_open = 1;
        }

        void close() {
            is_open = 0;
        }

        i32 isOpen() {
            return is_open;
        }

        i32 readRegisterByte(i32 dev_addr, i32 reg_addr) {
            if (dev_addr == 0x6D) {
                if (reg_addr == 0x01) {
                    return 42; // Retorna versão 42 no emulador/teste
                }
            }
            return 0;
        }
    }

    Result<Bus*, i32> open(i32 port) {
        Bus* b = new Bus();
        b->init(port);
        return Ok(b);
    }
}

