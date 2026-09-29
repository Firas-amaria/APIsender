# Physical-board acceptance checklist

Compilation and host checks cannot prove that the panel works. Run these checks
with the JC3248W535EN before handing devices to a class.

1. Build/upload with `TOUCH_TEST_ONLY=true`. Verify portrait image, correct colors,
   readable text and touch response. Restore false and upload the lesson firmware.
2. With Wi-Fi placeholders, confirm the menu remains usable and explains the missing
   SSID. Configure the teacher's network, upload, and check Connected plus an IP.
3. Enter a LAN Base URL with a trailing slash. Save, leave Settings, confirm the
   normalized active URL, then restart and confirm it persisted.
4. Try empty text, `https://host`, `http://host:0`, a path, spaces inside the host,
   and `localhost`. Save must report Invalid Base URL without changing the saved URL.
5. Enter a different URL and Test Connection without Save. Confirm Serial uses the
   entered URL and the main menu still shows the saved URL.
6. Implement the API contract. Check health, random zero/negative/positive integers,
   all four colors and all three events. Verify exact methods, paths and JSON in
   server logs and inspect the website's latest color/event.
7. Return 404/500, malformed JSON, missing fields, a string/fractional random number,
   and a body exceeding 1024 bytes. Check readable errors and continued touch operation.
8. Return HTTP 201 or 204 to POST: success. Return a redirect: failure. Confirm no
   automatic retry generates duplicate POSTs.
9. Stop the server or delay responses. Navigate Back while waiting. Verify a stale
   completion does not overwrite another screen. Press another action while busy.
10. Turn off the access point, then restore it. Confirm status changes, UI stays
    usable, and automatic/manual reconnect recovers. Repeat API requests.
11. Open/close the keyboard, edit a long URL, revisit all screens repeatedly and
    check for clipped text, touch alignment and stable memory in Serial.

Timeout is per network operation, not a guaranteed total wall-clock deadline.
The response buffer is bounded even if a server sends many chunks. A slow server
can keep its worker occupied; the UI remains available throughout.
