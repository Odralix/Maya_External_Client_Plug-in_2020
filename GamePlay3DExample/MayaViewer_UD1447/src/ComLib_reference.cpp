//Author: Ossian Edström

#include "ComLib_reference.h"

ComLib::ComLib(const std::string & secret, const size_t & buffSize, ClientType type)
{

	this->myType = type;

	mySecret = secret;

	myBuffSize = buffSize;

	offset = sizeof(int)*2; //~~ Change to *2 later since we want to start after head and tail.

	firstRun = true;

	msgNr = 0;

	// 2) API call to create FileMap (use pagefile as a backup)
	hFileMap = CreateFileMapping(
		INVALID_HANDLE_VALUE, // Use Paging File
		NULL,				  // Default security
		PAGE_READWRITE,		  // Read/write access
		(DWORD)0,			  // maximum object size (High DWORD)
		buffSize,			  // maximum object size (Low DWORD) Add 64 to allow for writing several different datatypes at end of buffer if neccesary.
		(LPCWSTR) mySecret.c_str());	  // name of the object

	//From the Docs:
	//The names of event, semaphore, mutex, waitable timer, job, and file mapping objects share the same namespace.
	//Therefore, the CreateFileMapping and OpenFileMapping functions fail if they specify a name that is IN USE by an object of another type.

	// 3) check if hFileMap is NULL -> FATAL ERROR
	if (hFileMap == NULL)
	{
		/*std::cout << "FATAL ERROR! 'hFileMap' == NULL!" << std::endl;
		system("Pause");*/
		exit(0);
	}

	//	  4) check 
	//    if (GetLastError() == ERROR_ALREADY_EXISTS)
	//    This means that the file map already existed!, but we
	//    still get a handle to it, we share it!
	//    THIS COULD BE USEFUL INFORMATION FOR OUR PROTOCOL.

	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		/*std::cout << "FILEMAP ALREADY EXISTS!! But We STILL generate another handle. Use for protocol??" << std::endl;*/
	}

	// 5) Create one VIEW of the map with a void pointer
	//    This API Call will create a view to ALL the memory
	//    of the File map.
	mData = MapViewOfFile(hFileMap, // Handle
		FILE_MAP_ALL_ACCESS,		// Read/Write permission
		0, 
		0, 
		0);//Buffer size??

	// IMPORTANT: https://docs.microsoft.com/sv-se/windows/win32/memory/creating-named-shared-memory
}

ComLib::~ComLib()
{
	UnmapViewOfFile((LPCVOID)mData);
	CloseHandle(hFileMap);
}

bool ComLib::send(const void * msg, const size_t length)
{
	// Return variable.
	bool success = true;
	// We're going to use temp to reach the start of the buffer after mData changes.
	// Should probably be a constant variable set in the constructor instead of re-itereated.
	void* temp = mData;

	// memcpy_s requires a void* so we make an int* to read tail.
	int a = -1;
	int * tailCheck = &a;

	// Move producer to current location.
	mData = static_cast<char*>(mData) + offset;

	// Read consumers location
	// Tail is in "int 2" of the memory, "head in int 1".
	memcpy_s(tailCheck, sizeof(int), ((char*)temp + sizeof(int)), sizeof(int));

	// Since this means the consumer hasn't started yet.
	// Different solution than consumer, similar idea.
	if (*tailCheck == 0)
	{
		*tailCheck = 8;
	}

	// Ensures that if the head is behind the tail we never go past or catch up.
	if (offset < *tailCheck)
	{
		// Current position + length of the message + the size of the message header which is a size_t.
		if ((offset + length + sizeof(length)) >= *tailCheck)
		{ 
			do
			{
				memcpy_s(tailCheck, sizeof(int), ((char*)temp + sizeof(int)), sizeof(int));
				// If tail reaches the end of the buffer and resets, we are no longer behind the tail before reseting.
				// Therefore we can freely go ahead to the end of the buffer.
			} while (((offset + length + sizeof(length)) >= *tailCheck) && (offset < *tailCheck));
		}
	}

	// Since we write length as a header for each message, it needs to be counted with to see if we're past the buffersize.
	// Remember, one extra size_t as we want to be able to write one at the end to tell the consumer to start over.
	if (offset + length + sizeof(length) + sizeof(size_t) >= myBuffSize)
	{
		// Space left in buffer is too small for the  message.

		// Check if the tail is in first position. Wait if it is.
		if (*tailCheck == 0 || *tailCheck == 8)
		{
			do
			{
				memcpy_s(tailCheck, sizeof(int), ((char*)temp + sizeof(int)), sizeof(int));
			} while (*tailCheck == 0 || *tailCheck == 8);
		}

		size_t size = -2;
		// Set the length to -2 as a message to the consumer that it should reset.
		memcpy_s(mData, sizeof(size_t), &size, sizeof(size_t));

		// Head and tail lay in the front of the buffer. Messages should start after.
		offset = sizeof(int)*2;
		mData = temp;

		//Actually send the message that couldn't be fit into the buffer:
		send(msg, length);

		// We should still return false when there's not enough space.
		// This however makes a correctly sent if reset message return false which is not optimal.
		return false;
	}
	else
	{
		// Write the length header for the consumer.
		memcpy_s(mData, sizeof(length), &length, sizeof(length));

		// Step over the size_t so we don't overwrite it with the message.
		mData = static_cast<char*>(mData) + sizeof(length);

	}

	// Send the actual message.
	memcpy_s(mData, length, msg, length);

	// Reset mData to the start of the buffer so that it can read tail location in next call.
	mData = temp;

	// Update offset.
	offset += length + sizeof(length);

	if (firstRun)
	{
		firstRun = false;
	}

	//Write head location once message has been sent.
	memcpy_s(temp, sizeof(int), &offset, sizeof(int));

	return success;
}

