# Tester runs — `b02_hil`

## 2026-11-17 — `B02_FIRMWARE_ADAPT_COPY` + `B02_GPIO_BRINGUP_SAFE` smoke

- **Role:** tester
- **Port / chip:** `/dev/ttyACM0`, ESP32-C3 v0.4, MAC `a4:cb:8f:22:59:80`
- **IDF:** v5.3.4
- **Commands:** `idf.py set-target esp32c3` → `fullclean` → `build` → `-p /dev/ttyACM0 flash` → ~20s serial capture
- **Coupling:** no `b00_basehil` / `EXTRA_COMPONENT_DIRS` under `CMakeLists.txt` / `main` / `components` / `sdkconfig.defaults`
- **Build:** PASS (`b02_hil_firmware.bin`, app ~0xf3560)
- **Flash:** PASS
- **Boot identity:** PASS — `Board: b02_hil`, SoftAP/mDNS-style id `HIL-02-5981`, portal `http://192.168.4.1`
- **GPIO bring-up log:** PASS — `board_ctrl` cold message before I2C; pins 2,3,4,9,20,21 as outputs
- **Display:** PASS — OLED `0x3D`
- **Relay chatter (audible):** PENDING human observation
- **Verdict:** technical smoke **PASS**; audible cold-boot criterion still needs human ear

Details: `agent-workspaces/tester/handoff.md`

## 2026-11-17 — SoftAP not visible (multi-client)

- **Clients:** 2 phones + 1 laptop — SSID `HIL-02-5981` **not listed**
- **Control:** sister HIL boards SoftAP **is** visible after restart (same room)
- **Serial (b02):** still `starting SoftAP ssid=HIL-02-5981` → `SoftAP started` → portal ready; no `SoftAP stopped`
- **Also every boot:** `phy_init: Saving new calibration data due to checksum failure or outdated calibration data`
- **Code compare:** `wifi_provisioning.c` identical to `b00_basehil` and `b03_hil`; only unique boot delta is `board_ctrl_init()` (same pin class as working `b03` `relay_ctrl`)
- **Verdict:** SoftAP **software start PASS**, **over-the-air visibility FAIL** → not a client-filter issue; next isolation needs authorized A/B (erase-flash, base image on same HW, or skip `board_ctrl`)
- **OLED:** human reports QR visible (portal UX up) while SSID still not on air

## 2026-11-17 — A/B flash `b00_basehil` on HIL-02 hardware

- **Action:** built + flashed `b00_basehil` to `/dev/ttyACM0` (same MAC `a4:cb:8f:22:59:80`)
- **Serial:** `Board: b00_basehil`; SoftAP `HIL-00-5981`; `SoftAP started`; portal ready
- **No** `board_ctrl` GPIO claim path (base has none)
- **PHY:** this boot did **not** reprint the checksum-failure recalibration line
- **Human result:** `HIL-00-5981` **still not visible** (phone + laptop)
- **Verdict:** SoftAP stack OK in software; **this board’s RF path FAIL**. Replacement SuperMini/PCB required. SoftAP code not returned to architect/implementer from this failure.

## 2026-11-17 — Replacement SuperMini flash (`b02`)

- **Port/MAC:** `/dev/ttyACM0`, STA-ish MAC `a4:cb:8f:21:df:98` (AP `…:df:99`)
- **Flash:** `b02_hil_firmware` OK
- **Boot:** `board_ctrl` cold; OLED `0x3D`; SoftAP **`HIL-02-DF99`** started; portal ready
- **PHY:** first-boot full cal (`failed to load RF calibration data` → save mode 2) — expected on blank NVS
- **Pending human:** phone/laptop see **`HIL-02-DF99`**? + relay chatter on reset?
- **Human:** still not visible with replacement module → suspects code, not RF distance

## 2026-11-17 — A/B flash existing `b03` binary (no rebuild)

- **Method:** `esptool write_flash` of prebuilt `b03_hil/firmware/build/*.bin` only (no `idf.py build`, no source edits)
- **Target:** same `/dev/ttyACM0` MAC `a4:cb:8f:21:df:98`
- **Serial:** `b03_hil` start; SoftAP **`HIL-03-DF99`** started; portal ready
- **Pending human:** is **`HIL-03-DF99`** visible with sisters disconnected?

