# Tester Host Procedures

Operational commands for WiFi provisioning validation on `b06_hil` (ESP32-C3).
Factory reset at runtime is implemented; see `docs/wifi_factory_reset_implementation_reference.md`.
Use host NVS erase below when GPIO7 reset is not the test focus.

## Prerequisites

- ESP-IDF environment sourced (`source ~/esp/esp-idf/export.sh` or equivalent).
- Device connected on `/dev/ttyACM0` (USB-Serial/JTAG on ESP32-C3 SuperMini).
- Firmware built at least once so partition offsets match the active build
  (`firmware/build/partition_table/partition-table.bin`).

## Erase WiFi credentials (NVS `wifi_prov`)

### When to use

- Start a **fresh provisioning** run after a previous POST saved credentials.
- Recover from **saved STA boot** (device no longer opens provisioning AP
  `HIL-06-<MAC4>`).
- **GPIO7 factory reset** at runtime: hold ≥ 10 s (Run 021 validated). Boot hold and
  short-press negative tests are recommended follow-up.
- Re-test POST from a clean NVS state without reflashing application firmware.

### What it erases

- Entire **NVS partition** on flash (not only namespace `wifi_prov`).
- On current `b06_hil` firmware, NVS is used only for namespace `wifi_prov`
  (`ssid`, `password`, `provisioned` in `components/wifi_credentials/`).
- Does **not** erase application firmware, bootloader, or `phy_init` partition.

### Partition offset (current build)

Default `partitions_singleapp.csv` layout:

| Partition | Offset   | Size   |
|-----------|----------|--------|
| `nvs`     | `0x9000` | `0x6000` |
| `phy_init`| `0xf000` | `0x1000` |
| `factory` | `0x10000`| `1M`   |

Confirm before erasing if the partition table changes:

```bash
source ~/esp/esp-idf/export.sh
cd firmware
python "$IDF_PATH/components/partition_table/parttool.py" \
  --partition-table-file build/partition_table/partition-table.bin \
  get_partition_info --partition-name nvs
```

Expected output: `0x9000 0x6000`

### Command

```bash
source ~/esp/esp-idf/export.sh
esptool.py --port /dev/ttyACM0 --chip esp32c3 erase_region 0x9000 0x6000
```

`esptool` performs a hard reset when finished. If the device stays in download
mode after other serial tools, run:

```bash
esptool.py --port /dev/ttyACM0 --chip esp32c3 run
```

### Expected result after reset

Serial boot should show provisioning AP, not STA-only boot:

```text
I (...) wifi_prov: starting SoftAP ssid=HIL-06-ABCD mode=AP
I (...) wifi:mode : softAP (...)
I (...) wifi_prov: SoftAP started
I (...) wifi_prov: provisioning portal ready at http://192.168.4.1
```

Must **not** show immediate STA auth failure (`auth -> init`) without prior
SoftAP logs when credentials were the reason for STA boot before erase.

Phone WiFi list should show an open AP matching **`HIL-06-[0-9A-F]{4}`**.

### Test run record

In `test_runs.md`, set NVS state to:

```text
NVS: erased via host (esptool erase_region 0x9000 0x6000)
```

Do not dump or log NVS contents before erase (contains WiFi credentials).

### Alternatives (not default for tester)

| Method | Notes |
|--------|--------|
| `idf.py erase-flash` | Erases entire flash; requires full reflash. Use only when intentional. |
| GPIO7 hold ≥ 10 s at boot or runtime | Normative factory reset per `docs/wifi_factory_reset_architecture.md`. Runtime **validated Run 021**; boot hold deferred. |
| Reflash only | Does **not** clear NVS unless flash image or erase step includes NVS. |

## Inject invalid WiFi credentials (saved-boot lock test)

### When to use

- Exercise **SAVED_FAILURE_LOCKED** without reflashing firmware.
- Validate error LED **fast blink** (`DISCONNECTED`) after 5 failed STA attempts.
- OLED should show `WIFI` / `FAILED` / `HOLD` / `RESET`.

### Command

```bash
source ~/esp/esp-idf/export.sh
cat > /tmp/nvs_wifi_bad.csv << 'EOF'
key,type,encoding,value
wifi_prov,namespace,,
ssid,data,string,NONEXISTENT_AP_XYZ
password,data,string,badpass12345
provisioned,data,u8,1
EOF
python "$IDF_PATH/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py" \
  generate /tmp/nvs_wifi_bad.csv /tmp/nvs_wifi_bad.bin 0x6000
esptool.py --port /dev/ttyACM0 --chip esp32c3 write_flash 0x9000 /tmp/nvs_wifi_bad.bin
```

