/* This file is Copyright 2000 Level Control Systems.  See the included LICENSE.txt file for details. */

#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <string.h>
#include <time.h>

#include "util/NetworkUtilityFunctions.h"
#include "dataio/TCPSocketDataIO.h"
#include "iogateway/MessageIOGateway.h"
#include "reflector/StorageReflectConstants.h"
#include "system/SetupSystem.h"
#include "util/StringTokenizer.h"
#include "util/SocketMultiplexer.h"

using namespace muscle;

#define NET_CLIENT_NEW_CHAT_TEXT 2  // from ShareNetClient.h

// A BeShare-compatible MOTD client for MUSCLE servers.
// It will connect to the muscled running on the local
// machine, and watch for new BeShare users.  Whenever
// one arrives, it will send them the motd.txt file.
AbstractMessageIOGatewayRef gwRef;

int Setup(ConstSocketRef & Socket)
{
	Socket = Connect(IPAddressAndPort("localhost", 2960, true), NULL, "motdbot", false);
	if (Socket() == NULL) return 10;
	gwRef = AbstractMessageIOGatewayRef(new MessageIOGateway);
	gwRef()->SetDataIO(DataIORef(new TCPSocketDataIO(Socket, false)));

	{
		// Subscribe to watch all arriving BeShare users
		MessageRef watchBeShareUsers = GetMessageFromPool(PR_COMMAND_SETPARAMETERS);
		watchBeShareUsers()->AddBool("SUBSCRIBE:beshare", true);
		watchBeShareUsers()->AddBool(PR_NAME_SUBSCRIBE_QUIETLY, true);  // so as not to spam when we are starting up
		gwRef()->AddOutgoingMessage(watchBeShareUsers);
	}
	return 0;
}

int main(int argc, char ** argv)
{
	CompleteSetupSystem css;
	ConstSocketRef Socket;
	int result = Setup(Socket);
	if (result) return result;


	MessageRef val(GetMessageFromPool());
	if (val())
	{
		String node = "beshare/name";
		val()->AddString("name", "MOTD");
		val()->AddString("version_name", "MOTD");
		val()->AddString("version_num", "1.06b");

		MessageRef ref(GetMessageFromPool(PR_COMMAND_SETDATA));
		if (ref())
		{
			ref()->AddMessage(node, val);
			gwRef()->AddOutgoingMessage(ref);
		}
	}

	val = GetMessageFromPool();
	if (val())
	{
		String node = "beshare/userstatus";
		val()->AddString("userstatus", "Serving...");

		MessageRef ref(GetMessageFromPool(PR_COMMAND_SETDATA));
		if (ref())
		{
			ref()->AddMessage(node, val);
			gwRef()->AddOutgoingMessage(ref);
		}
	}

	QueueGatewayMessageReceiver inputQueue;
	SocketMultiplexer multiplexer;
	while(1)
	{
		(void) multiplexer.RegisterSocketForReadReady(Socket.GetFileDescriptor());
		if (gwRef()->HasBytesToOutput())
			(void) multiplexer.RegisterSocketForWriteReady(Socket.GetFileDescriptor());

		if (multiplexer.WaitForEvents() < 0)
			return -1;

		bool reading = multiplexer.IsSocketReadyForRead(Socket.GetFileDescriptor());
		bool writing = multiplexer.IsSocketReadyForWrite(Socket.GetFileDescriptor());
		bool writeError = ((writing)&&(gwRef()->DoOutput() < 0));
		bool readError = ((reading) && (gwRef()->DoInput(inputQueue) < 0));
		if ((readError)||(writeError))
		{
			LogTime(MUSCLE_LOG_ERROR, "Connection to server closed!\n");
			Socket.Reset();
			while (Socket() == NULL)
			{
				if (Setup(Socket) == 10)
				{
					LogTime(MUSCLE_LOG_ERROR, "Oops, reconnect failed!  I'll wait 5 seconds, then try again.\n");
					Snooze64(5000000);
				}
			}
		}

		MessageRef incoming;
		while(inputQueue.RemoveHead(incoming) == B_NO_ERROR)
		{
			MessageFieldNameIterator it = incoming.GetItemPointer()->GetFieldNameIterator(B_MESSAGE_TYPE);

			while(it.HasData())
			{
				String s = it.GetFieldName();
				StringTokenizer tok(s.Cstr(), "/");
				(void) tok.GetNextToken();
				const char * sessionID = tok.GetNextToken();
				if (sessionID)
				{
					// send him the MOTD!
					const char * filename = (argc > 1) ? argv[1] : "motd.txt";
					FILE * f;
#if __STDC_WANT_SECURE_LIB__
					errno_t err = fopen_s(&f, filename, "r");
					if (err == 0)
#else
					f = fopen(filename, "r");
					if (f)
#endif
					{
						String motd("\n");
						char temp[1024];
						while(fgets(temp, sizeof(temp), f)) motd += temp;

						MessageRef msg = GetMessageFromPool(NET_CLIENT_NEW_CHAT_TEXT);
						if (msg())
						{
							LogTime(MUSCLE_LOG_INFO, "Sending MOTD message to [%s]\n", sessionID);
							String target("/*/");
							target += sessionID;
							msg()->AddString(PR_NAME_KEYS, target);
							msg()->AddString("text", motd);
							msg()->AddString("session", "Message Of The Day");
							// uncomment this to make the MOTD appear in blue
							//                            msg()->AddBool("private", true);
							gwRef()->AddOutgoingMessage(msg);
						}
						fclose(f);
					}
					else LogTime(MUSCLE_LOG_ERROR, "Error, couldn't open [%s]!!!\n", filename);
				}
				it++;
			}
		}
		Snooze64(1000000); // avoid busy-looping
	}

	return 0;
}
