/*
  Copyright (c) 2020 sogou, Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  Author: Wu Jiaxu (wujiaxu@sogou-inc.com)
*/

#include <signal.h>
#include <workflow/WFTaskFactory.h>
#include "echo_pb.srpc.h"

#ifndef _WIN32
#include <unistd.h>
#endif

using namespace srpc;

class ExampleServiceImpl : public Example::Service
{
public:
	void Echo(EchoRequest *request, EchoResponse *response, RPCContext *ctx) override
	{
		ctx->set_compress_type(RPCCompressGzip);

		auto *task = WFTaskFactory::create_http_task("https://www.sogou.com", 0, 0,
			[request, response](WFHttpTask *task) {
				if (task->get_state() == WFT_STATE_SUCCESS)
				{
					const void *data;
					size_t len;
					task->get_resp()->get_parsed_body(&data, &len);
					response->mutable_message()->assign((const char *)data, len);
				}
				else
					response->set_message("Error: " + std::to_string(task->get_error()));

				printf("Server Echo()\nget_req:\n%s\nset_resp:\n%s\n",
											request->DebugString().c_str(),
											response->DebugString().c_str());
			});

		ctx->get_series()->push_back(task);
	}
};

static void sig_handler(int signo) { }

int main(int argc, char *argv[])
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

	SRPCServer server;
	ExampleServiceImpl impl;

	server.add_service(&impl);
	if (server.start(1412) == 0)
	{
#ifndef _WIN32
		pause();
#else
		getchar();
#endif
		server.stop();
	}
	else
		perror("server start");

	google::protobuf::ShutdownProtobufLibrary();	
	return 0;
}

