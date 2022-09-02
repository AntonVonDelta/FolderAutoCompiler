#include <Windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>
#include <filesystem>
#include <functional>
#include <regex>

#define STOP MessageBoxA(NULL,"Stop","Stop",MB_OK)

using namespace std;

enum class CONFIG_SECTION :int {
	NONE,
	PRE,
	REPO_NAME,
	LIST,
	POST
};

typedef bool (*GENERIC_FILE_CALLBACK)(const filesystem::directory_entry&);

bool processConfig(string);
bool buildConfigData(ifstream& fi, map<CONFIG_SECTION, vector<string>>& config_data);
bool deleteFilesInFolder(string path, GENERIC_FILE_CALLBACK callback);

vector<string> getQuotesStrings(string input, bool exclude_quptes = true);
void executeCommand(ostringstream& stream);
string addTraillingSlash(string path);
void printTitle(string title);
void printDescription(string title);
bool isEmpty(string text);
string toLower(string line);


const string config_name = "repoconfig.txt";
const map<string, CONFIG_SECTION> section_mapping = {
	{"pre",CONFIG_SECTION::PRE},
	{"repo_name",CONFIG_SECTION::REPO_NAME},
	{"list",CONFIG_SECTION::LIST},
	{"post",CONFIG_SECTION::POST}
};
const string default_repo_name = "Repo";

//	Config file format:
// repo_name:
// <repo folder name>
// list:
// <folder to copy> <new folder name>
// .... list of above
// post:
// <post commands>


int main(int argc, char* argv[]) {
	string path = addTraillingSlash(filesystem::path(argv[0]).parent_path().string());
	if (!processConfig(path)) {
		system("pause");
		return 1;
	}

	system("timeout 10");
	return 0;
}

bool processConfig(string current_path_endslash) {
	string config_path = current_path_endslash + config_name;
	ifstream fi(config_path);
	map<CONFIG_SECTION, vector<string>> config_data;

	if (!fi) {
		cout << "Could not find config file \"" << config_path << "\"" << endl;
		return false;
	}

	if (!buildConfigData(fi, config_data)) {
		cout << "Could not parse config file \"" << config_path << "\"" << endl;
		return false;
	}


	// Add default values to config
	if (config_data[CONFIG_SECTION::REPO_NAME].size() == 0) {
		config_data[CONFIG_SECTION::REPO_NAME].push_back(default_repo_name);
	}

	ostringstream sys_command;
	string repo_folder = config_data[CONFIG_SECTION::REPO_NAME].back();
	string repo_folder_path = current_path_endslash + repo_folder;

	// Show detected arguments
	printDescription(string("REPO NAME: ") + repo_folder);
	printDescription("");

	// Run pre process commands
	printTitle("RUNNING PRE-PROCESS TASKS");
	for (auto& el : config_data[CONFIG_SECTION::PRE]) {
		system(el.c_str());
	}

	// Create repo folder
	printTitle("CREATING REPO FOLDER");
	sys_command << "mkdir \"" << repo_folder << "\"";
	executeCommand(sys_command);

	// Check and show if Repo folder contains .git folder. 
	string git_path = addTraillingSlash(repo_folder + string("\\")) + ".git";
	bool git_found = filesystem::exists(git_path);
	ostringstream temp_title;

	temp_title << "DOES .git FOLDER EXIST IN " << repo_folder << " FOLDER? " << (git_found ? "YES" : "NO");
	printTitle(temp_title.str());

	// Check if source folders contains .git
	printTitle("CHECK .git FOLDER DOES NOT EXIST IN SOURCE FOLDERS");
	for (auto& el : config_data[CONFIG_SECTION::LIST]) {
		string src_folder_path;
		string dest_folder_name;
		vector<string> quoted_strings = getQuotesStrings(el);

		if (quoted_strings.size() != 2) {
			cout << "Incorrect number of quoted strings in line: " << el << endl;
			return false;
		}

		src_folder_path = quoted_strings[0];
		dest_folder_name = quoted_strings[1];
		string git_path = addTraillingSlash(src_folder_path) + ".git";

		if (filesystem::exists(git_path)) {
			cout << "Error: found .git folder in " << repo_folder + string("\\") + src_folder_path << endl;
			return false;
		}
	}



	// Delete previous folders
	printTitle("DELETE PREVIOUSLY ADDED FOLDERS");
	for (auto& el : config_data[CONFIG_SECTION::LIST]) {
		string src_folder_path;
		string dest_folder_name;
		vector<string> quoted_strings = getQuotesStrings(el);

		if (quoted_strings.size() != 2) {
			cout << "Incorrect number of quoted strings in line: " << el << endl;
			return false;
		}

		src_folder_path = quoted_strings[0];
		dest_folder_name = quoted_strings[1];

		// Special case for .git folder when it is stored in "."
		if (dest_folder_name[0] == '.') {
			bool result = deleteFilesInFolder(repo_folder,
				[](const filesystem::directory_entry& entry) {
					string folder_name;
					if (!entry.is_directory()) return true;
					folder_name = entry.path().filename().generic_string();

					return folder_name != ".git";
				});

			if (!result) {
				cout << "Could not delete a files inside \"" << repo_folder << "\"" << endl;
				return false;
			}
		} else {
			sys_command << "rmdir /S /Q \"" << repo_folder << "\\" << dest_folder_name << "\"";
			executeCommand(sys_command);
		}
	}

	// Copy folders again
	printTitle("COPY SPECIFIED FOLDERS");
	for (auto& el : config_data[CONFIG_SECTION::LIST]) {
		ostringstream src_folder_path;
		ostringstream dest_folder_name;
		vector<string> quoted_strings = getQuotesStrings(el);

		if (quoted_strings.size() != 2) {
			cout << "Incorrect number of quoted strings in line: " << el << endl;
			return false;
		}

		src_folder_path << quoted_strings[0];
		dest_folder_name << addTraillingSlash(repo_folder_path) << quoted_strings[1];

		try {
			filesystem::copy(src_folder_path.str(), dest_folder_name.str(), filesystem::copy_options::recursive);
		} catch (exception ex) {
			cout << "Could not copy file " << src_folder_path.str() << " to " << dest_folder_name.str() << endl;
			return false;
		}

		/*sys_command << "xcopy /S /Y \"" << src_folder_path.str() << "\" \"" << repo_folder << "\\" << dest_folder_name.str() << "\\" << "\"";
		executeCommand(sys_command);*/
	}

	// Run post process commands
	printTitle("RUNNING POST-PROCESS TASKS");
	for (auto& el : config_data[CONFIG_SECTION::POST]) {
		sys_command << "cd \"" << current_path_endslash << "\" && " << el;
		executeCommand(sys_command);
	}

	return true;
}

