# Known Bugs

## Log level not persisting across restarts
- **Where:** Display tab in desktop app
- **Behavior:** Setting log level to DEBUG and hitting Save does not persist. On relaunch it shows INFO.
- **Suspected cause:** The log level Combo widget uses a local `static int current_level` that defaults to 1 (INFO) and is never initialized from `config.log_level` on startup. The save may also not be writing the log level to config.ini correctly — needs investigation of whether `config.log_level` is included in `SaveToFile()` and whether the CONF screen's save path interacts.
