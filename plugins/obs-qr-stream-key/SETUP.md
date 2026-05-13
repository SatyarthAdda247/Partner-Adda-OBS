# Setup Guide for OBS QR Stream Key Plugin

This guide will walk you through setting up and building the QR authentication plugin for OBS Studio.

## Prerequisites

### All Platforms
- CMake 3.28 or later
- Qt 6
- OpenSSL
- Git

### Windows
- Visual Studio 2022
- Windows SDK 10.0.22621.0 or later

### macOS
- Xcode 14 or later
- macOS 12.0 or later

### Linux
- GCC or Clang
- Development packages for Qt6, OpenSSL

## Step 1: Download cpp-httplib

The plugin requires the cpp-httplib header-only library for HTTP requests.

### Option A: Using the Download Script

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

### Option B: Manual Download

Download `httplib.h` from:
https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h

Save it to: `obs-studio/plugins/obs-qr-stream-key/httplib.h`

## Step 2: Verify Plugin Files

Ensure all these files exist in `obs-studio/plugins/obs-qr-stream-key/`:

```
obs-qr-stream-key/
├── CMakeLists.txt
├── obs-qr-stream-key.cpp
├── adda-login-client.hpp
├── adda-login-client.cpp
├── qr-relay-client.hpp
├── qr-relay-client.cpp
├── auth-selection-dialog.hpp
├── auth-selection-dialog.cpp
├── qr-dialog.hpp
├── qr-dialog.cpp
├── httplib.h                    ← Must be downloaded
├── README.md
└── SETUP.md
```

## Step 3: Build OBS Studio with the Plugin

### Windows

```cmd
cd obs-studio
cmake --preset windows-x64
cmake --build --preset windows-x64 --config RelWithDebInfo
```

The plugin will be built to:
`obs-studio/build_x64/plugins/obs-qr-stream-key/RelWithDebInfo/obs-qr-stream-key.dll`

### macOS

```bash
cd obs-studio
cmake --preset macos
cmake --build build_macos --config RelWithDebInfo
```

The plugin will be built to:
`obs-studio/build_macos/plugins/obs-qr-stream-key/RelWithDebInfo/obs-qr-stream-key.so`

### Linux

```bash
cd obs-studio
cmake --preset ubuntu
cmake --build build_ubuntu --config RelWithDebInfo
```

The plugin will be built to:
`obs-studio/build_ubuntu/plugins/obs-qr-stream-key/obs-qr-stream-key.so`

## Step 4: Run OBS Studio

### Windows
```cmd
cd obs-studio\build_x64\rundir\RelWithDebInfo\bin\64bit
obs64.exe
```

### macOS
```bash
open obs-studio/build_macos/UI/RelWithDebInfo/OBS.app
```

### Linux
```bash
obs-studio/build_ubuntu/rundir/RelWithDebInfo/bin/64bit/obs
```

## Step 5: Verify Plugin Installation

1. Launch OBS Studio
2. You should see an authentication dialog popup automatically
3. Alternatively, check **Tools → QR Authentication** in the menu

## Troubleshooting

### Build Errors

**Error: `httplib.h: No such file or directory`**
- Solution: Download httplib.h using the scripts in Step 1

**Error: `OpenSSL not found`**
- Windows: Install OpenSSL from https://slproweb.com/products/Win32OpenSSL.html
- macOS: `brew install openssl`
- Linux: `sudo apt-get install libssl-dev`

**Error: `Qt6 not found`**
- Ensure Qt 6 is installed and CMAKE_PREFIX_PATH is set correctly
- Windows: Set Qt6_DIR environment variable
- macOS: `brew install qt6`
- Linux: `sudo apt-get install qt6-base-dev`

### Runtime Errors

**Plugin not loading:**
1. Check OBS logs: Help → Log Files → View Current Log
2. Look for lines containing `[obs-qr-stream-key]`
3. Verify all dependencies (Qt6, OpenSSL) are available

**QR code not generating:**
1. Check internet connection
2. Verify firewall allows HTTPS connections to:
   - admin.adda247.com
   - obs-backend-chi.vercel.app
3. Check OBS logs for HTTP errors

**Authentication not working:**
1. Ensure mobile app is scanning the correct QR code
2. Verify backend servers are accessible
3. Check that session hasn't expired (2-minute timeout)

### Network Issues

**Testing Backend Connectivity:**

```bash
# Test Adda247 backend
curl -X POST https://admin.adda247.com/api/users/qr-login/generate

# Test Vercel backend
curl -H "x-auth-token: fpoa43edty5" https://obs-backend-chi.vercel.app/api/session?token=test
```

## Development Tips

### Debugging

**Enable Debug Logging:**
Add to your OBS startup:
```bash
obs --verbose --log-level debug
```

**Qt Creator Setup:**
1. Open `obs-studio/CMakeLists.txt` in Qt Creator
2. Configure with appropriate preset
3. Set breakpoints in plugin code
4. Run/Debug OBS from Qt Creator

**Visual Studio Setup:**
1. Open `obs-studio/build_x64/obs-studio.sln`
2. Set `obs64` as startup project
3. Set breakpoints in plugin code
4. Press F5 to debug

### Code Changes

After modifying plugin code:

```bash
# Rebuild just the plugin
cmake --build build_x64 --target obs-qr-stream-key --config RelWithDebInfo

# Or rebuild everything
cmake --build build_x64 --config RelWithDebInfo
```

### Testing Without Mobile App

You can test the HTTP endpoints directly:

**Adda247 Generate QR:**
```bash
curl -X POST https://admin.adda247.com/api/users/qr-login/generate \
  -H "accept: application/json" \
  -H "content-length: 0"
```

**Vercel Register Session:**
```bash
curl -X POST https://obs-backend-chi.vercel.app/api/session \
  -H "x-auth-token: fpoa43edty5" \
  -H "Content-Type: application/json" \
  -d '{"token":"test-uuid","facultyId":"150"}'
```

## Customization

### Changing Polling Interval

Edit the timer intervals in the client constructors:

```cpp
// adda-login-client.cpp
pollTimer_->setInterval(2000);  // 2 seconds

// qr-relay-client.cpp
pollTimer_->setInterval(2000);  // 2 seconds
```

### Changing Backend URLs

Edit the constants in the client headers:

```cpp
// adda-login-client.hpp
static constexpr const char *kHost = "admin.adda247.com";

// qr-relay-client.cpp
static constexpr const char *kRelayHost = "obs-backend-chi.vercel.app";
```

### Disabling Startup Dialog

Comment out this line in `obs-qr-stream-key.cpp`:

```cpp
// obs_frontend_add_event_callback(onFrontendEvent, nullptr);
```

## Uninstalling

To remove the plugin:

1. Delete the plugin file:
   - Windows: `obs-plugins/64bit/obs-qr-stream-key.dll`
   - macOS: `obs-plugins/obs-qr-stream-key.so`
   - Linux: `obs-plugins/obs-qr-stream-key.so`

2. Or rebuild OBS without the plugin:
   - Remove `add_obs_plugin(obs-qr-stream-key)` from `plugins/CMakeLists.txt`
   - Rebuild OBS

## Support

For issues:
1. Check OBS logs first
2. Verify all dependencies are installed
3. Test backend connectivity
4. Check GitHub issues for similar problems

## Next Steps

- Read [README.md](README.md) for usage instructions
- Check the API documentation for backend integration
- Customize the UI dialogs for your branding
- Add additional authentication methods

## License

This plugin is licensed under GPL v2+, same as OBS Studio.
