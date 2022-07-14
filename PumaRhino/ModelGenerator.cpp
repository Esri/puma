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

#include "ModelGenerator.h"

#include "AttrEvalCallbacks.h"
#include "Logger.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <filesystem>
#include <future>
#include <numeric>

namespace {

constexpr const wchar_t* ENCODER_ID_RHINO = L"com.esri.rhinoprt.RhinoEncoder";
constexpr const wchar_t* ENCODER_ID_CGA_ERROR = L"com.esri.prt.core.CGAErrorEncoder";
constexpr const wchar_t* ENCODER_ID_CGA_PRINT = L"com.esri.prt.core.CGAPrintEncoder";

constexpr std::array<const wchar_t*, 3> ALL_ENCODER_IDS = {ENCODER_ID_RHINO, ENCODER_ID_CGA_ERROR,
                                                           ENCODER_ID_CGA_PRINT};

constexpr const wchar_t* FILE_CGA_ERROR = L"CGAErrors.txt";
constexpr const wchar_t* FILE_CGA_PRINT = L"CGAPrint.txt";

constexpr const wchar_t* RESOLVEMAP_EXTRACTION_PREFIX = L"rhino_prt";
constexpr const wchar_t* ENCODER_ID_CGA_EVALATTR = L"com.esri.prt.core.AttributeEvalEncoder";

pcu::AttributeMapPtr getAttrEvalEncoderInfo() {
	const pcu::EncoderInfoPtr encInfo(prt::createEncoderInfo(ENCODER_ID_CGA_EVALATTR));
	const prt::AttributeMap* encOpts = nullptr;
	encInfo->createValidatedOptionsAndStates(nullptr, &encOpts);
	return pcu::AttributeMapPtr(encOpts);
}

void logAttributeTypeError(const std::wstring& key) {
	LOG_ERR << "Impossible to get default value for rule attribute: " << key
	        << " The expected type does not correspond to the actual type of this attribute.";
}

void logAttributeError(const std::wstring& key, prt::Status& status) {
	LOG_ERR << "Impossible to get default value for rule attribute: " << key
	        << " with error: " << prt::getStatusDescription(status);
}

template <typename T, typename D>
std::vector<T*> toRawPtrs(const std::vector<std::unique_ptr<T, D>>& smartPtrs) {
	std::vector<T*> rawPtrs(smartPtrs.size());
	std::transform(smartPtrs.begin(), smartPtrs.end(), rawPtrs.begin(), [](const auto& o) { return o.get(); });
	return rawPtrs;
}

std::vector<const wchar_t*> toRawPtrs(const std::vector<std::wstring>& strings) {
	std::vector<const wchar_t*> rawPtrs(strings.size());
	std::transform(strings.begin(), strings.end(), rawPtrs.begin(), [](const auto& s) { return s.c_str(); });
	return rawPtrs;
}

std::vector<size_t> distribute(size_t tasks, size_t slots) {
	const size_t base = tasks / slots;
	const size_t extra = tasks % slots;

	std::vector<size_t> d(slots, base);
	std::fill_n(d.begin(), extra, base + 1);

	return d;
}

std::vector<GeneratedModelPtr> batchGenerate(const std::vector<pcu::InitialShapePtr>& initialShapes,
                                             const std::vector<const prt::AttributeMap*>& encoderOptions,
                                             prt::Cache* prtCache) {
	const size_t nThreads = std::min<size_t>(std::thread::hardware_concurrency(), initialShapes.size());
	const std::vector<size_t> initialShapesPerThread = distribute(initialShapes.size(), nThreads);

	std::vector<size_t> offsets(nThreads, 0);
	std::partial_sum(initialShapesPerThread.begin(), std::prev(initialShapesPerThread.end()),
	                 std::next(offsets.begin()));

	// TODO: if nThreads is smaller than cpu cores we can enable multi-threaded generation within a shape with the
	// remaining cores

	std::vector<prt::InitialShape const*> rawInitialShapes = toRawPtrs<const prt::InitialShape>(initialShapes);
	std::vector<pcu::RhinoCallbacksPtr> callbacks(nThreads); // one callback per thread
	std::vector<std::future<void>> futures;
	futures.reserve(nThreads);

	for (int8_t ti = 0; ti < nThreads; ti++) {
		auto f = std::async(std::launch::async, [ti, &initialShapesPerThread, &offsets, &callbacks, &rawInitialShapes,
		                                         &encoderOptions, &prtCache] {
			LOG_DBG << "thread " << ti << ": shapes = " << initialShapesPerThread[ti] << ", offset = " << offsets[ti];

			prt::InitialShape const* const* isRangeStart = &rawInitialShapes[offsets[ti]];
			callbacks[ti] = std::make_unique<RhinoCallbacks>(initialShapesPerThread[ti]);

			const prt::Status generateStatus = prt::generate(
			        isRangeStart, initialShapesPerThread[ti], nullptr, ALL_ENCODER_IDS.data(), ALL_ENCODER_IDS.size(),
			        encoderOptions.data(), callbacks[ti].get(), prtCache, nullptr);

			if (generateStatus != prt::STATUS_OK) {
				LOG_WRN << "generation (batch " << ti << ") failed with status: '"
				        << prt::getStatusDescription(generateStatus) << "' (" << generateStatus << ")";
			}
		});
		futures.emplace_back(std::move(f));
	}
	std::for_each(futures.begin(), futures.end(), [](std::future<void>& f) { f.wait(); });

	std::vector<GeneratedModelPtr> generatedModels(initialShapes.size());
	for (size_t ri = 0; ri < callbacks.size(); ri++) {
		const std::vector<GeneratedModelPtr>& models = callbacks[ri]->getModels();
		for (size_t mi = 0; mi < models.size(); mi++) {
			generatedModels[offsets[ri] + mi] = models[mi];
		}
	}

	return generatedModels;
}

pcu::AttributeMapPtrVector createAttributeMaps(pcu::AttributeMapBuilderVector& ambv) {
	pcu::AttributeMapPtrVector attributeMaps;

	for (auto& amb : ambv) {
		prt::Status status = prt::STATUS_UNSPECIFIED_ERROR;
		pcu::AttributeMapPtr am{amb->createAttributeMap(&status)};
		if (status == prt::STATUS_OK)
			attributeMaps.emplace_back(std::move(am));
		else
			attributeMaps.push_back({});
	}

	return attributeMaps;
}

} // namespace

