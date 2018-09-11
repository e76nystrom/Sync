#!/cygdrive/c/Python27/python

from __future__ import print_function
import math
import sys

argLen = len(sys.argv)

if argLen < 2:
    exit()

val = sys.argv[1].lower()
if val.endswith("mm"):
    metric = True
    pitch = float(val[:-2])
else:
    metric = False
    tpi = float(val)

encoderPulse = 10160
leadscrewPitch = 5
motorSteps = 200
microSteps = 8

if argLen >= 3:
    encoderPulse = int(sys.argv[2])

if metric:
    nFactor = 1
    dFactor = 1

    while int(pitch) != pitch:
        pitch *= 10
        nFactor *= 10
    pitch = int(pitch)
        
    num = \
          (\
           (nFactor, "nfactor"), \
           (encoderPulse, "encoderPulse"), \
           (leadscrewPitch, "leadscrewPitch"), \
          )

    denom = \
            ( \
              (dFactor, "dFactor"), \
              (pitch, "pitch"), \
              (motorSteps, "motorSteps"), \
              (microSteps, "microSteps"), \
            )
else:
    nFactor = 5
    dFactor = 127

    # nFactor = 37
    # dFactor = 47 * 20

    while int(tpi) != tpi:
        tpi *= 10
        dFactor *= 10
    tpi = int(tpi)

    num = \
          (\
           (nFactor, "nfactor"), \
           (encoderPulse, "encoderPule"), \
           (tpi, "tpi"), \
           (leadscrewPitch, "leadscrewPitch"), \
          )

    denom = \
            ( \
              (dFactor, "dFactor"), \
              (motorSteps, "motorSteps"), \
              (microSteps, "microSteps"), \
            )

def calcPrimes(maxPrime):
    sieve = [True] * maxPrime
    sieve[0] = False
    sieve[1] = False

    for i in range(2, int(math.sqrt(maxPrime)) + 1):
        index = i * 2
        while index < maxPrime:
            sieve[index] = False
            index += i

    primes = []
    for i in range(maxPrime):
        if sieve[i] == True:
            primes.append(i)
    return(primes)

def factor(n):
    factors = []
    for i in primes:
        while n % i == 0:
            factors.append(i)
            n /= i
    return(factors)

primes = calcPrimes(128)

# for p in primes:
#     print(p)

print("numerator")
nFactors = []
for (n, name) in num:
    print("factor %-14s %5d -" % (name, n), end = " ")
    result = factor(n)
    if len(result) != 0:
        nFactors += result
        for n in result:
            print(n, end=' ')
    print()
print()

print("denominator")
dFactors = []
for (n, name) in denom:
    print("factor %-14s %5d -" % (name, n), end = " ")
    result = factor(n)
    if len(result) != 0:
        dFactors += result
        for n in result:
            print(n, end=' ')
    print()
print()

# print("remove common factors")
dResult = []
for d in dFactors:
    found = False
    # print("d %d" % (d))
    for (i, n) in enumerate(nFactors):
        if d == n:
            # print("found %d at %d\n" % (d, i))
            del nFactors[i]
            found = True
            break
    if not found:
        dResult.append(d)

cycle = 1
for n in nFactors:
    cycle *= n

output = 1
for d in dResult:
    output *= d

print("cycle  %4d - " % (cycle), end='')
for n in nFactors:
    print(n, end=' ')
print()

print("output %4d - " % (output), end='');
for d in dResult:
    print(d, end=' ')
print()

