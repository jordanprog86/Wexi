# Wexi
Simple Audio Converter Based on Qt6 and FFMPEG
<img width="1202" height="914" alt="Wexi1" src="https://github.com/user-attachments/assets/0880b866-32ab-44a7-9190-590baa427935" />


## Features

- Convert between many formats supported by your local `ffmpeg` build.
- Detect available output containers and audio encoders automatically.
- Change encoder settings:
  - codec (`-c:a`)
  - bitrate (`-b:a`)
  - sample rate (`-ar`)
  - channels (`-ac`)
  - additional custom ffmpeg arguments
- Live conversion log and cancel support.

## Requirements

- Qt 6.0+ (Quick + QuickControls2)
- CMake 3.+
- `ffmpeg` installed and available in your PATH 
- (optional) `ffprobe` in PATH

## Build (Windows example)

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.7.2\msvc2019_64"
cmake --build build --config Release
```

Run:

```powershell
.\build\Release\AudioConverter.exe
```

## Notes

- Supported formats/codecs depend on how your local `ffmpeg` was compiled.
- You can pass advanced options in **Extra ffmpeg args** (for example filters or quality tuning).

## Test
Download the zip file and launch the programm to test IT
