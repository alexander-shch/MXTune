# Troubleshooting

## Plugin fails to load or shows as "incompatible"

**DAW cache** — Most DAWs (Logic, Reaper, Ableton) cache failed plugin scans. After installing or updating MXTune, use **"Clear Cache and Rescan"** in your DAW's plugin preferences.

**macOS — "Developer cannot be verified"** — Go to **System Settings > Privacy & Security** and click **"Allow Anyway"** next to the MXTune entry.

**macOS — signature or dependency errors** — Run the Audio Unit validator in a terminal:
```bash
auval -v aufx MXTn Manu
```
This reports any code signature or missing dependency issues.

**Linux — missing libraries** — The Linux build is dynamically linked. Ensure the required runtime libraries are installed:
```bash
sudo apt install libfftw3-3 libsoundtouch1 libaubio5 librubberband2
```
