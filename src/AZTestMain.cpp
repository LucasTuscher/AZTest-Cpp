#include <AZTest/AZTest.h>

int main(int argc, char** argv) {
    AZTest::InitializeDefaultReporter();
    return AZTest::RunWithArgs(argc, argv);
}
