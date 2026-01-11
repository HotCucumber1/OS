#pragma once
#include "FileInfoOutput.h"

#include <boost/json/array.hpp>

class JsonConverter
{
public:
	static boost::json::object ConvertFileInfoToJson(const FileInfoOutput& fileInfo)
	{
		return {
			{"fileId", fileInfo.id},
			{"fileRelevant", fileInfo.relevant},
			{"fileUrl", fileInfo.fileUrl},
		};
	}
};