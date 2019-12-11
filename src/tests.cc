#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "utils.h"

#define BUFSZ  512

TEST_CASE("parse_verbosity() works correctly", "[parse_verbosity]") {
  char* options = (char*)"rule,label"; // The actual input options
  char* input_str_cpy = (char*)malloc(BUFSZ);
  strcpy(input_str_cpy, "rule,label garbage");
  char verbstr[BUFSZ];
  std::set<std::string> verbosity;

  SECTION("Copies verbstr") {
    bool result = parse_verbosity(input_str_cpy, &verbstr[0], BUFSZ, &verbosity);
    REQUIRE(strcmp(verbstr, options) == 0);
    REQUIRE(result);
  }

  SECTION("Correctly accepts all verbosities") {
    strcpy(input_str_cpy, "minor,samples,progress,loud,silent,label,rule");
    bool result = parse_verbosity(input_str_cpy, &verbstr[0], BUFSZ, &verbosity);
    REQUIRE(result);
    const char* strs[] = {"rule", "label", "minor", "samples", "progress", "loud", "silent"};
    for(int i = 0; i < 7; i++) {
      REQUIRE(verbosity.count(strs[i]));
    }
    REQUIRE(verbosity.size() == 7);
  }

  SECTION("Correctly accepts some verbosities") {
    strcpy(input_str_cpy, "silent,samples");
    bool result = parse_verbosity(input_str_cpy, &verbstr[0], BUFSZ, &verbosity);
    REQUIRE(result);
    REQUIRE(verbosity.count("silent"));
    REQUIRE(verbosity.count("samples"));
    REQUIRE(verbosity.size() == 2);

    strcpy(input_str_cpy, ",silent,samples,");
    result = parse_verbosity(input_str_cpy, &verbstr[0], BUFSZ, &verbosity);
    REQUIRE(result);
    REQUIRE(verbosity.count("silent"));
    REQUIRE(verbosity.count("samples"));
    REQUIRE(verbosity.size() == 2);
  }

  SECTION("Correctly rejects wrong verbosities") {
    strcpy(input_str_cpy, "nope");
    bool result = parse_verbosity(input_str_cpy, &verbstr[0], BUFSZ, &verbosity);
    REQUIRE(!result);
    REQUIRE(!verbosity.size());

    strcpy(input_str_cpy, "rule,asdf");
    result = parse_verbosity(input_str_cpy, &verbstr[0], BUFSZ, &verbosity);
    REQUIRE(!result);
    REQUIRE(!verbosity.size());

    strcpy(input_str_cpy, "label,rul");
    result = parse_verbosity(input_str_cpy, &verbstr[0], BUFSZ, &verbosity);
    REQUIRE(!result);
    REQUIRE(!verbosity.size());

    strcpy(input_str_cpy, "samples,ilent");
    result = parse_verbosity(input_str_cpy, &verbstr[0], BUFSZ, &verbosity);
    REQUIRE(!result);
    REQUIRE(!verbosity.size());
  }
}
