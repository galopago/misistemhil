# Implementer Backlog

All agentic development methodology documentation in this firmware tree is
maintained in English.

## Pending (authorized now)

- None.

## Pending (not authorized yet)

- `I2C_BUS_PHASE3`: `i2c_broker` when architect activates phase 3 handoff.
- SH1106 driver: only if hardware changes (v1 target is SSD1306).
- INA219 driver: separate future handoff.

## Completed

- `OLED_WIFI_CONNECTED_STATUS`: connected OLED shows WIFI OK + STA IPv4 + split MAC.
- `OLED_PROVISIONING_SETUP_UX`: provisioning OLED shows split SSID + join/scan instructions.
- `PRODUCT_BOOT_DISPLAY_AND_DYNAMIC_AP_SSID`: dynamic AP SSID `HIL-06-<MAC4>`,
  display demo retired from product boot.
- `WIFI_PROVISIONING_SAVED_BOOT_RELIABILITY`: WPA2/WPA3 STA config, saved boot retry
  policy (5 attempts, backoff, early disconnect), stale GOT_IP guard.
- `WIFI_PROVISIONING_POST_SUCCESS_COMMIT`: fresh GOT_IP, success ordering, rollback, deferred teardown.
- `WIFI_PROVISIONING_HTTP_HEADER_BUDGET`: CONFIG_HTTPD_MAX_REQ_HDR_LEN=2048, POST milestone logs.
- `WIFI_PROVISIONING_HTTP_STACK_SAFETY`: chunked HTML, static form body, httpd stack 8192.
- `WIFI_PROVISIONING_PORTAL_REACHABILITY`: AP/DHCP before wifi start, /health, diagnostics.
- `WIFI_PROVISIONING_ARCHITECTURE`: wifi_credentials, wifi_provisioning, app_core_wifi.
- `DISPLAY_VISUAL_DEMO_PROTOCOL`: validated in Run 006; retired from product boot.
- `DISPLAY_DELIVERY_CONTRACT`: app_core display callback API.
- `QR_ENCODER_INTERFACE`: setup_url, Nayuki qrcodegen, display_qr_generate.
- `I2C_BUS_ARCHITECTURE` phase 1: shared bus, board helper, app_core startup.
- `I2C_BUS_PHASE2`: `i2c_bus_transceive`, bus mutex, SSD1306 via transceive.
- `OLED_TEXT_DISPLAY_INTERFACE` base stack (renderer, task, SSD1306 when OLED present).

## Entry Format

```text
ID:
Source:
Files to modify:
Implementation plan:
Risks:
Status:
```
