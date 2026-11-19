# SoftAP visibility — detection plan (tester)

Goal: **name the failing layer** (app config vs driver/runtime vs host-side disturbance vs unit state), not re-audit SoftAP source.

Constraint: **do not write app/bootloader on boards that currently SoftAP-visible** (b03 now; b06 only if human explicitly allows). Prefer button credential-erase. Fail DUT units may take diagnostic flashes.

## Already ruled out (do not re-test as primary)

- SoftAP C source / SSID field layout
- Missing SoftAP NVS keys (`ap.ssid`, `chan`, `bcn.interval`, …)
- App image identity alone (b06 image on DUT still invisible; b03 different image visible)

## Protocol P0 — Visibility-first (every board, every run)

1. Power / USB connect.
2. Button credential erase → wait SoftAP (OLED / known identity).
3. **Human WiFi scan FIRST** (phone + laptop). Record: visible Y/N, SSID string, time.
4. **Only then** optional serial peek (10 s) — or skip if OLED already shows SoftAP identity.
5. **Only after visibility recorded** may host run `esptool` / dumps.
6. Log: board MAC, identity, visible Y/N, whether any esptool ran before the scan.

Pass/fail of this protocol is the ground truth. No analysis without P0 row.

## Phase A — Reproduce host-disturbance hypothesis (no firmware write)

Hypothesis: heavy `esptool`/`read_flash`/stub cycles before SoftAP check correlate with invisibility (b06: visible → later invisible after dump day).

| Step | Action | Board | Pass criterion |
| --- | --- | --- | --- |
| A1 | P0 only (no esptool before scan) | b06 after button erase | SoftAP visible? |
| A2 | P0 only | DUT failing | SoftAP visible? (expect N) |
| A3 | P0 only | b03 (control) | SoftAP visible? (expect Y) |
| A4 | If A1 = Y: run same dump set as 2026-11-18, then button erase + P0 again | b06 | Did visibility flip to N? |

- If A1=Y and A4=N → **host dump/reset path is implicated**; stop blaming SoftAP app.
- If A1=N already → b06 SoftAP broken in current state without new dumps; note and continue Phase B.

## Phase B — Runtime SoftAP config evidence (serial only, stock firmware)

On **one invisible** (DUT) and **one visible** (b03), after P0:

1. Capture 15 s boot/SoftAP log (full, not filtered).
2. Extract and table: `ssid=`, `mode : softAP`, MAC AP, `phy_init` lines, any `W (` / `E (`.
3. Compare side-by-side. Any unique error on invisible only → chase that string in IDF.

No code change yet.

## Phase C — Diagnostic probe firmware (implementer → flash **only on invisible DUT**)

Architect/implementer add a **temporary** build that after SoftAP start logs once:

- `esp_wifi_get_config(WIFI_IF_AP, …)` — ssid, channel, authmode, ssid_hidden, max_connection, beacon_interval
- `esp_wifi_get_max_tx_power` / current TX power
- WiFi country / protocol bitmap if available
- SoftAP associated station count

Tester flashes **only** failing DUT(s), runs P0 + serial capture of those lines.

Compare printed config on invisible DUT vs same probe on b03 **only if human authorizes** a probe flash on b03; otherwise compare DUT probe output to Espressif docs / expected OPEN/ch1/hidden=0.

Decision:

- If `ssid_hidden=1` or channel absurd or tx_power tiny → **config/runtime cause named**.
- If config looks identical to a known-good expectation and still invisible → **below app config** (PHY/driver/TX path); escalate with that evidence.

## Phase D — Independent over-the-air confirmation (optional)

If available: second ESP sniffer / Wireshark + ESP-H2 / phone app that shows **hidden** SSIDs and channel energy on ch1.

- Beacon frames present but clients hide OPEN? → client/policy issue.
- No beacons at all on ch1 from DUT MAC → TX/PHY path.
- Beacons on unexpected channel → channel mismatch.

## Stop conditions

| Finding | Conclusion to record |
| --- | --- |
| A4 flips b06 visible→invisible | Host flash/read procedure is a trigger; SoftAP app not root |
| Probe shows hidden/wrong ch/tx | Named software/runtime misconfig |
| Probe OK + no beacons on air | Named as PHY/TX path; SoftAP app closed |
| Probe OK + beacons on air + phones miss | Client/filter issue |

## Updated finding (2026-11-18 late)

OPEN SoftAP client visibility is **intermittent** on DUT(s): same firmware, serial
`AP_START` OK, SSID sometimes in scan / sometimes absent. Do not treat single
ch1/ch6 samples as final root cause without N-trial counts.

## Immediate next action — intermittency characterization (OPEN only)

**CLOSED 2026-11-18** — human cancelled further busywork trials; tester workstream
closed and returned to architect. Keep this plan as historical detection notes only.
