// PrimeSieveParallel.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <agents.h>

using namespace concurrency;
using namespace std;

typedef unsigned long int itype;
const int numberSieves = 2;
const itype MAXNUM = 10000;

class starter : public concurrency::agent
{
public:
	starter(ISource<itype>& primes, unbounded_buffer<itype>* users[], int numUsers)
		: inputPrimes(primes), primeUsers(users), numPrimeUsers(numUsers)
	{
	};

private:
	ISource<itype> &inputPrimes;
	unbounded_buffer<itype>** primeUsers;
	int numPrimeUsers;

	void run()
	{
		for (int i = 0; i < numPrimeUsers; i++)
		{
			send(primeUsers[i], (itype) 2);
		}

		// Wait for the first returned prime
		itype p = receive(inputPrimes);

		while (p < MAXNUM)
		{
			cout << "Prime found: " << p << endl;
			// Send to all users
			for (int i = 0; i < numPrimeUsers; i++)
			{
				send(primeUsers[i], p);
			}
			// Wait for the next prime
			p = receive(inputPrimes);
		}

		done();
	};
};

class sieve : public agent
{
public:
	sieve(ISource<itype>& primes, ITarget<itype>& primeReceiver, itype startRange, itype endRange)
		: inputPrimes(primes), outputPrimes(primeReceiver), start(startRange), end(endRange)
	{
	};

private:
	ISource<itype>& inputPrimes;
	ITarget<itype>& outputPrimes;
	itype start, end;

	void run()
	{
		itype rangeSize = end - start;
		char* numbers = new char[rangeSize];
		for (itype i = 0; i < rangeSize; i++)
			numbers[i] = 0;

		itype sqrtMAXNUM = sqrt((double)end) + 1;
		itype gate = 2;
		itype p = 2;

		// Wait for the first prime
		p = receive(inputPrimes);

		while (p < sqrtMAXNUM)
		{
			// Cross out all multiples
			for (itype i = p * p; ; i += p)
			{
				numbers[i] = 1;
				if (i > end - p)
					break;
			}

			// Find next prime
			p++;
			while (p < sqrtMAXNUM && numbers[p] != 0)
				p++;
			if (p > gate)
			{
				cout << "Current prime " << p << " is larger than " << gate << endl;
				gate <<= 1;
			}

			// Send this prime to the dispatcher
			if (p < sqrtMAXNUM)
			{
				send(outputPrimes, p);
				// Wait for the next prime
				p = receive(inputPrimes);
			}
		}

		// Remaining primes in sieve must also be sent
		do
		{
			while (p < MAXNUM && numbers[p] != 0)
				p++;
			send(outputPrimes, p);
			p++;
		} while (p < MAXNUM);

		send(outputPrimes, MAXNUM);

		done();
	};
};

int main()
{
	unbounded_buffer<itype> primes;

	unbounded_buffer<itype>* users[numberSieves];

	for (int i = 0; i < numberSieves; i++)
	{
		users[i] = new unbounded_buffer<itype>;
	}

	starter first(primes, users, numberSieves);

	sieve sieve1(*users[0], primes);

	first.start();
	sieve1.start();

	agent::wait(&first);
	agent::wait(&sieve1);
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
