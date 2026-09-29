# Student assignment: build the device's server

Your website/backend must implement the four routes below. The ESP32 client is
already programmed. Your website should show the latest received color and button.
Choose your own server language and website design. No authentication, database,
WebSockets, MQTT or cloud account is required.

A **GET** asks for data. A **POST** sends data. **JSON** is the text format exchanged.
The Base URL is the address entered on the ESP32 Settings screen. For these examples
it is `http://192.168.1.100:3000`. Replace it with your laptop's LAN address and port.

| Method | Endpoint | ESP32 sends | Server should return |
| --- | --- | --- | --- |
| GET | `/api/health` | No body | `{"status":"ok"}` |
| GET | `/api/random` | No body | `{"number":42}` |
| POST | `/api/color` | `{"color":"red"}` | `{"success":true}` |
| POST | `/api/event` | `{"button":"A"}` | `{"success":true}` |

Return HTTP **200** and `Content-Type: application/json` for the GET examples.
The recommended POST reply is HTTP **200** with `{"success":true}`. The ESP32
accepts any **2xx** POST status, including 201 or 204, and does not parse the POST
response body. Thus `{"success":false}` with HTTP 200 still counts as success:
use a non-2xx status for failures. `{"success"}` alone is not valid JSON.
Keep responses at or below **1024 bytes**, encoded as plain UTF-8 (no compression).
Extra JSON fields are allowed, but the required GET field names are case-sensitive.
Redirects are not followed. Reply promptly: each network operation has a 5-second timeout.
The ESP32 sends `Accept: application/json` and POSTs use `Content-Type: application/json`.

## 1. Health check

**Why:** find out whether your backend is reachable and implements the expected API.

```http
GET /api/health HTTP/1.1
Host: 192.168.1.100:3000
Accept: application/json
```

There is no request body. Complete URL: `http://192.168.1.100:3000/api/health`.
Return:

```http
HTTP/1.1 200 OK
Content-Type: application/json

{"status":"ok"}
```

The touchscreen shows **Server Online** only when HTTP 200 contains a JSON object
whose `status` is exactly the string `ok`. Incorrect JSON shows **Invalid Response**;
network/status failures show the error listed below. Settings → Test Connection
uses this same endpoint, with the URL currently entered (even before Save).

## 2. Random number

**Why:** let the backend compute data and the device display it.

```http
GET /api/random HTTP/1.1
Host: 192.168.1.100:3000
Accept: application/json
```

No request body. Complete URL: `http://192.168.1.100:3000/api/random`.
Return HTTP 200 with:

```json
{"number":42}
```

Generate an integer, for example between 0 and 100. The accepted range is
-2147483648 through 2147483647. A quoted number (`"42"`), fraction, missing field
or invalid JSON is an **Invalid Response**. The device displays the returned
number prominently and **Number Received**. While loading or after failure,
the value is `--`, so students do not mistake an old number for a new result.

## 3. Send color

**Why:** communicate a touchscreen choice to your website.

```http
POST /api/color HTTP/1.1
Host: 192.168.1.100:3000
Content-Type: application/json
Accept: application/json

{"color":"red"}
```

Complete URL: `http://192.168.1.100:3000/api/color`.
Allowed values are exactly `red`, `green`, `blue`, `yellow` (lowercase).
Read the JSON body, store the latest choice in your server, and make your website
show that color. Return HTTP 200 with `{"success":true}`. The ESP32 displays
**Sent: red / Status: Success** (or the selected value and error on failure).
The device only confirms HTTP acceptance; it cannot verify that your website updated.

## 4. Send device event

**Why:** report a physical-device interaction to a backend.

```http
POST /api/event HTTP/1.1
Host: 192.168.1.100:3000
Content-Type: application/json
Accept: application/json

{"button":"A"}
```

Complete URL: `http://192.168.1.100:3000/api/event`.
Allowed values: `A`, `B`, `C` (uppercase). Your website should display, for example,
**Last Button Pressed: A**. Return HTTP 200 with `{"success":true}`.
The ESP32 shows **Last Event: A / Status: Success**, or the selected event and error.
These are touchscreen buttons, so no external GPIO button wiring is required.

## Errors shared by all endpoints

| Device message | Meaning / what to check |
| --- | --- |
| Wi-Fi Disconnected | Check credentials, 2.4 GHz network and signal |
| Invalid Base URL | Enter `http://host:port`, without an endpoint path |
| Server Unreachable | Check server running, LAN IP, port, firewall and `0.0.0.0` binding |
| Request Timeout | Server/network did not respond in time |
| Request Failed (HTTP N) | GET was not 200, or POST was not 2xx; e.g. route missing (404) |
| Invalid Response | Health/random JSON did not match the contract |
| Response Too Large | Response exceeded 1024 bytes |
| Save Failed | Device could not commit URL to NVS; retry and inspect Serial |

HTTP errors take precedence over parsing the response. Failed requests do not
crash or lock the UI. Press a button to try again. POSTs are **not automatically
retried**, because a timeout can happen after your server has already received an
event. Repeated button presses intentionally create repeated events.

Your laptop and ESP32 must be on a network that permits device-to-device traffic.
Use the laptop's LAN address, not `localhost`. The ESP32 is not a browser and does
not require CORS; your website may need CORS if its own frontend uses another origin.
You can make the website refresh/poll your backend for the latest color and event.
The website's own routes are your choice; the ESP32 calls only these four.

## Try your backend from a terminal

```sh
curl http://192.168.1.100:3000/api/health
curl http://192.168.1.100:3000/api/random
curl -H "Content-Type: application/json" -d '{"color":"red"}' http://192.168.1.100:3000/api/color
curl -H "Content-Type: application/json" -d '{"button":"A"}' http://192.168.1.100:3000/api/event
```

These quoting examples are for a POSIX shell. In Windows PowerShell, use
`Invoke-RestMethod -Method Post -ContentType application/json -Body '{"color":"red"}' -Uri http://192.168.1.100:3000/api/color`.
