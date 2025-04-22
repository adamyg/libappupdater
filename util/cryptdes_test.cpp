////////////////////////////////////////////////////////////////////////////////
//  DES test application

#include "CryptDES.h"

static void
PrintBytes(const BYTE *pbPrintData, const DWORD cbDataLen) 
{
    DWORD dwCount = 0;
    for (dwCount = 0; dwCount < cbDataLen; dwCount++) {
        printf("0x%02x, ", pbPrintData[dwCount]);
        if (0 == (dwCount + 1) % 10) putchar('\n');
    }
}


int
main(int argc, char *argv[])
{
    const BYTE text[] = {
            "1. The quick brown fox jumps over the lazy dog. "\
            "2. The quick brown fox jumps over the lazy dog. "\
            "3. The quick brown fox jumps over the lazy dog. "\
            "4. The quick brown fox jumps over the lazy dog. "
        };
    struct Updater::DES::DESKey key = {
            { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F },
            { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F }
        };

    DWORD encrypted_length = 0;
    const BYTE *encrypted = Updater::DES::Encrypt(&key, text, sizeof(text), &encrypted_length);
    
    printf("plain_text=%u\n%s\n", sizeof(text), text);
    printf("encrypted_length=%u\n", (unsigned)encrypted_length);
    PrintBytes(encrypted, encrypted_length);
    printf("\n");

    DWORD decrypted_length = 0;
    const BYTE *decrypted = Updater::DES::Decrypt(&key, encrypted, encrypted_length, &decrypted_length);

    printf("decrypted_length=%u\n", (unsigned)decrypted_length);
    PrintBytes(decrypted, decrypted_length);
    printf("\n");
    printf("plain_text=%u\n%s\n", sizeof(decrypted), decrypted);
    return 0;
}

//end
