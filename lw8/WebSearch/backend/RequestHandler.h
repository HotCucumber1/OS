#pragma once
#include "FileInfoOutput.h"
#include "JsonConverter.h"
#include "MtSearch/MtSearch.h"

#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <boost/url/parse.hpp>
#include <iostream>

namespace http = boost::beast::http;
namespace json = boost::json;

class RequestHandler
{
public:
	RequestHandler()
	{
		m_search->AddDirToIndex("/home/dmitriy.rybakov/projects/crm/crm-app", true);
	}

	template <class Body, class Allocator>
	http::message_generator Handle(http::request<Body, http::basic_fields<Allocator>>&& request)
	{
		auto url = boost::urls::parse_origin_form(request.target());

		if (request.method() == http::verb::options)
		{
			http::response<http::empty_body> res{http::status::ok, request.version()};
			res.set(http::field::access_control_allow_origin, "*");
			res.set(http::field::access_control_allow_methods, "GET, POST, OPTIONS");
			res.set(http::field::access_control_allow_headers, "Content-Type, Authorization");
			res.prepare_payload();
			return res;
		}

		if (request.target().contains("/list"))
		{
			auto it = url->params().find("words");
			const std::string wordsStr = (it != url->params().end())
				? (*it).value
				: "";

			const auto words = SplitBySpaces(wordsStr);
			auto filesData = m_search->ListMostRelevantDocIds(words);

			return ListFileLinksResponse(filesData, request);
		}

		return NotFound(request);
	}

private:
	template <class Body, class Allocator>
	http::message_generator ListFileLinksResponse(
		const std::vector<FileInfoOutput>& filesInfo,
		const http::request<Body, http::basic_fields<Allocator>>& request)
	{
		auto filesData = filesInfo;
		std::ranges::sort(filesData, std::ranges::greater{}, &FileInfoOutput::relevant);

		json::array fileLinks;
		for (const auto& fileInfo : filesData)
		{
			const auto value = JsonConverter::ConvertFileInfoToJson(fileInfo);
			fileLinks.push_back(value);
		}

		return GetJsonResponse(
			http::status::ok,
			request.version(),
			{ { "data", std::move(fileLinks) } });
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
		res.set(http::field::access_control_allow_origin, "*");
		res.set(http::field::access_control_allow_headers, "Content-Type, Authorization");
		res.set(http::field::access_control_allow_methods, "GET, POST, OPTIONS");
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
