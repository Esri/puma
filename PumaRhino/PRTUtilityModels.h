#pragma once

#include "utils.h"
#include "MaterialAttribute.h"
#include "ReportAttribute.h"
#include "RhinoCallbacks.h"

#include <vector>

const std::wstring INIT_SHAPE_ID_KEY = L"InitShapeIdx";

using MeshBundle = std::vector<ON_Mesh>;

/**
* The Initial shape that will be given to PRT
*/
class InitialShape {
public:
	InitialShape() = default;
	InitialShape(const ON_Mesh& mesh, const int seed);
	~InitialShape() {}

	const int getID() const {
		return mID;
	}

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

	const int getSeed() const {
		return mSeed;
	}

protected:

	int mID;
	int mSeed;
	std::vector<double> mVertices;
	std::vector<uint32_t> mIndices;
	std::vector<uint32_t> mFaceCounts;
};

/**
* The model generated by PRT
*/
class GeneratedModel final {
public:

	GeneratedModel(const size_t& initialShapeIdx, const Model& model);

	GeneratedModel() = default;
	~GeneratedModel() = default;

	const MeshBundle getMeshesFromGenModel() const;

	int getMeshPartCount() const {
		return static_cast<int>(mModel.getModelParts().size());
	}

	size_t getInitialShapeIndex() const {
		return mInitialShapeIndex;
	}
	
	const Reporting::ReportMap& getReport() const {
		return mModel.getReports();
	}

	const Materials::MaterialsMap& getMaterials() const {
		return mModel.getMaterials();
	}

private:
	size_t mInitialShapeIndex = 0;

	Model mModel;

	/// Creates an ON_Mesh and setup its uv coordinates, for a given ModelPart.
	const ON_Mesh toON_Mesh(const ModelPart& modelPart) const;
};