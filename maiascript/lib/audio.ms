#pragma once

namespace audio {
    namespace Sound {
        const i32 Click = 1;
        const i32 Notification = 2;
        const i32 Alert = 3;
        const i32 Chime = 4;
    }

    namespace Channel {
        const i32 Master = 0;
        const i32 System = 1;
        const i32 Media = 2;
        const i32 Voice = 3;
    }

    i32 g_system_volume = 0;
    i32 g_last_sound = 0;

    Result<i32, i32> playSystemSound(i32 soundId) {
        g_last_sound = soundId;
        return Ok(soundId);
    }

    Result<i32, i32> setVolume(i32 channel, i32 volume) {
        if (channel == Channel.System) {
            g_system_volume = volume;
        }
        return Ok(volume);
    }

    i32 getVolume(i32 channel) {
        if (channel == Channel.System) {
            return g_system_volume;
        }
        return 0;
    }

    i32 getLastSound() {
        return g_last_sound;
    }
}

