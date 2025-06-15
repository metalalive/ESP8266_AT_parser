#include "unity_fixture.h"

static void RunAllTests(void) {
    RUN_TEST_GROUP(EspUtility);
    RUN_TEST_GROUP(EspATcmd);
    RUN_TEST_GROUP(EspRespParser);
    RUN_TEST_GROUP(EspThread);
}

int main(int argc, const char *argv[]) { return UnityMain(argc, argv, RunAllTests); }
