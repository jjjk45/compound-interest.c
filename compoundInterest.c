#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>

#define RATE_SCALE 1000000ULL

typedef enum {
    yearly = 1,
    monthly = 12
} frequency_t;

typedef struct {
    uint64_t principalInCents; //in total cents
    uint64_t interestRate2Decimals; //2 decimal/fractional digits
    uint32_t investmentYears;
    frequency_t compoundFreq;
} inputs_t;

bool parseDollarAmounts(const char *dollarAmount, uint64_t *out, const char *purpose) {
    if (!dollarAmount || !out)   {
        return false;
    }
    uint64_t dollars = 0;
    if (sscanf(dollarAmount, "%"SCNu64, &dollars) < 1)  {
        printf("couldn't parse %s\n", purpose);
        return false;
    }
    short unsigned int cents = 0;
    const char *dot = strchr(dollarAmount, '.');
    if (dot != NULL) {
        char centsStr[3];
        if (sscanf(dot + 1, "%2s", centsStr) == 1) {
            cents = atoi(centsStr);
            if (strlen(centsStr) == 1) {
                cents *= 10;
            }
        }
    }
    if (dollars > (UINT64_MAX - cents) / 100) {
        printf("dollar amount for %s is too large\n", purpose);
        return false;
    }
    *out = dollars * 100 + cents;
    return true;
}
bool parseFrequency(const char *freq, frequency_t *out, const char *purpose) {
    if (!freq || !out) {
        return false;
    }
    char frequency[9] = {0};
    size_t len = strlen(freq);
    if (len > 8)    {
        len = 8;
    }
    memcpy(frequency, freq, len);
    frequency[len] = '\0'; //is this necessary?
    for (int i=0; i<8; i++) {
        frequency[i] = tolower(frequency[i]);
    }
    if (strcmp(frequency, "yearly") == 0) {
        *out = yearly;
        return true;
    }
    if (strcmp(frequency, "monthly") == 0) {
        *out = monthly;
        return true;
    }
    printf("the frequency input for %s is invalid\n", purpose);
    return false;
}
bool parseInterestRate(const char *rate, uint64_t *out, const char *purpose) {
    if (!rate || !out) {
        return false;
    }
    uint64_t preDecimal = 0;
    if (sscanf(rate, "%"SCNu64, &preDecimal) < 1) {
        printf("couldn't parse %s\n", purpose);
        return false;
    }
    short unsigned int postDecimal = 0;
    const char *dot = strchr(rate, '.');
    if (dot != NULL) {
        char postStr[3];
        if (sscanf(dot + 1, "%2s", postStr) == 1) {
            postDecimal = atoi(postStr);
            if (strlen(postStr) == 1) {
                postDecimal *= 10;
            }
        }
    }
    if (preDecimal > (UINT64_MAX - 99) / 100) {
        printf("predecimal value for %s is too high", purpose);
        return false;
    }
    *out = preDecimal * 100 + postDecimal;
    return true;
}
bool parseYears(const char *input, uint32_t *out, const char *purpose) {
    if(!input || !out) {
        return false;
    }
    uint32_t length = 0;
    if(sscanf(input, "%"SCNu32, &length) < 1) {
        printf("couldn't parse %s\n", purpose);
        return false;
    }
    *out = length;
    return true;
}

uint64_t scaledPow(uint64_t base, uint64_t exp) {
    uint64_t result = RATE_SCALE;
    while (exp > 0) {
        if (exp & 1) {
            if (result > UINT64_MAX/base)    {
                return 0;
            }
            result = (result*base) / RATE_SCALE;
        }
        exp = exp >> 1;
        if (exp) {
            if (base > UINT64_MAX/base)  {
                return 0;
            }
            base = (base*base) / RATE_SCALE;
        }
    }
    return result;
}

int main(int argc, char *argv[])   {
    if (argc != 5)   {
        printf(
            "Usage: <program> <initial_deposit> <years> <interest_rate> <frequency>\n"
            "  initial_deposit: e.g. 1000.00\n"
            "  years:           e.g. 10\n"
            "  interest_rate:   annual percentage, e.g. 5.25 for 5.25%%\n"
            "  frequency:       'monthly' or 'yearly'\n"
        );
        return EXIT_FAILURE;
    }

    inputs_t I = {0};
    if (!parseDollarAmounts(argv[1], &I.principalInCents, "initial deposit") ||
        !parseFrequency(argv[4], &I.compoundFreq, "compound frequency") ||
        !parseInterestRate(argv[3], &I.interestRate2Decimals, "interest rate") ||
        !parseYears(argv[2], &I.investmentYears, "length"))   {
        return EXIT_FAILURE;
    }
    // Compute (1 + annual_rate/freq) in fixed-point.
    // interestRate2Decimals is rate * 100 (e.g. 5.25% -> 525).
    // As a true decimal: rate = interestRate2Decimals / 10000.
    // Rate per period:   rate / compoundFreq = interestRate2Decimals / (10000 * compoundFreq).
    // Scaled:            RATE_SCALE + (interestRate2Decimals * RATE_SCALE) / (10000 * compoundFreq)
    uint64_t divisor = 10000ULL * (uint64_t)I.compoundFreq;
    if (I.interestRate2Decimals > UINT64_MAX / RATE_SCALE) {
        printf("Interest rate is too large to compute\n");
        return EXIT_FAILURE;
    }
    uint64_t ratePerPeriodScaled = RATE_SCALE + (I.interestRate2Decimals*RATE_SCALE) / divisor;
    uint64_t totalPeriods = (uint64_t)I.compoundFreq * I.investmentYears; //without this cast it does 32-bit arithmetic REMEMBER THIS
    uint64_t growthMultiplierScaled = scaledPow(ratePerPeriodScaled, totalPeriods);
    if (growthMultiplierScaled == 0) {
        printf("scaledPow() returned 0, due to either division by 0 or overflow error\n");
        return EXIT_FAILURE;
    }
    if (I.principalInCents > UINT64_MAX / growthMultiplierScaled) {
        printf("Overflow computing final amount\n");
        return EXIT_FAILURE;
    }
    uint64_t finalAmountCents = (I.principalInCents*growthMultiplierScaled) / RATE_SCALE;

    printf("Final amount: $%" PRIu64 ".%02" PRIu64 "\n", finalAmountCents/100, finalAmountCents%100);
    return EXIT_SUCCESS;
}
