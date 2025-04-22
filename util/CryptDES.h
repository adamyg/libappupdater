#ifndef CRYPTDES_H_INCLUDED
#define CRYPTDES_H_INCLUDED
//  $Id: CryptDES.h,v 1.1 2025/04/21 13:58:28 cvsuser Exp $
//
//  AutoUpdater: DES crypt interface
//
//  This file is part of libappupdater (https://github.com/adamyg/libappupdater)
//
//  Copyright (c) 2024 - 2025, Adam Young
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

#if !defined(WINDOWS_MEAN_AND_LEAN)
#define  WINDOWS_MEAN_AND_LEAN
#include <windows.h>
#endif
#include <stdio.h>
#include <bcrypt.h>

#pragma comment(lib, "BCrypt.lib")

namespace Updater {

class DES {
public:
    struct DESKey {
        BYTE rgbIV[16];
        BYTE rgbAES128Key[16];
    };

#ifndef BCRYPT_SUCCESS
#define BCRYPT_SUCCESS(Status)  (((NTSTATUS)(Status)) >= 0)
#endif

    static const BYTE *
    Encrypt(const struct DESKey *key, const BYTE *text, DWORD text_length, DWORD *result)
    {
        BCRYPT_ALG_HANDLE hAesAlg = NULL;
        BCRYPT_KEY_HANDLE hKey = NULL;
        NTSTATUS status = (NTSTATUS)-1L;
        DWORD cbCipherText = 0,
            cbData = 0,
            cbKeyObject = 0,
            cbBlockLen = 0;
        PBYTE pbCipherText = NULL,
            pbKeyObject = NULL,
            pbIV = NULL;

        // Open an algorithm handle.
        if (!BCRYPT_SUCCESS(status =
                BCryptOpenAlgorithmProvider(&hAesAlg, BCRYPT_AES_ALGORITHM,NULL, 0))) {
            printf("**** Error 0x%x returned by BCryptOpenAlgorithmProvider\n", status);
            goto cleanup;
        }

        // Calculate the size of the buffer to hold the KeyObject.
        if (!BCRYPT_SUCCESS(status =
                BCryptGetProperty(hAesAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&cbKeyObject, sizeof(DWORD), &cbData, 0))) {
            printf("**** Error 0x%x returned by BCryptGetProperty\n", status);
            goto cleanup;
        }

        // Allocate the key object on the heap.
        pbKeyObject = (PBYTE)HeapAlloc(GetProcessHeap(), 0, cbKeyObject);
        if (NULL == pbKeyObject) {
            printf("**** Memory allocation failed\n");
            goto cleanup;
        }

        // Calculate the block length for the IV.
        if (!BCRYPT_SUCCESS(status =
                BCryptGetProperty(hAesAlg, BCRYPT_BLOCK_LENGTH, (PBYTE)&cbBlockLen, sizeof(DWORD), &cbData, 0))) {
            printf("**** Error 0x%x returned by BCryptGetProperty\n", status);
            goto cleanup;
        }

        // Determine whether the cbBlockLen is not longer than the IV length.
        if (cbBlockLen > sizeof(key->rgbIV)) {
            printf("**** block length is longer than the provided IV length\n");
            goto cleanup;
        }

        // Allocate a buffer for the IV. The buffer is consumed during the encrypt/decrypt process.
        pbIV = (PBYTE)HeapAlloc(GetProcessHeap(), 0, cbBlockLen);
        if (NULL == pbIV) {
            printf("**** memory allocation failed\n");
            goto cleanup;
        }
        memcpy(pbIV, key->rgbIV, cbBlockLen);

        if (!BCRYPT_SUCCESS(status =
                BCryptSetProperty(hAesAlg, BCRYPT_CHAINING_MODE, (PBYTE)BCRYPT_CHAIN_MODE_CBC, sizeof(BCRYPT_CHAIN_MODE_CBC), 0))) {
            printf("**** Error 0x%x returned by BCryptSetProperty\n", status);
            goto cleanup;
        }

        // Generate the key from supplied input key bytes.
        if (!BCRYPT_SUCCESS(status =
                BCryptGenerateSymmetricKey(hAesAlg, &hKey, pbKeyObject, cbKeyObject, (PBYTE)key->rgbAES128Key, sizeof(key->rgbAES128Key), 0))) {
            printf("**** Error 0x%x returned by BCryptGenerateSymmetricKey\n", status);
            goto cleanup;
        }

        // Get the output buffer size.
        if (!BCRYPT_SUCCESS(status =
                BCryptEncrypt(hKey, (PBYTE)text, text_length, NULL, pbIV, cbBlockLen, NULL, 0, &cbCipherText, BCRYPT_BLOCK_PADDING))) {
            printf("**** Error 0x%x returned by BCryptEncrypt\n", status);
            goto cleanup;
        }

        pbCipherText = (PBYTE) HeapAlloc(GetProcessHeap(), 0, cbCipherText);
        if (NULL == pbCipherText) {
            printf("**** Memory allocation failed\n");
            goto cleanup;
        }

        // Encrypt the plain-text buffer.
        if (!BCRYPT_SUCCESS(status =
                BCryptEncrypt(hKey, (PBYTE)text, text_length, NULL, pbIV, cbBlockLen, pbCipherText,cbCipherText, &cbData, BCRYPT_BLOCK_PADDING))) {
            printf("**** Error 0x%x returned by BCryptEncrypt\n", status);
            HeapFree(GetProcessHeap(), 0, pbCipherText);
            pbCipherText = NULL;
            goto cleanup;
        }

    cleanup:
        if (hAesAlg) {
            BCryptCloseAlgorithmProvider(hAesAlg, 0);
        }

        if (hKey) {
            BCryptDestroyKey(hKey);
        }

        if (pbKeyObject) {
            HeapFree(GetProcessHeap(), 0, pbKeyObject);
        }

        if (pbIV) {
            HeapFree(GetProcessHeap(), 0, pbIV);
        }

        if (pbCipherText) {
            *result = cbCipherText;
        }

        return pbCipherText;
    }

