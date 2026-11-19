# Board Main UART OLED Architecture — `b02_hil`

Normative particular architecture for the **main product OLED screen** shown
while WiFi STA is connected: board identity plus a **minimal, human-readable**
summary of UART path, RS-485 line resistors (when relevant), and baud.

Source handoff: `agent-workspaces/architect/handoff.md`, `B02_MAIN_UART_OLED`.

Sister-board pattern: `b03_hil` `docs/board_main_relay_oled_architecture.md`
(STA connected → replace base `WIFI OK` five-line screen with particular
`FULL_FOUR_LINES`).

All agentic development methodology documentation in this firmware tree is
maintained in English.

## Purpose

After the board has STA IPv4 / identity, a human looking at the OLED MUST be
able to answer, without a host client:

1. Which board is this? (`HIL-02-<MAC4>`)
2. Is the DUT path **TTL** or **RS-485**?
3. If RS-485: are **term / pull-up / pull-down** on or off?
4. What **baud** is the DUT UART using right now?

Layout MUST stay minimal (four short lines, MEDIUM font, ≤16 characters each).

## Related documents

| Document | Role |
| --- | --- |
| `docs/board_uart_path_line_architecture.md` | Path / line / config shadows |
| `docs/board_uart_stream_architecture.md` | `dut_uart` (baud source) |
| `docs/board_host_api_relay_architecture.md` | Coil shadows (same K1–K4) |
| `docs/display_delivery_contract.md` | Only `app_core` → display |
| `docs/oled_text_display_interface.md` | `FULL_FOUR_LINES`, MEDIUM ≤16 |
| `docs/device_discovery_mdns_architecture.md` | Hostname form |
| `docs/wifi_provisioning_architecture.md` | Connected events |

## Normative language

`MUST` / `MUST NOT` / `SHOULD` / `MAY` as in base OLED docs.

## Scope

### In scope

- Particular STA-connected OLED layout (four lines)
- Exact string grammar (human HMI)
- When to show / hide vs base provisioning screens
- Refresh triggers (event-driven)
- `app_core` orchestration file boundaries

### Out of scope

- Showing IPv4 / full MAC on this screen (identity line is enough; IP remains
  in SoftAP UX / host `info`)
- SoftAP QR / connecting / failure / factory-reset screens (stay **base**)
- Icons, graphics, menus, touch
- Showing DE drive/receive bit (transient; not a stable HMI state)
- Stream open/closed indicator (optional future handoff)

## Visibility rules

```text
Provisioning SoftAP / QR / connecting / failure / factory-reset
        →  base OLED templates (unchanged)

STA connected (same event class as base WIFI OK):
  WIFI_PROV_EVENT_SUBMITTED_SUCCESS
  WIFI_PROV_EVENT_SAVED_SUCCESS
  WIFI_PROV_EVENT_RUNTIME_RESTORED
  WIFI_PROV_EVENT_CONNECT_RESTORED
        →  show UART main FULL_FOUR_LINES (NOT base WIFI OK five-line)

STA lost / reconnecting
        →  stop pushing UART main updates; follow base reconnect OLED rules
```

Normative:

- On `b02_hil`, `show_wifi_connected_display()` (or equivalent) **MUST NOT**
  paint base `FULL_FIVE_LINES` `WIFI OK` / IP / split MAC.
- It **MUST** paint the layout in **Main OLED layout** below.
- UART main updates **MUST NOT** run during SoftAP provisioning.
- Enter the screen only when identity `HIL-02-<MAC4>` is available (no
  `.local`).

## Main OLED layout

Template: `DISPLAY_TEMPLATE_FULL_FOUR_LINES`.  
Font policy: **MEDIUM** → max **16** printable ASCII characters per line.

### Canonical picture (RS-485 example)

```text
Line 0: HIL-02-A1B2
Line 1: PATH RS485
Line 2: T:1 U:1 D:0
Line 3: BAUD 9600 8N1
```

### Canonical picture (TTL example)

```text
Line 0: HIL-02-A1B2
Line 1: PATH TTL
Line 2: T:- U:- D:-
Line 3: BAUD 9600 8N1
```

### Line 0 — board identity

| Rule | Value |
| --- | --- |
| Content | Exact SoftAP / mDNS hostname **without** `.local` |
| Form | `HIL-02-<MAC4>` (11 characters typical) |
| Source | `device_identity` (same string used for discovery) |

**MUST NOT** append `.local`, IP, or RSSI on this line.

### Line 1 — electrical path

Exactly one of these two strings (uppercase path token):

| Path shadow | Line 1 exact string | Length |
| --- | --- | --- |
| TTL (K1 off) | `PATH TTL` | 8 |
| RS-485 (K1 on) | `PATH RS485` | 10 |

**MUST NOT** use other spellings (`rs485`, `Rs485`, `M:RS485`, etc.).

### Line 2 — line resistors (human glance)

Fixed three fields: **T** = termination (K2), **U** = pull-up (K4),
**D** = pull-down (K3).