Change `ssid` / `password` as needed. Namespace and keys must match
`components/wifi_credentials/wifi_credentials.c` (`wifi_prov`, `ssid`, `password`,
`provisioned`).

### Expected result after reset (~25–35 s)

```text
I (...) wifi_prov: saved boot attempt 5/5 failed reason=...
E (...) wifi_prov: saved boot failed attempts=5 last_reason=...
```

Device stays in locked state (no portal until factory reset or NVS erase).
Error LED: **fast blink** (~125 ms).

### Recovery

```bash
esptool.py --port /dev/ttyACM0 --chip esp32c3 erase_region 0x9000 0x6000
```

Then re-provision via portal, or inject valid credentials with the same CSV flow.

## WiFi POST troubleshooting notes (field findings)

Reference for future connectivity debug sessions. Full serial evidence:
`feedback.md` Entries 025–026; captures `/tmp/b06_hil_wifi_debug_new_ap2.txt` (fail),
`/tmp/b06_hil_wifi_debug_new_ap3.txt` (pass).

### Firmware STA profile (v2, unchanged)

- `apply_sta_config()` in `wifi_provisioning.c`:
  - `threshold.authmode = WIFI_AUTH_WPA2_WPA3_PSK`
  - `auth_profile=wpa2_wpa3_personal`, PMF capable, PMF not required
  - POST timeout: 30 s (`WIFI_PROV_STA_TIMEOUT_MS`)

### Disconnect `reason=211` — auth mode threshold (2026-06-28)

**Symptom:** Portal POST fails; serial shows:

```text
POST /provision attempting STA ssid=<ssid>
STA disconnected reason=211 source=submitted
POST /provision failure response sent
```

**Meaning (ESP-IDF 5.3):** `WIFI_REASON_NO_AP_FOUND_IN_AUTHMODE_THRESHOLD` — the AP
was seen in scan but **rejected** because its security mode is below the firmware
threshold. This is **not** a wrong-password symptom (`202` = `AUTH_FAIL`).

**Common AP configs that trigger 211:**

| AP encryption | Typical result |
|---------------|----------------|
| Open / OWE | Fail (211) |
| WPA-PSK only or WPA/WPA2 mixed (legacy) | Fail (211) |
| WPA-Enterprise / 802.1X | Fail (211) |
| SSID only on 5 GHz | Fail (201 or 211) |
| WPA2-PSK personal on **2.4 GHz** | Usually OK |
| WPA3-SAE personal on 2.4 GHz | OK (validated: vitriolina) |
| **WPA2-PSK/WPA3-SAE mixed** on 2.4 GHz | OK (validated: openwrt-iot) |

**Validated fix (openwrt-iot on OpenWrt):** set encryption to **WPA2-PSK/WPA3-SAE
Mixed Mode**, SSID on **2.4 GHz** radio, apply & save. After fix:

```text
wifi:connected with openwrt-iot
STA got IP ip=192.168.1.9 source=submitted
credentials saved source=submitted
POST /provision success response sent
```

### Serial capture tips

- Reset ESP (RTS) before capture so POST milestones are not missed.
- Capture 90–120 s from boot through POST.
- Key lines: `attempting STA ssid=`, `STA disconnected reason=`, `STA got IP`.

### When to escalate to implementer vs operator

| Observation | Owner |
|-------------|--------|
| reason=211 on WPA2/WPA3 personal 2.4 GHz AP | Implementer (threshold / scan) |
| reason=211 on open/enterprise/legacy AP | Operator (AP config) |
| reason=202 | Operator (password) or WPA3 transition |
| reason=201 | Operator (SSID typo, 5 GHz only, out of range) |
| No `POST /provision from client` | Portal reachability (phone not on HIL AP) |

## Standard flash and monitor (reference)

```bash
source ~/esp/esp-idf/export.sh
cd firmware
idf.py build
idf.py -p /dev/ttyACM0 flash
idf.py -p /dev/ttyACM0 monitor
```

Hardware: ESP32-C3 SuperMini, OLED SSD1306 @ I2C `0x3C`, SCL GPIO0, SDA GPIO1.