    static const BYTE *
    Decrypt(const struct DESKey *key, const BYTE *cipher, DWORD cipher_length, DWORD *result)
    {
        BCRYPT_ALG_HANDLE hAesAlg = NULL;
        BCRYPT_KEY_HANDLE hKey = NULL;
        NTSTATUS status = (NTSTATUS)-1L;
        DWORD cbPlainText = 0,
            cbData = 0,
            cbKeyObject = 0,
            cbBlockLen = 0;
        PBYTE pbPlainText = NULL,
            pbKeyObject = NULL,
            pbIV = NULL;

        // Open an algorithm handle.
        if (!BCRYPT_SUCCESS(status = 
                BCryptOpenAlgorithmProvider(&hAesAlg, BCRYPT_AES_ALGORITHM, NULL, 0))) {
            printf("**** Error 0x%x returned by BCryptOpenAlgorithmProvider\n", status);
            goto cleanup;
        }

        // Calculate the size of the buffer to hold the KeyObject.
        if (!BCRYPT_SUCCESS(status = 
                BCryptGetProperty(hAesAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&cbKeyObject, sizeof(DWORD), &cbData, 0))) {
            printf("**** Error 0x%x returned by BCryptGetProperty\n", status);
            goto cleanup;
        }

        // Allocate the key object on the heap.
        pbKeyObject = (PBYTE)HeapAlloc(GetProcessHeap(), 0, cbKeyObject);
        if (NULL == pbKeyObject) {
            printf("**** Memory allocation failed\n");
            goto cleanup;
        }

        // Calculate the block length for the IV.
        if (!BCRYPT_SUCCESS(status =
                BCryptGetProperty(hAesAlg, BCRYPT_BLOCK_LENGTH, (PBYTE)&cbBlockLen, sizeof(DWORD), &cbData, 0))) {
            printf("**** Error 0x%x returned by BCryptGetProperty\n", status);
            goto cleanup;
        }

        // Determine whether the cbBlockLen is not longer than the IV length.
        if (cbBlockLen > sizeof(key->rgbIV)) {
            printf("**** Block length is longer than the provided IV length\n");
            goto cleanup;
        }

        // Allocate a buffer for the IV. The buffer is consumed during the encrypt/decrypt process.
        pbIV = (PBYTE)HeapAlloc(GetProcessHeap(), 0, cbBlockLen);
        if (NULL == pbIV) {
            printf("**** Memory allocation failed\n");
            goto cleanup;
        }
        memcpy(pbIV, key->rgbIV, cbBlockLen);

        if (!BCRYPT_SUCCESS(status =
                BCryptSetProperty(hAesAlg, BCRYPT_CHAINING_MODE, (PBYTE)BCRYPT_CHAIN_MODE_CBC, sizeof(BCRYPT_CHAIN_MODE_CBC), 0))) {
            printf("**** Error 0x%x returned by BCryptSetProperty\n", status);
            goto cleanup;
        }

        // Generate the key from supplied input key bytes.
        if (!BCRYPT_SUCCESS(status =
                BCryptGenerateSymmetricKey(hAesAlg, &hKey, pbKeyObject, cbKeyObject, (PBYTE)key->rgbAES128Key, sizeof(key->rgbAES128Key), 0))) {
            printf("**** Error 0x%x returned by BCryptGenerateSymmetricKey\n", status);
            goto cleanup;
        }

        // Get the output buffer size.
        if (!BCRYPT_SUCCESS(status =
                BCryptDecrypt(hKey, (PBYTE)cipher, cipher_length, NULL, pbIV, cbBlockLen, NULL, 0, &cbPlainText, BCRYPT_BLOCK_PADDING))) {
            printf("**** Error 0x%x returned by BCryptDecrypt\n", status);
            goto cleanup;
        }

        pbPlainText = (PBYTE)HeapAlloc(GetProcessHeap(), 0, cbPlainText);
        if (NULL == pbPlainText) {
            printf("**** memory allocation failed\n");
            goto cleanup;
        }

        // Decrypt the cipher-text buffer.
        if (!BCRYPT_SUCCESS(status =
                BCryptDecrypt(hKey, (PBYTE)cipher, cipher_length, NULL, pbIV, cbBlockLen, pbPlainText, cbPlainText, &cbPlainText, BCRYPT_BLOCK_PADDING))) {
            printf("**** Error 0x%x returned by BCryptDecrypt\n", status);
            goto cleanup;
        }

    cleanup:
        if (hAesAlg) {
            BCryptCloseAlgorithmProvider(hAesAlg, 0);
        }

        if (hKey) {
            BCryptDestroyKey(hKey);
        }

        if (pbKeyObject) {
            HeapFree(GetProcessHeap(), 0, pbKeyObject);
        }

        if (pbIV) {
            HeapFree(GetProcessHeap(), 0, pbIV);
        }

        if (pbPlainText) {
            *result = cbPlainText;
        }

        return pbPlainText;
    }
};

} // namespace Updater

#endif //CRYPTDES_H_INCLUDED
