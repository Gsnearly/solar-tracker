# Solar Tracker — Arduino UNO R4 WiFi

A standalone dual-axis solar tracker with four photoresistors, two servos, and an onboard Wi-Fi access point/web dashboard.

## Wi-Fi and dashboard

- SSID: `solar tracker`
- Password: `password`
- URL: `http://192.168.4.1` (the Serial Monitor prints the actual AP address)
- The dashboard shows a moving sun on a curved sunrise-to-sunset clock, local 12-hour AM/PM time, approximate sunrise/sunset, calculated sun altitude and azimuth, four LDR readings, and two servo angles.
- The browser supplies the current time. The UNO R4 is not independently time-synchronized in this build; opening the dashboard on a phone/computer with the correct date/time is necessary. The sun clock is calculated in the browser, while **physical tracking is controlled by the LDRs**, not by the calculated sun position.
- The browser stores your chosen latitude/longitude locally. Defaults are Canyon, Texas (34.98, -101.92). Set your own coordinates in the dashboard for correct solar estimates.
- The clock uses the **viewing device's local time zone**; ensure it matches the tracker's location. Sunrise/sunset are approximate. The curve is an illustrative daily arc, not an exact plot of solar altitude.

## Hardware

- Arduino **UNO R4 WiFi** (not UNO R4 Minima)
- 4 photoresistors (LDRs) and 4 × 10 kΩ resistors
- 2 hobby servos (SG90 for a lightweight mount; stronger servos require an appropriate supply)
- External regulated 5 V supply with sufficient current for servo stall peaks
- Cross-shaped opaque divider between the four LDRs, mount, and jumper wires

## Wiring

Each LDR forms a voltage divider: `5V -> LDR -> analog input -> 10kΩ -> GND`.

| LDR position | Analog pin |
|---|---|
| Top left | A0 |
| Top right | A1 |
| Bottom left | A2 |
| Bottom right | A3 |

| Servo | Signal pin |
|---|---|
| Horizontal | D9 |
| Vertical | D10 |

Connect both servo power leads to the **external regulated 5 V supply**, not the UNO's 5 V pin. Connect the servo power supply GND to Arduino GND (common ground). Never connect servo supply voltage to Arduino signal pins. Protect outdoor connections from water and corrosion.

## Upload

1. Install Arduino IDE 2 and the **Arduino UNO R4 Boards** board package.
2. Select **Arduino UNO R4 WiFi** and the correct USB port.
3. Ensure the **WiFiS3** and **Servo** libraries are available (normally included/installed through Arduino tooling).
4. Open `SolarTrackerUNO_R4/SolarTrackerUNO_R4.ino` and upload.
5. Open Serial Monitor at **115200 baud**; check that the AP started and note its IP address.
6. On your phone or laptop, join **solar tracker** with password **password**. Your device may say **No Internet**; stay connected.
7. Open `http://192.168.4.1` in a browser. If that does not work, use the IP printed in Serial Monitor.
8. Set your tracker's latitude/longitude and click **Save location**.

## Tracking and calibration

- `DEADBAND = 65` reduces servo hunting. Adjust if needed.
- `TRACK_INTERVAL = 250` milliseconds controls servo update frequency.
- Servo angles are limited to 15°–165° to avoid most mechanical endpoints; adjust to your actual mount's safe range.
- The code assumes more light yields a **higher analog reading**, matching the divider wiring above.
- **If an axis moves away from the light**, reverse the `++` and `--` for that axis in `updateTracker()`.
- Test with a flashlight before installing a solar panel. Prevent cables from tangling during rotation.
- Tracking stops at the last position in darkness; this design does not automatically return east at night.

## API

`GET /api/status` returns JSON, e.g.:

```json
{"h":90,"v":90,"tl":520,"tr":510,"bl":500,"br":490}
```

The dashboard polls this endpoint every second. The page and API are only reachable by clients on the local AP.

## Notes

- The password `password` is weak and intended for a demonstration. Change `AP_PASS` for outdoor deployment.
- On some phone models, the OS may switch away from a Wi-Fi network without internet; choose **Stay connected**.
- The UNO R4 WiFi has limited memory; the dashboard is deliberately self-contained without external JS or CSS files.
- No persistent timekeeping is provided. For autonomous date/time across reboots without a browser, add a battery-backed DS3231 RTC and extend the sketch.
- If AP initialization fails, check the UNO R4 WiFi connectivity firmware and Arduino core versions.