ModelGenerator::ModelGenerator() {
	pcu::AttributeMapBuilderPtr optionsBuilder(prt::AttributeMapBuilder::create());

	mRhinoEncoderOptions = pcu::createValidatedOptions(ENCODER_ID_RHINO);

	optionsBuilder->setString(L"name", FILE_CGA_ERROR);
	const pcu::AttributeMapPtr errOptions(optionsBuilder->createAttributeMapAndReset());
	mCGAErrorOptions = pcu::createValidatedOptions(ENCODER_ID_CGA_ERROR, errOptions.get());

	optionsBuilder->setString(L"name", FILE_CGA_PRINT);
	const pcu::AttributeMapPtr printOptions(optionsBuilder->createAttributeMapAndReset());
	mCGAPrintOptions = pcu::createValidatedOptions(ENCODER_ID_CGA_PRINT, printOptions.get());
}

pcu::ResolveMapSPtr ModelGenerator::getResolveMap(const std::wstring& rulePkg) {
	const ResolveMap::ResolveMapCache::LookupResult lookup = PRTContext::get()->getResolveMap(rulePkg);
	return lookup.first;
}

const RuleAttributes ModelGenerator::getRuleAttributes(const std::wstring& rulePkg) {
	pcu::ResolveMapSPtr resolveMap = getResolveMap(rulePkg);

	// Extract the rule package info.
	std::wstring ruleFile = pcu::getRuleFileEntry(resolveMap);
	if (ruleFile.empty()) {
		LOG_ERR << "Could not find rule file in rule package" << rulePkg;
		throw std::exception("Could not find rule file in rule package ");
	}

	// To create the ruleFileInfo, we first need the ruleFileURI
	const wchar_t* ruleFileURI = resolveMap->getString(ruleFile.c_str());
	if (ruleFileURI == nullptr) {
		LOG_ERR << "Could not find rule file URI in resolve map of rule package." << rulePkg;
		throw std::exception("Could not find rule file URI in resolve map of rule package.");
	}

	// Create RuleFileInfo
	prt::Status infoStatus = prt::STATUS_UNSPECIFIED_ERROR;
	pcu::RuleFileInfoPtr ruleFileInfo(
	        prt::createRuleFileInfo(ruleFileURI, PRTContext::get()->mPRTCache.get(), &infoStatus));

	if (!ruleFileInfo || infoStatus != prt::STATUS_OK) {
		LOG_ERR << "could not get rule file info from rule file " << ruleFile;
		throw std::exception("Could not get rule file info from rule file.");
	}

	RuleAttributes attributes;
	createRuleAttributes(ruleFile, *ruleFileInfo.get(), attributes);
	return attributes;
}

