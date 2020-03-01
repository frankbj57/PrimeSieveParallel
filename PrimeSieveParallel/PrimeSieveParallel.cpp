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
itype maxRangeSize = 0x100000000ULL - 1;
// const itype maxRangeSize = 1238;
const int MAXSIEVES = 32;
int numberSieves = MAXSIEVES;
//                        123456789012345
const itype MAXNUM =     100000000000ULL;

class sieve : public agent
{
public:
	sieve(ISource<itype>& primes, itype startRange, itype endRange)
		: inputPrimes(primes), startRange(startRange), end(endRange)
	{
	};

	char* numbers = nullptr;

	~sieve()
	{
		delete[] numbers;
	}

private:
	ISource<itype>& inputPrimes;
	itype startRange, end;

	void run()
	{
		itype rangeSize = end - startRange;
		numbers = new char[rangeSize];
		for (itype i = 0; i < rangeSize; i++)
			numbers[i] = 0;

		itype sqrtEnd = sqrt((double)end) + 1;
		itype p = 2;

		// Wait for the first prime
		p = receive(inputPrimes);

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

			p = receive(inputPrimes);
		}

		// Remaining primes must also be received
		while (p < MAXNUM)
		{
			p = receive(inputPrimes);
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
	void run()
	{
		agent* sievesArray[MAXSIEVES];

		unbounded_buffer<itype> primeUsers[MAXSIEVES];

		// Calculate how many primes the starter must calculate on its own
		itype sqrtMAXNUM = sqrt(double(MAXNUM)) + 1;
		itype sqrtSqrtMAXNUM = sqrt((double)sqrtMAXNUM) + 1;

		cout << endl;
		cout << "Initial sieve size: " << sqrtMAXNUM << endl;
		cout << "Initial sieve seeding prime size: " << sqrtSqrtMAXNUM << endl;

		// Prepare the primary sieve in the starter
		char* numbers = new char[sqrtMAXNUM];
		for (itype i = 0; i < sqrtMAXNUM; i++)
			numbers[i] = 0;

		// Calculate the number of rounds and sieves on the basis of the max range
		// This is decided by the maximum number of bytes new char[] can allocate
		int totalSieves = (MAXNUM-sqrtMAXNUM+maxRangeSize-1) / maxRangeSize;
		if (totalSieves < 1)
			totalSieves = 1;
		// Round to multiple of numberSieves per round
		int rounds = (totalSieves + numberSieves - 1) / numberSieves;
		totalSieves = rounds * numberSieves;

		cout << endl;
		cout << "Number of rounds: " << rounds << endl;
		cout << "Total number of sieves: " << totalSieves << endl;

		itype rangeSize = (MAXNUM - sqrtMAXNUM) / totalSieves;

		cout << endl;
		cout << "Range for each sieve: " << rangeSize;
			   
		itype numprimes = 0;
		itype pp = 0;
		for (int round = 0; round < rounds; round++)
		{
			cout << endl;
			cout << "Round: " << round << endl;

			for (int i = 0; i < numberSieves; i++)
			{
				itype start = sqrtMAXNUM + (round * numberSieves + i) * rangeSize;
				itype end = start + rangeSize;

				sievesArray[i] = 
					new sieve(
						primeUsers[i], 
						start, 
						end);

				cout << "Making sieve " << round * numberSieves + i
					<< " with start " << start
					<< " and end " << end << endl;
				sievesArray[i]->start();
			}

			itype p = 2;

			// In the first round, calculate the first sqrtMAXNUM primes on the fly
			if (round == 0)
			{
				do
				{
					// Send to all users
					// cout << "Sending " << p << endl;
					for (int i = 0; i < numberSieves; i++)
					{
						asend(primeUsers[i], p);
					}

					for (itype i = p * p; i < sqrtMAXNUM; i += p)
					{
						numbers[i] = 1;
					}

					// Find next prime
					p++;
					while (p < sqrtSqrtMAXNUM && numbers[p] != 0)
						p++;

				} while (p < sqrtSqrtMAXNUM);

				// Make sure p is prime again
				while (p < sqrtMAXNUM && numbers[p] != 0)
					p++;

				// Count my own primes
				for (int i = pp = 2; i < sqrtMAXNUM; i++, pp++)
				{
					if (numbers[i] == 0)
					{
						// cout << pp << " ";
						numprimes++;
					}
				}

				cout << "Initial sieve number of primes: " << numprimes << endl;
			}

			// Send the rest of the first sqrtMAXNUM primes to the users
			// In rounds after round 0, get all primes from this loop
			while (p < sqrtMAXNUM)
			{
				// Send to all users
				// cout << "Sending " << p << endl;
				for (int i = 0; i < numberSieves; i++)
				{
					asend(primeUsers[i], p);
				}

				// Find next prime
				p++;
				while (p < sqrtMAXNUM && numbers[p] != 0)
					p++;
			}

			// Send end marker to all users
			for (int i = 0; i < numberSieves; i++)
			{
				send(primeUsers[i], MAXNUM);
			}

			agent::wait_for_all(numberSieves, sievesArray);

			// Now harvest the primes from all the sieves
			for (int s = 0; s < numberSieves; s++)
			{
				itype sievePrimes = 0;
				for (itype i = 0; i < rangeSize; i++, pp++)
				{
					if (((sieve*)sievesArray[s])->numbers[i] == 0)
					{
						// cout << pp << " ";
						sievePrimes++;
					}
				}
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




int main(int argc, char *argv[])
{
	argc -= (argc > 0); argv += (argc > 0); // skip program name argv[0] if present
	option::Stats  stats(usage, argc, argv);
	std::vector<option::Option> options(stats.options_max);
	std::vector<option::Option> buffer(stats.buffer_max);
	option::Parser parse(true, usage, argc, argv, &options[0], &buffer[0]);

	if (parse.error())
		return 1;

	if (options[HELP] || argc == 0) {
		option::printUsage(std::cout, usage, 40);
		return 0;
	}


	for (int i = 0; i < parse.optionsCount(); ++i)
	{
		option::Option& opt = buffer[i];
		fprintf(stdout, "Argument #%d is ", i);
		switch (opt.index())
		{
		case HELP:
			// not possible, because handled further above and exits the program
		case SIEVES:
			if (opt.arg)
				fprintf(stdout, "--sieves with optional argument '%s'\n", opt.arg);
			else
				fprintf(stdout, "--sieves without the optional argument\n");
			break;
		case MEMORY:
			if (opt.arg)
				fprintf(stdout, "--memory with optional argument '%s'\n", opt.arg);
			else
				fprintf(stdout, "--memory without the optional argument\n");
			break;
		case PRIME:
			if (opt.arg)
				fprintf(stdout, "--prime with optional argument '%s'\n", opt.arg);
			else
				fprintf(stdout, "--prime without the optional argument\n");
			break;
		case ROUNDS:
			if (opt.arg)
				fprintf(stdout, "--rounds with optional argument '%s'\n", opt.arg);
			else
				fprintf(stdout, "--rounds without the optional argument\n");
			break;
		}
	}


	for (option::Option* opt = options[UNKNOWN]; opt; opt = opt->next())
		std::cout << "Unknown option: " << std::string(opt->name, opt->namelen) << "\n";

	for (int i = 0; i < parse.nonOptionsCount(); ++i)
		std::cout << "Non-option #" << i << ": " << parse.nonOption(i) << std::endl;

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

	maxRangeSize = memoryInfo.ullAvailPhys / numberSieves;
	if (maxRangeSize > (0xffffffffULL))
	{
		// C++ new only allows 4GB-1 for allocation
		maxRangeSize = 0xffffffffULL;
	}

	cout << "Searching for primes less than " << MAXNUM << endl;
	cout << "Using " << numberSieves << " parallel sieves" << endl;
	cout << "Each with a max range of " << maxRangeSize << endl << endl;

	first.start();

	agent::wait(&first);

	start = GetTickCount64() - start;

	cout << "Total time: " << (double)(start / 1000) << " seconds" << endl;
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
