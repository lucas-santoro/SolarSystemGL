#pragma once

#include <string>

/**
 * @file WebPersistence.h
 * @brief Thin bridge between C++ and the browser for persistent storage and
 *        file I/O. On desktop every function is a sensible no-op.
 *
 * Web build maps:
 *  - `userSaveDir()`  -> "/user/"        (mounted IDBFS, persisted to IndexedDB)
 *  - read-only bundle -> "/presets/"     (Emscripten --preload-file)
 *
 * Desktop build maps:
 *  - `userSaveDir()`  -> "presets/"      (same directory used today)
 *  - no separate built-in concept; the modal shows one list.
 */
namespace web
{
    /**
     * @brief Mount IDBFS at `/user` and sync existing IDB contents into it.
     *
     * Blocks via Emscripten ASYNCIFY until the synchronous-style call returns.
     * Call once, before any read/write to `userSaveDir()`. Safe to call on
     * desktop (no-op, returns `true`).
     *
     * @return `true` on success. `false` if IndexedDB is unavailable (private
     *         browsing, very old browser); the caller should warn the user
     *         that saves won't survive a reload and continue with the
     *         in-memory FS.
     */
    bool initPersistence();

    /**
     * @brief Persist the contents of `/user/` to IndexedDB. Fire-and-forget;
     *        failures are logged to the browser console only. No-op on desktop.
     *
     * Call after every successful save, delete, or import.
     */
    void syncToIDB();

    /**
     * @brief Trigger a browser download of @p contents as @p filename. The
     *        contents are sent as a binary blob, so any byte sequence is safe.
     *        No-op on desktop.
     */
    void downloadBlob(const std::string& filename, const std::string& contents);

    /**
     * @brief Open the browser file picker (`.txt`), block until the user
     *        selects a file or cancels, then read it into memory. No-op on
     *        desktop (always returns `false`).
     *
     * @param outFilename Receives the picked file's base name on success.
     * @param outContents Receives the file's full text on success.
     * @return `true` if the user picked a file. `false` on cancel or read error.
     */
    bool pickFile(std::string& outFilename, std::string& outContents);

    /**
     * @brief Directory holding the user's read-write saves (with trailing
     *        slash). `"/user/"` on web, `"presets/"` on desktop.
     */
    const std::string& userSaveDir();

    /**
     * @brief Whether this build separates built-in presets from user saves.
     *
     * `true` on web (built-ins live under `/presets/`, user saves under
     * `/user/`). `false` on desktop (both share `presets/`).
     */
    bool hasSeparateBuiltIns();

    /**
     * @brief Read-only directory for shipped presets (with trailing slash).
     *        `"/presets/"` on web; same as `userSaveDir()` on desktop.
     */
    const std::string& builtInPresetsDir();
}
