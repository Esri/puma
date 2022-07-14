/**
 * Puma - CityEngine Plugin for Rhinoceros
 *
 * See https://esri.github.io/cityengine/puma for documentation.
 *
 * Copyright (c) 2021 Esri R&D Center Zurich
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * https://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef _MSC_VER
#	pragma warning(push)
#	pragma warning(disable : 26451)
#	pragma warning(disable : 26495)
#endif
#include "stdafx.h"
#ifdef _MSC_VER
#	pragma warning(pop)
#endif

#include "Logger.h"
#include "utils.h"

#include "prt/StringUtils.h"

#include <Windows.h>
#include <conio.h>
#include <rpc.h>

#include <cwchar>
#include <filesystem>
#include <sstream>
#include <string>

namespace {

void checkLastError(const std::string& exceptionPrefix) {
	const DWORD lastError = GetLastError();
	if (lastError != ERROR_SUCCESS) {
		char msg[255];
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, 0, lastError, 0, msg, 255, 0);
		throw std::runtime_error(exceptionPrefix + msg);
	}
}

} // namespace

namespace pcu {

ShapeAttributes::ShapeAttributes(pcu::RuleFileInfoPtr ruleFileInfo, const std::wstring rulef, const std::wstring startRl,
                                 const std::wstring shapeN, int seed)
    : ruleFileInfo(std::move(ruleFileInfo)), ruleFile(rulef), startRule(startRl), shapeName(shapeN), seed(seed) {}

// location of RhinoPRT shared library
std::filesystem::path getDllLocation() {
	HMODULE hModule = 0;
	if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
	                       (LPCWSTR)getDllLocation, &hModule))
		checkLastError("Failed to get plugin library handle: ");

	char libraryPathStr[_MAX_PATH];
	if (!GetModuleFileNameA(hModule, libraryPathStr, _MAX_PATH))
		checkLastError("Failed to get plugin file system location: ");

	const std::filesystem::path libraryPath(libraryPathStr);
	return libraryPath.parent_path();
}

template <>
char getDirSeparator() {
	static const char SEPARATOR = '\\';
	return SEPARATOR;
}

template <>
wchar_t getDirSeparator() {
	static const wchar_t SEPARATOR = L'\\';
	return SEPARATOR;
}

template <>
std::string getDirSeparator() {
	return std::string(1, getDirSeparator<char>());
}

template <>
std::wstring getDirSeparator() {
	return std::wstring(1, getDirSeparator<wchar_t>());
}

std::filesystem::path getTempDir(const std::wstring& tmp_prefix) {
	const auto tempPath = std::filesystem::temp_directory_path();

	auto prtTempPath = tempPath / tmp_prefix;
	if (!std::filesystem::exists(prtTempPath)) {
		if (!std::filesystem::create_directory(prtTempPath)) {
			LOG_ERR << L"Could not create PRT temporary directory at " << prtTempPath.wstring()
			        << L". Returning the default temp dir at " << tempPath.wstring();
			return tempPath;
		}
	}

	return prtTempPath;
}

std::filesystem::path getUniqueTempDir(const std::filesystem::path& tempDir, const std::wstring& basename) {
	const std::wstring uuid = getUUID();
	const auto uniqueTempDir = tempDir / (basename + uuid);

	if (!std::filesystem::create_directory(uniqueTempDir)) {
		LOG_ERR << L"Could not create unique temporary directory at " << uniqueTempDir.wstring()
		        << L", returning default temp dir.";
		return tempDir;
	}

	return uniqueTempDir;
}

std::wstring getUUID() {
	RPC_STATUS status;

	UUID uid;
	status = UuidCreate(&uid);
	if (status != RPC_S_OK)
		throw std::runtime_error("Failed to create UUID");

	wchar_t* str = nullptr;
	status = UuidToStringW(&uid, (RPC_WSTR*)&str);
	if (status == RPC_S_OK && str != nullptr && std::wcslen(str) > 0) {
		return std::wstring(str);
	}

	LOG_ERR << "Failed to create UUID";
	throw std::runtime_error("Failed to create UUID");
}

const std::string FILE_SCHEMA = "file:/";

/**
 * Helper function to add the ShapeAttributes infos to an existing prt::AttributeMap.
 */
AttributeMapPtr createAttributeMapForShape(const ShapeAttributes& attrs, prt::AttributeMapBuilder& bld) {
	bld.setString(L"ruleFile", attrs.ruleFile.c_str());
	bld.setString(L"startRule", attrs.startRule.c_str());
	bld.setString(L"shapeName", attrs.shapeName.c_str());
	return AttributeMapPtr{bld.createAttributeMap()};
}

AttributeMapPtr createValidatedOptions(const wchar_t* encID, const prt::AttributeMap* unvalidatedOptions) {
	const EncoderInfoPtr encInfo{prt::createEncoderInfo(encID)};

	if (!encInfo) {
		LOG_ERR << "Failed to create encoder info: encoder not found for given ID.";
		return {};
	}

	const prt::AttributeMap* validatedOptions = nullptr;
	encInfo->createValidatedOptionsAndStates(unvalidatedOptions, &validatedOptions);
	return AttributeMapPtr(validatedOptions);
}

