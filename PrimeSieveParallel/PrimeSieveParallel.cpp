// PrimeSieveParallel.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <algorithm>
#include <agents.h>

using namespace concurrency;
using namespace std;

typedef unsigned long long int itype;
const int numberSieves = 8;
const itype MAXNUM = 100000000000ULL;

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
			itype offset = (startRange % p);

			if (p * p > startRange)
				i = p * p - startRange;
			else
				i = (offset == 0 && p != startRange) ? 0 : (p - offset);

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
		agent* sievesArray[numberSieves];

		unbounded_buffer<itype> primeUsers[numberSieves];

		itype sqrtMAXNUM = sqrt(double(MAXNUM)) + 1;
		itype sqrtSqrtMAXNUM = sqrt((double)sqrtMAXNUM) + 1;
		char* numbers = new char[sqrtMAXNUM];
		for (itype i = 0; i < sqrtMAXNUM; i++)
			numbers[i] = 0;

		itype rangeSize = (MAXNUM - sqrtMAXNUM) / numberSieves;
		for (int i = 0; i < numberSieves; i++)
		{
			sievesArray[i] = new sieve(primeUsers[i], sqrtMAXNUM + i * rangeSize, sqrtMAXNUM + i * rangeSize + rangeSize);
			sievesArray[i]->start();
		}
		
		itype p = 2;
		itype numprimes = 0;
		do
		{
			// Send to all users
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

		// Send the rest of the first sqrtMAXNUM primes to the users
		while (p < sqrtMAXNUM)
		{
			// Send to all users
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

		// First my own primes
		for (p = 2; p < sqrtMAXNUM; p++)
		{
			if (numbers[p] == 0)
			{
				// cout << p << " ";
				numprimes++;
			}
		}

		// Now harvest the primes from all the sieves
		for (int s = 0; s < numberSieves; s++)
		{
			for (itype i = 0; i < rangeSize; i++, p++)
			{
				if (((sieve*)sievesArray[s])->numbers[i] == 0)
				{
					// cout << p << " ";
					numprimes++;
				}
			}
		}

		cout << endl << numprimes << " primtal" << endl;
		done();
	};
};

int main()
{
	starter first;

	first.start();

	agent::wait(&first);
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
