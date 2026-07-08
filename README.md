# fz-ac

Universal AC remote for Flipper Zero. Instead of supporting one AC brand, it learns any AC's remote over IR, stores multiple ACs, and can press buttons on a schedule.

UI design based on [flipperzero-mitsubishi-ac-remote](https://github.com/achistyakov/flipperzero-mitsubishi-ac-remote) by Anton Chistyakov.

## Two remote types

AC remotes transmit their complete state (power, mode, temperature, fan, vane) with every button press. A recorded button is therefore a snapshot of one full config, not an incremental step. The app offers two ways to deal with that:

### Smart AC (recommended for real ACs)

Learns temperature sweeps. When adding a preset you set your remote to one config (say cool, quiet fan) at its lowest temperature, then press its TEMP + once per step while the Flipper captures every frame. Each capture is labeled with its real temperature.

- A preset is one mode+fan+vane combo you name: Cool, Heat, Cool Turbo, ...
- On the remote screen, MODE cycles presets and TEMP +/- moves through the captured temperatures, sending the exact frame for the shown temp. The number on screen is the AC's real target temperature.
- OFF is captured once per AC.
- About 17 remote presses per preset.

### Simple remote (6 buttons)

Captures one signal per button (OFF, MODE, TEMP +, TEMP -, FAN, VANE) and replays it. Fits devices with stateless discrete commands (fans, some window units, TV-style gear). On a real AC each button replays the frozen state it was recorded with, and the temperature number is only a cosmetic counter.

## Features

- Multiple ACs, each saved as a standard .ir file in `/ext/apps_data/fz_ac/` (also usable with the stock Infrared app; smart signals are named like `Cool 24`).
- Remote screen styled like the Mitsubishi remote app, with the AC's name as the title.
- Alarms: once or daily per AC. For a smart AC an alarm targets a real config ("07:00 Cool 24 Bedroom") or OFF; for a simple remote it presses a learned button.
- Manage ACs: add or delete presets, re-learn, rename, delete.

## Notes and limitations

- Flipper apps cannot run in the background. Alarms only fire while the app is open. Leave the app running; the screen turning off is fine.
- Signals are loaded from the SD card one at a time, so large captures do not fill RAM.
- Keep preset names short (about 6 characters show on the remote screen).

## Build and install

Requires [ufbt](https://github.com/flipperdevices/flipperzero-ufbt) and a Flipper Zero over USB:

```
ufbt        # build only
ufbt launch # build, install and run on the Flipper
```

## Usage

1. Add AC, name it, pick the type.
2. Smart AC: capture OFF, then add presets by sweeping temperatures as prompted.
3. Pick the AC from the main menu to open its remote.
4. Alarms: add an alarm, pick the AC, action (preset + temp or OFF), time, once or daily.

## Credits

Button panel view and icon assets from [flipperzero-mitsubishi-ac-remote](https://github.com/achistyakov/flipperzero-mitsubishi-ac-remote) by Anton Chistyakov, MIT license.
