#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>

#define RATE_SCALE 100

typedef enum    {
    yearly = 1,
    monthly = 12
} frequency_t;

typedef struct  {
    uint64_t principalInCents; //in total cents
    uint64_t interestRate2Decimals; //2 decimal/fractional digits
    uint32_t investmentYears;    //in years
    frequency_t compoundFreq;
} inputs_t;

bool parseDollarAmounts(const char *dollarAmount, uint64_t *out, const char *purpose)   {
    if(!dollarAmount || !out)   {
        return false;
    }
    uint64_t cents = 0;
    uint64_t dollars = 0;
    if(sscanf(dollarAmount, "%"SCNu64".%2"SCNu64, &dollars, &cents) < 1)  {
        printf("couldn't parse %s\n", purpose);
        return false;
    }
    if(cents < 10)      {
        cents *= 10;
    }
    dollars *= 100;
    dollars += cents;
    *out = dollars;
    return true;
}
bool parseFrequency(const char *freq, frequency_t *out, const char *purpose)    {
    if(!freq || !out)   {
        return false;
    }
    char frequency[9] = {0};
    size_t len = strlen(freq);
    if (len > 8)    {
        len = 8;
    }
    memcpy(frequency, freq, len);
    frequency[len] = '\0';
    for(int i=0; i<8; i++)  {
        frequency[i] = toLower(frequency[i]);
    }
    if(strcmp(frequency, "yearly") == 0)  {
        *out = yearly;
        return true;
    }
    if(strcmp(frequency, "monthly") == 0)  {
        *out = monthly;
        return true;
    }
    printf("the frequency input for %s is invalid\n", purpose);
    return false;
}
bool parseInterestRate(const char *rate, uint64_t *out, const char *purpose)    {
    if(!rate || !out) {
        return false;
    }
    uint64_t preDecimal = 0;
    uint16_t postDecimal = 0;
    if(sscanf(rate, "%" SCNu64 ".%2" SCNu16, &preDecimal, &postDecimal) < 1)   {
        printf("couldn't parse %s\n", purpose);
        return false;
    }
    if(preDecimal > (UINT64_MAX - 99))    { //2 digit max number
        printf("predecimal value for %s is too high", purpose);
        return false;
    }
    preDecimal *= 100;
    if(postDecimal < 10)    { //need to add a check for 01,02,03,etc.
        postDecimal *= 10;
    }
    preDecimal += postDecimal;
    *out = preDecimal;
    return true;
}
bool parseLength(const char *input, uint32_t *out, const char *purpose) {
    if(!input || !out)  {
        return false;
    }
    uint32_t length = 0;
    if(sscanf(input, "%"SCNu32, &length) < 1)   {
        printf("couldn't parse %s\n", purpose);
        return false;
    }
    *out = length;
    return true;
}

uint64_t scaledPow(uint64_t base, uint64_t exp)
{
    if(base == 0)   {
        return 0;
    }
    uint64_t result = RATE_SCALE;
    while(exp > 0) {
        if(exp & 1) {
            if(result > UINT64_MAX/base)    {
                return 0;
            }
            result = (result*base) / RATE_SCALE;
        }
        exp = exp >> 1;
        if(exp) {
            if(base > UINT64_MAX/base)  {
                return 0;
            }
            base = (base*base) / RATE_SCALE;
        }
    }
    return result;
}

int main(int argc, char *argv[])   {
    if(argc != 5)   {
        printf(
            "Please run with arguments: \n"
            "\tInitial deposit,\n"
            "\tLength of investment in years, \n"
            "\tInterest rate, X.XX\n"
            "\tCompound frequency, monthly or yearly\n"
        );
        return EXIT_FAILURE;
    }

    inputs_t I = {0};
    if(!parseDollarAmounts(argv[1], &I.principalInCents, "initial deposit") ||
        !parseFrequency(argv[4], &I.compoundFreq, "compound frequency") ||
        !parseInterestRate(argv[3], &I.interestRate2Decimals, "interest rate") ||
        !parseLength(argv[2], &I.investmentYears, "length"))   {
        return EXIT_FAILURE;
    }

    uint64_t ratePerPeriodScaled = RATE_SCALE + I.interestRate2Decimals;
    uint64_t totalPeriods = I.compoundFreq * I.investmentYears;
    uint64_t growthMultiplierScaled = scaledPow(ratePerPeriodScaled, totalPeriods);
    if(growthMultiplierScaled == 0) {
        printf("scaledPow() returned 0, due to either division by 0 or overflow error\n");
        return EXIT_FAILURE;
    }
    uint64_t finalAmountCents = (I.principalInCents*growthMultiplierScaled) / RATE_SCALE;

    printf("Final amount: %" PRIu64 ".%02" PRIu64 "\n", finalAmountCents/100, finalAmountCents%100);
    return EXIT_SUCCESS;
}
