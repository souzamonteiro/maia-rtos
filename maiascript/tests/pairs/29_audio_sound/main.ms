import maia.audio;

i32 main() {
    audio.playSystemSound(audio.Sound.Notification)?;
    audio.setVolume(audio.Channel.System, 40)?;

    if (audio.getVolume(audio.Channel.System) == 40) {
        if (audio.getLastSound() == audio.Sound.Notification) {
            return 40 + audio.getLastSound(); // 40 + 2 = 42
        }
    }
    return 0;
}
