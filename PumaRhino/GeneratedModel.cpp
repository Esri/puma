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

#include "GeneratedModel.h"
#include "InitialShape.h"

ModelPart& GeneratedModel::addModelPart() {
	mModelParts.push_back(ModelPart());
	return mModelParts.back();
}

const std::vector<ModelPart>& GeneratedModel::getModelParts() const {
	return mModelParts;
}

int GeneratedModel::getMeshPartCount() const {
	return static_cast<int>(getModelParts().size());
}

ModelPart& GeneratedModel::getCurrentModelPart() {
	return mModelParts.back();
}

void GeneratedModel::addReport(const Reporting::ReportAttribute& ra) {
	mReports.emplace(ra.mReportName, ra);
}

const Reporting::ReportMap& GeneratedModel::getReports() const {
	return mReports;
}

const Materials::MaterialsMap& GeneratedModel::getMaterials() const {
	return mMaterials;
}

void GeneratedModel::addPrintOutput(const std::wstring_view& message) {
	assert(message.length() > 0); // we expect at least the newline character added by PRT
	mPrintOutput.emplace_back(message.substr(0, message.length() - 1)); // let's trim the newline away
}

const std::vector<std::wstring>& GeneratedModel::getPrintOutput() const {
	return mPrintOutput;
}

void GeneratedModel::addErrorOutput(const std::wstring_view& error) {
	mErrorOutput.emplace_back(error);
}

const std::vector<std::wstring>& GeneratedModel::getErrorOutput() const {
	return mErrorOutput;
}

void GeneratedModel::addMaterial(const Materials::MaterialAttribute& ma) {
	mMaterials.insert_or_assign(ma.mMatId, ma);
}

const GeneratedModel::MeshBundle GeneratedModel::getMeshesFromGenModel(size_t initialShapeIndex) const {
	const std::wstring idKey = std::to_wstring(initialShapeIndex);

	MeshBundle mesh;
	mesh.reserve(mModelParts.size());
	std::transform(mModelParts.begin(), mModelParts.end(), std::back_inserter(mesh),
	               [this, &idKey](const ModelPart& part) -> ON_Mesh { return toON_Mesh(part, idKey); });
	return mesh;
}

ON_Mesh GeneratedModel::toON_Mesh(const ModelPart& modelPart, const std::wstring& idKey) const {
	ON_Mesh mesh(static_cast<int>(modelPart.mFaces.size()), static_cast<int>(modelPart.mIndices.size()), true, true);

	// Set the initial shape id.
	mesh.SetUserString(INIT_SHAPE_ID_KEY.c_str(), idKey.c_str());

	// Duplicate vertices
	for (size_t v_id = 0; v_id < modelPart.mIndices.size(); ++v_id) {
		auto index = modelPart.mIndices[v_id];
		mesh.SetVertex(static_cast<int>(v_id),
		               ON_3dPoint(modelPart.mVertices[index * 3], -modelPart.mVertices[index * 3 + 2],
		                          modelPart.mVertices[index * 3 + 1]));
		mesh.SetVertexNormal(static_cast<int>(v_id),
		                     ON_3dVector(modelPart.mNormals[index * 3], -modelPart.mNormals[index * 3 + 2],
		                                 modelPart.mNormals[index * 3 + 1]));
	}

	int faceid(0);
	int currindex(0);
	for (int face : modelPart.mFaces) {
		if (face == 3) {
			mesh.SetTriangle(faceid, currindex, currindex + 1, currindex + 2);
			currindex += face;
			faceid++;
		}
		else if (face == 4) {
			mesh.SetQuad(faceid, currindex, currindex + 1, currindex + 2, currindex + 3);
			currindex += face;
			faceid++;
		}
		else {
			// ignore face because it is invalid
			currindex += face;
			LOG_WRN << "Ignored face with invalid number of vertices :" << face;
		}
	}

	for (int i = 0; i < modelPart.mUVs.Count(); ++i) {
		mesh.SetTextureCoord(i, modelPart.mUVs[i].x, modelPart.mUVs[i].y);
	}

	mesh.Compact();

	// Printing a rhino error log if the created mesh is invalid
	ON_wString log_str;
	ON_TextLog log(log_str);
	if (!mesh.IsValid(&log)) {
		mesh.Dump(log);
		LOG_ERR << log_str;
	}

	return mesh;
}