// $Id: signmanifest.cpp,v 1.5 2025/04/23 11:17:53 cvsuser Exp $
//
//  AutoUpdater: Manifest generation tool.
//
//  This file is part of libautoupdater (https://github.com/adamyg/libappupdater)
//
//  Copyright (c) 2012 - 2025 Adam Young
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all
//  copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//  SOFTWARE.
//

#if !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif
#if !defined(NOMINMAX)
#define NOMINMAX
#endif

#include <cstdio>
#include <cstdlib>

#include <limits.h>
#include <time.h>
#include <assert.h>

#include <string>
#include <iostream>

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

#include "../libautoupdater.h"
#include "../ed25519/src/ed25519.h"

#include "../util/Hex.h"
#include "../util/Base64.h"
#include "../util/Util.h"

#include "signmanifest.h"

#pragma comment(lib, "Advapi32.lib")            // Cypt.h
#pragma comment(lib, "User32.lib")              // MessageBox

namespace {

    ///////////////////////////////////////////////////////////////////////////
    //  Support

    std::string
    SysError(const char *message, DWORD dwError = GetLastError())
    {
        std::string msg;
        LPSTR buf;

        if (message && *message) {
            msg = message, msg += ":\n\n";
        }

        if (FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            0, dwError, 0, (LPSTR)&buf, 0, NULL)) {
            msg += buf;
            LocalFree(buf);
        }

        return msg;
    }

    const char *
    ReplaceString(const char *str, const char *orig, const char *rep, char *buffer, size_t buflen)
    {
        size_t leading;
        const char* p;

        if (NULL == (p = strstr(str, orig))) {
#if defined(__WATCOMC__)
            std::snprintf(buffer, buflen, "%s", str);
#else
            _snprintf(buffer, buflen, "%s", str);
#endif
            return buffer;
        }

        if ((leading = p - str) >= buflen) {
            strncpy(buffer, str, buflen);

        } else {
            strncpy(buffer, str, leading);
#if defined(__WATCOMC__)
            std::snprintf(buffer + leading, buflen - leading, "%s%s", rep, p + strlen(orig));
#else
            _snprintf(buffer + leading, buflen - leading, "%s%s", rep, p + strlen(orig));
#endif
        }
        return buffer;
    }

    ///////////////////////////////////////////////////////////////////////////
    //  File object

    struct File {
        File() : fileBuffer(NULL), fileSize(0) {
        }

        void load(const char *filename) {
            HANDLE hFile;

            if (INVALID_HANDLE_VALUE == (hFile = CreateFileA(filename,
                        GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL))) {
                throw std::runtime_error(SysError("Unable to open source image."));
            }

            LARGE_INTEGER t_fileSize;
            BYTE *t_fileBuffer = NULL;

            if (! GetFileSizeEx(hFile, &t_fileSize) || t_fileSize.QuadPart == 0) {
                throw std::runtime_error(SysError("Empty source image."));
            }

            if (t_fileSize.QuadPart >= INT_MAX) {
                throw std::runtime_error(SysError("Source image exceeds 2GB."));
            }

            fileSize = t_fileSize.LowPart;

            if (NULL == (t_fileBuffer = static_cast<BYTE*>(malloc(fileSize)))) {
                throw std::runtime_error(SysError("Memory allocation error."));
            }

            DWORD ioSize = 0;

            if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, 0, FILE_BEGIN) ||
                    ! ReadFile(hFile, t_fileBuffer, fileSize, &ioSize, NULL) || ioSize != fileSize) {
                throw std::runtime_error(SysError("Cannot read source image.", GetLastError()));
            }

            fileBuffer = t_fileBuffer;
            CloseHandle(hFile);
        }

        ~File() {
            free((void*)fileBuffer);
        }

        const BYTE *fileBuffer;
        DWORD fileSize;
    };

};  // namespace anon


static std::string Hash(const File &file, int type);
static std::string Sign(const File &file, const struct SignKeyPair *key);

#if defined(__WATCOMC__) && (__WATCOMC__ <= 1300)
/*missing string operators*/
std::ostream& operator<<(std::ostream &stream, const std::string &s);
inline std::ostream& operator<<(std::ostream &stream, const std::string &s) {
    stream.write(s.c_str(), s.length()); return stream;
}
#endif //__WATCOMC__


//  Function: Manifest
//      Generate the manifest signature for the specified installer image.
//
//  Parameters:
//      filename - Installer image.
//      version - Version label.
//      url - URL to manifest.
//
//  Returns:
//      nothing
//

void
SignManifest(const char *filename, const char *version, const char *url)
{
    SignManifestEd(filename, version, url, NULL, 0);
}


//  Function: Manifest
//      Generate the manifest signature for the specified installer image.
//
//  Parameters:
//      filename - Installer image.
//      version - Version label.
//      url - URL to manifest.
//      keypair - Key-pair.
//      keyversion - KeyVersion.
//
//  Returns:
//      nothing
//

