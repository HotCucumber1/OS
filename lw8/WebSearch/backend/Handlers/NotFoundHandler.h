#pragma once
#include "BaseHandler.h"

class NotFoundHandler : public BaseHandler
{
public:
	template <class Body, class Allocator>
	static http::message_generator Handle(
		const http::request<Body, http::basic_fields<Allocator>>& request)
	{
		return GetJsonResponse(
			http::status::not_found,
			request.version(),
			{
				{ "error", "Not Found" },
				{ "path", std::string(request.target()) },
			});
	}
};
