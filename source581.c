#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE 4096

/* Parses a run of digits starting at s[*pos] (bounded by len).
 * Requires between 1 and maxDigits digits, no leading zero unless the
 * value is exactly 0, and value <= maxValue. Digits are accumulated by hand.
 * Returns 1 and advances *pos on success, 0 on failure. */
static int parseNumber(const char* s, size_t len, size_t* pos,
                       int maxDigits, long maxValue, long* out)
{
    size_t i = *pos;
    int count = 0;
    long value = 0;

    while (i < len && isdigit((unsigned char)s[i])) {
        count++;
        if (count > maxDigits) return 0;
        value = value * 10 + (s[i] - '0');
        i++;
    }
    if (count == 0) return 0;
    if (count > 1 && s[*pos] == '0') return 0;   /* leading zero */
    if (value > maxValue) return 0;

    *out = value;
    *pos = i;
    return 1;
}

/* Validates one complete token (s[0..len)) against the full grammar. */
static int parseToken(const char* s, size_t len,
                      unsigned long* addr, int* port, int octets[4])
{
    size_t pos = 0;
    long v;
    unsigned long a = 0;
    int k;

    for (k = 0; k < 4; k++) {
        if (!parseNumber(s, len, &pos, 3, 255, &v)) return 0;
        octets[k] = (int)v;
        a = (a << 8) | (unsigned long)v;
        if (k < 3) {
            if (pos >= len || s[pos] != '.') return 0;
            pos++;
        }
    }

    if (pos == len) {                 /* no port */
        *addr = a;
        *port = -1;
        return 1;
    }

    if (s[pos] != ':') return 0;      /* stray '.' or anything else */
    pos++;
    if (!parseNumber(s, len, &pos, 5, 65535, &v)) return 0;
    if (pos != len) return 0;         /* trailing junk, second colon, etc. */

    *addr = a;
    *port = (int)v;
    return 1;
}

static int gOctets[4];  /* octets of the last extracted address, for display */

int extractIPv4(const char* str, unsigned long* outAddress, int* outPort)
{
    size_t i = 0, start, len;

    *outAddress = 0;
    *outPort = -1;
    if (str == NULL) return 0;

    while (str[i] != '\0') {
        /* skip garbage */
        if (!(isdigit((unsigned char)str[i]) || str[i] == '.' || str[i] == ':')) {
            i++;
            continue;
        }
        /* maximal run of token characters */
        start = i;
        while (str[i] != '\0' &&
               (isdigit((unsigned char)str[i]) || str[i] == '.' || str[i] == ':'))
            i++;
        len = i - start;

        while (len > 0 && (str[start] == '.' || str[start] == ':')) {
            start++;
            len--;
        }
         while (len > 0 &&
             (str[start + len - 1] == '.' || str[start + len - 1] == ':'))
            len--;

        {
            unsigned long a;
            int p, oct[4];
            if (parseToken(str + start, len, &a, &p, oct)) {
                *outAddress = a;
                *outPort = p;
                memcpy(gOctets, oct, sizeof(oct));
                return 1;
            }
        }
    }
    return 0;
}

int main(void)
{
    char line[MAX_LINE];

    for (;;) {
        unsigned long addr;
        int port;
        size_t n;

        printf("Enter text (END to quit): ");
        fflush(stdout);
        if (fgets(line, sizeof(line), stdin) == NULL) break;

        n = strlen(line);
        while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r'))
            line[--n] = '\0';

        if (strcmp(line, "END") == 0) break;

        if (extractIPv4(line, &addr, &port)) {
            printf("Extracted IPv4 address: %d.%d.%d.%d (decimal value: %lu, port: ",
                   gOctets[0], gOctets[1], gOctets[2], gOctets[3], addr);
            if (port < 0) printf("none)\n");
            else          printf("%d)\n", port);
        } else {
            printf("No valid IPv4 address found.\n");
        }
    }

    printf("Program terminated.\n");
    return 0;
}