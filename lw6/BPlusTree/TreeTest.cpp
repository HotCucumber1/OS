#include "BPlusTree.h"
#include "catch2/catch_all.hpp"

#include <filesystem>

class BPlusTreeFixture
{
protected:
	std::string m_testFilePath;
	std::unique_ptr<BPlusTree> m_tree;
	std::stringstream m_output;

	BPlusTreeFixture()
	{
		m_testFilePath = (std::filesystem::temp_directory_path() / "bplustree_test_XXXXXX").string();

		try
		{
			m_tree = std::make_unique<BPlusTree>(m_testFilePath, m_output);
		}
		catch (const std::exception& e)
		{
			FAIL("BPlusTree initialization failed: " << e.what());
		}
	}

	~BPlusTreeFixture()
	{
		m_tree.reset();

		if (std::filesystem::exists(m_testFilePath))
		{
			if (unlink(m_testFilePath.c_str()) != 0)
			{
				std::cerr << "Warning: Failed to delete temporary file: " << m_testFilePath << std::endl;
			}
		}
	}

	std::string getOutput()
	{
		std::string line;
		if (std::getline(m_output, line))
		{
			return line;
		}
		return "";
	}
};

TEST_CASE_METHOD(BPlusTreeFixture, "Initialization and Basic PUT/GET/UPDATE", "[basic]")
{
	SECTION("Initial State and Basic STATS")
	{
		m_output.str("");
		m_tree->Stats();
		REQUIRE(m_output.str().find("Root Page ID: 0") != std::string::npos);
		REQUIRE(m_output.str().find("Total Keys: 0") != std::string::npos);
		REQUIRE(m_output.str().find("Height: 0") != std::string::npos);
	}

	SECTION("First PUT creates root and GET works")
	{
		m_tree->Put(10, "Hello World");
		REQUIRE(getOutput() == "OK");

		m_tree->Get(10);
		REQUIRE(getOutput() == "Hello World");

		m_tree->Get(99);
		REQUIRE(getOutput() == "NOT FOUND");

		m_output.str("");
		m_output.clear();
		m_tree->Stats();
		REQUIRE(m_output.str().find("Root Page ID: 1") != std::string::npos);
		REQUIRE(m_output.str().find("Total Keys: 1") != std::string::npos);
		REQUIRE(m_output.str().find("Height: 1") != std::string::npos);
	}

	SECTION("Update existing key")
	{
		m_tree->Put(50, "OldValue");
		REQUIRE(getOutput() == "OK");

		m_tree->Put(50, "NewUpdatedValue");
		REQUIRE(getOutput() == "OK (Updated)");

		m_tree->Get(50);
		REQUIRE(getOutput() == "NewUpdatedValue");
	}

	SECTION("Value too long check")
	{
		std::string longValue(MAX_VALUE_LEN + 1, 'X');
		REQUIRE_THROWS(m_tree->Put(1, longValue));
	}
}

TEST_CASE_METHOD(BPlusTreeFixture, "02. Leaf Split and New Root Creation", "[split]")
{
	SECTION("Triggering Leaf Split")
	{
		for (KEY k = 1; k <= M_LEAF; ++k)
		{
			m_tree->Put(k, "Data_" + std::to_string(k));
			REQUIRE(getOutput() == "OK");
		}

		m_output.str("");
		m_output.clear();
		m_tree->Stats();
		REQUIRE(m_output.str().find("Total Keys: 31") != std::string::npos);
		REQUIRE(m_output.str().find("Height: 1") != std::string::npos);

		m_tree->Put(M_LEAF + 1, "Data_Split");

		m_output.str("");
		m_output.clear();
		m_tree->Stats();
		REQUIRE(m_output.str().find("Total Keys: 32") != std::string::npos);
		REQUIRE(m_output.str().find("Height: 2") != std::string::npos);
		REQUIRE(m_output.str().find("Total Nodes (used pages): 3") != std::string::npos);
	}
}

TEST_CASE_METHOD(BPlusTreeFixture, "Deletion and Leaf Merge", "[delete]")
{
	for (KEY k = 1; k <= M_LEAF + 1; ++k)
	{
		m_tree->Put(k, "V_" + std::to_string(k));
		if (k == M_LEAF + 1)
		{
			REQUIRE(getOutput() == "OK (Split occurred)");
		}
		else
		{
			REQUIRE(getOutput() == "OK");
		}
	}

	SECTION("Basic Deletion")
	{
		m_tree->Put(5, "Hihih");
		m_tree->Delete(5);

		m_tree->Get(5);

		m_output.str("");
		m_output.clear();
		m_tree->Stats();
		REQUIRE(m_output.str().find("Total Keys: 31") != std::string::npos);
	}

	SECTION("Triggering Leaf Merge")
	{
		for (KEY k = 1; k < M_LEAF_MIN; ++k)
		{
			m_tree->Delete(k);
		}

		m_tree->Delete(M_LEAF_MIN);

		m_output.str("");
		m_output.clear();
		m_tree->Stats();
		REQUIRE(m_output.str().find("Total Keys: 16") != std::string::npos);
	}

	SECTION("Delete last key in tree")
	{
		for (KEY k = 1; k <= M_LEAF + 1; ++k)
		{
			m_tree->Delete(k);
		}

		m_tree->Delete(M_LEAF + 1);

		m_output.str("");
		m_output.clear();
		m_tree->Stats();
		REQUIRE(m_output.str().find("Total Keys: 0") != std::string::npos);
		REQUIRE(m_output.str().find("Height: 0") != std::string::npos);
	}
}

TEST_CASE_METHOD(BPlusTreeFixture, "Persistence Check", "[persistence]")
{
	m_tree->Put(100, "PersistentValueA");
	m_tree->Put(200, "PersistentValueB");

	m_tree->Get(100);

	m_output.str("");
	m_output.clear();
	m_tree->Stats();
	REQUIRE(m_output.str().find("Total Keys: 2") != std::string::npos);

	std::string path = m_testFilePath;
	m_tree.reset();

	std::stringstream newOutput;
	std::unique_ptr<BPlusTree> newTree;

	try
	{
		newTree = std::make_unique<BPlusTree>(path, newOutput);
	}
	catch (const std::exception& e)
	{
		FAIL("Reopening BPlusTree failed: " << e.what());
	}

	newTree->Get(100);
	REQUIRE(newOutput.str().find("PersistentValueA") != std::string::npos);
	newOutput.str("");
	newOutput.clear();

	newTree->Get(200);
	REQUIRE(newOutput.str().find("PersistentValueB") != std::string::npos);
	newOutput.str("");
	newOutput.clear();

	newTree->Stats();
	REQUIRE(newOutput.str().find("Total Keys: 2") != std::string::npos);
}