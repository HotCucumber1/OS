#pragma once
#include <boost/beast/http/message_generator.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/json/serialize.hpp>
#include <boost/json/value.hpp>

namespace http = boost::beast::http;

class BaseHandler
{
protected:
	static http::message_generator GetJsonResponse(
		const http::status& status,
		const unsigned version,
		const boost::json::value& jsonValue)
	{
		http::response<http::string_body> res{ status, version };
		res.set(http::field::content_type, "application/json");
		res.set(http::field::access_control_allow_origin, "*");
		res.set(http::field::access_control_allow_headers, "Content-Type, Authorization");
		res.set(http::field::access_control_allow_methods, "GET, POST, OPTIONS");
		res.body() = serialize(jsonValue);
		res.prepare_payload();
		return res;
	}
};