## 2026-11-17 — Flash-path fix attempt (`erase-flash` + `idf.py flash` on b03)

- **Hypothesis:** prior flashes left NVS/PHY; sister procedure is `idf.py -p … flash` (does not erase NVS between apps)
- **Done:** `idf.py -p /dev/ttyACM0 erase-flash` then `idf.py -p /dev/ttyACM0 flash` in `b03_hil/firmware` (CMake/ninja did rebuild as part of flash)
- **Serial:** SoftAP **`HIL-03-DF99`** started; first-boot full PHY cal after erase
- **Pending human:** visibility of **`HIL-03-DF99`** after clean flash
- **Human:** still not visible after clean flash

## 2026-11-17 — Facts-only SoftAP investigation (no conclusions beyond measurements)

### Measured / read from repo + IDF (no OTA observation from host)

1. `wifi_provisioning_start_ap_portal` sets only: `ssid`, `ssid_len`, `channel=1`, `authmode=OPEN`, `max_connection=4` on a zeroed `wifi_config_t` (`b03`/`b02`/`b00` same file).
2. Fields **not** set in that struct: `beacon_interval` (remains 0), `ssid_hidden` (0), password, TX power APIs not called in this function.
3. ESP-IDF header `esp_wifi_types_generic.h`: `beacon_interval` documented range **100–60000**, default **100**.
4. Espressif example `examples/wifi/getting_started/softAP` also leaves `beacon_interval` unset (0 via designated init); default example SSID=`myssid`, password=`mypassword`, channel=1, WPA2 (not open).
5. Serial logs on this unit (prior boots): `WIFI_EVENT` path logged `SoftAP started` and `mode : softAP` for `HIL-02-*` / `HIL-03-DF99` / `HIL-00-*`.
6. Flash map used by HIL firmwares: `dio` / `80m` / **4MB**; Espressif SoftAP example build reports flash_size **2MB** in its flash_args.
7. Official SoftAP example **built** and ready at `$IDF_PATH/examples/wifi/getting_started/softAP/build/wifi_softAP.bin`.

### Blocked measurement

- `read_flash` compare file↔chip: **failed** — `/dev/ttyACM0` absent (`No such file or directory`) at investigation time.
- Control flash of Espressif SoftAP example: **blocked** until serial port returns.

### Next measurement (when board reconnected)

- `erase-flash` + flash Espressif `wifi_softAP` only.
- Human reports whether SSID **`myssid`** (pass `mypassword`) is listed — binary yes/no.

## 2026-11-17 — Control: Espressif SoftAP example flashed

- **Commands:** `idf.py -p /dev/ttyACM0 erase-flash` + `flash` in `$IDF_PATH/examples/wifi/getting_started/softAP` (flash_size 4MB)
- **Serial fact:** `mode : softAP (a4:cb:8f:21:df:99)`; `wifi_init_softap finished. SSID:myssid password:mypassword channel:1`
- **Pending human:** is **`myssid`** visible? (WPA2 pass `mypassword`)
- **Human:** **no** — not on primary clients; also not on a second older laptop
- **Stop condition:** Espressif SoftAP example invisible ⇒ defect not localized to HIL SoftAP source under this DUT
- **Fact (b03 history):** Nov 3–4 `b03` tester runs validated STA/`HIL-03-F06F.local` on LAN after provision; they did **not** record phone SoftAP-SSID visibility as a pass criterion
- **Fact:** default IDF SoftAP example used `CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT=y` → **WPA3_PSK** for `myssid`

## 2026-11-17 — SoftAP diagnostic image (open / ch6 / beacon100)

- **Module MAC now on port:** `a4:cb:8f:22:59:80` (first module, not DF98)
- **Flash:** `idf.py build flash` (no erase); patched example
- **Serial get_config:** `ssid=HILTESTAP len=9 ch=6 auth=0 hidden=0 beacon=100 max_conn=4 tx_power=80`
- **Pending human:** is **`HILTESTAP`** (open) visible?

## 2026-11-18 — SoftAP **code-only** audit (no chip)

### Identity of SoftAP source

