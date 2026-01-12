#pragma once
#include "FileInfoOutput.h"
#include "Handlers/NotFoundHandler.h"
#include "Handlers/ProcessOptionsHandler.h"
#include "JsonConverter.h"
#include "MtSearch/MtSearch.h"

#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <boost/url/parse.hpp>
#include <iostream>

namespace http = boost::beast::http;
namespace json = boost::json;

class RequestHandler : public BaseHandler
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
			return ProcessOptionsHandler::Handle(request);
		}

		if (request.target().contains("/list") && request.method() == http::verb::get)
		{
			return ProcessListRequest(request);
		}

		// if (request.target().contains("/add") && request.method() == http::verb::post)
		// {
		// 	return ProcessAddRequest(request);
		// }

		return NotFoundHandler::Handle(request);
	}

private:
	template <class Body, class Allocator>
	http::message_generator ProcessListRequest(const http::request<Body, http::basic_fields<Allocator>>& request)
	{
		auto url = boost::urls::parse_origin_form(request.target());

		auto it = url->params().find("words");
		const std::string wordsStr = (it != url->params().end())
			? (*it).value
			: "";

		auto fromIt = url->params().find("from");
		int from = 0;

		auto toIt = url->params().find("to");
		int to = 10;

		if (fromIt != url->params().end() && toIt != url->params().end())
		{
			try
			{
				from = std::stoi((*fromIt).value);
				to = std::stoi((*toIt).value);
			}
			catch (const std::exception&)
			{
				return GetJsonResponse(
					http::status::bad_request,
					request.version(),
					{ { "error", "Parameters 'from' and 'to' must be a number" } });
			}
		}

		const auto words = SplitBySpaces(wordsStr);
		auto filesData = m_search->ListMostRelevantDocIds(words, from, to);

		return ListFileLinksResponse(filesData, request);
	}

	// template <class Body, class Allocator>
	// http::message_generator ProcessAddRequest(const http::request<Body, http::basic_fields<Allocator>>& request)
	// {
	// 	auto data = json::parse(request.body());
	// }

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
