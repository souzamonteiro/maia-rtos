#pragma once

namespace storage {
    class StorageResult {
        i32 status;
        string message;

        void set(i32 s, string msg) {
            status = s;
            message = msg;
        }

        i32 isOk() {
            if (status == 0) {
                return 1;
            }
            return 0;
        }

        string getMessage() {
            return message;
        }
    }

    class Device {
        string id;
        string label;
        string mount_point;
        i32 capacity_mb;
        i32 is_mounted;

        void init(string dev_id, string lbl, string mnt, i32 cap, i32 mnt_status) {
            id = dev_id;
            label = lbl;
            mount_point = mnt;
            capacity_mb = cap;
            is_mounted = mnt_status;
        }

        string getId() {
            return id;
        }

        string getLabel() {
            return label;
        }

        i32 isMounted() {
            return is_mounted;
        }
    }

    StorageResult* mount(string deviceId, string mountPoint) {
        StorageResult* r = new StorageResult();
        r->set(0, "Mounted successfully");
        return r;
    }

    StorageResult* unmount(string deviceId) {
        StorageResult* r = new StorageResult();
        r->set(0, "Device safely unmounted");
        return r;
    }

    i32 getDeviceCount() {
        return 2;
    }
}
