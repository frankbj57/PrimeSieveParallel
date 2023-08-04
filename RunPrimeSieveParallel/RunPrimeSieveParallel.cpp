// RunPrimeSieveParallel.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <sstream>
#include <fstream>

int main()
{
    const unsigned long long int maxPrime = 1000000000;
    std::ostringstream commandLine;

    std::ofstream logfile("Results.txt", std::ios_base::out | std::ios_base::trunc);

    logfile << maxPrime << std::endl;

    logfile.close();

    unsigned long long memValues[] = {
        16000000000,
        8000000000, 4000000000, 2000000000, 1000000000, 500000000, 250000000, 100000000,
        50000000, 20000000, 10000000, 8000000, 6000000, 4000000, 2000000, 1000000
    };
    int numMemValues = sizeof(memValues) / sizeof(memValues[0]);

    int sieveValues[] = { 1, 2, 4, 5, 6, 8, 10, 12, 14, 16 };
    int numSieveValues = sizeof(sieveValues) / sizeof(sieveValues[0]);

    for (int s = 0; s < numSieveValues; s++)
    {
        for (int m = 0; m < numMemValues; m++)
        {
            std::ostringstream commandLine;

            commandLine << "..\\..\\PrimeSieveParallel\\x64\\Release\\PrimeSieveParallel.exe ";
            commandLine << " -p " << maxPrime;
            commandLine << " -s " << sieveValues[s];
            commandLine << " -m " << memValues[m] << std::endl;

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
