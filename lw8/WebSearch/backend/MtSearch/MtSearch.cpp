#include "MtSearch.h"

#include <algorithm>
#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>

constexpr double NANO_IN_SECOND = 1000000000;

std::vector<std::pair<uint64_t, double>> GetTopMapItems(const std::unordered_map<uint64_t, double>& inputMap, int top);

bool IsLatinChar(const char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

std::vector<std::string> SplitBySpaces(const std::string& str)
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

MtSearch::MtSearch(std::istream& input, std::ostream& output, const int threads)
	: m_input(input)
	, m_output(output)
	, m_threads(threads)
{
}

void MtSearch::Run()
{
	std::string line;
	while (!m_input.eof())
	{
		std::getline(m_input, line);
		if (line == "exit")
		{
			break;
		}
		ProcessLine(line);
	}
}

void MtSearch::ProcessLine(const std::string& line)
{
	const std::string addFileCommand = "add_file";
	const std::string addDirCommand = "add_dir";
	const std::string addDirRecCommand = "add_dir_recursive";
	const std::string findCommand = "find";
	const std::string findBatchCommand = "find_batch";
	const std::string removeFileCommand = "remove_file";
	const std::string removeDirCommand = "remove_dir";
	const std::string removeDirRecCommand = "remove_dir_recursive";

	if (line.empty())
	{
		return;
	}

	const size_t pos = line.find(' ');
	const auto command = line.substr(0, pos);
	const auto arg = line.substr(pos + 1);

	if (command == addFileCommand)
	{
		AddFileToIndex(arg);
	}
	else if (command == addDirCommand)
	{
		AddDirToIndex(arg, false);
	}
	else if (command == addDirRecCommand)
	{
		AddDirToIndex(arg, true);
	}
	else if (command == findCommand)
	{
		const auto wordsInfo = FindMostRelevantDocIds(SplitBySpaces(arg));
		PrintFilesRelevantInfo(wordsInfo);
	}
	else if (command == findBatchCommand)
	{
		ProcessFindBatch(arg);
	}
	else if (command == removeFileCommand)
	{
		RemoveFileFromIndex(arg);
	}
	else if (command == removeDirCommand)
	{
		RemoveDirFromIndex(arg, false);
	}
	else if (command == removeDirRecCommand)
	{
		RemoveDirFromIndex(arg, true);
	}
	else if (command == "all")
	{
		PrintAllFiles();
	}
	else if (command == "printIndex")
	{
		PrintIndexInfo();
	}
	else
	{
		throw std::invalid_argument("Invalid command");
	}
}

void InsertWord(std::unordered_map<std::string, int>& words, const std::string& word)
{
	std::string result = word;

	std::transform(result.begin(), result.end(), result.begin(), [](const unsigned char c) {
		return std::tolower(c);
	});

	auto it = words.find(result);
	if (it != words.end())
	{
		it->second++;
	}
	else
	{
		words[result] = 1;
	}
}

uint ReadWordsFromFile(std::ifstream& input, std::unordered_map<std::string, int>& wordsCountMap)
{
	std::string line;
	uint totalWordCount = 0;
	while (std::getline(input, line))
	{
		std::string currentWord;

		for (const auto c : line)
		{
			if (IsLatinChar(c))
			{
				currentWord += c;
				continue;
			}

			if (currentWord.empty())
			{
				continue;
			}
			InsertWord(wordsCountMap, currentWord);
			++totalWordCount;
			currentWord.clear();
		}
		if (!currentWord.empty())
		{
			InsertWord(wordsCountMap, currentWord);
			++totalWordCount;
		}
	}
	return totalWordCount;
}

void MtSearch::AddFileToIndex(const std::string& filePath)
{
	std::ifstream file(filePath);
	if (!file.is_open())
	{
		std::cerr << "Cannot open file: " << filePath << std::endl;
		return;
	}

	auto docId = m_docIndex.fetch_add(1, std::memory_order_relaxed); // TODO почему memory_order_relaxed
	{
		// TODO какое отношение порядка, и как операции связаны с
		std::unique_lock lock(m_indexMutex);
		m_files[docId] = filePath;
	}
	std::unordered_map<std::string, int> wordsCount;
	auto totalWordCount = ReadWordsFromFile(file, wordsCount);

	InvertIndex localUpdates;
	for (const auto& [word, count] : wordsCount)
	{
		localUpdates[word].emplace_back(
			docId,
			static_cast<double>(count) / totalWordCount,
			totalWordCount);
	}

	std::unique_lock lock(m_indexMutex);
	for (auto& [word, docs] : localUpdates)
	{
		auto& targetList = m_invertIndex[word]; // TODO нужно сделать эффективное удаление
		targetList.insert(
			targetList.end(),
			std::make_move_iterator(docs.begin()),
			std::make_move_iterator(docs.end())); // TODO сравнить через бенчмарк этот подход с более гранулярным блоком
	}
}

void MtSearch::AddDirToIndex(const std::string& dirPath, const bool recursively)
{
	MtProcessDirectory(
		dirPath,
		[this](const std::string& filePath) {
			AddFileToIndex(filePath);
		},
		recursively);
}

std::vector<MtSearch::WordData> MtSearch::GetWordsDataFromIndex(const std::vector<std::string>& words)
{
	std::vector<WordData> wordDataList;

	std::shared_lock readLock(m_indexMutex);
	const auto totalDocsCount = m_files.size();

	for (const auto& word : words)
	{
		std::string result = word;

		std::transform(result.begin(), result.end(), result.begin(), [](const unsigned char c) {
			return std::tolower(c);
		});

		if (auto it = m_invertIndex.find(result); it != m_invertIndex.end())
		{
			const auto docsWithTermCount = it->second.size();
			wordDataList.push_back({ std::log(static_cast<double>(totalDocsCount) / docsWithTermCount),
				it->second });
		}
	}
	return wordDataList;
}

std::vector<std::pair<uint64_t, double>> MtSearch::FindMostRelevantDocIds(const std::vector<std::string>& words)
{
	const auto wordDataList = GetWordsDataFromIndex(words);

	std::unordered_map<uint64_t, double> fileRelevant;
	std::mutex relevantMutex;
	boost::asio::thread_pool threadPool(m_threads);

	// TODO тредпул, возможно, лучше прикрутить снаружи
	for (const auto& wordData : wordDataList)
	{
		for (const auto& doc : wordData.docs)
		{
			boost::asio::post(threadPool, [&fileRelevant, &relevantMutex, wordData, doc]() { // TODO либо в один поток, либо разбить на более крупные крупные
				const double wordRelevant = doc.termFrequency * wordData.idf;

				std::unique_lock lock(relevantMutex);
				fileRelevant[doc.docId] += wordRelevant;
			});
		}
	}
	threadPool.join();

	return GetTopMapItems(fileRelevant, 10);
}

std::vector<std::pair<uint64_t, double>> GetTopMapItems(
	const std::unordered_map<uint64_t, double>& inputMap,
	const int top)
{
	std::vector<std::pair<uint64_t, double>> pairs(inputMap.begin(), inputMap.end());
	const auto n = std::min<size_t>(top, pairs.size());

	std::partial_sort(
		pairs.begin(),
		pairs.begin() + n,
		pairs.end(),
		[](const auto& a, const auto& b) {
			return a.second > b.second;
		});

	if (pairs.size() > n)
	{
		pairs.resize(n);
	}
	return pairs;
}

void MtSearch::PrintFilesRelevantInfo(const std::vector<std::pair<uint64_t, double>>& filesRelevantInfo)
{
	int n = 1;
	for (const auto& [docId, relevant] : filesRelevantInfo)
	{
		std::shared_lock lock(m_indexMutex);
		auto fileUrl = m_files[docId];
		m_output << n << ". " << "id: " << docId << ", relevance: " << relevant << ", path: " << fileUrl << std::endl;
		n++;
	}
}

std::vector<FileInfoOutput> MtSearch::ListMostRelevantDocIds(
	const std::vector<std::string>& words,
	int from,
	int to)
{
	std::vector<FileInfoOutput> result;
	const auto docsInfo = FindMostRelevantDocIds(words);

	for (const auto& [docId, relevant] : docsInfo)
	{
		std::shared_lock lock(m_indexMutex);
		const auto fileUrl = m_files[docId];
		result.push_back({
			docId,
			relevant,
			fileUrl,
		});
	}

	std::ranges::reverse(result);
	return result;
}

void MtSearch::PrintAllFiles()
{
	int n = 1;
	std::shared_lock lock(m_indexMutex);
	for (const auto& [docId, file] : m_files)
	{
		m_output << n << ") id: " << docId << ", path: " << file << std::endl;
		++n;
	}
}

void MtSearch::PrintIndexInfo()
{
	int n = 1;
	std::shared_lock lock(m_indexMutex);
	for (const auto& [word, fileInfo] : m_invertIndex)
	{
		m_output << n << ") word: " << word << " [";

		for (const auto& file : fileInfo)
		{
			m_output << file.docId << ", ";
		}
		m_output << "]" << std::endl;
		n++;
	}
}

void MtSearch::ProcessFindBatch(const std::string& fileUrl)
{
	std::ifstream file(fileUrl);
	if (!file.is_open())
	{
		std::cerr << "Cannot open file: " << fileUrl << std::endl;
		return;
	}

	int queryNum = 0;
	std::string line;
	boost::asio::thread_pool threadPool(m_threads);

	while (std::getline(file, line))
	{
		auto words = SplitBySpaces(line);
		queryNum++;

		boost::asio::post(threadPool, [this, words, queryNum, line]() {
			const auto startTime = std::chrono::high_resolution_clock::now();
			const auto filesInfo = FindMostRelevantDocIds(words);
			const auto endTime = std::chrono::high_resolution_clock::now();
			const auto time = (endTime - startTime).count();

			std::cout << queryNum << ". query: " << line << std::endl;
			std::cout << "Search took: " << time / NANO_IN_SECOND << "sec" << std::endl;
			PrintFilesRelevantInfo(filesInfo);
		});
	}
	threadPool.join();
}

uint64_t MtSearch::GetFileIdByUrl(const std::string& fileUrl)
{
	std::shared_lock lock(m_indexMutex);
	for (const auto& [docId, file] : m_files)
	{
		if (file == fileUrl)
		{
			return docId;
		}
	}
	throw std::invalid_argument("File does not exist");
}

void MtSearch::RemoveFileFromIndex(const std::string& fileUrl)
{
	std::unique_lock lock(m_indexMutex);

	uint64_t docId = 0;
	for (const auto& [id, file] : m_files)
	{
		if (file == fileUrl)
		{
			docId = id;
			break;
		}
	}
	m_files.erase(docId);

	for (auto it = m_invertIndex.begin(); it != m_invertIndex.end();)
	{
		auto& files = it->second;

		std::erase_if(files, [docId](const DocInfo& docInfo) {
			return docInfo.docId == docId;
		});

		if (files.empty())
		{
			it = m_invertIndex.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void MtSearch::RemoveDirFromIndex(const std::string& dirPath, const bool recursively)
{
	MtProcessDirectory(
		dirPath,
		[this](const std::string& filePath) {
			RemoveFileFromIndex(filePath);
		},
		recursively);
}

void MtSearch::MtProcessDirectory(
	const std::string& dirPath,
	std::function<void(const std::string&)> callback,
	const bool recursively) const
{
	boost::asio::thread_pool threadPool(m_threads);

	auto processEntry = [&](const auto& entry) {
		if (!entry.is_regular_file())
		{
			return;
		}
		boost::asio::post(threadPool, [path = entry.path(), callback]() {
			callback(path);
		});
	};

	if (recursively)
	{
		for (const auto& entry : std::filesystem::recursive_directory_iterator(dirPath))
		{
			processEntry(entry);
		}
	}
	else
	{
		for (const auto& entry : std::filesystem::directory_iterator(dirPath))
		{
			processEntry(entry);
		}
	}
	threadPool.join();
}