template <typename inC, typename outC, typename FUNC>
std::basic_string<outC> callAPI(FUNC f, const std::basic_string<inC>& s) {
	std::vector<outC> buffer(s.size());
	size_t size = buffer.size();
	f(s.c_str(), buffer.data(), &size, nullptr);
	if (size > buffer.size()) {
		buffer.resize(size);
		f(s.c_str(), buffer.data(), &size, nullptr);
	}
	return std::basic_string<outC>{buffer.data()};
}

std::wstring filename(const std::wstring& path) {
	size_t pos = path.find_last_of(L'/');
	if (pos != std::string::npos) {
		return path.substr(pos + 1);
	}
	else
		return path;
}

std::string toOSNarrowFromUTF16(const std::wstring& osWString) {
	return callAPI<wchar_t, char>(prt::StringUtils::toOSNarrowFromUTF16, osWString);
}

std::wstring toUTF16FromOSNarrow(const std::string& osString) {
	return callAPI<char, wchar_t>(prt::StringUtils::toUTF16FromOSNarrow, osString);
}

std::wstring toUTF16FromUTF8(const std::string& utf8String) {
	return callAPI<char, wchar_t>(prt::StringUtils::toUTF16FromUTF8, utf8String);
}

std::string toUTF8FromUTF16(const std::wstring& utf16String) {
	return callAPI<wchar_t, char>(prt::StringUtils::toUTF8FromUTF16, utf16String);
}

std::string toUTF8FromOSNarrow(const std::string& osString) {
	std::wstring utf16String = toUTF16FromOSNarrow(osString);
	return toUTF8FromUTF16(utf16String);
}

void appendToRhinoString(ON_wString& rhinoString, const std::wstring& appendee) {
	rhinoString += appendee.c_str();
}

void appendColor(const ON_Color& color, ON_SimpleArray<double>* pArray){
	pArray->Append(color.Red());
	pArray->Append(color.Green());
	pArray->Append(color.Blue());
}

std::string percentEncode(const std::string& utf8String) {
	return callAPI<char, char>(prt::StringUtils::percentEncode, utf8String);
}

std::string toFileURI(const std::string& osNarrowPath) {
	const std::string utf8Path = toUTF8FromOSNarrow(osNarrowPath);
	const std::string u8PE = percentEncode(utf8Path);
	return FILE_SCHEMA + u8PE;
}

std::string toFileURI(const std::wstring& utf16Path) {
	const std::string utf8Path = toUTF8FromUTF16(utf16Path);
	const std::string u8PE = percentEncode(utf8Path);
	return FILE_SCHEMA + u8PE;
}

bool isDefaultStyle(const std::wstring& attrName) {
	return attrName.find(DEFAULT_STYLE_PREFIX) != std::wstring::npos;
}

std::wstring removePrefix(const std::wstring& attrName, wchar_t delim) {
	const auto sepPos = attrName.find(delim);

	if (sepPos == std::wstring::npos)
		return attrName;
	if (sepPos == attrName.length() - 1)
		return {};
	if (attrName.length() <= 1)
		return {};
	return attrName.substr(sepPos + 1);
}

std::wstring removeStyle(const std::wstring& attrName) {
	return removePrefix(attrName, STYLE_DELIMITER);
}

std::wstring removeImport(const std::wstring& attrName) {
	return removePrefix(attrName, IMPORT_DELIMITER);
}

std::wstring getImportPrefix(const std::wstring& attrName) {
	const auto sepPos = attrName.find(IMPORT_DELIMITER);

	if (sepPos == std::wstring::npos)
		return {};
	return attrName.substr(0, sepPos);
}

std::wstring getRuleFileEntry(const ResolveMapSPtr& resolveMap) {
	const std::wstring sCGB(L".cgb");

	size_t nKeys;
	wchar_t const* const* keys = resolveMap->getKeys(&nKeys);
	for (size_t k = 0; k < nKeys; ++k) {
		const std::wstring key(keys[k]);
		if (std::equal(sCGB.rbegin(), sCGB.rend(), key.rbegin()))
			return key;
	}

	return {};
}

std::wstring detectStartRule(const RuleFileInfoPtr& ruleFileInfo) {
	for (size_t r = 0; r < ruleFileInfo->getNumRules(); r++) {
		const auto* rule = ruleFileInfo->getRule(r);

		// start rules must not have any parameters
		if (rule->getNumParameters() > 0)
			continue;

		for (size_t a = 0; a < rule->getNumAnnotations(); a++) {
			if (std::wcscmp(rule->getAnnotation(a)->getName(), ANNOT_START_RULE) == 0) {
				return rule->getName();
			}
		}
	}
	return {};
}

std::wstring toAssetKey(std::wstring key) {
	// add the "asset" prefix to the key
	std::wstring assetKey(L"assets/");
	assetKey = assetKey.append(key);
	return assetKey;
}