- `cmp` `wifi_provisioning.c`: **b02 == b00 == b03** (byte-identical)
- sdkconfig keys `CONFIG_ESP_WIFI_*` / `CONFIG_ESP_PHY_*` / flash size / partition / console: **0 diffs** among b00/b02/b03 (96 keys compared)

### What SoftAP code sets (`wifi_provisioning_start_ap_portal`)

Sets only: `ssid`, `ssid_len`, `channel=1`, `authmode=WIFI_AUTH_OPEN`, `max_connection=4` on zeroed `wifi_config_t`.

Left at **0** (struct zero-init): `beacon_interval`, `ssid_hidden`, `csa_count`, `dtim_period`, password, pmf, etc.

IDF header (`esp_wifi_types_generic.h`): `beacon_interval` documented range **100–60000**, documented default **100**; `dtim_period` range **1–10**, default **1**; `csa_count` default **3**.

### Unique b02 code before WiFi (vs b00)

- `app_core_start` calls `board_ctrl_init()` before I2C/display/WiFi (only structural SoftAP-adjacent delta vs base).
- `app_core_wifi.c` diffs vs b00 are type rename `b02_hil_board_pins_t` + comment `HIL-02` only.

### Not claimed

- This audit does **not** assert that `beacon_interval==0` is the OTA failure cause (same SoftAP file is on sister trees).
- This audit does **not** examine the physical module.

## 2026-11-18 — Read-only dump of connected **b06** (no erase/write)

- **MAC:** `8c:d0:b2:a9:24:cd`
- **Partition table on chip:** nvs `0x9000/24K`, phy `0xf000/4K`, factory `0x10000/4032K` (same large layout)
- **App strings:** `Starting b06_hil`, `starting SoftAP ssid=%s mode=AP`
- **SoftAP codegen in chip image** (site `@0x3cef4`): `memset` 180; `channel=1`; `max_connection=4`; `WIFI_MODE_AP=2`; **no** store of `beacon_interval=100` next to that sequence (same pattern as b02/b03 ELFs)
- **Source:** `b06` `wifi_provisioning.c` **cmp-equal** to `b02`
- **Boot observation (reset only):** board came up in **STA** (saved creds) → `openwrt-iot`, IP `192.168.1.9`, identity `HIL-06-24CE` — SoftAP portal path **not** entered this boot
- **Hash:** on-chip app ≠ current `b06_hil/firmware/build/b06_hil_firmware.bin` (different build revision; SoftAP field immediates still match)

## 2026-11-18 — b06 SoftAP after credential erase (human visibility)

- **Trigger:** human factory-reset / credential erase on connected b06
- **Serial:** `ssid=HIL-06-24CE` → `mode : softAP` → `SoftAP started` → portal ready
- **Human:** SSID **`HIL-06-24CE` visible** on phone **and** laptop
- **Fact:** SoftAP field immediates on this working image match b02 SoftAP codegen (`ch=1`, `max_conn=4`, OPEN, no `beacon_interval=100` store)
- **Implication for prior b02 failure:** SoftAP **source field layout is sufficient** for OTA visibility on this b06 unit; invisibility on previously tested modules is **not explained** by those SoftAP struct omissions alone

## 2026-11-18 — Flash b06 dump onto new module (MAC a4:cb:8f:22:59:80)

- **First attempt:** wrote truncated `/tmp/hil_sister_dump/app_exact.bin` (1003577 B) → boot loop `Image hash failed - image is corrupt` / factory not bootable. Cause: dump cut mid-SHA256 (24/32 hash bytes). Checksum OK; validation hash invalid.
- **Fix:** wrote `/tmp/hil_sister_dump/app_bootable.bin` (1003585 B, full SHA256 from `app_region.bin`) + NVS erase `0x9000/24K`.
- **Boot result:** SoftAP `HIL-06-5981` started, portal `http://192.168.4.1` — serial OK.
- **Human visibility:** **FAIL** — phone and laptop see **nothing** (`HIL-06-5981` absent).
- **A/B fact:** identical on-chip image that was **visible** as SoftAP on donor b06 (`HIL-06-24CE`) is **invisible** on this module after successful SoftAP start in serial (no wifi/phy E-level errors; `phy_version 1201`, `mode : softAP`).

