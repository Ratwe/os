#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <stdbool.h>
#include <signal.h>

#define WRITERS_COUNT 3
#define READERS_COUNT 5

HANDLE canReadEvent;
HANDLE canWriteEvent;
HANDLE mutex;

LONG activeReadersCount = 0;
LONG activeWriter = false;
LONG waitingReadersCount = 0;
LONG waitingWritersCount = 0;

DWORD thread_id[WRITERS_COUNT + READERS_COUNT];

int value = 0;
int fl = 1;
int handler(int sig_num)
{
	printf("Pid %d handled sig_num %d\n", getpid(), sig_num);
	fl = 0;
}
void start_write()
{
	InterlockedIncrement(&waitingWritersCount);
	if (activeReadersCount > 0 || activeWriter)
		WaitForSingleObject(canWriteEvent, INFINITE);
	InterlockedDecrement(&waitingWritersCount);
	InterlockedExchange(&activeWriter, true);
}
void stop_write()
{
	ResetEvent(canWriteEvent);
	InterlockedExchange(&activeWriter, false);
	if (waitingReadersCount > 0)  
		SetEvent(canReadEvent);
	else
		SetEvent(canWriteEvent);
}
DWORD writer(PVOID param)
{
	int writer_num = *((int*)param);
	while (fl)
	{
		Sleep(rand() % 5000);
		start_write();
		value++;
		printf("Thread %d writer #%d wrote: %d\n", thread_id[writer_num - 1], writer_num, value);
		stop_write();
	}
	return 0;
}
void start_read()
{
	InterlockedIncrement(&waitingReadersCount);
	if (activeWriter || waitingWritersCount > 0)
		WaitForSingleObject(canReadEvent, INFINITE);
	WaitForSingleObject(mutex, INFINITE);
	InterlockedDecrement(&waitingReadersCount);
	InterlockedIncrement(&activeReadersCount);
	SetEvent(canReadEvent);
	ReleaseMutex(mutex);  
}
void stop_read()
{
	InterlockedDecrement(&activeReadersCount);
	if (activeReadersCount == 0) 
		SetEvent(canWriteEvent);
}
DWORD reader(PVOID param)
{
	int reader_num = *((int*)param);
	while (fl)
	{
		Sleep(rand() % 5000);
		start_read();
		printf("Thread %d reader #%d got: %d\n", thread_id[reader_num - 1], reader_num, value);
		stop_read();
	}
	return 0;
}
int main()
{
	printf("PID: %d\n", getpid());
	HANDLE thread_handler[WRITERS_COUNT + READERS_COUNT];
	int process_num[WRITERS_COUNT + READERS_COUNT];
    if (signal(SIGINT, handler) == -1)
	{
		perror("Can't sig_num");
		exit(1);
	}
	canReadEvent = CreateEvent(NULL, false, false, NULL);
	if (canReadEvent == NULL)
	{
		perror("Can't create canRead");
		exit(1);
	}
	canWriteEvent = CreateEvent(NULL, true, false, NULL);
	if (canWriteEvent == NULL)
	{
		perror("Can't create canWrite");
		exit(1);
	}
	mutex = CreateMutex(NULL, 0, NULL);
	if (mutex == NULL)
	{
		perror("Can't create mutex");
		exit(1);
	}
	for (int i = 0; i < WRITERS_COUNT; i++)
	{
		process_num[i] = i + 1;
		thread_handler[i] = CreateThread(NULL, 0, writer, &process_num[i], 0, &thread_id[i]);
		if (thread_handler[i] == NULL)
		{
			perror("Can't create writer thread");
			exit(1);
		}
	}
	for (int i = 0; i < READERS_COUNT; i++)
	{
		process_num[i + WRITERS_COUNT] = i + 1;
		thread_handler[i + WRITERS_COUNT] = CreateThread(NULL, 0, reader, &process_num[i + WRITERS_COUNT], 0, &thread_id[i + WRITERS_COUNT]);
		if (thread_handler[i + WRITERS_COUNT] == NULL)
		{
			perror("Can't create reader thread");
			exit(1);
		}
	}
	for (int i = 0; i < WRITERS_COUNT + READERS_COUNT; i++)
	{
		DWORD thread_rc = WaitForSingleObject(thread_handler[i], INFINITE);
		if(thread_rc == WAIT_OBJECT_0 || thread_rc == WAIT_TIMEOUT)
			printf("thread %d finished\n", thread_id[i]);
		else if(thread_rc == WAIT_FAILED)
			printf("waitThread %d failed %d\n", thread_id[i], thread_rc);
	}
	CloseHandle(canReadEvent);
	CloseHandle(canWriteEvent);
	CloseHandle(mutex);
	return 0;
}
