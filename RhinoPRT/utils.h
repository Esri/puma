#pragma once

#include "RhinoCallbacks.h"

#include "prt/API.h"
#include "prt/FileOutputCallbacks.h"
#include "prt/LogHandler.h"

#include <memory>
#include <string>

namespace pcu {
	struct ShapeAttributes {
		std::wstring ruleFile;
		std::wstring startRule;
		std::wstring shapeName;
		int seed;
		float height;

		ShapeAttributes(const std::wstring rulef = L"bin/rule.cgb", const std::wstring startRl = L"Default$Lot",
			const std::wstring shapeN = L"Lot", int sd = 555, float hgt = 10.0f) :
			ruleFile(rulef), startRule(startRl), shapeName(shapeN), seed(sd), height(hgt) { }
	};

	struct EncoderOptions {
		bool emitReport = false;
		bool emitGeometry = true;
	};


	/**
	 * helpers for prt object management
	 */
	struct PRTDestroyer {
		void operator()(const prt::Object* p) const {
			if (p)
				p->destroy();
		}
	};

	using ObjectPtr = std::unique_ptr<const prt::Object, PRTDestroyer>;
	using CachePtr = std::unique_ptr<prt::CacheObject, PRTDestroyer>;
	using ResolveMapPtr = std::unique_ptr<const prt::ResolveMap, PRTDestroyer>;
	using InitialShapePtr = std::unique_ptr<const prt::InitialShape, PRTDestroyer>;
	using InitialShapeBuilderPtr = std::unique_ptr<prt::InitialShapeBuilder, PRTDestroyer>;
	using AttributeMapPtr = std::unique_ptr<const prt::AttributeMap, PRTDestroyer>;
	using AttributeMapBuilderPtr = std::unique_ptr<prt::AttributeMapBuilder, PRTDestroyer>;
	using FileOutputCallbacksPtr = std::unique_ptr<prt::FileOutputCallbacks, PRTDestroyer>;
	using ConsoleLogHandlerPtr = std::unique_ptr<prt::ConsoleLogHandler, PRTDestroyer>;
	using FileLogHandlerPtr = std::unique_ptr<prt::FileLogHandler, PRTDestroyer>;
	using EncoderInfoPtr = std::unique_ptr<const prt::EncoderInfo, PRTDestroyer>;
	using DecoderInfoPtr = std::unique_ptr<const prt::DecoderInfo, PRTDestroyer>;
	using SimpleOutputCallbacksPtr = std::unique_ptr<prt::SimpleOutputCallbacks, PRTDestroyer>;
	using RhinoCallbacksPtr = std::unique_ptr<RhinoCallbacks>;

	AttributeMapPtr createAttributeMapForShape(const ShapeAttributes& attrs, prt::AttributeMapBuilder& bld);
	AttributeMapPtr createAttributeMapForEncoder(const EncoderOptions& attrs, prt::AttributeMapBuilder& bld);
	AttributeMapPtr createValidatedOptions(const std::wstring& encID, const AttributeMapPtr& unvalidatedOptions);


	/**
	 * String and URI helpers
	 */
	using URI = std::string;

	std::string toOSNarrowFromUTF16(const std::wstring& osWString);
	std::wstring toUTF16FromOSNarrow(const std::string& osString);
	std::wstring toUTF16FromUTF8(const std::string& utf8String);
	std::string toUTF8FromOSNarrow(const std::string& osString);
	std::string percentEncode(const std::string& utf8String);
	URI toFileURI(const std::string& p);
}