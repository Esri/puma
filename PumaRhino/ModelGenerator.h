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

#pragma once

#include "GeneratedModel.h"
#include "PRTContext.h"
#include "RawInitialShape.h"
#include "ResolveMapCache.h"
#include "RhinoCallbacks.h"
#include "RuleAttributes.h"
#include "utils.h"

/**
 * Entry point of the PRT. Is given an initial shape and rpk package, gives them to the PRT and gets the results.
 */
class ModelGenerator {
public:
	ModelGenerator();

	std::vector<GeneratedModelPtr> generateModel(const std::vector<RawInitialShape>& rawInitialShapes,
	                                             const std::vector<pcu::ShapeAttributes>& shapeAttributes,
	                                             pcu::AttributeMapBuilderVector& aBuilders);

	bool evalDefaultAttributes(const std::vector<RawInitialShape>& rawInitialShapes,
	                           std::vector<pcu::ShapeAttributes>& shapeAttributes);

	void updateRuleFiles(const std::wstring& rulePkg);

	void updateEncoderOptions(bool emitMaterials);

	const RuleAttributes& getRuleAttributes() const {
		return mRuleAttributes;
	}

	std::wstring getRuleFile();
	std::wstring getStartingRule();
	std::wstring getDefaultShapeName();
	inline const prt::ResolveMap* getResolveMap() {
		return mResolveMap.get();
	};
	const pcu::AttributeMapPtrVector& getDefaultValueAttributeMap() {
		return mDefaultValuesMap;
	};

	const std::wstring getPackagePath() const {
		return mCurrentRPK;
	}

	bool getDefaultValuesBoolean(const std::wstring& key, ON_SimpleArray<int>* pValues);
	bool getDefaultValuesNumber(const std::wstring& key, ON_SimpleArray<double>* pValues);
	bool getDefaultValuesText(const std::wstring& key, ON_ClassArray<ON_wString>* pTexts);
	bool getDefaultValuesBooleanArray(const std::wstring& key, ON_SimpleArray<int>* pValues,
	                                  ON_SimpleArray<int>* pSizes);
	bool getDefaultValuesNumberArray(const std::wstring& key, ON_SimpleArray<double>* pValues,
	                                 ON_SimpleArray<int>* pSizes);
	bool getDefaultValuesTextArray(const std::wstring& key, ON_ClassArray<ON_wString>* pTexts,
	                               ON_SimpleArray<int>* pSizes);

private:
	pcu::RuleFileInfoPtr mRuleFileInfo;
	pcu::ResolveMapSPtr mResolveMap;
	RuleAttributes mRuleAttributes;

	pcu::AttributeMapPtr mRhinoEncoderOptions;
	pcu::AttributeMapPtr mCGAErrorOptions;
	pcu::AttributeMapPtr mCGAPrintOptions;

	// contains the rule attributes evaluated
	pcu::AttributeMapPtrVector mDefaultValuesMap;

	std::wstring mRulePkg;
	std::wstring mRuleFile = L"bin/rule.cgb";
	std::wstring mStartRule = L"default$Lot";
	int32_t mSeed = 0;
	std::wstring mShapeName = L"Lot";
	std::wstring mCurrentRPK;

	bool createInitialShapes(const std::vector<RawInitialShape>& rawInitialShapes,
	                         const std::vector<pcu::ShapeAttributes>& shapeAttributes,
	                         pcu::AttributeMapBuilderVector& aBuilders,
	                         std::vector<pcu::InitialShapePtr>& initialShapes,
	                         std::vector<pcu::AttributeMapPtr>& initialShapeAttributes) const;

	void extractMainShapeAttributes(pcu::AttributeMapBuilderPtr& aBuilder, const pcu::ShapeAttributes& shapeAttr,
	                                std::wstring& ruleFile, std::wstring& startRule, int32_t& seed,
	                                std::wstring& shapeName, pcu::AttributeMapPtr& convertShapeAttr) const;
};