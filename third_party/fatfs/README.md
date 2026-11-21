# FatFs

This directory contains FatFs R0.15 with Espressif patch2, taken from
`components/fatfs/src` at ESP-IDF commit
`f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0` (v5.4.2).

The portable FatFs core is authored by ChaN and retains its source-file
copyright, permission, and disclaimer notices; see `LICENSE.txt`. The MaiaRTOS
configuration and `diskio` adapter are project files. Only `ff.c`, `ff.h`,
`ffunicode.c`, and `diskio.h` are compiled; `00readme.txt` and `00history.txt`
are retained as upstream documentation. ESP-IDF VFS, FreeRTOS synchronization,
SDK configuration, and storage glue are not used.

Keep the source notices and `LICENSE.txt` when redistributing.