Grammar when path is **RS485**:

```text
T:<t> U:<u> D:<d>
```

| Field | Char | Meaning |
| --- | --- | --- |
| `<t>` / `<u>` / `<d>` | `0` or `1` | Coil shadow off/on |

Example: `T:1 U:0 D:1` (13 characters).

Grammar when path is **TTL** (resistors not the active story; keep columns
stable so the eye does not hunt):

```text
T:- U:- D:-
```

Exact string (11 characters). **MUST NOT** invent `n/a` or blank line.

Separators: single spaces between the three tokens (exactly as shown).

### Line 3 — baud + framing (tester feedback 2026-11-18)

Humans need data bits and parity on glass, not baud alone.

```text
BAUD <n> <d><p><s>
```

| Token | Rule |
| --- | --- |
| `BAUD` | Literal prefix + one space |
| `<n>` | Decimal baud from current `uart_config` / `dut_uart` |
| `<d>` | Data bits digit (`8` in v1) |
| `<p>` | Parity letter: `N` = none, `E` = even, `O` = odd |
| `<s>` | Stop token: `1`, `2`, or `15` meaning **1.5** stop bits |

Exact spacing: one space after `BAUD`, one space between `<n>` and `<d><p><s>`.
**No** extra spaces inside the cluster. **No** decimal point on glass for 1.5.

| Example | Length | Notes |
| --- | --- | --- |
| `BAUD 9600 8N1` | 12 | Boot default |
| `BAUD 9600 8E1` | 12 | Even parity (habitual) |
| `BAUD 9600 8O1` | 12 | Odd parity (habitual) |
| `BAUD 115200 8N1` | 14 | Fits MEDIUM ≤16 |
| `BAUD 9600 5N15` | 13 | 5 data + 1.5 stop |

Full allow-list: `docs/board_uart_framing_architecture.md`. Format **MUST** come
from live `data_bits` / `parity` / `stop_bits` (map stop 1.5 → token `15`).

**MUST NOT** show `BAUD ?` / omit framing. If config init failed, keep last
known or default `BAUD 9600 8N1` after successful `dut_uart_init`.

## Data sources (implementer wiring)

| Line | Read from |
| --- | --- |
| 0 | Device identity hostname string |
| 1 | `board_ctrl` path shadow (`uart_path_get` / K1) |
| 2 | `board_ctrl` line shadows (`term`/`pullup`/`pulldown`); if path TTL force `-` |
| 3 | `dut_uart` config: baud + data_bits + parity + stop_bits → `BAUD n dps` |


All reads **MUST** use software shadows (same as host get commands), not pin
sampling.

## Refresh policy

Event-driven only (no periodic 1 Hz paint required):

| Trigger | Action |
| --- | --- |
| Enter STA connected (events listed above) | Full paint of all four lines |
| `uart_path_set` / `relay_set` affecting channel 1 | Repaint lines 1–2 (path + T/U/D form) |
| `uart_line_set` / `relay_set` affecting channels 2–4 | Repaint line 2 |
| `uart_config_set` or stream open that changes baud/framing | Repaint line 3 |

| Leave STA connected | Stop UART main pushes; base owns OLED again |

**MUST** route paints through `app_core` → `display_controller_*` per
`docs/display_delivery_contract.md`.  
**MUST NOT** call display APIs from `board_ctrl` or `host_api` handlers
directly; handlers notify `app_core` (callback/flag) or `app_core` polls
shadows when it already handles the command path.

Recommended structure (deterministic):

- New file `components/app_core/app_core_uart_oled.c` (+ header if needed)
  owning format + `app_core_uart_oled_show(void)` / `leave`.
- `app_core_wifi.c` `show_wifi_connected_display` → call show; set “uart OLED
  active” flag.
- After path/line/config mutations that `app_core` or host already funnel, call
  show **if** uart OLED active.

## Human HMI cheat-sheet (print-side legend)

Operators may use this as silk/manual legend (not drawn on OLED):

```text
Line0  Board id (mDNS / SoftAP name)
Line1  PATH TTL | PATH RS485
Line2  T=term  U=pull-up  D=pull-down   (1=on 0=off  -=N/A in TTL)
Line3  BAUD n 8N1   (n=baud; 8=data N=parity 1=stop)
```

## Acceptance criteria

1. STA connected shows four-line layout above; not base `WIFI OK` five-line.
2. Identity line matches `HIL-02-<MAC4>` without `.local`.
3. Path string exact `PATH TTL` / `PATH RS485`.
4. RS-485 shows `T:/U:/D:` with `0`/`1`; TTL shows `T:- U:- D:-`.
5. Line 3 tracks live baud **and** framing (`BAUD 9600 8N1` style; ≤16 chars).
6. Changing path/line/baud via host updates OLED without reboot while STA up.
7. SoftAP / failure UX unchanged (base).

## Non-goals

- IPv4 on this screen
- DE TX indicator
- Stream session open flag
- Animations or multi-page UI

## Open questions

- None for v1 layout (frozen for implementer).
