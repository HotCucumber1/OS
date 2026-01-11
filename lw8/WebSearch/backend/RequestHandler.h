#pragma once
#include "MtSearch/MtSearch.h"

#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <iostream>

namespace http = boost::beast::http;
namespace json = boost::json;

class RequestHandler
{
public:
	template <class Body, class Allocator>
	http::message_generator Handle(http::request<Body, http::basic_fields<Allocator>>&& request)
	{
		if (request.target() == "/my-route")
		{
			const auto words = SplitBySpaces("TODO"); // TODO
			auto data = GetFiles(words);
		}

		return NotFound(request);
	}

private:
	std::vector<std::string> GetFiles(const std::vector<std::string>& words)
	{
		return {};
	}

	template <class Body, class Allocator>
	static http::message_generator BadRequest(
		const std::string& message,
		const http::request<Body, http::basic_fields<Allocator>>& request)
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

	template <class Body, class Allocator>
	static http::message_generator NotFound(
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

	static http::message_generator GetJsonResponse(
		const http::status& status,
		const unsigned version,
		const json::value& jsonValue)
	{
		http::response<http::string_body> res{ status, version };
		res.set(http::field::content_type, "application/json");
		res.body() = json::serialize(jsonValue);
		res.prepare_payload();
		return res;
	}

	static std::vector<std::string> SplitBySpaces(const std::string& str)
	{
		std::istringstream iss(str);
		std::vector<std::string> words;
		std::string word;

		while (iss >> word)
		{
			words.push_back(word);
		}

		return words;
	}

private:
	static constexpr int THREADS = 16;

	std::unique_ptr<MtSearch> m_search = std::make_unique<MtSearch>(
		std::cin,
		std::cout,
		THREADS);
};
