/** HELPER FILE FOR TESTS.CC ***/

#include "../src/rule.h"
#include "../src/utils.h"

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

rule_t * rules;
rule_t * labels;
rule_t * minority;
int nrules;
int nsamples;
int nlabels;
int nminority;

NullLogger * logger;

int main(int argc, char* argv[])
{
    rules = NULL;
    labels = NULL;

    int verbosity = 0;

    if(argc > 1 && strcmp(argv[argc-1], "-v") == 0) {
        verbosity = 1;

        // Because we don't want Catch to get tripped up on the -v, we remove it
        argc--;
    }

    int nsamples_chk, nsamples_check;

    logger = new NullLogger();

    if(!logger)
    {
        printf("ERROR: Coult not create logger\n");
        return 1;
    }

    int r = rules_init("tests.out", &nrules, &nsamples, &rules, 1);
    int l = rules_init("tests.label", &nlabels, &nsamples_chk, &labels, 0);
    int m = rules_init("tests.minor", &nminority, &nsamples_check, &minority, 0);

    if(r != 0 || l != 0 || m != 0 || nsamples != nsamples_chk)
    {
        printf("ERROR: Could not load rules or samples\n");
        delete logger;
        return 1;
    }

    if(verbosity) {
        printf("\n/********** RULE DUMP **********/\n\n");
        printf("Printing rules:\n");
        rule_print_all(rules, nrules, nsamples, 1);
        printf("\n\nPrinting labels:\n");
        rule_print_all(labels, nlabels, nsamples, 1);
        printf("\n\nPrinting minority:\n");
        rule_print_all(minority, nminority, nsamples, 1);
        printf("\n\n/******** END RULE DUMP ********/\n\n\n");
    }

    int ret = Catch::Session().run(argc, argv);

    rules_free(rules, nrules, 1);
    rules_free(labels, nlabels, 0);

    delete logger;

    return (ret < 0xff ? ret : 0xff);
}
