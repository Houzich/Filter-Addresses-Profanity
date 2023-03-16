
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <iostream>
#include <locale> // std::locale, std::tolower
#include <thread>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

struct ConfigClass
{
public:
	//Путь к обрабатываемому файлу с адресами + имя файла path/name.
	//Если оставить пустым "", то путь будет запрашиваться в начале программы
	std::string path_file_in = "../../accounts.tsv";
	//Путь + имя файла куда будут сохранены результаты фильтрации.
	//Если оставить пустым "", то путь будет запрашиваться в начале программы
	std::string path_file_out = "ADDRESSES.csv";
	//Количество идущих подряд "двойных" чисел. Если указать 0, то фильтрация по "двойным" числам производиться не будет
	int score_double = 4;
	//Строки которые хотим найти в адресе.
	std::vector<std::string> pattern = {
		"000000",
		"111111",
		"222222",
		"333333",
		"444444",
		"555555",
		"666666",
		"777777",
		"888888",
		"999999",
		"aaaaaa",
		"bbbbbb",
		"cccccc",
		"dddddd",
		"012345",
		"123456",
		"234567",
		"345678",
		"456789",
		"567890",
		"67890a",
		"7890ab",
		"890abc",
		"90abcd",
		"0abcdf",
		"fdcba0",
		"dcba09",
		"cba098",
		"ba0987",
		"a09876",
		"098765",
		"987654",
		"876543",
		"765432",
		"654321",
		"543210"};
};

int parse_gonfig(ConfigClass *config, std::string path)
{
	try
	{
		std::ifstream f(path);
		json data = json::parse(f);
		config->path_file_in = data["path_file_in"];
		config->path_file_out = data["path_file_out"];
		config->score_double = data["score_double"];
		config->pattern = data["pattern"];
	}
	catch (std::runtime_error &e)
	{
		std::cerr << "Error parse config file " << path << " : " << e.what() << '\n';
		throw;
	}
	catch (...)
	{
		std::cerr << "Error parse config file, unknown exception occured" << std::endl;
		throw;
	}

	return 0;
}

bool isPattern(ConfigClass *config, std::string str)
{
	for (int x = 0; x < config->pattern.size(); x++)
	{
		std::size_t found = str.find(config->pattern[x]);
		if (found != std::string::npos)
		{
			return true;
		}
	}
	return false;
}

bool isDouble(ConfigClass *config, std::string str)
{
	if (config->score_double == 0)
		return false;
	int score = 0;
	for (int i = 0; i < str.size() / 2; ++i)
	{
		std::string substr = str.substr((size_t)i * 2, 2);
		if ((substr == "00") || (substr == "11") || (substr == "22") || (substr == "33") || (substr == "44") || (substr == "55") || (substr == "66") || (substr == "77") || (substr == "88") || (substr == "99") ||
			(substr == "aa") || (substr == "bb") || (substr == "cc") || (substr == "dd") || (substr == "ee") || (substr == "ff"))
		{
			++score;
			if (score == config->score_double)
				return true;
		}
		else
		{
			score = 0;
		}
	}
	return false;
}

bool check_address(ConfigClass *config, std::string addr)
{
	std::locale loc;
	for (auto &c : addr)
	{
		c = tolower(c);
	}
	if (isPattern(config, addr))
		return true;
	if (isDouble(config, addr))
		return true;

	return false;
}

int FilterAddresses(ConfigClass *config)
{
	std::ifstream input_file(config->path_file_in, std::ifstream::in);
	std::ofstream out_file(config->path_file_out, std::ios::app);
	if (!input_file.is_open())
	{
		std::cerr << "\n!!!ERROR OPEN FILE " << config->path_file_in << "!!!\n"
				  << std::endl;
		return -1;
	}
	if (!out_file.is_open())
	{
		std::cerr << "\n!!!ERROR OPEN FILE " << config->path_file_out << "!!!\n"
				  << std::endl;
		return -1;
	}
	std::string line;
	size_t count = 0;
	size_t count_addr_save = 0;
	std::vector<std::string> lines;
	while (1)
	{
		bool end = false;
		if (getline(input_file, line))
		{
			if ((line[0] == '0') && (line[1] == 'x'))
				line = line.substr(2, 40);
			else
				line = line.substr(0, 40);
			lines.push_back(line);
			count++;
		}
		else
		{
			end = true;
		}
		if (((count % 1000000) == 0) || ((end) && (lines.size() != 0)))
		{
			std::vector<std::string> lines_to_save;
#pragma omp parallel for
			for (int x = 0; x < lines.size(); x++)
			{
				if (check_address(config, lines[x]))
				{
#pragma omp critical
					lines_to_save.push_back(lines[x]);
				}
			}
			for (int x = 0; x < lines_to_save.size(); x++)
			{
				out_file << lines_to_save[x] << std::endl;
			}
			count_addr_save += lines_to_save.size();
			std::wcout << L"\rTOTAL PROCESSED " << count << L" LINES, SAVE " << count_addr_save << L" ADDRESSES" << std::flush;
			lines.clear();
			lines_to_save.clear();
		}
		if (end)
			break;
	}
	std::cout << "\nSAVE " << count_addr_save << " ADDRESSES IN FILE " << config->path_file_out << std::endl;
	input_file.close();
	out_file.close();

	return 0;
}
#include <chrono>
#include <io.h>
#include <fcntl.h>

int main(int, char **)
{
	std::wcout << "..........\n";
	int err;
	ConfigClass *config = new ConfigClass();
	try
	{
		parse_gonfig(config, "config.json");
	}
	catch (...)
	{
		return -1;
	}

	if (config->path_file_in == "")
	{
		std::wcout << L"Enter the path + name of the file to be processed: ";
		std::getline(std::cin, config->path_file_in);
	}

	if (config->path_file_out == "")
	{
		std::wcout << L"Enter the path + name where the results will be saved: ";
		std::getline(std::cin, config->path_file_out);
	}
	std::wcout << L"\nSTART!\n";
	int ret = FilterAddresses(config);
	if (ret != 0)
	{
		std::wcout << L"\n\nERROR TO SAVE FILE!!!" << std::endl;
		return ret;
	}
	std::wcout << L"FINISH!\n";
	while (1)
	{
		std::chrono::milliseconds duration(2000);
		std::this_thread::sleep_for(duration);
	}
}
