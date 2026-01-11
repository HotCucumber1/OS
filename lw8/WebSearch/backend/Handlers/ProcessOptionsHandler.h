#pragma once
#include "BaseHandler.h"
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/message_generator.hpp>

class ProcessOptionsHandler : public BaseHandler
{
public:
	template <class Body, class Allocator>
	static http::message_generator Handle(
		const http::request<Body, http::basic_fields<Allocator>>& request)
	{
		http::response<http::empty_body> res{ http::status::ok, request.version() };
		res.set(http::field::access_control_allow_origin, "*");
		res.set(http::field::access_control_allow_methods, "GET, POST, OPTIONS");
		res.set(http::field::access_control_allow_headers, "Content-Type, Authorization");
		res.prepare_payload();
		return res;
	}
};
