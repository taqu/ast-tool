Intentionally holds no source files ast-tool recognizes (only this .txt), so
open_workspace()/ensure_all_loaded() report parsedCount == 0 and
failedCount == 0 — used to exercise the "empty workspace" diagnostic.