## 2026-11-18 — DUT flash-side: rewrite + erase NVS + erase phy_init

- **Target:** same connected DUT MAC `a4:cb:8f:22:59:80`
- **Write:** b06 dump bootloader@0x0 + PT@0x8000 + `app_bootable.bin`@0x10000 (hash verified on write)
- **Erase:** NVS `0x9000/24K` + **phy_init `0xf000/4K`**
- **Readback:** app SHA256 matched dump; `image_info` Checksum/Validation Hash **valid**
- **Boot:** SoftAP `HIL-06-5981` / portal ready; phy full-cal after erase (`0x1102` → full cal → save)
- **Human visibility:** **FAIL** — `HIL-06-5981` still not seen on phone/laptop
- **Flash-side implication:** erasing/rebuilding `phy_init` + verified rewrite of donor app does **not** restore SoftAP visibility on this DUT

## 2026-11-18 — READ-ONLY dump of working b06 (MAC 8c:d0:b2:a9:24:cd)

- **Ops:** `read_mac`, `chip_id`, `flash_id`, `get_security_info`, `read_flash_status`, `espefuse summary/dump/adc_info/get_custom_mac`, `read_flash 0x0 0x400000` — **no** write/erase/burn
- **Artifacts:** `/tmp/hil_b06_readonly_dump/` and `agent-workspaces/tester/evidence/b06_readonly_dump_2026-11-18/`
- **Chip:** ESP32-C3 rev v0.4, flash XMC 4MB (mfg 0x20 device 0x4016), Secure Boot **off**, Flash Encryption **off**
- **eFuse flash:** CAP=4M TEMP=85C VENDOR=XMC; USB-Serial-JTAG enabled; custom MAC empty
- **App exact SHA256:** `22f14d88…48c505` — **identical** to image previously written to failing DUT
- **phy_init @0xf000:** **all 0xFF** (empty) on this working unit at dump time
- **Boot capture (reset only):** STA to `openwrt-iot`, identity `HIL-06-24CE` (SoftAP not this boot; creds present in NVS)

## 2026-11-18 — READ-ONLY dump failing DUT (MAC a4:cb:8f:22:59:80) + compare vs b06

- **Ops:** same read-only set as b06; **no** write/erase/burn
- **Artifacts:** `/tmp/hil_dut_readonly_dump/` + `agent-workspaces/tester/evidence/dut_readonly_dump_2026-11-18/`
- **Same as b06:** app SHA `22f14d88…`, bootloader, partition table, `phy_init` all `0xFF`
- **Different:** SPI JEDEC mfg `0x46` (b06 `0x20`); eFuse `FLASH_TEMP` 105C vs 85C; flash status `0x0000` vs `0x0200`; UNIQUE_ID/USR_DATA; NVS; MAC
- **Boot:** SoftAP `HIL-06-5981` started (serial)

## 2026-11-18 — b06 SoftAP NVS after human credential erase (read-only)

- **Board:** `8c:d0:b2:a9:24:cd` SoftAP `HIL-06-24CE` (serial OK); **no write** to app/bootloader
- **Artifacts:** `evidence/b06_softap_nvs_2026-11-18/`
- **phy_init:** still all `0xFF`
- **NVS vs DUT SoftAP NVS:** not identical (~10K differing bytes)
- **Both** contain SoftAP/net80211 keys (`ap.ssid`, `ap.chan`, `bcn.interval`, `ap.max.conn`, `nvs.net80211`, …)
- **b06 NVS** still has leftover STA strings (`openwrt-iot`, `sta.*`, `provisioned`) after board cred-erase
- **DUT NVS** cleaner SoftAP-oriented (`HIL-06-5981`); **not** missing SoftAP key namespace vs b06
- **Implication:** SoftAP invisibility on DUT is **not** explained by “empty/missing SoftAP NVS keys” relative to working SoftAP b06

## 2026-11-18 — CRITICAL: b06 SoftAP not visible after 2nd credential erase

