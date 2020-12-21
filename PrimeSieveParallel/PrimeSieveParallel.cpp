// PrimeSieveParallel.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <agents.h>

#include <windows.h>
#include <sysinfoapi.h>

using namespace concurrency;
using namespace std;

#include "optionparser.h"

struct Arg : public option::Arg
{
	static option::ArgStatus Required(const option::Option& option, bool)
	{
		return option.arg == 0 ? option::ARG_ILLEGAL : option::ARG_OK;
	}
};

enum  optionIndex { UNKNOWN, SIEVES, MEMORY, ROUNDS, PRIME, HELP };

const option::Descriptor usage[] = {
{ UNKNOWN,  0, "", "",            Arg::None, "USAGE: primesieveparallel [options]\n\n"
										  "Options:" },
{ HELP,     0, "h", "help",      Arg::None,    "  \t--help  \tPrint usage and exit." },
{ SIEVES,   0, "s","sieves",    Arg::Required,"  -s <arg>, \t--sieves=<arg>"
										  "  \tMaximum number of sieves. Default is number of logical CPU cores" },
{ MEMORY,   0, "m","memory",    Arg::Required,"  -m<arg>, \t--memory=<arg>  \tMaximum amount of memory to use"
										  "  \t [-]<amount>[%]. Default is free physical RAM"
										  "  \t either absolute max, or '-' deduct from free physical RAM. '%' means this is in percent of that" },
{ ROUNDS,   0, "r","rounds",    Arg::Required, "  -r <num>, \t--rounds=<num>  \tMinimum number of rounds of sieves" },
{ PRIME,    0, "p","prime",     Arg::Required,"  -p <arg>, \t--prime=<arg>"
										  "  \tFind primes up to this number" },
{ UNKNOWN,  0, "", "",          Arg::None,
 "\nExamples:\n"
 "  primesieveparallel -s4 -m-5% -p 1000000\n"
},
{ 0, 0, 0, 0, 0, 0 } }; // End of table


typedef unsigned long long int itype;

const int MAXSIEVES = 32;
int numberSieves = MAXSIEVES;

// Range to survey/sift
//              123456789012345
itype MaxNum = 1000000000ULL;

// How much RAM to use
unsigned long long int maxMemory = 0xFFFFFFFFFFFFFFFFULL;

int minRounds = 1;

class sieve : public agent
{
public:
	sieve(itype *primes, itype numPrimes, itype startRange, itype endRange)
		: inputPrimes(primes), numPrimes(numPrimes), startRange(startRange), end(endRange)
	{
	};

	~sieve()
	{
		delete[] numbers;
	}

	itype getRangeSize() const { return end - startRange; }

	itype count = 0;

private:
	itype *inputPrimes;
	itype startRange, end;
	char* numbers = nullptr;
	itype numPrimes;

	itype nextPrime = 0;

	// Get the next prime from the common list
	// return MaxNum on end
	itype receive()
	{
		if (nextPrime < numPrimes)
		{
			itype currentPrime = inputPrimes[nextPrime];

			// Make the next prime ready

			nextPrime++;

			return currentPrime;
		}
		else
			return MaxNum;
	}

	void run()
	{
		itype rangeSize = end - startRange;
		numbers = new char[rangeSize];
		for (itype i = 0; i < rangeSize; i++)
			numbers[i] = 0;

		itype sqrtEnd = sqrt((double)end) + 1;
		itype p = 2;

		// Wait for the first prime
		p = receive();

		while (p < sqrtEnd)
		{
			// Cross out all multiples
			itype i;

			// Calculate starting point

			if (p * p > startRange)
				i = p * p - startRange;
			else
			{
				itype offset = (startRange % p);
				i = (offset == 0 && p != startRange) ? 0 : (p - offset);
			}

			if (i < rangeSize)
			{
				for (; ; i += p)
				{
					numbers[i] = 1;
					// Do the end condition before the increment, to ensure maximum number range within type
					if (i >= rangeSize - p)
						break;
				}
			}

			p = receive();
		}

		// Count my primes
		itype sieveSize = getRangeSize();
		for (itype i = 0; i < sieveSize; i++)
		{
			if (numbers[i] == 0)
			{
				count++;
			}
		}

		done();
	};
};