void
SignManifestEd(const char *filename, const char *version, const char *url,
        const struct SignKeyPair *keypair, unsigned keyversion)
{
    try {
        const char *basename = Updater::Util::Basename(filename);
        File file;

        file.load(filename);

        const std::string sha = Hash(file, CALG_SHA);
        const std::string md5 = Hash(file, CALG_MD5);
        const std::string dsa = (keypair ? Sign(file, keypair) : "");

#if defined(__MINGW64_VERSION_MAJOR)
        const __time64_t now = _time64(NULL);
#else
        const time_t now = time(NULL);
#endif
        struct tm pubtime = {0};
        char pubDate[32] = {0};

#if defined(__WATCOMC__)
        _localtime(&now, &pubtime); /* Wed, 01 Jan 2006 12:20:11 +0000 */
#else
        _localtime64_s(&pubtime, &now); /* Wed, 01 Jan 2006 12:20:11 +0000 */
#endif
        strftime(pubDate, sizeof(pubDate), "%a, %d %b %Y %H:%M:%S +0000", &pubtime);

        char t_url[1024] = {0};
        ReplaceString(url, "%%", basename, t_url, sizeof(t_url));

        std::cout
            << "\n"
            << "\t<title></title>\n"
            << "\t<link></link>\n"
            << "\t<description></description>\n"
            << "\t<published>" << now << "</published>\n"
            << "\t<pubDate>" << pubDate << "</pubDate>\n"
            << "\t<enclosure url=\"" << t_url << "\"\n"
                << "\t\tos=\"windows\"\n"
                << "\t\tname=\"" << basename << "\"\n"
                << "\t\tversion=\"" << version << "\"\n"
                << "\t\tlength=\"" << file.fileSize << "\"\n"
                << "\t\tmd5Signature=\"" << md5 << "\"\n"
                << "\t\tshaSignature=\"" << sha << "\"\n"
                << "\t\tedSignature=\"" << dsa << "\"\n"
                << "\t\tedKeyVersion=\"1." << keyversion << "\"\n"
                << "\t\ttype=\"application/octet-stream\" />\n"
            << "\n";

    } catch (std::exception &e) {
        std::string msg;

        msg += "An error occurred during signature operations\n\n";
        msg += e.what();
        MessageBoxA(NULL, msg.c_str(), "Signature", MB_ICONWARNING | MB_OK);

    } catch (...) {
        const char *msg = "An unknown error occurred during updater operations\n";

        MessageBoxA(NULL, msg, "Signature", MB_ICONERROR | MB_OK);
    }
}


//  Function: Hash
//      Generate the manifest hash for the specified installer image.
//
//  Parameters:
//      hFile - Image file handle.
//      filename - Filename of installer image.
//      type - Signature type (CALG_SHA, CALG_MD5).
//
//  Returns:
//      String containing the signature.
//
static std::string
Hash(const File &file, int type)
{
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    BOOL ioResult = FALSE;

    // Crypt provider handle
    if (! CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT) ||
            ! CryptCreateHash(hProv, type, 0, 0, &hHash)) {
        DWORD dwStatus = GetLastError();
        if (hProv) CryptReleaseContext(hProv, 0);
        throw std::runtime_error(SysError("CryptAcquireContext failed.", dwStatus));
    }

    // Calculate hash
    if (! CryptHashData(hHash, file.fileBuffer, file.fileSize, 0)) {
        DWORD dwStatus = GetLastError();
        CryptReleaseContext(hProv, 0);
        CryptDestroyHash(hHash);
        throw std::runtime_error(SysError("CryptHashData failed.", dwStatus));
    }

    // Export
    BYTE hashBuffer[20] = {0};                  // 16=MD5,20=SHA
    DWORD hashSize = sizeof(hashBuffer);
    std::string hash;

    if (CryptGetHashParam(hHash, HP_HASHVAL, hashBuffer, &hashSize, 0)) {
        hash = Updater::Hex::to_string(hashBuffer, hashSize);
    } else {
        throw std::runtime_error(SysError("CryptGetHashParam failed."));
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    return hash;
}


static std::string
Sign(const File& file, const struct SignKeyPair *key)
{
    uint8_t signature[ED25519_SIGNATURE_LENGTH] = {0};
    ed25519_sign(signature, file.fileBuffer, file.fileSize, key->public_key, key->private_key);

#if !defined(NDEBUG) // verify unit-test

    // success
    assert(1 == ed25519_verify(signature, file.fileBuffer, file.fileSize, key->public_key));
    {
        size_t size = file.fileSize;
        void *context;

        context = ed25519_verify_init(signature, key->public_key);
        for (const BYTE *message = file.fileBuffer; size;) {
            const size_t messagelen = (size > (8 * 1024) ? (8 * 1024) : size);

            ed25519_verify_update(context, message, messagelen);
            message += messagelen;
            size -= messagelen;
        }
        assert(1 == ed25519_verify_final(context));
    }
    
    // failure
    {
        uint8_t t_signature[sizeof(signature)] = {0};
        memcpy(t_signature, signature, sizeof(signature));
        t_signature[1] ^= 1;
        assert(1 != ed25519_verify(t_signature, file.fileBuffer, file.fileSize, key->public_key));
    }
    assert(1 != ed25519_verify(signature, file.fileBuffer, file.fileSize - 1, key->public_key));

#endif //NDEBUG

    return Updater::Base64::encode_to_string(signature, sizeof(signature));
}

/*end*/