void PathRemover::operator()(std::filesystem::path const* p) {
	if ((p != nullptr) && std::filesystem::exists(*p)) {
		std::error_code errorCode;
		std::filesystem::remove_all(*p, errorCode);
		if (errorCode)
			LOG_ERR << "Unable to remove path at " << *p << ", " << errorCode.message();
		else
			LOG_DBG << "Removed path at " << *p;
		delete p;
	}
}

std::vector<const wchar_t*> split(const std::wstring& i_str, const std::wstring& i_delim) {
	std::vector<const wchar_t*> result;

	size_t found = i_str.find(i_delim);
	size_t startIndex = 0;

	while (found != std::wstring::npos) {
		result.push_back(std::wstring(i_str.begin() + startIndex, i_str.begin() + found).c_str());
		startIndex = found + i_delim.size();
		found = i_str.find(i_delim, startIndex);
	}
	if (startIndex != i_str.size())
		result.push_back(std::wstring(i_str.begin() + startIndex, i_str.end()).c_str());
	return result;
}

std::vector<const wchar_t*> fromCeArray(const std::wstring& stringArray) {
	return pcu::split(stringArray, CE_ARRAY_DELIMITER);
}

const std::wstring toCeArray(const wchar_t* const* values, size_t count) {
	std::wstring serializedArray;
	for (size_t i = 0; i < count; ++i) {
		serializedArray += std::wstring(values[i]) + CE_ARRAY_DELIMITER;
	}
	return serializedArray;
}

const std::wstring toCeArray(const bool* values, size_t count) {
	std::wstring serializedArray;
	for (size_t i = 0; i < count; ++i) {
		serializedArray += std::wstring(values[i] ? L"true" : L"false") + CE_ARRAY_DELIMITER;
	}
	return serializedArray;
}

const std::wstring toCeArray(const double* values, size_t count) {
	std::wstring serializedArray;
	for (size_t i = 0; i < count; ++i) {
		serializedArray +=  std::to_wstring(values[i]) + CE_ARRAY_DELIMITER;
	}
	return serializedArray;
}

const std::wstring toCeArray(const int* values, size_t count) {
	std::wstring serializedArray;
	for (size_t i = 0; i < count; ++i) {
		serializedArray +=  std::to_wstring(values[i]) + CE_ARRAY_DELIMITER;
	}
	return serializedArray;
}

/**
 * Interop helpers
 */

void unpackDoubleAttributes(int start, int count, ON_ClassArray<ON_wString>* keys, ON_SimpleArray<double>* values,
                      AttributeMapBuilderPtr& aBuilder) {
	for (int i = start; i < start + count; ++i) {
		const std::wstring key(keys->At(i)->Array());

		const double value(*values->At(i));
		pcu::fillMapBuilder(key, value, aBuilder);
	}
}

void unpackBoolAttributes(int start, int count, ON_ClassArray<ON_wString>* keys, ON_SimpleArray<int>* values,
	AttributeMapBuilderPtr& aBuilder) {
	for (int i = start; i < start + count; ++i) {
		const std::wstring key(keys->At(i)->Array());

		const int value(*values->At(i));
		pcu::fillMapBuilder(key, static_cast<bool>(value), aBuilder);
	}
}

void unpackDoubleArrayAttributes(int start, int count, ON_ClassArray<ON_wString>* keys, ON_ClassArray<ON_wString>* values,
                           AttributeMapBuilderPtr& aBuilder) {
	for (int i = start; i < start + count; ++i) {
		const std::wstring key(keys->At(i)->Array());
		const auto vArray = pcu::fromCeArray(values->At(i)->Array());
		pcu::fillArrayMapBuilder<double>(key, vArray, aBuilder);
	}
}


void unpackBoolArrayAttributes(int start, int count, ON_ClassArray<ON_wString>* keys, ON_ClassArray<ON_wString>* values,
                              AttributeMapBuilderPtr& aBuilder) {
	for (int i = start; i < start + count; ++i) {
		const std::wstring key(keys->At(i)->Array());
		const auto vArray = pcu::fromCeArray(values->At(i)->Array());
		pcu::fillArrayMapBuilder<bool>(key, vArray, aBuilder);
	}
}

void unpackStringAttributes(int start, int count, ON_ClassArray<ON_wString>* keys, ON_ClassArray<ON_wString>* values,
                            AttributeMapBuilderPtr& aBuilder, bool isArray) {
	for (int i = start; i < start + count; ++i) {
		const std::wstring key(keys->At(i)->Array());

		if (isArray) {
			auto strings = pcu::fromCeArray(values->At(i)->Array());
			aBuilder->setStringArray(key.c_str(), strings.data(), strings.size());
		}
		else
			aBuilder->setString(key.c_str(), values->At(i)->Array());
	}
}

void logAttributeTypeError(const std::wstring& key) {
	LOG_ERR << "Impossible to get default value for rule attribute: " << key
	        << " The expected type does not correspond to the actual type of this attribute.";
}

void logAttributeError(const std::wstring& key, prt::Status& status) {
	LOG_ERR << "Impossible to get default value for rule attribute: " << key
	        << " with error: " << prt::getStatusDescription(status);
}
} // namespace pcu
