#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace std;

int main() {
    cout << "\n=== Selective Build ===\n";

    vector<string> impacted;
    ifstream in("impacted_files.txt");
    string line;

    while (getline(in, line)) {
        impacted.push_back(line);
    }

    for (auto& file : impacted) {
        if (file.find(".cpp") == string::npos) continue;

        size_t pos = file.find("src/");
        if (pos == string::npos) continue;

        string relative = file.substr(pos); // src/auth/auth.cpp

        cout << "Building: " << relative << endl;

        string cmd = "make CMakeFiles/main.dir/" + relative + ".o";
        system(cmd.c_str());
    }

    return 0;
}