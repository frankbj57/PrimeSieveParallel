// RunPrimeSieveParallel.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <sstream>
#include <fstream>

int main()
{
#define G1 (1000000000)
#define M1 (1000000)
#define K1 (1000)

    const unsigned long long int maxPrime = 10LL * G1;
    std::ostringstream commandLine;
    unsigned long long memValues[] = {
        10LL * G1,
        5LL * G1,
        2LL * G1,
        1LL * G1,
        500LL * M1,
        200LL * M1,
        100LL * M1,
        50LL * M1,
        20LL * M1,
        10LL * M1,
        5LL * M1,
        2LL * M1,
        1LL * M1,
    };
    int numMemValues = sizeof(memValues) / sizeof(memValues[0]);

    int sieveValues[] = { 1, 2, 4, 6, 8, 10, 12, 14, 16, 20, 24, 28, 32 };
    int numSieveValues = sizeof(sieveValues) / sizeof(sieveValues[0]);

    for (int s = 0; s < numSieveValues; s++)
    {
        for (int m = 0; m < numMemValues; m++)
        {
            std::ostringstream commandLine;

            commandLine << "..\\..\\PrimeSieveParallel\\x64\\Release\\PrimeSieveParallel.exe ";
            commandLine << " -p " << maxPrime;
            commandLine << " -s " << sieveValues[s];
            commandLine << " -m " << memValues[m];
            commandLine << " -l " << "p10Gmbig.csv";
            commandLine << std::endl;

            std::cout << commandLine.str();

            system(commandLine.str().c_str());
        }
    }
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