class starter : public concurrency::agent
{
public:
	starter()
	{
	};

private:
	sieve* sievesArray[MAXSIEVES];

	void run()
	{
		itype maxRangeSize = 0x100000000ULL - 1;

		// Calculate how many primes the starter must calculate on its own
		itype sqrtMaxNum = sqrt(double(MaxNum)) + 1;
		itype sqrtSqrtMaxNum = sqrt((double)sqrtMaxNum) + 1;

		// Prepare the primary sieve in the starter
		char* numbers = new char[sqrtMaxNum];
		for (itype i = 0; i < sqrtMaxNum; i++)
			numbers[i] = 0;

		unsigned long long int startUse = sizeof(starter) + sqrtMaxNum;
		maxRangeSize = (maxMemory - startUse) / numberSieves;
		if (maxRangeSize > (0xffffffffULL))
		{
			// C++ new only allows 4GB-1 for allocation
			maxRangeSize = 0xffffffffULL;
		}

		cout << "Each with a max range of " << maxRangeSize << endl << endl;
		cout << endl;
		cout << "Initial sieve size: " << sqrtMaxNum << endl;
		cout << "Initial sieve seeding prime size: " << sqrtSqrtMaxNum << endl;

		// Calculate the number of rounds and sieves on the basis of the max range
		// This is decided by the maximum number of bytes new char[] can allocate
		int totalSieves = (MaxNum - sqrtMaxNum + maxRangeSize - 1) / maxRangeSize;
		if (totalSieves < 1)
			totalSieves = 1;
		
		// Round to multiple of numberSieves per round
		int rounds = (totalSieves + numberSieves - 1) / numberSieves;
		if (rounds < minRounds)
			rounds = minRounds;

		totalSieves = rounds * numberSieves;

		cout << endl;
		cout << "Number of rounds: " << rounds << endl;
		cout << "Total number of sieves: " << totalSieves << endl;


		itype rangeSize = (MaxNum - sqrtMaxNum) / totalSieves;

		cout << endl;
		cout << "Range for each sieve: " << rangeSize << endl;

		itype numprimes = 0;

		// Calculate the first sqrtMaxNum primes
		itype p = 2;
		do
		{
			for (itype i = p * p; i < sqrtMaxNum; i += p)
			{
				numbers[i] = 1;
			}

			// Find next prime
			p++;
			while (p < sqrtSqrtMaxNum && (numbers[p] != 0))
				p++;

		} while (p < sqrtSqrtMaxNum);

		// Count my own primes
		for (int i = 2; i < sqrtMaxNum; i++)
		{
			if (numbers[i] == 0)
			{
				numprimes++;
			}
		}

		cout << "Initial sieve number of primes: " << numprimes << endl;

		// Put them in a list
		itype initialNumPrimes = numprimes;
		itype* initialPrimes = new itype[numprimes];
		itype index = 0;
		for (int i = 2; i < sqrtMaxNum; i++)
		{
			if (numbers[i] == 0)
			{
				initialPrimes[index] = i;
				index++;
			}
		}

		// The initial sieve can be deleted
		delete[] numbers;
		numbers = nullptr;

		for (int round = 0; round < rounds; round++)
		{
			cout << endl;
			cout << "Round: " << round << endl;

			for (int i = 0; i < numberSieves; i++)
			{
				itype start = sqrtMaxNum + (round * numberSieves + i) * rangeSize;
				itype end = start + rangeSize;

				if ((round == rounds - 1) && (i == numberSieves - 1))
					end = MaxNum;

				sievesArray[i] =
					new sieve(
						initialPrimes,
						initialNumPrimes,
						start,
						end);

				cout << "Making sieve " << round * numberSieves + i
					<< " with start " << start
					<< " and end " << end << endl;
				sievesArray[i]->start();
			}

			agent::wait_for_all(numberSieves, (agent**) sievesArray);

			// Now harvest the primes from all the sieves
			for (int s = 0; s < numberSieves; s++)
			{
				itype sievePrimes = sievesArray[s]->count;
				numprimes += sievePrimes;

				cout << "Sieve " << round * numberSieves + s << " has " << sievePrimes << " primes" << endl;
			}

			// Remove sieves
			for (int i = 0; i < numberSieves; i++)
			{
				delete sievesArray[i];
			}
		}

		cout << endl << numprimes << " primtal" << endl;
		done();
	};
};




