#include "core/WebPersistence.h"

#ifdef SOLARSYSTEM_BUILD_WEB
#  include <emscripten.h>
#  include <cstdlib>
#endif

namespace web
{

#ifdef SOLARSYSTEM_BUILD_WEB

namespace
{
    // Mount IDBFS at /user and pull existing IDB contents into the in-memory
    // FS. Returns 1 on success, 0 on any failure (private browsing, IDB
    // disabled, etc.). EM_ASYNC_JS requires -sASYNCIFY at link time.
    EM_ASYNC_JS(int, jsInitUserFs, (), {
        try {
            // mkdir may throw if /user already exists from a prior init; ignore.
            try { FS.mkdir('/user'); } catch (e) {}
            FS.mount(IDBFS, {}, '/user');
            await new Promise((resolve, reject) => {
                FS.syncfs(true, err => err ? reject(err) : resolve());
            });
            return 1;
        } catch (e) {
            console.warn('[SolarSystemGL] IDBFS init failed:', e);
            return 0;
        }
    });

    // Fire-and-forget push of /user/* to IndexedDB. Errors are console-only;
    // we don't surface them back to C++ to keep the call site simple.
    EM_JS(void, jsSyncToIdb, (), {
        FS.syncfs(false, err => {
            if (err) console.error('[SolarSystemGL] IDBFS syncfs(out) failed:', err);
        });
    });

    // Create a Blob from the raw heap bytes and click a temporary <a download>.
    // Binary-safe (uses Uint8Array, not a UTF-8 string), though our saves are
    // plain ASCII today.
    EM_JS(void, jsDownloadBlob, (const char* nameUtf8, const char* dataPtr, int dataLen), {
        const name = UTF8ToString(nameUtf8);
        const bytes = HEAPU8.slice(dataPtr, dataPtr + dataLen);
        const blob = new Blob([bytes], { type: 'text/plain' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = name;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        // Revoke after a beat — revoking too early aborts the download in Safari.
        setTimeout(() => URL.revokeObjectURL(url), 1000);
    });

    // Open <input type=file> and wait for a selection. Resolves to a malloc'd
    // UTF-8 string "<filename>\n<contents>", or 0 on cancel / read error.
    // The newline separator is unambiguous because file.name never contains it.
    EM_ASYNC_JS(char*, jsPickFile, (), {
        return await new Promise((resolve) => {
            const input = document.createElement('input');
            input.type = 'file';
            input.accept = '.txt,text/plain';

            let settled = false;
            const settle = (value) => {
                if (settled) return;
                settled = true;
                resolve(value);
            };

            input.onchange = async () => {
                const file = input.files && input.files[0];
                if (!file) { settle(0); return; }
                try {
                    const text = await file.text();
                    const payload = file.name + '\n' + text;
                    const lenBytes = lengthBytesUTF8(payload) + 1;
                    const ptr = _malloc(lenBytes);
                    stringToUTF8(payload, ptr, lenBytes);
                    settle(ptr);
                } catch (e) {
                    console.error('[SolarSystemGL] file read failed:', e);
                    settle(0);
                }
            };

            // The file picker has no reliable cancel event across browsers.
            // The standard workaround: once the window regains focus we wait a
            // short beat for `onchange` to fire; if it doesn't, treat as cancel.
            const onFocus = () => {
                window.removeEventListener('focus', onFocus);
                setTimeout(() => settle(0), 500);
            };
            window.addEventListener('focus', onFocus);

            input.click();
        });
    });
}

bool initPersistence()
{
    return jsInitUserFs() != 0;
}

void syncToIDB()
{
    jsSyncToIdb();
}

void downloadBlob(const std::string& filename, const std::string& contents)
{
    jsDownloadBlob(filename.c_str(), contents.data(), static_cast<int>(contents.size()));
}

bool pickFile(std::string& outFilename, std::string& outContents)
{
    char* raw = jsPickFile();
    if (raw == nullptr) return false;

    const std::string payload(raw);
    std::free(raw);

    const auto sep = payload.find('\n');
    if (sep == std::string::npos) return false;

    outFilename = payload.substr(0, sep);
    outContents = payload.substr(sep + 1);
    return true;
}

const std::string& userSaveDir()
{
    static const std::string kDir = "/user/";
    return kDir;
}

bool hasSeparateBuiltIns()
{
    return true;
}

const std::string& builtInPresetsDir()
{
    static const std::string kDir = "/presets/";
    return kDir;
}

#else  // !SOLARSYSTEM_BUILD_WEB — desktop no-ops

bool initPersistence() { return true; }
void syncToIDB() {}
void downloadBlob(const std::string&, const std::string&) {}
bool pickFile(std::string&, std::string&) { return false; }

const std::string& userSaveDir()
{
    static const std::string kDir = "presets/";
    return kDir;
}

bool hasSeparateBuiltIns() { return false; }

const std::string& builtInPresetsDir()
{
    return userSaveDir();
}

#endif

} // namespace web
