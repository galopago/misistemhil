# Implementer Backlog — `b06_hil`

- [x] `B06_MAIN_METERING_OLED` — port base + `ina219` + STA-connected metering
      OLED; no host API metering in that package
- [x] `B06_HOST_API_METERING` — `meter_read` / `meter_read_all` + capabilities
      (source done; tester owns build/validation)
- [x] `BASE-CHANGE-20261103-6` — add `partitions.csv` + 4 MB flash in
      `sdkconfig.defaults` (docs already absorbed; see `handoff.md`)