int main(int argc, char* argv[])
{
	// Imbue with thousand separators because we have big numbers
	std::cout.imbue(std::locale("dk_DK"));
	// std::cout.imbue(std::locale(std::locale, new numpunct<char>()));

	argc -= (argc > 0); argv += (argc > 0); // skip program name argv[0] if present
	option::Stats  stats(usage, argc, argv);
	std::vector<option::Option> options(stats.options_max);
	std::vector<option::Option> buffer(stats.buffer_max);
	option::Parser parse(true, usage, argc, argv, &options[0], &buffer[0]);

	for (option::Option* opt = options[UNKNOWN]; opt; opt = opt->next())
		std::cout << "Unknown option: " << std::string(opt->name, opt->namelen) << "\n";

	for (int i = 0; i < parse.nonOptionsCount(); ++i)
		std::cout << "Non-option #" << i << ": " << parse.nonOption(i) << std::endl;

	if (parse.error() || options[HELP] || options[UNKNOWN] || parse.nonOptionsCount()) {
		option::printUsage(std::cout, usage, 40);
		return 1;
	}

	unsigned long long start = GetTickCount64();

	starter first;

	// Get number of processors (logical cores)
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);

	numberSieves = systemInfo.dwNumberOfProcessors;

	if (options[SIEVES])
	{
		int sieves = std::atol(options[SIEVES].arg);

		if (sieves > 0 && sieves < numberSieves)
			numberSieves = sieves;
	}

	numberSieves = min(MAXSIEVES, numberSieves);

	// Available amount of physical memory to avoid paging
	MEMORYSTATUSEX memoryInfo;
	memoryInfo.dwLength = sizeof(memoryInfo);
	GlobalMemoryStatusEx(&memoryInfo);

	maxMemory = memoryInfo.ullAvailPhys;

	if (options[MEMORY])
	{
		long long int memory = 0;
		const char* pMem = options[MEMORY].arg;
		memory = atoll(pMem);
		if (memory < 0)
			memory = -memory;

		if (pMem[strlen(pMem) - 1] == '%')
		{
			// Percentage of available memory
			// Convert percentage to absolute value
			memory = maxMemory * memory / 100;
		}

		if (*pMem == '-')
		{
			// Deduct from maximum
			maxMemory -= memory;
		}
		else
		{
			// Take straight value
			maxMemory = memory;
		}
	}

	if (options[PRIME])
	{
		itype maxnum = (itype)atoll(options[PRIME].arg);

		if (maxnum > 0)
			MaxNum = maxnum;
	}

	if (options[ROUNDS])
	{
		minRounds = atol(options[ROUNDS].arg);
		if (minRounds < 0)
			minRounds = -minRounds;

		if (minRounds < 1)
			minRounds = 1;
	}

	cout << "Searching for primes less than " << MaxNum << endl;
	cout << "Using " << numberSieves << " parallel sieves" << endl;

	first.start();

	agent::wait(&first);

	start = GetTickCount64() - start;

	cout << "Total time: " << (double)(start) / 1000 << " seconds" << endl;
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