bool buildConfigData(ifstream& fi, map<CONFIG_SECTION, vector<string>>& config_data) {
	CONFIG_SECTION current_config_section = CONFIG_SECTION::NONE;
	string line;

	while (getline(fi, line)) {
		string lower_case_line = toLower(line);
		regex header_regex("\\s*(\\w+)\\s*:\\s*");
		smatch match;

		if (regex_match(lower_case_line, match, header_regex)) {
			// Found section header
			string header_name = match[1];

			auto found_section = section_mapping.find(header_name);
			if (found_section== section_mapping.end()) {
				return false;
			}

			current_config_section = (*found_section).second;
			continue;
		}

		if (isEmpty(line)) continue;

		config_data[current_config_section].push_back(line);
	}

	return true;
}

// Callback called for all folders inside path.
// If returns true then file deleted.
bool deleteFilesInFolder(string path, GENERIC_FILE_CALLBACK callback) {
	try {
		for (const filesystem::directory_entry& entry : filesystem::directory_iterator(path)) {
			if (callback(entry)) {
				filesystem::remove_all(entry.path());
			}
		}
	} catch (...) {
		return false;
	}

	return true;
}


vector<string> getQuotesStrings(string input, bool exclude_quotes) {
	vector<string> result;
	size_t start_index = 0;
	size_t first_quote;
	size_t second_quote;

	while (true) {
		first_quote = input.find('"', start_index);
		if (first_quote == string::npos) return result;

		second_quote = input.find('"', first_quote + 1);
		if (second_quote == string::npos) return result;

		if (exclude_quotes) {
			// Cut the string excluding quotes
			result.push_back(input.substr(first_quote + 1, second_quote - first_quote - 1));
		} else {
			// Cut the whole string with quotes
			result.push_back(input.substr(first_quote, second_quote - first_quote + 1));
		}

		start_index = second_quote + 1;
	}

	return result;
}

void executeCommand(ostringstream& stream) {
	string str_command = stream.str();
	system(str_command.c_str());
	stream.str("");
	stream.clear();
}


string addTraillingSlash(string path) {
	if (!path.empty() && !(path.back() == '/' || path.back() == '\\')) {
		return path + '\\';
	}
	return path;
}

void printTitle(string title) {
	cout << "====== " << title << " ======" << endl;
}

void printDescription(string title) {
	cout << "       " << title << endl;
}

bool isEmpty(string text) {
	if (text.empty()) return true;

	string::iterator it(text.begin()), end(text.end());
	for (; it != end && *it == ' '; ++it);
	return it == end;
}

string toLower(string line) {
	string result = line;

	transform(line.begin(), line.end(), result.begin(), [](unsigned char c) { return tolower(c); });
	return result;
}