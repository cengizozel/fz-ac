# fz-ac

Universal AC remote for Flipper Zero. Instead of supporting one AC brand, it learns any AC's remote over IR, stores multiple ACs, and can press buttons on a schedule.

UI design based on [flipperzero-mitsubishi-ac-remote](https://github.com/achistyakov/flipperzero-mitsubishi-ac-remote) by Anton Chistyakov.

## Features

- Learn flow: capture each button (OFF, MODE, TEMP +, TEMP -, FAN, VANE) from the AC's original remote, one at a time. Any button can be skipped or redone.
- Multiple ACs: each AC is saved as a standard .ir file in `/ext/apps_data/fz_ac/`, so the files also work with the stock Infrared app.
- Remote screen styled like the Mitsubishi remote app, with the AC's name as the title.
- Alarms: schedule a button press per AC (for example turn the AC on at 07:00 and off at 08:30), once or daily.

## Notes and limitations

- Flipper apps cannot run in the background. Alarms only fire while the app is open. Leave the app running; the screen turning off is fine.
- Learned buttons replay the exact captured signal. AC remotes send their full state with every press, so TEMP + always replays the state the original remote had when you learned it. To make an alarm turn the AC on with a specific mode and temperature, set the original remote to that config, learn a button while it is set that way, and point the alarm at that button.
- The temperature number on the remote screen is a visual counter, not the AC's real state.

## Build and install

Requires [ufbt](https://github.com/flipperdevices/flipperzero-ufbt) and a Flipper Zero over USB:

```
ufbt        # build only
ufbt launch # build, install and run on the Flipper
```

## Usage

1. Add AC, give it a name, then press each button on the original remote when prompted.
2. Pick the AC from the main menu to open its remote.
3. Alarms: add an alarm, pick the AC, button, time, and once or daily.
4. Manage ACs: re-learn, rename, delete.

## Credits

Button panel view and icon assets from [flipperzero-mitsubishi-ac-remote](https://github.com/achistyakov/flipperzero-mitsubishi-ac-remote) by Anton Chistyakov, MIT license.
