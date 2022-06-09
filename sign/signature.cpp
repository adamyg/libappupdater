//  $Id: signature.cpp,v 1.9 2022/06/09 08:46:30 cvsuser Exp $
//
//  AutoUpdater: Manifest generation tool.
//

#if !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <cstdio>
#include <cstdlib>
#include <time.h>

#include <string>
#include <iostream>

#include <windows.h>

#pragma comment(lib, "Advapi32.lib")            // Cypt.h
#pragma comment(lib, "User32.lib")              // MessageBox

static std::string Signature(HANDLE hFile, const char *filename, int type);
static const char *Basename(const char *name);
static std::string SysError(const char *message, DWORD dwError = GetLastError());
static const char *ReplaceString(const char *str, const char *orig, const char *rep, char *buffer, size_t buflen);

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
//      filename - Installer exe image.
//      version - Version label.
//
//  Returns:
//      nothing
//
void
sign_manifest(const char *filename, const char *version, const char *hosturl)
{
    HANDLE hFile = 0;

    try {
        if (INVALID_HANDLE_VALUE == (hFile = CreateFileA(filename,
                    GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL))) {
            throw std::runtime_error(SysError("Unable to open source file"));
        }

        const char *basename = Basename(filename);
        char url[1024] = {0};

        ReplaceString(hosturl, "%%", basename, url, sizeof(url));

        DWORD fileSize = GetFileSize(hFile, NULL);
        const std::string sha = Signature(hFile, filename, CALG_SHA);
        const std::string md5 = Signature(hFile, filename, CALG_MD5);

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

        std::cout
            << "\n"
            << "\t<title></title>\n"
            << "\t<link></link>\n"
            << "\t<description></description>\n"
            << "\t<published>" << now << "</published>\n"
            << "\t<pubDate>" << pubDate << "</pubDate>\n"
            << "\t<enclosure url=\"" << url << "\"\n"
                << "\t\tos=\"windows\"\n"
                << "\t\tname=\"" << basename << "\"\n"
                << "\t\tversion=\"" << version << "\"\n"
                << "\t\tlength=\"" << fileSize << "\"\n"
                << "\t\tmd5Signature=\"" << md5 << "\"\n"
                << "\t\tshaSignature=\"" << sha << "\"\n"
                << "\t\ttype=\"application/octet-stream\" />\n"
            << "\n";

    } catch (std::exception &e) {
        std::string msg;
        msg += "An error occurred during signature operations\n\n";
        msg += e.what();
        MessageBoxA(NULL, msg.c_str(), "Signature", MB_ICONWARNING | MB_OK);

    } catch(...) {
        const char *msg = "An unknown error occurred during updater operations\n";
        MessageBoxA(NULL, msg, "Signature", MB_ICONERROR | MB_OK);
    }

    CloseHandle(hFile);
}


//  Function: Signature
//      Generate the manifest signature for the specified installer image.
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
Signature(HANDLE hFile, const char *filename, int type)
{
    BYTE ioBuffer[8 * 1024];                    // io buffer
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    BOOL ioResult = FALSE;
    DWORD ioSize = 0;

    // Get handle to the crypto provider
    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT) ||
            !CryptCreateHash(hProv, type, 0, 0, &hHash)) {
        DWORD dwStatus = GetLastError();
        if (hProv) CryptReleaseContext(hProv, 0);
        throw std::runtime_error(SysError("CryptAcquireContext failed."));
    }

    // Calculate hash
    if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, 0, FILE_BEGIN)) {
        throw std::runtime_error(SysError("Cannot read exe image."));
    }

    while ((ioResult = ReadFile(hFile, ioBuffer, sizeof(ioBuffer), &ioSize, NULL)) != FALSE) {
        if (0 == ioSize) {
            break;                              // EOF
        }
        if (! CryptHashData(hHash, ioBuffer, ioSize, 0)) {
            DWORD dwStatus = GetLastError();
            CryptReleaseContext(hProv, 0);
            CryptDestroyHash(hHash);
            throw std::runtime_error(SysError("CryptHashData failed."));
        }
    }
    if (!ioResult) {
        DWORD dwStatus = GetLastError();
        CryptReleaseContext(hProv, 0);
        CryptDestroyHash(hHash);
        throw std::runtime_error(SysError("Cannot read exe image."));
    }

    const char hashDigits[] = "0123456789abcdef";
    BYTE hashBuffer[20] = {0};                  // 16=MD5,20=SHA
    DWORD hashSize = sizeof(hashBuffer);
    std::string hash;

    if (CryptGetHashParam(hHash, HP_HASHVAL, hashBuffer, &hashSize, 0)) {
        for (DWORD i = 0; i < hashSize; i++) {
            hash += hashDigits[hashBuffer[i] >> 4];
            hash += hashDigits[hashBuffer[i] & 0xf];
        }
    } else {
        throw std::runtime_error(SysError("CryptGetHashParam failed."));
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    return hash;
}


static const char *
Basename(const char *filename)
{
    const char *name;
    return (NULL != (name = strrchr(filename, '/')))
                || (NULL != (name = strrchr(filename, '\\'))) ? name + 1 : filename;
}


static std::string
SysError(const char *message, DWORD dwError)
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


static const char *
ReplaceString(const char *str, const char *orig, const char *rep, char *buffer, size_t buflen)
{
    size_t leading;
    const char *p;

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

/*end*/

