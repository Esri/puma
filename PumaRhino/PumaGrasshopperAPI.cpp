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

#include "RhinoPRT.h"
#include "version.h"
#include "utils.h"

#define RHINOPRT_API __declspec(dllexport)

constexpr bool DBG = false;

namespace {

template <typename T, typename T1>
T static_cast_fct(const T1& x) {
	return static_cast<T>(x);
}

} // namespace

extern "C" {

RHINOPRT_API void GetProductVersion(ON_wString* version_str) {
	*version_str = VER_FILE_VERSION_STR;
}

RHINOPRT_API bool InitializeRhinoPRT() {
	return RhinoPRT::get().InitializeRhinoPRT();
}

RHINOPRT_API void ShutdownRhinoPRT() {
	RhinoPRT::get().ShutdownRhinoPRT();
}

RHINOPRT_API bool Generate(const wchar_t* rpk_path, ON_wString* errorMsg,
						   // rule attributes
						   const int shapeCount,
						   ON_SimpleArray<int>* pBoolStarts, const int boolCount,
						   ON_ClassArray<ON_wString>* pBoolKeys, ON_SimpleArray<int>* pBoolVals,

						   ON_SimpleArray<int>* pDoubleStarts, const int doubleCount,
						   ON_ClassArray<ON_wString>* pDoubleKeys, ON_SimpleArray<double>* pDoubleVals,

						   ON_SimpleArray<int>* pStringStarts, const int stringCount, 
						   ON_ClassArray<ON_wString>* pStringKeys, ON_ClassArray<ON_wString>* pStringVals,

						   ON_SimpleArray<int>* pBoolArrayStarts, const int boolArrayCount,
                           ON_ClassArray<ON_wString>* pBoolArrayKeys, ON_ClassArray<ON_wString>* pBoolArrayVals,

						   ON_SimpleArray<int>* pDoubleArrayStarts, const int doubleArrayCount,
                           ON_ClassArray<ON_wString>* pDoubleArrayKeys, ON_ClassArray<ON_wString>* pDoubleArrayVals,

						   ON_SimpleArray<int>* pStringArrayStarts, const int stringArrayCount,
                           ON_ClassArray<ON_wString>* pStringArrayKeys, ON_ClassArray<ON_wString>* pStringArrayVals,

						   // Initial geometry
                           ON_SimpleArray<const ON_Mesh*>* pMesh,

						   // Resulting geometry
						   ON_SimpleArray<int>* pMeshCounts,
                           ON_SimpleArray<ON_Mesh*>* pMeshArray,
							
						   // Materials,
                           ON_SimpleArray<double>* pColorsArray, ON_SimpleArray<int>* pMatIndices,
                           ON_ClassArray<ON_wString>* pTexKeys, ON_ClassArray<ON_wString>* pTexPaths,
	
						   // Reports
						   ON_SimpleArray<int>* pReportsCountArray,
                           ON_ClassArray<ON_wString>* pKeysArray, ON_SimpleArray<double>* pDoubleReports,
                           ON_SimpleArray<bool>* pBoolReports, ON_ClassArray<ON_wString>* pStringReports) {

	if (pMesh == nullptr)
		return false;

	std::vector<RawInitialShape> rawInitialShapes;
	rawInitialShapes.reserve(pMesh->Count());
	for (int i = 0; i < pMesh->Count(); ++i) {
		rawInitialShapes.emplace_back(**pMesh->At(i));
	}

	// Initialise the attribute map builders for each initial shape.
	pcu::AttributeMapBuilderVector aBuilders(shapeCount);
	for (auto& it : aBuilders) {
		it.reset(prt::AttributeMapBuilder::create());
	}
	
	for (int i = 0; i < shapeCount; ++i) {
		// by initial shape
		int indexStartBool = *pBoolStarts->At(i);
		int indexStartDouble = *pDoubleStarts->At(i);
		int indexStartString = *pStringStarts->At(i);
		int indexStartBoolArray = *pBoolArrayStarts->At(i);
		int indexStartDoubleArray = *pDoubleArrayStarts->At(i);
		int indexStartStringArray = *pStringArrayStarts->At(i);

		bool notLastShape = i < shapeCount - 1;

		int nextIndexStartBool = notLastShape ? *pBoolStarts->At(i + 1) : boolCount;
		int nextIndexStartDouble = notLastShape ? *pDoubleStarts->At(i + 1) : doubleCount;
		int nextIndexStartString = notLastShape ? *pStringStarts->At(i + 1) : stringCount;
		int nextIndexStartBoolArray = notLastShape ? *pBoolArrayStarts->At(i + 1) : boolArrayCount;
		int nextIndexStartDoubleArray = notLastShape ? *pDoubleArrayStarts->At(i + 1) : doubleArrayCount;
		int nextIndexStartStringArray = notLastShape ? *pStringArrayStarts->At(i + 1) : stringArrayCount;

		int boolAttrCount = nextIndexStartBool - indexStartBool;
		int doubleAttrCount = nextIndexStartDouble - indexStartDouble;
		int stringAttrCount = nextIndexStartString - indexStartString;
		int boolAttrArrayCount = nextIndexStartBoolArray - indexStartBoolArray;
		int doubleAttrArrayCount = nextIndexStartDoubleArray - indexStartDoubleArray;
		int stringAttrArrayCount = nextIndexStartStringArray - indexStartStringArray;

		pcu::unpackBoolAttributes(indexStartBool, boolAttrCount, pBoolKeys, pBoolVals, aBuilders[i]);
		pcu::unpackDoubleAttributes(indexStartDouble, doubleAttrCount, pDoubleKeys, pDoubleVals, aBuilders[i]);
		pcu::unpackStringAttributes(indexStartString, stringAttrCount, pStringKeys, pStringVals,
		                      aBuilders[i], false);
		pcu::unpackBoolArrayAttributes(indexStartBoolArray, boolAttrArrayCount, pBoolArrayKeys, pBoolArrayVals,
		                           aBuilders[i]);
		pcu::unpackDoubleArrayAttributes(indexStartDoubleArray, doubleAttrArrayCount, pDoubleArrayKeys, pDoubleArrayVals, aBuilders[i]);
		pcu::unpackStringAttributes(indexStartStringArray, stringAttrArrayCount, pStringArrayKeys, pStringArrayVals,
		                            aBuilders[i], true);

		indexStartBool += boolAttrCount;
		indexStartDouble += doubleAttrCount;
		indexStartString += stringAttrCount;
		indexStartBoolArray += boolAttrArrayCount;
		indexStartDoubleArray += doubleAttrArrayCount;
		indexStartStringArray += stringAttrArrayCount;
	}

	const auto& models = RhinoPRT::get().GenerateGeometry(std::wstring(rpk_path), rawInitialShapes, aBuilders);

	for (size_t i = 0; i < models.size(); i++) {
		if (models[i]) {
			const GeneratedModel::MeshBundle meshBundle =
			        models[i]->createRhinoMeshes(i);
			pMeshCounts->Append(static_cast<int>(meshBundle.size()));
			for (const auto& meshPart : meshBundle) {
				pMeshArray->Append(new ON_Mesh(meshPart));
			}

			// Materials
			pMatIndices->Append(meshBundle.size());

			const auto& materials = models[i]->getMaterials();
			for (const auto& material : materials) {
				const auto& matAttributes = material.second;
				pcu::appendColor(matAttributes.mDiffuseCol, pColorsArray);
				pcu::appendColor(matAttributes.mAmbientCol, pColorsArray);
				pcu::appendColor(matAttributes.mSpecularCol, pColorsArray);
				pColorsArray->Append(matAttributes.mOpacity);
				pColorsArray->Append(matAttributes.mShininess);

				pMatIndices->Append(matAttributes.mTexturePaths.size());

				for (auto& texture : matAttributes.mTexturePaths) {

					if constexpr (DBG) {
						LOG_DBG << L"texture: [ " << texture.first << " : " << texture.second << "]";
					}

					pTexKeys->Append(ON_wString(texture.first.c_str()));
					pTexPaths->Append(ON_wString(texture.second.c_str()));
				}
			}

			// Reports
			auto reports = Reporting::ToReportsVector(models[i]->getReports());

			/*
			left.float	-> right.all OK
			left.bool	-> right.float OK
						-> right.bool OK
						-> right.string OK
			left.string -> right.all OK
			*/
			// Sort the reports by Type. The order is Double -> Bool -> String
			std::sort(reports.begin(), reports.end(),
			          [](Reporting::ReportAttribute left, Reporting::ReportAttribute right) -> bool {
				          if (left.mType == right.mType)
					          return left.mReportName.compare(right.mReportName) <
					                 0; // assuming case sensitivity. assuming two reports can't have the same name.
				          if (left.mType == prt::AttributeMap::PrimitiveType::PT_FLOAT)
					          return true;
				          if (right.mType == prt::AttributeMap::PrimitiveType::PT_FLOAT)
					          return false;
				          if (left.mType == prt::AttributeMap::PrimitiveType::PT_STRING)
					          return false;
				          if (left.mType == prt::AttributeMap::PrimitiveType::PT_BOOL &&
				              right.mType == prt::AttributeMap::PrimitiveType::PT_STRING)
					          return true;
				          return false;
			          });

			int doubleReportsCount = 0;
			int boolReportsCount = 0;
			int stringReportsCount = 0;

			for (const auto& report : reports) {
				pKeysArray->Append(ON_wString(report.mReportName.c_str()));

				switch (report.mType) {
					case prt::AttributeMap::PrimitiveType::PT_FLOAT:
						pDoubleReports->Append(report.mDoubleReport);
						doubleReportsCount++;
						break;
					case prt::AttributeMap::PrimitiveType::PT_BOOL:
						pBoolReports->Append(report.mBoolReport);
						boolReportsCount++;
						break;
					case prt::AttributeMap::PrimitiveType::PT_STRING:
						pStringReports->Append(ON_wString(report.mStringReport.c_str()));
						stringReportsCount++;
						break;
					default:
						// REMOVE LAST KEY
						pKeysArray->Remove(pKeysArray->Count() - 1);
				}
			}

			pReportsCountArray->Append(doubleReportsCount);
			pReportsCountArray->Append(boolReportsCount);
			pReportsCountArray->Append(stringReportsCount);
		}
		else {
			pMeshCounts->Append(0);
		}
	}

	return !models.empty();
}

RHINOPRT_API int GetRuleAttributes(const wchar_t* rpk_path, ON_ClassArray<ON_wString>* pAttributesBuffer, 
	ON_SimpleArray<int>* pAttributesTypes, ON_SimpleArray<int>* pBaseAnnotations, ON_SimpleArray<double>* pDoubleAnnotations,
	ON_ClassArray<ON_wString>* pStringAnnotations) {

	RuleAttributes ruleAttributes = RhinoPRT::get().GetRuleAttributes(rpk_path);

	for (const RuleAttributeUPtr& attribute : ruleAttributes) {
		pAttributesBuffer->Append(ON_wString(attribute->mRuleFile.c_str()));
		pAttributesBuffer->Append(ON_wString(attribute->mFullName.c_str()));
		pAttributesBuffer->Append(ON_wString(attribute->mNickname.c_str()));
		if (attribute->groups.size() > 0)
			pAttributesBuffer->Append(ON_wString(attribute->groups.front().c_str()));
		else
			pAttributesBuffer->Append({});

		pAttributesTypes->Append(attribute->mType);
		pAttributesTypes->Append(attribute->mAnnotations.size());

		for (const auto& annot : attribute->mAnnotations) {
			pBaseAnnotations->Append((int)annot->getType());
			

			switch (annot->getType()) { 
				case AttributeAnnotation::RANGE:
					RangeAttributes range = dynamic_cast<AnnotationRange*>(annot.get())->getAnnotArguments();
					pDoubleAnnotations->Append(range.mMin);
					pDoubleAnnotations->Append(range.mMax);
					pDoubleAnnotations->Append(range.mStepSize);
					break;
				case AttributeAnnotation::ENUM:
					pBaseAnnotations->Append((int)annot->getEnumType());

					switch (annot->getEnumType()) {
						case EnumAnnotationType::DOUBLE: {

							const std::vector<double> annotEnum =
							        dynamic_cast<AnnotationEnum<double>*>(annot.get())->getAnnotArguments();
							pBaseAnnotations->Append(annotEnum.size());
							std::for_each(annotEnum.begin(), annotEnum.end(),
							              [pDoubleAnnotations](double value) { pDoubleAnnotations->Append(value); });
							break;	
						}
						case EnumAnnotationType::STRING: {

							const std::vector<std::wstring> annotEnum =
							        dynamic_cast<AnnotationEnum<std::wstring>*>(annot.get())->getAnnotArguments();
							pBaseAnnotations->Append(annotEnum.size());
							std::for_each(annotEnum.begin(), annotEnum.end(), [pStringAnnotations](std::wstring value) {
								pStringAnnotations->Append(ON_wString(value.c_str()));
							});
							break;
						}
						default:
							break;
					}
					break;
				default:
					break;
			}
		}
	}

	return static_cast<int>(ruleAttributes.size());
}

RHINOPRT_API bool GetDefaultAttributes(	const wchar_t* rpk_path, ON_SimpleArray<const ON_Mesh*>* pMesh,
										ON_SimpleArray<int>* pBoolStarts, ON_ClassArray<ON_wString>* pBoolKeys, ON_SimpleArray<int>* pBoolVals,
										ON_SimpleArray<int>* pDoubleStarts, ON_ClassArray<ON_wString>* pDoubleKeys, ON_SimpleArray<double>* pDoubleVals,
										ON_SimpleArray<int>* pStringStarts, ON_ClassArray<ON_wString>* pStringKeys, ON_ClassArray<ON_wString>* pStringVals,
										ON_SimpleArray<int>* pBoolArrayStarts, ON_ClassArray<ON_wString>* pBoolArrayKeys, ON_ClassArray<ON_wString>* pBoolArrayVals,
										ON_SimpleArray<int>* pDoubleArrayStarts, ON_ClassArray<ON_wString>* pDoubleArrayKeys, ON_ClassArray<ON_wString>* pDoubleArrayVals,
										ON_SimpleArray<int>* pStringArrayStarts, ON_ClassArray<ON_wString>* pStringArrayKeys, ON_ClassArray<ON_wString>* pStringArrayVals) {
	if (rpk_path == nullptr || pMesh == nullptr || pMesh->Count() == 0)
		return false;

	std::vector<RawInitialShape> rawInitialShapes;
	rawInitialShapes.reserve(pMesh->Count());
	for (int i = 0; i < pMesh->Count(); ++i) {
		rawInitialShapes.emplace_back(**pMesh->At(i));
	}

	const pcu::AttributeMapPtrVector defaultValues = RhinoPRT::get().getDefaultAttributes(rpk_path, rawInitialShapes);

	if (defaultValues.empty())
		return false;

	for (const auto& shapeDefaultValues : defaultValues) {
		// Group attributes by type: bool -> double/int -> string -> boolArray -> double/int array -> stringArray
		size_t keysCount(0);
		prt::Status status = prt::Status::STATUS_UNSPECIFIED_ERROR;
		const auto keys = shapeDefaultValues->getKeys(&keysCount, &status);
		if (status != prt::Status::STATUS_OK)
			return false;

		// Set starting indices for current shape
		pBoolStarts->Append(pBoolKeys->Count());
		pDoubleStarts->Append(pDoubleKeys->Count());
		pStringStarts->Append(pStringKeys->Count());
		pBoolArrayStarts->Append(pBoolArrayKeys->Count());
		pDoubleArrayStarts->Append(pDoubleArrayKeys->Count());
		pStringArrayStarts->Append(pStringArrayKeys->Count());

		for (size_t keyIdx = 0; keyIdx < keysCount; keyIdx++) {
			const wchar_t* key = keys[keyIdx];
			const prt::AttributeMap::PrimitiveType type = shapeDefaultValues->getType(key, &status);
			if (status != prt::Status::STATUS_OK) {
				pcu::logAttributeTypeError(key);
				return false;
			}

			switch (type) { 
			case prt::AttributeMap::PrimitiveType::PT_BOOL: {
				const bool value = shapeDefaultValues->getBool(key, &status);
				if (status == prt::Status::STATUS_OK) {
					pBoolKeys->Append(key);
					pBoolVals->Append(value);
				}
				break;
			}
			case prt::AttributeMap::PrimitiveType::PT_FLOAT: {
				const double value = shapeDefaultValues->getFloat(key, &status);
				if (status == prt::Status::STATUS_OK) {
					pDoubleKeys->Append(key);
					pDoubleVals->Append(value);
				}
				break;
			}
			case prt::AttributeMap::PrimitiveType::PT_INT: {
				const int value = shapeDefaultValues->getInt(key, &status);
				if (status == prt::Status::STATUS_OK) {
					pDoubleKeys->Append(key);
					pDoubleVals->Append(value);
				}
				break;
			}
			case prt::AttributeMap::PrimitiveType::PT_STRING: {
				const wchar_t* const value = shapeDefaultValues->getString(key, &status);
				if (status == prt::Status::STATUS_OK) {
					pStringKeys->Append(key);
					pStringVals->Append(value);
				}
				break;
			}
			case prt::AttributeMap::PrimitiveType::PT_BOOL_ARRAY: {
				size_t count(0);
				const bool* const value = shapeDefaultValues->getBoolArray(key, &count, &status);
				if (status == prt::Status::STATUS_OK) {
					pBoolArrayKeys->Append(key);
					pBoolArrayVals->Append(pcu::toCeArray(value, count).c_str());
				}
				break;
			}	
			case prt::AttributeMap::PrimitiveType::PT_FLOAT_ARRAY: {
				size_t count(0);
				const double* const value = shapeDefaultValues->getFloatArray(key, &count, &status);
				if (status == prt::Status::STATUS_OK) {
					pDoubleArrayKeys->Append(key);
					pDoubleArrayVals->Append(pcu::toCeArray(value, count).c_str());
				}
				break;
			}
			case prt::AttributeMap::PrimitiveType::PT_INT_ARRAY: {
				size_t count(0);
				const int* const value = shapeDefaultValues->getIntArray(key, &count, &status);
				if (status == prt::Status::STATUS_OK) {
					pDoubleArrayKeys->Append(key);
					pDoubleArrayVals->Append(pcu::toCeArray(value, count).c_str());
				}
				break;
			}
			case prt::AttributeMap::PrimitiveType::PT_STRING_ARRAY: {
				size_t count(0);
				const wchar_t* const* value = shapeDefaultValues->getStringArray(key, &count, &status);
				if (status == prt::Status::STATUS_OK) {
					pStringArrayKeys->Append(key);
					pStringArrayVals->Append(pcu::toCeArray(value, count).c_str());
				}
				break;
			}
			default:
				// Ignore unknown types
				break;
			}

			if (status != prt::Status::STATUS_OK) {
				pcu::logAttributeError(key, status);
				return false;
			}
		}
	}

	return true;
}

RHINOPRT_API void GetCGAPrintOutput(int initialShapeIndex, ON_ClassArray<ON_wString>* pPrintOutput) {
	const std::vector<GeneratedModelPtr> models; // = RhinoPRT::get().getGenModels();

	if ((models.size() <= initialShapeIndex) || !models[initialShapeIndex])
		return;

	const std::vector<std::wstring>& printOutput = models[initialShapeIndex]->getPrintOutput();
	for (const std::wstring& item : printOutput)
		pPrintOutput->Append(ON_wString(item.c_str()));
}

RHINOPRT_API void GetCGAErrorOutput(int initialShapeIndex, ON_ClassArray<ON_wString>* pErrorOutput) {
	const std::vector<GeneratedModelPtr> models; // = RhinoPRT::get().getGenModels();

	if ((models.size() <= initialShapeIndex) || !models[initialShapeIndex])
		return;

	const std::vector<std::wstring>& errorOutput = models[initialShapeIndex]->getErrorOutput();
	for (const std::wstring& item : errorOutput)
		pErrorOutput->Append(ON_wString(item.c_str()));
}

RHINOPRT_API void SetMaterialGenerationOption(bool doGenerate) {
	RhinoPRT::get().setMaterialGeneration(doGenerate);
}
}