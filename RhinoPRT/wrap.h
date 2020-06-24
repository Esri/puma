#pragma once

// Wrapper for PRT classes

#include "RhinoCallbacks.h"
#include "utils.h"

#include "prt/prt.h"
#include "prt/API.h"
#include "prt/LogLevel.h"
#include "prt/ContentType.h"

#include <map>
#include <vector>
#include <string>

/**
* Helper struct to manage PRT lifetime
*/
struct PRTContext {
	PRTContext(prt::LogLevel minimalLogLevel) {
		mLogHandler = prt::ConsoleLogHandler::create(prt::LogHandler::ALL, prt::LogHandler::ALL_COUNT);
		auto fileLogHandler = prt::FileLogHandler::create(prt::LogHandler::ALL, prt::LogHandler::ALL_COUNT, L"C:/Windows/Temp/rhino_log.txt");
		prt::addLogHandler(mLogHandler);
		prt::addLogHandler(fileLogHandler);

		const wchar_t* prt_path[2] = { L"C:/Users/lor11212/Documents/Rhino/rhino-plugin-prototype/esri_sdk/lib", L"C:/Users/lor11212/Documents/Rhino/rhino-plugin-prototype/x64/Release/codecs_rhino.dll" };
		mPRTHandle = prt::init(prt_path, 2, minimalLogLevel);
	}

	~PRTContext() {
		// shutdown PRT
		mPRTHandle->destroy();

		prt::removeLogHandler(mLogHandler);
		mLogHandler->destroy();
	}

	explicit operator bool() const {
		return (bool)mPRTHandle;
	}
	
	prt::ConsoleLogHandler* mLogHandler;
	const prt::Object* mPRTHandle;
};

/**
* The Initial shape that will be given to PRT
*/
class InitialShape {
public:
	InitialShape();
	InitialShape(const std::vector<double> &vertices);
	~InitialShape() {}

	const double* getVertices() const {
		return mVertices.data();
	}

	size_t getVertexCount() const {
		return mVertices.size();
	}

	const uint32_t* getIndices() const {
		return mIndices.data();
	}

	size_t getIndexCount() const {
		return mIndices.size();
	}

	const uint32_t* getFaceCounts() const {
		return mFaceCounts.data();
	}

	size_t getFaceCountsCount() const {
		return mFaceCounts.size();
	}

protected:

	const std::vector<double> mVertices;
	std::vector<uint32_t> mIndices;
	std::vector<uint32_t> mFaceCounts;
};

/**
* The model generated by PRT
*/
class GeneratedModel {
public:
	GeneratedModel(const size_t& initialShapeIdx, const std::vector<double>& vert, const std::vector<uint32_t>& indices,
		const std::vector<uint32_t>& face, const std::map<std::string, std::string>& rep);
	GeneratedModel() {}
	~GeneratedModel() {}

	size_t getInitialShapeIndex() const {
		return mInitialShapeIndex;
	}
	const std::vector<double>& getVertices() const {
		return mVertices;
	}
	const std::vector<uint32_t>& getIndices() const {
		return mIndices;
	}
	const std::vector<uint32_t>& getFaces() const {
		return mFaces;
	}
	const std::map<std::string, std::string>& getReport() const {
		return mReport;
	}

private:
	size_t mInitialShapeIndex;
	std::vector<double> mVertices;
	std::vector<uint32_t> mIndices;
	std::vector<uint32_t> mFaces;
	std::map<std::string, std::string> mReport;
};


/**
* Entry point of the PRT. Gets an initial shape and rpk package, gives them to the PRT and gets the results.
*/
class ModelGenerator {
public:
	ModelGenerator(const std::vector<InitialShape>& initial_geom);
	~ModelGenerator() {}

	std::vector<GeneratedModel> generateModel(std::vector<pcu::ShapeAttributes>& shapeAttributes,
												const std::string& rulePackagePath,
												const std::wstring& geometryEncoderName,
												const pcu::EncoderOptions& geometryEncoderOptions);
private:
	pcu::ResolveMapPtr mResolveMap;
	pcu::CachePtr mCache;

	pcu::AttributeMapBuilderPtr mEncoderBuilder;
	std::vector<pcu::InitialShapeBuilderPtr> mInitialShapesBuilders;
	std::vector<std::wstring> mEncodersNames;
	std::vector<pcu::AttributeMapPtr> mEncodersOptionsPtr;
	
	std::wstring mRuleFile = L"bin/rule.cgb";
	std::wstring mStartRule = L"default$Lot";
	int32_t mSeed = 0;
	std::wstring mShapeName = L"Lot";

	bool mValid = true;

	void setAndCreateInitialShape(const std::vector<pcu::ShapeAttributes>& shapesAttr,
		std::vector<const prt::InitialShape*>& initShapes,
		std::vector<pcu::InitialShapePtr>& initShapesPtrs,
		std::vector<pcu::AttributeMapPtr>& convertedShapeAttr);

	void initializeEncoderData(const std::wstring& encName, const pcu::EncoderOptions& encOpt);

	void getRawEncoderDataPointers(std::vector<const wchar_t*>& allEnc,
		std::vector<const prt::AttributeMap*>& allEncOpt);

	void extractMainShapeAttributes(const pcu::ShapeAttributes& shapeAttr, std::wstring& ruleFile, std::wstring& startRule,
		int32_t& seed, std::wstring& shapeName, pcu::AttributeMapPtr& convertShapeAttr);
};