- **Human:** after this credential erase, **`HIL-06-24CE` NOT seen** on WiFi clients (same symptom as “bad” cards)
- **Prior same day:** first SoftAP after cred erase **was** visible (phone+laptop)
- **Between those events (host, claimed read-only on b06):** full `read_flash` 4MB, espefuse summary/dump/adc, many `esptool` connect/stub/hard_reset cycles, NVS/phy readback
- **No intentional write/erase of app/bootloader on b06** in that window
- **Serial at SoftAP check:** SoftAP started / portal ready (`HIL-06-24CE`) while human reports invisible

## 2026-11-18 — READ-ONLY dump b03 after button credential erase

- **MAC:** `8c:d0:b2:a7:f0:6e` → SoftAP identity **`HIL-03-F06F`**
- **Artifacts:** `evidence/b03_readonly_dump_2026-11-18/` (full 4MB + efuse/id)
- **Flash JEDEC:** mfg **`0x20`** device `4016` 4MB (like b06; unlike DUT `0x46`)
- **eFuse:** `FLASH_TEMP=85C` VENDOR=XMC; `UART_PRINT_CONTROL=Disable`; UNIQUE_ID empty; USR_DATA populated
- **Flash status:** `0x0000` (like DUT; b06 was `0x0200`)
- **App:** distinct SHA `476fcb06…` (b03 firmware, not b06 clone); image valid
- **phy_init:** all `0xFF`
- **Serial after dump reset:** SoftAP **`HIL-03-F06F` started**, portal ready
- **Human visibility:** **PASS** — SoftAP `HIL-03-F06F` visible on client WiFi
- **NVS:** in dump; vs b06 SoftAP-invisible + DUT SoftAP-invisible: all three have SoftAP keys
  (`ap.ssid`/`ap.chan`/`bcn.interval`/`ap.max.conn`/`nvs.net80211`). No NVS SoftAP-key gap
  explains b03 visible vs others invisible. b03 NVS still has STA leftovers (`openwrt-iot`, `wifi_prov`).

## 2026-11-18 — Flash b03 SoftAP-visible dump onto DUT `…:59:80` + NVS erase only

- **Write:** b03 dump bootloader+PT+app (hash verified); **erase NVS only** `0x9000/24K` (no full erase, no efuse)
- **Boot:** `b03_hil` SoftAP **`HIL-03-5981`** started; portal ready
- **Human visibility:** **FAIL** — `HIL-03-5981` not seen (b03 SoftAP-visible image on this DUT still invisible)

## 2026-11-18 — ESP-IDF SoftAP example on DUT `…:59:80`

- **Build:** `examples/wifi/getting_started/softAP` → `/tmp/idf_softap_min_c3` (esp32c3)
- **Flash:** `idf.py flash` + NVS erase only
- **Serial:** `WIFI_EVENT_AP_START`; `AP cfg ssid=HILTESTAP ch=6 auth=0 hidden=0 beacon=100 max_conn=4 tx_power=80`
- **Human visibility:** **FAIL** — `HILTESTAP` not seen on any client

## 2026-11-18 — Same SoftAP example on 2nd DUT `a4:cb:8f:21:df:98`

- **Flash:** same `/tmp/idf_softap_min_c3` + NVS erase
- **Serial:** SoftAP `HILTESTAP` ch6 OPEN hidden=0 tx_power=80; AP MAC `…:df:99`
- **Prior unit `…:59:80`:** SoftAP example also invisible
- **Human visibility (this unit):** **PASS** — `HILTESTAP` seen on phone **and** laptop

### Contrast (same SoftAP example image)

| Unit | SoftAP example serial | Client visibility |
| --- | --- | --- |
| `a4:cb:8f:22:59:80` | `HILTESTAP` started | **FAIL** |
| `a4:cb:8f:21:df:98` | `HILTESTAP` started | **PASS** (phone+laptop) |

→ The two DUTs are **not** equivalent under identical SoftAP example firmware.

## 2026-11-18 — b03 SoftAP-visible dump on DUT `…:df:98` (example was visible here)

- **Write:** b03 dump + NVS erase only
- **Boot:** SoftAP **`HIL-03-DF99`** started; portal ready
- **Contrast:** same b03 image on `…:59:80` was SoftAP-invisible
- **Human visibility:** **FAIL** — `HIL-03-DF99` not seen phone/PC
- **Decisive A/B on same unit `…:df:98`:** SoftAP example `HILTESTAP` **visible**; HIL b03 SoftAP **invisible**