bool ComLib::recv(char * msg, size_t & length)
{
	// return variable.
	bool success = true;

	// Reference point to first location in buffer.
	void* temp = mData;

	// memcpy_s requires void*. Make int* to recieve info.
	int a = -1;
	int * headCheck = &a;

	//Read current location of head in first 4 bytes of buffer.
	memcpy_s(headCheck, sizeof(int), temp, sizeof(int));

	// Move consumer to current location.
	mData = static_cast<char*>(mData) + offset;

	// Write tailLocation 4 bytes into memory buffer. Since head is written to the first 4.
	memcpy_s((char*)temp + sizeof(int), sizeof(int), &offset, sizeof(int));

	// If the consumer is run first it needs to wait for the producer when it hasn't been initialized.
	// When uninitialised *headcheck is equal to 0.
	if (firstRun)
	{
		do
		{
			memcpy_s(headCheck, sizeof(int), temp, sizeof(int));
		} while (*headCheck == 0 || *headCheck == 8 );
	}

	// Head is NEVER allowed to catch up to the consumer.
	// But the consumer has to catch up to head in order to read the last message after the producer stops.
	//As such it has to wait if they are in the same location as the producer may still be writing.
	if (offset == *headCheck)
	{
		do
		{
			memcpy_s(headCheck, sizeof(int), temp, sizeof(int));
		} while (offset == *headCheck);
	}

	// Read the length of the incoming message.
	size_t len = -1;
	memcpy_s(&len, sizeof(size_t), mData, sizeof(size_t));

	// Ensures we never step past the Head while behind it.
	if (offset < *headCheck)
	{
		// Current position + length of the message + the size of the message header which is a size_t.
		if ((offset + len + sizeof(size_t)) > *headCheck)
		{
			do
			{
				memcpy_s(headCheck, sizeof(int), temp, sizeof(int));
			} while (((offset + len + sizeof(size_t)) > *headCheck) && offset < *headCheck);
		}
	}

	// Check if producer left a dummy message.
	if (len == -2)
	{
		// Length of the message is -2! Producer ran out of space. Start over at first location.

		// Ensure we never step past Head if it is in starting position:
		if (*headCheck == 8)
		{
			do
			{
				memcpy_s(headCheck, sizeof(int), temp, sizeof(int));
			} while (*headCheck == 8);
		}

		// Since two ints for head and tail are always in front of the messages.
		offset = sizeof(int) * 2;
		mData = temp;

		// Actually read the message this dispatch of the function was intended to read.
		recv(msg, length);

		// We should still return false when there's not enough space.
		// This however makes a correctly sent if reset message return false which is not optimal.
		return false;
	}
	else
	{
		// Step over the size_t for length in the header for the message.
		mData = static_cast<char*>(mData) + sizeof(size_t);
	}

	// Read the actual message.
	memcpy_s(msg, len, mData, len);
	length = len;

	// Reset Mdata to first position in buffer.
	mData = temp;

	// Update the offset.
	offset += len + sizeof(len);

	if (firstRun)
	{
		firstRun = false;
	}

	return success;
}

size_t ComLib::nextSize()
{
	// This is never used.


	//size_t size;
	//bool msg = false;

	//if(msg)
	//{
	//	//size = msgSize;
	//}
	//else
	//{
	//	size = 0;
	//}
	//return size;

	return -1;
}
