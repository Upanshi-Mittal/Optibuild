#include "optibuild_cli.h"

#include <string>
#include <vector>

int main(int argc, char** argv) {
    return runOptiBuildCli(std::vector<std::string>(argv + 1, argv + argc), "--scan");
}
