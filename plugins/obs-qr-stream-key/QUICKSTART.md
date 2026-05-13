# Quick Start Guide

Get the QR authentication plugin running in 5 minutes!

## 1. Download httplib.h

**Windows:**
```cmd
cd obs-studio\plugins\obs-qr-stream-key
download-httplib.bat
```

**macOS/Linux:**
```bash
cd obs-studio/plugins/obs-qr-stream-key
chmod +x download-httplib.sh
./download-httplib.sh
```

## 2. Build OBS

**Windows:**
```cmd
cd obs-studio
cmake --preset windows-x64
cmake --build --preset windows-x64
```

**macOS:**
```bash
cd obs-studio
cmake --preset macos
cmake --build build_macos
```

**Linux:**
```bash
cd obs-studio
cmake --preset ubuntu
cmake --build build_ubuntu
```

## 3. Run OBS

Launch OBS from the build directory and you'll see the authentication dialog!

## What You'll See

1. **On Startup**: A dialog with two buttons:
   - 🟢 Admin Login (Adda247)
   - 🔴 YouTube Live Stream

2. **After Clicking**: A QR code to scan with your mobile app

3. **After Scanning**: Automatic stream configuration!

## Need Help?

- Full setup: [SETUP.md](SETUP.md)
- Usage guide: [README.md](README.md)
- Check OBS logs: Help → Log Files → View Current Log

## Common Issues

**httplib.h not found?**
→ Run the download script in step 1

**OpenSSL errors?**
→ Install OpenSSL for your platform

**Plugin not showing?**
→ Check Tools → QR Authentication menu
