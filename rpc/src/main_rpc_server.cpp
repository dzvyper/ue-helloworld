/*
 * Copyright (c) 2024 General Motors GTO LLC
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * SPDX-FileType: SOURCE
 * SPDX-FileCopyrightText: 2024 General Motors GTO LLC
 * SPDX-License-Identifier: Apache-2.0
 */
#include <spdlog/spdlog.h>
#include <unistd.h>
#include <up-cpp/communication/RpcServer.h>

#include <chrono>
#include <csignal>
#include <iostream>

//#include "SocketUTransport.h"
#include "up-transport-zenoh-cpp/ZenohUTransport.h"
#include "common.h"

using namespace uprotocol::v1;
using namespace uprotocol::communication;
using namespace uprotocol::datamodel::builder;
using namespace uprotocol;

constexpr std::string_view ZENOH_CONFIG_FILE = BUILD_REALPATH_ZENOH_CONF;

bool gTerminate = false;

void signalHandler(int signal) {
	if (signal == SIGINT) {
		std::cout << "Ctrl+C received. Exiting..." << std::endl;
		gTerminate = true;
	}
}

std::optional<uprotocol::datamodel::builder::Payload> OnReceive(
    const UMessage& message) {
	// Validate message is an RPC request
	if (message.attributes().type() != UMessageType::UMESSAGE_TYPE_REQUEST) {
		spdlog::error("Received message is not a request\n{}",
		              message.DebugString());
		return {};
	}

#if 0
	// Validate message has no payload (no payload expected)
	if (message.has_payload()) {
		spdlog::error("Received message has non-empty payload\n{}",
		              message.DebugString());
		return {};
	}
#endif

	// Received request with empty payload, generate response with
	// sequence number, current time, and random value
	static uint64_t seqNum = 0;
	uint64_t randVal = std::rand();
	uint64_t timeVal = std::chrono::duration_cast<std::chrono::milliseconds>(
	                       std::chrono::system_clock::now().time_since_epoch())
	                       .count();
	std::vector<uint64_t> payload_data = {seqNum++, timeVal, randVal};

	spdlog::debug("(Server) Received request:\n{}", message.DebugString());

	Payload payload(reinterpret_cast<std::vector<uint8_t>&>(payload_data),
	                UPayloadFormat::UPAYLOAD_FORMAT_RAW);


	spdlog::info("Sending payload:  {} - {}, {}", payload_data[0],
	             payload_data[1], payload_data[2]);

	return payload;
}

std::shared_ptr<transport::UTransport> getTransport(
    const v1::UUri& uuri = getRpcUUri(0)) 
{
	return std::make_shared<transport::ZenohUTransport>(uuri,
	                                                    ZENOH_CONFIG_FILE);
}


/* The sample RPC server applications demonstrates how to receive RPC requests
 * and send a response back to the client -
 * The response in this example will be the current time */
int main(int argc, char** argv) 
{
	(void)argc;
	(void)argv;

	signal(SIGINT, signalHandler);

	UUri source = getRpcUUri(0);
	UUri method = getRpcUUri(12);
	auto transport = getTransport(source);
	auto server = RpcServer::create(transport, 
									method, 
									OnReceive, 
									UPayloadFormat::UPAYLOAD_FORMAT_RAW);

	if (!server.has_value()) {
		spdlog::error("Failed to create RPC server: {}",
		              server.error().DebugString());
		return 1;
	}

	while (!gTerminate) {
		sleep(1);
	}

	return 0;

}