### SoftAP knob diff (same unit)

| | Example (visible) | HIL b03 (invisible) |
| --- | --- | --- |
| channel | **6** | **1** |
| auth | OPEN | OPEN |
| max_conn | 4 | 4 |
| beacon_interval | set 100 | omitted (driver still reports 100) |
| set_max_tx_power(80) | yes | no (driver still reports 80) |

## 2026-11-18 — Example with HIL-like knobs on `…:df:98`

- **SSID:** `HILLIKEAP`, channel **1**, OPEN, no explicit beacon/tx set
- **Serial:** AP_START; got cfg `ch=1 beacon=100 tx_power=80`
- **Human visibility:** **FAIL** — `HILLIKEAP` (ch1) not seen
- **Conclusion on `…:df:98`:** SoftAP **channel 1 → invisible**; SoftAP **channel 6 → visible** (prior `HILTESTAP`)

## 2026-11-18 — SoftAP `HILTESTAP` visible but phone connect fails (`…:df:98`)

- **Human:** SSID seen earlier; phone connect attempt **failed**
- **Serial ~45s live (no reset):** **no** `AP_STACONNECTED` / disconnect / DHCP client events
- **Implication:** failure is at/before association from phone side (or no probe/assoc reached AP), not post-assoc portal

## 2026-11-18 — SoftAP connect retry monitored (`…:df:98` / `HILTESTAP`)

- **Serial:** `station 56:de:6b:ba:f7:de join, AID=1` on ch6
- **DHCP:** assigned `192.168.4.2`
- **Human:** prior attempt failed; this monitor saw a successful join+DHCP
- **Follow-up:** human reports AP “dropped” / no longer listed; live serial before reset was idle (0 B, no SoftAP-stop/crash). After reset: SoftAP `HILTESTAP` ch6 started again cleanly.
- **Connect retry OPEN:** SoftAP up; **no join** in 50s window
- **Note:** brief WPA2 trial aborted per human — onboarding requires **OPEN**; restored OPEN `HILTESTAP` ch6 auth=0
- **Human:** OPEN `HILTESTAP` **not listed again** after restore
- **Serial check:** SoftAP still `AP_START` OPEN ch6 auth=0 tx_power=80; flash JEDEC mfg **0x46** on this unit; status `0x0200`
- **Human clarification:** SoftAP visibility is **intermittent** — sometimes appears, sometimes not (same OPEN setup). Downgrade “ch1 vs ch6 = root cause” to partial/correlation only.

## 2026-11-18 — Swap: flash b03 UART dump onto connected module

- **Connected MAC:** `8c:d0:b2:a7:f0:6e` (JEDEC mfg `0x20`)
- **Image:** b03 SoftAP-visible dump + NVS erase only
- **Boot:** SoftAP **`HIL-03-F06F`** started; portal ready

## 2026-11-18 — Flash b02_hil (UART/dev) onto working module `…:f0:6e`

- **Intent:** continue b02 development on a working spare while parts arrive
- **Image:** `b02_hil` build + NVS erase
- **Boot:** SoftAP **`HIL-02-F06F`** started; portal ready; `Board: b02_hil`
- **Human:** connects **without problem** on this module with b02 firmware
- **Conclusion (human):** defect lies in the **two prior modules** (`…:59:80`, `…:df:98`), not in b02 SoftAP/onboarding software path under this working unit

## 2026-11-18 — `B02_GPIO_BRINGUP_SAFE` compile + flash (implementer slice B)

- **Target:** `/dev/ttyACM0` MAC `8c:d0:b2:a7:f0:6e`
- **Commands:** `idf.py set-target esp32c3` → `fullclean` → `build` → `-p /dev/ttyACM0 flash`
- **Coupling:** no `b00_basehil` / `EXTRA_COMPONENT_DIRS` in tree paths
- **Build:** PASS — `board_ctrl` linked; `b02_hil_firmware.bin` ~996704 B
- **Boot serial:**
  - `Board: b02_hil`
  - GPIO claim order includes `GPIO2` (OE), then `3/4/9/20/21` (CTR/DE)
  - `board_ctrl: OE enabled; K1..K4 and 485_CTR held inactive (cold)`
  - OLED `0x3D`; identity `HIL-02-F06F`; STA to `openwrt-iot` (NVS creds present)
