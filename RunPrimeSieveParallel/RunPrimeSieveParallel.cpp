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

    const unsigned long long int maxPrime = 100LL * G1;
    std::ostringstream commandLine;
    unsigned long long memValues[] = {
        1LL * M1,
        2LL * M1,
        20LL * M1,
        50LL * M1,
        100LL * M1,
        10LL * G1,
        20LL * G1,
        40LL * G1,
    };
    int numMemValues = sizeof(memValues) / sizeof(memValues[0]);

    int sieveValues[] = { 16, 12, 10, 8, 4, 2, 1 };
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
            commandLine << " -l " << "p100Gmbig.csv";
            commandLine << std::endl;

            std::cout << commandLine.str();

            system(commandLine.str().c_str());
        }
    }
}