pcu::ShapeAttributes ModelGenerator::getShapeAttributes(const std::wstring& rulePkg) {
	pcu::ResolveMapSPtr resolveMap;
	try {
		resolveMap = getResolveMap(rulePkg);
	}
	catch (std::exception&) {
		throw;
	}

	// Extract the rule package info.
	std::wstring ruleFile = pcu::getRuleFileEntry(resolveMap);
	if (ruleFile.empty()) {
		LOG_ERR << "Could not find rule file in rule package" << rulePkg;
		throw std::exception("Could not find rule file in rule package ");
	}

	// To create the ruleFileInfo, we first need the ruleFileURI
	const wchar_t* ruleFileURI = resolveMap->getString(ruleFile.c_str());
	if (ruleFileURI == nullptr) {
		LOG_ERR << "Could not find rule file URI in resolve map of rule package." << rulePkg;
		throw std::exception("Could not find rule file URI in resolve map of rule package.");
	}

	// Create RuleFileInfo
	prt::Status infoStatus = prt::STATUS_UNSPECIFIED_ERROR;
	pcu::RuleFileInfoPtr ruleFileInfo(prt::createRuleFileInfo(ruleFileURI, PRTContext::get()->mPRTCache.get(), &infoStatus));

	if (!ruleFileInfo || infoStatus != prt::STATUS_OK) {
		LOG_ERR << "could not get rule file info from rule file " << ruleFile;
		throw std::exception("Could not get rule file info from rule file.");
	}

	std::wstring startRule = pcu::detectStartRule(ruleFileInfo);

	return pcu::ShapeAttributes(std::move(ruleFileInfo), ruleFile, startRule);
}

pcu::AttributeMapPtrVector ModelGenerator::evalDefaultAttributes(const std::wstring& rulePkg,
										   const std::vector<RawInitialShape>& rawInitialShapes,
                                           pcu::ShapeAttributes& shapeAttributes) {
	pcu::ResolveMapSPtr resolveMap = getResolveMap(rulePkg);

	// setup encoder options for attribute evaluation encoder
	constexpr const wchar_t* encs[] = {ENCODER_ID_CGA_EVALATTR};
	constexpr size_t encsCount = sizeof(encs) / sizeof(encs[0]);
	const pcu::AttributeMapPtr encOpts = getAttrEvalEncoderInfo();
	const prt::AttributeMap* encsOpts[] = {encOpts.get()};

	const size_t numShapes = rawInitialShapes.size();

	pcu::AttributeMapBuilderVector attribMapBuilders;
	attribMapBuilders.reserve(numShapes);

	for (size_t isIdx = 0; isIdx < numShapes; ++isIdx) {
		pcu::AttributeMapBuilderPtr amb(prt::AttributeMapBuilder::create());
		attribMapBuilders.emplace_back(std::move(amb));
	}

	std::vector<pcu::InitialShapePtr> initialShapes;
	std::vector<pcu::AttributeMapPtr> initialShapeAttributes; // put here to ensure same life time as initialShapes
	if (!createInitialShapes(resolveMap, rawInitialShapes, shapeAttributes, attribMapBuilders, initialShapes,
	                         initialShapeAttributes))
		return {};

	// run generate
	AttrEvalCallbacks aec(attribMapBuilders, shapeAttributes.ruleFileInfo); // TODO: What if rule file info is not the same for all shapes?
	const std::vector<prt::InitialShape const*> rawInitialShapePtrs = toRawPtrs<const prt::InitialShape>(initialShapes);
	const prt::Status status = prt::generate(rawInitialShapePtrs.data(), rawInitialShapePtrs.size(), nullptr, encs,
	                                         encsCount, encsOpts, &aec, PRTContext::get()->mPRTCache.get(), nullptr);
	if (status != prt::STATUS_OK) {
		LOG_ERR << "Failed to get default rule attributes: '" << prt::getStatusDescription(status) << "' (" << status
		        << ")";
		return {};
	}

	pcu::AttributeMapPtrVector defaultValuesMap = createAttributeMaps(attribMapBuilders);
	
	return defaultValuesMap;
}

