#ifndef RHINOPRT
#define RHINOPRT

#include "PRTContext.h"

#include "ModelGenerator.h"

#include "Logger.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <string>
#include <map>

#pragma comment(lib, "ole32.lib") // Workaround for "combaseapi.h(229): error C2187: syntax error: 'identifier' was unexpected here" when using /permissive-

#define RHINOPRT_API __declspec(dllexport)

namespace RhinoPRT {

	class RhinoPRTAPI {
	public:
		const RuleAttribute RULE_NOT_FOUND{};

		bool InitializeRhinoPRT();
		void ShutdownRhinoPRT();
		bool IsPRTInitialized();

		void SetRPKPath(const std::wstring &rpk_path);

		int GetRuleAttributeCount();
		RuleAttributes GetRuleAttributes();

		void AddInitialShape(const std::vector<InitialShape>& shapes);
		void ClearInitialShapes();

		std::vector<GeneratedModel> GenerateGeometry();

		template<typename T>
		void fillAttributeFromNode(const std::wstring& ruleName, const std::wstring& attrFullName, T value);

		template<typename T>
		void setRuleAttributeValue(const RuleAttribute& rule, T value);

		int groupReportsByKeys();

		const Reporting::GroupedReports& getReports() const { return mGroupedReports; }
		Reporting::ReportsVector getReportsOfModel(int initialShapeID);

	private:

		std::vector<InitialShape> mShapes;
		std::wstring mPackagePath;
		std::vector<pcu::ShapeAttributes> mAttributes;

		RuleAttributes mRuleAttributes;
		pcu::AttributeMapBuilderPtr mAttrBuilder;

		pcu::EncoderOptions options;

		std::unique_ptr<ModelGenerator> mModelGenerator;
		std::vector<GeneratedModel> mGeneratedModels;

		Reporting::GroupedReports mGroupedReports;
	};

	// Global PRT handle
	RhinoPRTAPI& get();

} // namespace RhinoPRT

// Define exposed functions here
extern "C" {

	RHINOPRT_API bool InitializeRhinoPRT();

	RHINOPRT_API void ShutdownRhinoPRT();

	RHINOPRT_API void SetPackage(const wchar_t* rpk_path);

	RHINOPRT_API bool AddMeshTest(ON_SimpleArray<const ON_Mesh*>* pMesh);

	RHINOPRT_API void ClearInitialShapes();

	RHINOPRT_API bool GenerateTest(ON_SimpleArray<ON_Mesh*>* pMeshArray);

	RHINOPRT_API int GetRuleAttributesCount();

	RHINOPRT_API bool GetRuleAttribute(int attrIdx, wchar_t* rule, int rule_size, wchar_t* name, int name_size, wchar_t* nickname, int nickname_size, prt::AnnotationArgumentType* type);

	RHINOPRT_API void SetRuleAttributeDouble(const wchar_t* rule, const wchar_t* fullName, double value);

	RHINOPRT_API void SetRuleAttributeBoolean(const wchar_t* rule, const wchar_t* fullName, bool value);

	RHINOPRT_API void SetRuleAttributeInteger(const wchar_t* rule, const wchar_t* fullName, int value);

	RHINOPRT_API void SetRuleAttributeString(const wchar_t* rule, const wchar_t* fullName, const wchar_t* value);

	RHINOPRT_API int GroupeReportsByKeys();

	RHINOPRT_API bool GetReportKeys(ON_ClassArray<ON_wString>* pKeysArray, ON_SimpleArray<int>* pKeyTypeArray);

	RHINOPRT_API void GetDoubleReports(const wchar_t* key, ON_SimpleArray<double>* pReportsArr);

	RHINOPRT_API void GetStringReports(const wchar_t* key, ON_ClassArray<ON_wString>* pReportsArr);

	RHINOPRT_API void GetBoolReports(const wchar_t* key, ON_SimpleArray<int>* pReportsArr);

	RHINOPRT_API void GetReports(int initialShapeId, ON_ClassArray<ON_wString>* pKeysArray,
		ON_SimpleArray<double>* pDoubleReports,
		ON_SimpleArray<bool>* pBoolReports,
		ON_ClassArray<ON_wString>* pStringReports);

}

#endif RHINOPRT