- **Human (acceptance):** audible relay chatter on reset/power-cycle? **pending**

## 2026-11-18 — `B02_HOST_API_RELAY_SINGLE` build + slow auditory K1..K4

- **Target:** `/dev/ttyACM0` MAC `8c:d0:b2:a7:f0:6e` → STA `192.168.1.10` (`HIL-02-F06F`)
- **Build/flash:** PASS (`board_ctrl` + `host_api` rebuilt; bin ~999312 B)
- **Boot:** `OE enabled; K1..K4 and 485_CTR held inactive (cold)`; `host API started port=80`
- **Sequence:** for ch 1..4 — `relay_set on` (hold 5s) → `relay_set off` (pause 3s)
- **API:** all `relay_set` `ok:true`; final `relay_get` all `on:false`
- **Human auditory (per channel click ON/OFF):** K1/K3/K4 **yes**; **K2 no**
- **Log:** `/tmp/b02_relay_slow_auditory.log`

## 2026-11-18 — K2 silence isolation + confirm

- **Human:** “no escuché la conmutación de K2” (after slow K1..K4 and isolate pass)
- **Confirm:** 3 rounds K1 (click expected) / K2 (suspect) / K3 (click expected) + K2 hold 8s
- **API:** all `relay_set` for ch2 `ok:true` with `result.on` matching request
- **Pin map (doc):** `K2CTR` = `GPIO9` (**strapping** note in `board_hardware_description.md`)
- **Verdict (superseded):** was PARTIAL FAIL on K2; see follow-up PASS below
- **Logs:** `/tmp/b02_relay_isolate_3of4.log`, `/tmp/b02_relay_k2_confirm.log`

## 2026-11-18 — Relay close-out PASS (human)

- Zombie K2 toggle loop (`PID 43100`) found still running after stop; killed; K1–K4 forced `off`
- Human: clicks stopped; **relays work well** (includes K2 — earlier silence treated as miss / timing)
- **Verdict:** `B02_HOST_API_RELAY_SINGLE` **PASS** on `…:f0:6e`

## 2026-11-18 — `B02_UART_PATH_LINE` + `B02_UART_STREAM` build/flash + API smoke

- **Build/flash:** PASS (`dut_uart` linked; bin ~1026960 B) on `…:f0:6e`
- **Boot:** `dut_uart init 9600 8N1`; OE cold; STA `192.168.1.10`; `HIL-02-F06F`
- **capabilities.summary:** exact match (`…dut_uart stream and path/line control`); `streams_supported=true`
- **API:** path rs485, line term+PU+PD, config 9600 8N1, stream open/close, config_set→`busy` while open
- **USB-RS485:** requested SN **`05AA02`** — **not present**; only **`03AA01`** on `/dev/ttyUSB0`
- **Physical A↔B echo:** **blocked** pending correct adapter

## 2026-11-18 — CuteCom peer RX (SN `03AA01`)

- CuteCom 9600 8N1; tester TX only via WiFi `dut_uart` (did not open ttyUSB)
- HIL path=rs485, K1–K4 on, writes `ok` (`HELLO-FROM-HIL-*`, later `AAA/BBB/CCC-TEST-*`)
- Human: HIL TX LED + converter LED lit earlier; **CuteCom received nothing**
- **Verdict:** FAIL peer RX on **03AA01** — superseded by 03AA02 PASS below

## 2026-11-18 — RS-485 PASS with USB peer SN `03AA02`

- Peer: `/dev/ttyUSB0` by-id `...03AA02...`; CuteCom 9600 8N1
- HIL→PC: CuteCom received `HELLO-03AA02-1..3` (WiFi `stream/write` only)
- PC→HIL: CuteCom TX `PC2HIL` → HIL `stream/read` `b'PC2HIL'`
- DE level 1 confirmed OK on bench
- **Verdict:** `B02_UART_PATH_LINE` + `B02_UART_STREAM` **PASS**

## 2026-11-18 — Automated echo with `03AA02` (CuteCom closed)

