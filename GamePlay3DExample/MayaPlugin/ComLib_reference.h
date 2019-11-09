//Author: Ossian Edström

#pragma once
#include <Windows.h>
#include <iostream>
#include <string>


class ComLib
{

public:
	enum ClientType { PRODUCER, CONSUMER };

	// Constructor
	// secret is the known name for the shared memory
	// buffSize is in MEGABYTES (multiple of 1<<20). No. Kilobytes now, requirements changed.
	// type is TYPE::PRODUCER or TYPE::CONSUMER
	ComLib(const std::string& secret, const size_t& buffSize, ClientType type);
	/* destroy all resources */
	~ComLib();

	// returns "true" if data was sent successfully.
	// false if for ANY reason the data could not be sent.
	// we will not implement an "error handling" mechanism, so we will assume
	// that false means that there was no space in the buffer to put the message.
	// msg is a void pointer to the data.
	// length is the amount of bytes of the message to send.
	bool send(const void* msg, const size_t length);

	// returns: "true" if a message was received.
	// false if there was nothing to read.
	// "msg" is expected to have enough space for the message.
	// use "nextSize()" to check whether our temporary buffer is big enough
	// to hold the next message.
	// @length returns the size of the message just read.
	// @msg contains the actual message read.
	bool recv(char* msg, size_t& length);

	// return the length of the next message
	// return 0 if no message is available.
	size_t nextSize();

private:
	//~~ In template from comLib.h
	ClientType myType;
	std::string  mySecret;
	size_t  myBuffSize;

	//~~ In template from shared.h
	void * mData;
	bool exists = false;
	unsigned int mSize = 1 << 10;

	//~~ Defined by me
	HANDLE hFileMap;

	// This should most likely be a size_t.
	// Currently I am too afraid of breaking something if I change it.
	int offset;

	bool firstRun;
	int msgNr;
};

