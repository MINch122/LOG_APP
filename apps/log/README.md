# LOG

cFS application generated from `nasa/sample_app` @ `v7.0.0`.

## Integration

1. Place this directory under `apps/` in your cFS mission tree.
2. Add `log` to your `targets.cmake`.
3. Add to `cfe_es_startup.scr`:
   ```
   CFE_APP, log, LOG_Main, LOG, 80, 16384, 0x0, 0;
   ```

## Customization

- **Message IDs**: Edit `fsw/platform_inc/log_msgids.h`
  (or `config/` defaults for Draco+)
- **Performance ID**: Edit the perfids header
- **Table structure**: Edit the table header and `fsw/tables/log_tbl.c`
- **Commands**: Add new command codes and handlers following the existing pattern
