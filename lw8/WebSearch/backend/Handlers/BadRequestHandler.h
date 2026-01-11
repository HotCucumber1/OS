#pragma once
#include "BaseHandler.h"

class BadRequestHandler : public BaseHandler
{
public:
	template <class Body, class Allocator>
	static http::message_generator Handle(
		const std::string& message,
		const http::request<Body,http::basic_fields<Allocator>>& request)
	{
		return GetJsonResponse(
			http::status::bad_request,
			request.version(),
			{
				{ "error", "Bad request" },
				{ "path", std::string(request.target()) },
				{ "message", message },
			});
	}
};
