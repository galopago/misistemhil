# Implementer Handoff — `b06_hil`

All agentic development methodology documentation in this firmware tree is
maintained in English.

Before starting, read `ROLE.md`, `../../AGENTS.md`, and
`../../docs/methodology.md`. Confirm this task is **firmware implementation**,
not architecture-only docs or formal tester validation reports.

## Build-config follow-up: `BASE-CHANGE-20261103-6` (done)

Status: **done** — `partitions.csv` + `sdkconfig.defaults` / `sdkconfig` set to
4 MB custom single-factory table; fullclean / build / flash verified
(factory **4032 K**, ~**76 %** free of app partition). NVS layout unchanged
(`0x9000` / `0x6000`). No dual-OTA.

Normative: `docs/flash_partition_architecture.md`,
`docs/board_hardware_description.md` (b06 adopts 4 MB layout).

---

## Active package: `B06_HOST_API_METERING` (implemented — awaiting tester)

Status: **implemented** — source complete; formal build/hardware validation is
tester-owned. Implementer does not treat compile as a closeout requirement for
this session.

### Normative source of truth

`docs/board_host_api_metering_architecture.md`

Architect handoff: `../architect/handoff.md`, id `B06_HOST_API_METERING`.

### Work completed

1. New component `components/app_meter/` with normative sample API:
   - `app_core_meter_init`
   - `app_core_meter_read_channel`
   - `app_core_meter_read_all`
   - Mutex serializes host samples with OLED metering path.
2. `app_core_metering` OLED loop samples via `app_core_meter_read_all` (no
   direct I2C in the display path).
3. `host_api` commands `meter_read` / `meter_read_all` with JSON schemas,
   soft per-channel `ok: false`, hard `bad_request` on bad args.
4. `capabilities` advertises both metering commands; `streams: []`; no
   `echo` / `loopback`.
5. `ina219` has no host API includes; `host_api` does not include `ina219.h`
   (calls `app_meter` only).

### Modified / added paths

- `components/app_meter/` (new)
- `components/app_core/app_core_metering.c`, `CMakeLists.txt`
- `components/host_api/host_api_command.c`, `host_api_capabilities.c`,
  `CMakeLists.txt`

### Explicit non-goals (still out)

- Metering streams
- Host API on SoftAP
- OLED power display
- Shunt calibration commands

### Next owner

**Tester** — build/flash and validate acceptance criteria 1–9.

---

## Prior package: `B06_MAIN_METERING_OLED` (implemented — awaiting tester)

Normative: `docs/board_main_metering_oled_architecture.md`.

Summary: base port + `ina219` + STA-connected `FULL_FOUR_LINES` metering OLED
with fixed current sign column.