bool ModelGenerator::createInitialShapes(
	pcu::ResolveMapSPtr& resolveMap,
	const std::vector<RawInitialShape>& rawInitialShapes,
    const pcu::ShapeAttributes& shapeAttributes,
    pcu::AttributeMapBuilderVector& aBuilders,
    std::vector<pcu::InitialShapePtr>& initialShapes,
    std::vector<pcu::AttributeMapPtr>& initialShapesAttributes) const {

	pcu::InitialShapeBuilderPtr isb(prt::InitialShapeBuilder::create());
	initialShapes.reserve(rawInitialShapes.size());
	initialShapesAttributes.reserve(rawInitialShapes.size());

	for (size_t i = 0; i < rawInitialShapes.size(); ++i) {
		const RawInitialShape& ris = rawInitialShapes[i];

		const prt::Status geometryStatus =
		        isb->setGeometry(ris.getVertices(), ris.getVertexCount(), ris.getIndices(), ris.getIndexCount(),
		                         ris.getFaceCounts(), ris.getFaceCountsCount());
		if (geometryStatus != prt::STATUS_OK) {
			LOG_ERR << "Encountered invalid initial shape geometry: " << prt::getStatusDescription(geometryStatus);
			return false;
		}

		// Set to default values
		std::wstring ruleF = shapeAttributes.ruleFile;
		std::wstring startR = shapeAttributes.startRule;
		int32_t randomS = shapeAttributes.seed;
		std::wstring shapeN = shapeAttributes.shapeName;

		pcu::AttributeMapPtr initialShapeAttributes;
		extractMainShapeAttributes(aBuilders[i], shapeAttributes, ruleF, startR, randomS, shapeN, initialShapeAttributes);

		const prt::Status attributeStatus = isb->setAttributes(ruleF.c_str(), startR.c_str(), randomS, shapeN.c_str(),
		                                                       initialShapeAttributes.get(), resolveMap.get());
		if (attributeStatus != prt::STATUS_OK) {
			LOG_ERR << "Failed to set initial shape attributes: " << prt::getStatusDescription(attributeStatus);
			return false;
		}

		prt::Status creationStatus = prt::STATUS_UNSPECIFIED_ERROR;
		pcu::InitialShapePtr initialShape(isb->createInitialShapeAndReset(&creationStatus));
		if (creationStatus != prt::STATUS_OK) {
			LOG_ERR << "Failed to create initial shape: " << prt::getStatusDescription(creationStatus);
			return false;
		}

		initialShapes.emplace_back(std::move(initialShape));
		initialShapesAttributes.emplace_back(std::move(initialShapeAttributes));
	}

	return true;
}

std::vector<GeneratedModelPtr> ModelGenerator::generateModel(const std::wstring& rulePkg,
															 const std::vector<RawInitialShape>& rawInitialShapes,
                                                             const pcu::ShapeAttributes& shapeAttributes,
                                                             pcu::AttributeMapBuilderVector& aBuilders) {

	pcu::ResolveMapSPtr resolveMap = getResolveMap(rulePkg);

	try {
		std::vector<pcu::InitialShapePtr> initialShapes;
		std::vector<pcu::AttributeMapPtr> initialShapeAttributes; // put here to ensure same life time as initialShapes
		if (!createInitialShapes(resolveMap, rawInitialShapes, shapeAttributes, aBuilders, initialShapes, initialShapeAttributes))
			return {};

		const std::vector<const prt::AttributeMap*> encoderOptions = {mRhinoEncoderOptions.get(),
		                                                              mCGAErrorOptions.get(), mCGAPrintOptions.get()};

		const std::vector<GeneratedModelPtr> generatedModels =
		        batchGenerate(initialShapes, encoderOptions, PRTContext::get()->mPRTCache.get());

		return generatedModels;
	}
	catch (const std::exception& e) {
		LOG_ERR << "caught exception: " << e.what();
	}
	catch (...) {
		LOG_ERR << "caught unknown exception.";
	}

	return {};
}

void ModelGenerator::updateEncoderOptions(bool emitMaterials) {
	pcu::AttributeMapBuilderPtr optionsBuilder(prt::AttributeMapBuilder::create());
	optionsBuilder->setBool(L"emitMaterials", emitMaterials);
	pcu::AttributeMapPtr rawOptions(optionsBuilder->createAttributeMap());
	mRhinoEncoderOptions = pcu::createValidatedOptions(ENCODER_ID_RHINO, rawOptions.get());
}

void ModelGenerator::extractMainShapeAttributes(pcu::AttributeMapBuilderPtr& aBuilder,
                                                const pcu::ShapeAttributes& shapeAttr, std::wstring& ruleFile,
                                                std::wstring& startRule, int32_t& seed, std::wstring& shapeName,
                                                pcu::AttributeMapPtr& convertShapeAttr) const {
	convertShapeAttr = pcu::createAttributeMapForShape(shapeAttr, *aBuilder.get());

	if (convertShapeAttr) {
		if (convertShapeAttr->hasKey(L"ruleFile") &&
		    convertShapeAttr->getType(L"ruleFile") == prt::AttributeMap::PT_STRING)
			ruleFile = convertShapeAttr->getString(L"ruleFile");
		if (convertShapeAttr->hasKey(L"startRule") &&
		    convertShapeAttr->getType(L"startRule") == prt::AttributeMap::PT_STRING)
			startRule = convertShapeAttr->getString(L"startRule");
		if (convertShapeAttr->hasKey(SEED_KEY) && convertShapeAttr->getType(SEED_KEY) == prt::AttributeMap::PT_FLOAT)
			seed = convertShapeAttr->getFloat(SEED_KEY);
		if (convertShapeAttr->hasKey(L"shapeName") &&
		    convertShapeAttr->getType(L"shapeName") == prt::AttributeMap::PT_STRING)
			shapeName = convertShapeAttr->getString(L"shapeName");
	}
}
