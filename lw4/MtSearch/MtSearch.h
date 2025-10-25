#pragma once
#include <atomic>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

class MtSearch
{
private:
	struct DocInfo
	{
		uint64_t docId;
		double termFrequency;
		int totalWordCount;
	};

	struct WordData
	{
		double idf;
		std::vector<DocInfo> docs;
	};

	using InvertIndex = std::unordered_map<std::string, std::vector<DocInfo>>;
	using FileInfo = std::vector<std::pair<uint64_t, double>>;

public:
	explicit MtSearch(std::istream& input, std::ostream& output, int threads);
	void Run();

private:
	void ProcessLine(const std::string& line);
	void AddFileToIndex(const std::string& filePath);
	void AddDirToIndex(const std::string& dirPath, bool recursively);
	void PrintFilesRelevantInfo(const FileInfo& filesRelevantInfo);
	FileInfo FindMostRelevantDocIds(const std::vector<std::string>& words);
	void ProcessFindBatch(const std::string& fileUrl);
	void RemoveFileFromIndex(const std::string& fileUrl);
	void RemoveDirFromIndex(const std::string& dirPath, bool recursively);
	uint64_t GetFileIdByUrl(const std::string& fileUrl);
	std::vector<WordData> GetWordsDataFromIndex(const std::vector<std::string>& words);

	void MtProcessDirectory(
		const std::string& dirPath,
		std::function<void(const std::string&)> callback,
		bool recursively) const;

	void PrintAllFiles();
	void PrintIndexInfo();

	std::unordered_map<uint64_t, std::string> m_files;

	std::atomic<uint64_t> m_docIndex{ 0 };
	InvertIndex m_invertIndex;

	std::istream& m_input;
	std::ostream& m_output;

	std::shared_mutex m_indexMutex;
	int m_threads;
};