- Host opened by-id `...03AA02...` (DTR/RTS false); adapter stayed enumerated
- T1 HIL→PC `AUTO-HIL2PC-03AA02` PASS
- T2 PC→HIL `AUTO-PC2HIL-03AA02` PASS
- T3 HIL→PC `AUTO-HIL2PC-AGAIN` PASS
- **SUMMARY PASS** — confirms prior `03AA01` peer HW issue; firmware path OK
- Log: `/tmp/b02_rs485_auto_03AA02.log`

## 2026-11-18 — Baud × length matrix (`03AA02`)

- Bauds: 9600, 19200, 38400, 57600, 115200
- Lengths: 64, 256, 512, 1024 (both HIL→PC and PC→HIL)
- **20/20 PASS**, adapter stayed up
- Log: `/tmp/b02_rs485_baud_len.log`

## 2026-11-18 — Line resistor matrix (term × PU × PD)

- Peer `03AA02`; path=`rs485`; 9600; payload 128 B both dirs
- All 8 combinations of term/pullup/pulldown: **8/8 PASS**
- Short cable: no functional difference observed (as expected)
- Log: `/tmp/b02_rs485_line_matrix.log`

## 2026-11-18 — TTL path matrix (USB-TTL SN `05AA02`, crossed)

- `uart_path_set ttl` → K1 `on:false`; line all false
- Peer: `/dev/serial/by-id/...05AA02...`
- Bauds 9600…115200 × lengths 64/256/512/1024 both dirs
- **20/20 PASS**; adapter stayed up
- Log: `/tmp/b02_ttl_matrix.log`

## 2026-11-18 — Initial validation CLOSED

Tester initial bring-up closed: GPIO, relays, RS-485 (`03AA02`), TTL (`05AA02`),
baud/length and line matrices PASS. See `handoff.md` status CLOSED.

## 2026-11-18 — `B02_MAIN_UART_OLED` slow HMI + dual peer

- Build/flash PASS (`app_core_uart_oled`); STA `HIL-02-F06F`
- Peers: TTL `05AA02` (`ttyUSB0`), RS-485 `03AA02` (`ttyUSB1`)
- Slow OLED steps 1–6 (8 s holds): path/line/baud changes driven via `/v1/command`
- Echo PASS: TTL, RS485@9600, RS485@115200, TTL again
- **Human OLED visual match:** **PASS**
- **Human→architect note:** also show data bits + parity on OLED (not baud only)
- Log: `/tmp/b02_oled_slow_dual.log`

## 2026-11-18 — `B02_MAIN_UART_OLED_FRAMING` slow visual

- Build/flash PASS; STA `HIL-02-F06F`
- Demo steps: TTL@9600 → RS485@9600 → 115200 → 19200 → TTL@9600
- Expected line 3: `BAUD <n> 8N1`
- **Human visual:** **PASS** (`8N1` visible on line 3)
- Log: `/tmp/b02_oled_framing_slow.log`
- **Return→architect:** widen framing to habitual **`8E1` / `8O1`** (baud list OK; parity locked today). See `feedback.md`.

## 2026-11-18 — `B02_UART_FRAMING_HW_FULL` conclusive

- Build/flash PASS; STA `HIL-02-F06F`
- Peers matched: TTL `05AA02`, RS-485 `03AA02`
- API 27 profiles accept **27/27**; reject mark/space/1.5@8/9bit **PASS**
- Echo H↔P **64/64 PASS**: paths TTL+RS485 × baud 9600…115200 × framing 8N1/8E1/8O1; smoke 7E1+8N2 @9600 TTL
- OLED holds: `BAUD n 8N1|8E1|8O1` (plus 7E1/8N2 smoke)
- Log: `/tmp/b02_framing_full_test.log`
- **Verdict: PASS — CLOSED / DONE**




## Module status summary (2026-11-18)

| MAC | SoftAP/onboarding with known-good software | Verdict |
| --- | --- | --- |
| `8c:d0:b2:a7:f0:6e` | b02 SoftAP `HIL-02-F06F` — human connect OK | **usable spare for b02 dev** |
| `a4:cb:8f:22:59:80` | SoftAP start OK; client visibility/connect intermittent or fail | **defective unit** |
| `a4:cb:8f:21:df:98` | SoftAP start OK; visibility intermittent | **defective unit** |
