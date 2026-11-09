#pragma once

namespace wifi {
    namespace Auth {
        const i32 Open = 0;
        const i32 WEP = 1;
        const i32 WPA_PSK = 2;
        const i32 WPA2_PSK = 3;
        const i32 WPA_WPA2_PSK = 4;
        const i32 WPA3_PSK = 5;
    }

    class Network {
        string ssid;
        i32 rssi;
        i32 auth_mode;

        void set(string s, i32 r, i32 a) {
            ssid = s;
            rssi = r;
            auth_mode = a;
        }

        string getSsid() {
            return ssid;
        }

        i32 getRssi() {
            return rssi;
        }
    }

    class ScanResult {
        i32 count;
        Network* net0;
        Network* net1;
        Network* net2;

        void clear() {
            count = 0;
        }

        void add(Network* n) {
            if (count == 0) {
                net0 = n;
            } else if (count == 1) {
                net1 = n;
            } else if (count == 2) {
                net2 = n;
            }
            count = count + 1;
        }

        i32 getCount() {
            return count;
        }

        Network* get(i32 idx) {
            if (idx == 0) {
                return net0;
            }
            if (idx == 1) {
                return net1;
            }
            if (idx == 2) {
                return net2;
            }
            return 0;
        }
    }

    i32 scan(ScanResult* res) {
        if (res == 0) {
            return 0 - 1;
        }
        res->clear();

        Network* n1 = new Network();
        n1->set("MaiaMesh-HQ", 0 - 45, Auth.WPA2_PSK);
        res->add(n1);

        Network* n2 = new Network();
        n2->set("M5Tab5-Lab", 0 - 68, Auth.WPA2_PSK);
        res->add(n2);

        return res->getCount();
    }

    i32 isConnected() {
        return 1;
    }
}
