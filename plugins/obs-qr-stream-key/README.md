# OBS QR Stream Key Plugin

This plugin adds QR-based authentication for OBS Studio, supporting two authentication methods:

1. **Admin Login (Adda247 Backend)** - Authenticate faculty through the Adda247 admin system
2. **YouTube Live (Vercel Backend)** - Receive stream keys via a relay server for YouTube streaming

## Features

- **Startup Dialog**: Automatically shows authentication options when OBS starts
- **QR Code Display**: Shows QR codes that can be scanned with a mobile app
- **Real-time Polling**: Continuously polls for authentication confirmation
- **Automatic Configuration**: Automatically applies stream keys to OBS settings
- **Tools Menu Integration**: Accessible via Tools → QR Authentication

## Dependencies

This plugin requires:
- Qt 6 (Core, Widgets, Network)
- OpenSSL
- cpp-httplib (header-only HTTP library)

## Installation

### 1. Download cpp-httplib

Download the `httplib.h` file from: https://github.com/yhirose/cpp-httplib

Place it in this directory: `obs-studio/plugins/obs-qr-stream-key/httplib.h`

```bash
cd obs-studio/plugins/obs-qr-stream-key
curl -O https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h
```

### 2. Build OBS with the Plugin

The plugin will be automatically built with OBS Studio if it's in the plugins directory.

```bash
# Windows
cmake --preset windows-x64
cmake --build --preset windows-x64

# macOS
cmake --preset macos
cmake --build build_macos

# Linux
cmake --preset ubuntu
cmake --build build_ubuntu
```

## Usage

### On OBS Startup

1. When OBS starts, a dialog will appear with two options:
   - **Admin Login (Adda247)** - Green button
   - **YouTube Live Stream** - Red button

2. Click your preferred authentication method

### Admin Login Flow

1. Click "Admin Login (Adda247)"
2. A QR code will be displayed
3. Scan the QR code with the Adda247 mobile app
4. Confirm the login on your mobile device
5. OBS will automatically configure your stream settings

### YouTube Live Flow

1. Click "YouTube Live Stream"
2. Enter your Faculty ID (default: 150)
3. A QR code with the pairing URL will be displayed
4. Scan the QR code with your mobile app
5. The app will send the stream key to the relay server
6. OBS will automatically configure your YouTube stream

### Manual Access

You can also access the authentication dialog anytime via:
**Tools → QR Authentication**

## API Endpoints

### Adda247 Backend (admin.adda247.com)

**Generate QR:**
- Method: POST
- Path: `/api/users/qr-login/generate`
- Response: `{ "success": true, "sessionId": "...", "qrData": "..." }`

**Poll Status:**
- Method: GET
- Path: `/api/users/qr-login/status/{sessionId}?t={timestamp}`
- Response: `{ "success": true, "status": "confirmed", "authData": {...} }`

### Vercel Relay Backend (obs-backend-chi.vercel.app)

**Register Session:**
- Method: POST
- Path: `/api/session`
- Headers: `x-auth-token: fpoa43edty5`
- Body: `{ "token": "UUID", "facultyId": "150" }`
- Response: `{ "success": true, "data": { "pairUrl": "..." } }`

**Poll Session:**
- Method: GET
- Path: `/api/session?token={UUID}`
- Headers: `x-auth-token: fpoa43edty5`
- Response: `{ "success": true, "data": { "streamKey": "...", "classId": "...", "sceneCollection": "..." } }`

## Architecture

```
┌─────────────────────────────────────┐
│   OBS Frontend (Qt UI)              │
│   - AuthSelectionDialog             │
│   - QRDialog                        │
└─────────────┬───────────────────────┘
              │
              ├─────────────────────────┐
              │                         │
┌─────────────▼──────────┐  ┌──────────▼─────────────┐
│  AddaLoginClient       │  │  QRRelayClient         │
│  - Generate QR         │  │  - Register Session    │
│  - Poll Status         │  │  - Poll Stream Key     │
└─────────────┬──────────┘  └──────────┬─────────────┘
              │                         │
              │                         │
┌─────────────▼──────────┐  ┌──────────▼─────────────┐
│  admin.adda247.com     │  │  obs-backend-chi       │
│  (HTTPS/443)           │  │  .vercel.app           │
└────────────────────────┘  └────────────────────────┘
```

## Threading Model

- **UI Thread**: Qt event loop, dialogs, user interactions
- **HTTP Threads**: Background threads for polling (detached)
- **Qt Signals**: Thread-safe communication via `QMetaObject::invokeMethod`

## Security

- All connections use HTTPS (TLS/SSL)
- Server certificate verification disabled for compatibility (can be enabled)
- Auth token required for Vercel backend (`x-auth-token: fpoa43edty5`)
- Session tokens are UUIDs (randomly generated)

## Troubleshooting

### QR Code Not Generating

- Check internet connection
- Verify firewall settings allow HTTPS connections
- Check OBS logs for error messages

### Polling Not Working

- Ensure mobile app is scanning the correct QR code
- Check that the session hasn't expired
- Verify backend servers are accessible

### Stream Key Not Applied

- Check OBS streaming service is configured
- Verify you have a YouTube service selected
- Check OBS logs for configuration errors

## Development

### File Structure

```
obs-qr-stream-key/
├── CMakeLists.txt                  # Build configuration
├── obs-qr-stream-key.cpp           # Main plugin entry point
├── adda-login-client.hpp/cpp       # Adda247 authentication
├── qr-relay-client.hpp/cpp         # YouTube relay client
├── auth-selection-dialog.hpp/cpp   # Initial selection dialog
├── qr-dialog.hpp/cpp               # QR code display dialog
├── httplib.h                       # HTTP library (download separately)
└── README.md                       # This file
```

### Adding New Authentication Methods

1. Create a new client class (similar to `AddaLoginClient`)
2. Implement QR generation and polling logic
3. Add a new button to `AuthSelectionDialog`
4. Add handler function in `obs-qr-stream-key.cpp`

## License

This plugin follows the same license as OBS Studio (GPL v2+).

## Credits

- **OBS Studio**: https://obsproject.com
- **cpp-httplib**: https://github.com/yhirose/cpp-httplib
- **Qt Framework**: https://www.qt.io

## Support

For issues or questions:
- Check OBS logs: Help → Log Files → View Current Log
- Report issues on the OBS Studio GitHub repository
- Contact Adda247 support for backend-related issues
