#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef enum    {
    yearly,
    monthly,
    daily
} frequency_t;
typedef struct  {
    uint64_t principal; //in total cents
    uint64_t contribution; //in total cents
    unsigned int interestRate;
    unsigned int length;
    frequency_t compoundFreq;
    frequency_t contributionFreq;
} inputs_t;

bool parseDollarAmounts(const char *dollarAmount, uint64_t *out, const char *purpose)   {
    if(!dollarAmount || !out)   {
        return false;
    }
    unsigned int cents = 0;
    unsigned long int dollars = 0;
    if(sscanf(dollarAmount, "%lu.%2u", &dollars, &cents) < 1)  {
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
    if(strcmp(freq, "yearly") == 0)  {
        *out = yearly;
        return true;
    }
    if(strcmp(freq, "monthly") == 0)  {
        *out = monthly;
        return true;
    }
    if(strcmp(freq, "daily") == 0)  {
        *out = daily;
        return true;
    }
    printf("the frequency input for %s is invalid\n", purpose);
    return false;
}

int main(int argc, char *argv[])   {
    if(argc != 7)   {
        printf(
            "Please run with arguments: \n"
            "Initial deposit,\n"
            "Length of investment in years,\n"
            "Interest rate,\n"
            "Compound frequency,\n"
            "Contribution amount,\n"
            "and Contribution frequency\n"
        );
        return EXIT_FAILURE;
    }
    inputs_t *I = malloc(sizeof(inputs_t));
    if(!I)  {
        perror("malloc failed, inputs_t *I");
        return EXIT_FAILURE;
    }

    int exitStatus = EXIT_FAILURE;

    if(!parseDollarAmounts(argv[1], &I->principal, "initial deposit") ||
        !parseDollarAmounts(argv[5], &I->contribution, "contribution") ||
        !parseFrequency(argv[4], &I->compoundFreq, "compound frequency") ||
    !parseFrequency(argv[6], &I->contributionFreq, "contribution frequency"))   {
        goto the_frees;
    }




    exitStatus = EXIT_SUCCESS;
    the_frees:
        free(I);
        return exitStatus;
}
