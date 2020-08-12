#include "RhinoEncoder.h"

#include "Logger.h"

#include "prtx/EncoderInfoBuilder.h"
#include "prtx/GenerateContext.h"
#include "prtx/Exception.h"
#include "prtx/Geometry.h"
#include "prtx/Mesh.h"
#include "prtx/ReportsCollector.h"
#include "prtx/Shape.h"
#include "prtx/ShapeIterator.h"
#include "prtx/prtx.h"

#include <iostream>
#include <fstream>
#include <numeric>
#include <algorithm>

#define DEBUG

namespace {

	const wchar_t* EO_BASE_NAME = L"baseName";
	const wchar_t* EO_ERROR_FALLBACK = L"errorFallback";
	const std::wstring ENCFILE_EXT = L".txt";
	const wchar_t* EO_EMIT_REPORTS = L"emitReport";
	const wchar_t* EO_EMIT_GEOMETRY = L"emitGeometry";

	const prtx::EncodePreparator::PreparationFlags ENC_PREP_FLAGS =
		prtx::EncodePreparator::PreparationFlags()
		.instancing(false)
		.triangulate(true)
		.mergeVertices(false)
		.cleanupUVs(false)
		.cleanupVertexNormals(false)
		.mergeByMaterial(true);

	const prtx::PRTUtils::AttributeMapPtr convertReportToAttributeMap(const prtx::ReportsPtr& r) {

		prtx::PRTUtils::AttributeMapBuilderPtr amb(prt::AttributeMapBuilder::create());

		for (const auto& b : r->mBools)
			amb->setBool(b.first->c_str(), b.second);
		for (const auto& f : r->mFloats)
			amb->setFloat(f.first->c_str(), f.second);
		for (const auto& s : r->mStrings)
			amb->setString(s.first->c_str(), s.second->c_str());

		return prtx::PRTUtils::AttributeMapPtr{ amb->createAttributeMap() };
	}
}

const std::wstring RhinoEncoder::ID = L"com.esri.rhinoprt.RhinoEncoder";
const std::wstring RhinoEncoder::NAME = L"Rhino Geometry and Report Encoder";
const std::wstring RhinoEncoder::DESCRIPTION = L"Encodes geometry and CGA report for Rhino.";

void RhinoEncoder::init(prtx::GenerateContext& context) {
	prtx::NamePreparator::NamespacePtr nsMaterials = mNamePreparator.newNamespace();
	prtx::NamePreparator::NamespacePtr nsMeshes = mNamePreparator.newNamespace();
	mEncodePreparator = prtx::EncodePreparator::create(true, mNamePreparator, nsMeshes, nsMaterials);
}

void RhinoEncoder::encode(prtx::GenerateContext& context, size_t initialShapeIndex) {

	const prtx::InitialShape& initialShape = *context.getInitialShape(initialShapeIndex);

	auto* cb = dynamic_cast<IRhinoCallbacks*>(getCallbacks());
	if (cb == nullptr)
		throw prtx::StatusException(prt::STATUS_ILLEGAL_CALLBACK_OBJECT);

	// Initialization of report accumulator and strategy
	prtx::ReportsAccumulatorPtr reportsAccumulator{ prtx::SummarizingReportsAccumulator::create() };
	prtx::ReportingStrategyPtr reportsCollector{ prtx::AllShapesReportingStrategy::create(context, initialShapeIndex, reportsAccumulator) };

#ifdef DEBUG
	LOG_DBG << L"Starting leaf iteration";
#endif

	try {
		prtx::LeafIteratorPtr li = prtx::LeafIterator::create(context, initialShapeIndex);

		for (prtx::ShapePtr shape = li->getNext(); shape.get() != nullptr; shape = li->getNext()) {
			mEncodePreparator->add(context.getCache(), shape, initialShape.getAttributeMap());
		}
	}
	catch (std::exception& e) {
		LOG_ERR << e.what();
			
		mEncodePreparator->add(context.getCache(), initialShape, initialShapeIndex);
	}
	catch (...) {
		LOG_ERR << "Unknown exception while encoding geometry." << std::endl;

		mEncodePreparator->add(context.getCache(), initialShape, initialShapeIndex);
	}

	prtx::EncodePreparator::InstanceVector finalizedInstances;
	mEncodePreparator->fetchFinalizedInstances(finalizedInstances, ENC_PREP_FLAGS);
	convertGeometry(initialShape, finalizedInstances, cb);

	if (getOptions()->getBool(EO_EMIT_REPORTS)) {
		auto reports = reportsCollector->getReports();

		if (reports) {
			auto reportMap = convertReportToAttributeMap(reports);
			// a single report map by initial shape.
			cb->addReport(initialShapeIndex, reportMap);
		}
	}
}

void RhinoEncoder::convertGeometry(const prtx::InitialShape& initialShape,
								   const prtx::EncodePreparator::InstanceVector& instances,
								   IRhinoCallbacks* cb) 
{
	std::vector<uint32_t> shapeIDs;

	uint32_t vertexIndexBase = 0;
	uint32_t maxNumUVSets = 0; // for now we support only 1 uv set.
	std::vector<uint32_t> uvIndexBases(maxNumUVSets, 0u);

	std::vector<double> vertexCoords;
	std::vector<uint32_t> faceIndices;
	std::vector<uint32_t> faceCounts;

	std::vector<prtx::DoubleVector> uvs;
	std::vector<prtx::IndexVector> uvCounts;
	std::vector<prtx::IndexVector> uvIndices;

	prtx::GeometryPtrVector geometries;
	std::vector<prtx::MaterialPtrVector> materials;

	prtx::PRTUtils::AttributeMapBuilderPtr amb(prt::AttributeMapBuilder::create());

	for (const auto& instance : instances) {

		const prtx::MeshPtrVector& meshes = instance.getGeometry()->getMeshes();

		vertexCoords.clear();
		faceIndices.clear();
		faceCounts.clear();

		// 1st pass: scan the geometries to preallocate the sizes of vectors
		uint32_t numCoords = 0;
		uint32_t numFaceCounts = 0;
		uint32_t numIndices = 0;

		for (const auto& mesh : meshes)
		{
			numCoords += mesh->getVertexCoords().size();
			numFaceCounts += mesh->getFaceCount();

			const auto& vtxCnts = mesh->getFaceVertexCounts();
			numIndices = std::accumulate(vtxCnts.begin(), vtxCnts.end(), numIndices);
		}

		vertexCoords.reserve(3 * numCoords);
		faceCounts.reserve(numFaceCounts);
		faceIndices.reserve(numIndices);

		// 2nd pass: fill the vectors
		for (const auto& mesh : meshes) {
			const prtx::DoubleVector& verts = mesh->getVertexCoords();
			vertexCoords.insert(vertexCoords.end(), verts.begin(), verts.end());

			for (uint32_t fi = 0; fi < mesh->getFaceCount(); ++fi) {
				const uint32_t* vtxIdx = mesh->getFaceVertexIndices(fi);
				const uint32_t vtxCnt = mesh->getFaceVertexCount(fi);
				faceCounts.push_back(vtxCnt);

				for (uint32_t vi = 0; vi < vtxCnt; vi++)
				{
					faceIndices.push_back(vtxIdx[vi] + vertexIndexBase);
				}
			}
			vertexIndexBase += (uint32_t)verts.size() / 3;

			if (emitMaterials) {
				const prtx::MaterialPtr& mat = material.at(mi);
				const uint32_t requiredUVSetsByMaterial = scanValidTextures(mat);

				//for now, only 1 uv set is supported
				if (mesh->getUVSetsCount() > 0 && (bool)requiredUVSetsByMaterial) 
				{ 
					maxNumUVSets = 1; 
					uvIndexBases.resize(maxNumUVSets, 0u);
				}

				// copy first uv set data. 
				// TODO: For now only one is supported
				const uint32_t numUVSets = mesh->getUVSetsCount();
				const prtx::DoubleVector& uvs0 = (numUVSets > 0) ? mesh->getUVCoords(0) : EMPTY_UVS;
				const prtx::IndexVector faceUVCounts0 = (numUVSets > 0) ? mesh->getFaceUVCounts(0) : prtx::IndexVector(mesh->getFaceCount(), 0);
				
#ifdef DEBUG 
				LOG_DBG << "-- mesh: numUVSets = " << numUVSets;
#endif

				// FOR NOW THIS WILL ONLY BE EXECUTED FOR THE COLORMAP
				for (auto uvSet = 0; uvSet < uvs.size(); uvSet++) {
					// append texture coordinates
					const prtx::DoubleVector& currUVs = (uvSet < numUVSets) ? mesh->getUVCoords(uvSet) : EMPTY_UVS;
					const auto& src = currUVs.empty() ? uvs0 : currUVs;

					//insert the curent uvs into the corresponding position in the vector of uvs
					auto& tgt = uvs[uvSet];
					tgt.insert(tgt.end(), src.begin(), src.end());

					// append uv face counts
					const prtx::IndexVector& faceUVCounts = (uvSet < numUVSets && !currUVs.empty()) ? mesh->getFaceUVCounts(uvSet) : faceUVCounts0;
					assert(faceUVCounts.size() == mesh->getFaceCount());
					auto& tgtCounts = uvCounts[uvSet];
					tgtCounts.insert(tgtCounts.end(), faceUVCounts.begin(), faceUVCounts.end());

#ifdef DEBUG
					LOG_DBG << "  -- uvset " << uvSet << ": face counts size = " << faceUVCounts.size();
#endif // DEBUG
					
					// append uv vertex indices
					for (uint32_t faceId = 0, faceCount = faceUVCounts.size(); faceId < faceCount; ++faceId) {
						const uint32_t* faceUVIdx0 = (numUVSets > 0) ? mesh->getFaceUVIndices(faceId, 0) : EMPTY_IDX.data();
						const uint32_t* faceUVIdx = (uvSet < numUVSets && !currUVs.empty()) ? mesh->getFaceUVIndices(faceId, uvSet) : faceUVIdx0;
						const uint32_t faceUVCnt = faceUVCounts[faceId];

#ifdef DEBUG
						LOG_DBG << "      faceId " << faceId << ": faceUVCnt = " << faceUVCnt << ", faceVtxCnt = " << mesh->getFaceVertexCount(faceId);
#endif // DEBUG

						for (uint32_t vrtxId = 0; vrtxId < faceUVCnt; ++vrtxId) {
							uvIndices[uvSet].push_back(uvIndexBases[uvSet] + faceUVIdx[vrtxId]);
						}

					}

					uvIndexBases[uvSet] += src.size() / 2u;
				}

				convertMaterialToAttributeMap(amb, *(mat.get()), mat->getKeys());
				matAttrMap.push_back(amb->createAttributeMapAndReset());
			}
		}

		cb->addGeometry(instance.getInitialShapeIndex(), vertexCoords.data(), vertexCoords.size(),
			faceIndices.data(), faceIndices.size(), faceCounts.data(), faceCounts.size());
	}
}

void RhinoEncoder::finish(prtx::GenerateContext& context) {
	LOG_DBG << "In finish  function...";

}


RhinoEncoderFactory* RhinoEncoderFactory::createInstance() {
	prtx::EncoderInfoBuilder encoderInfoBuilder;

	encoderInfoBuilder.setID(RhinoEncoder::ID);
	encoderInfoBuilder.setName(RhinoEncoder::NAME);
	encoderInfoBuilder.setDescription(RhinoEncoder::DESCRIPTION);
	encoderInfoBuilder.setType(prt::CT_GEOMETRY);
	encoderInfoBuilder.setExtension(ENCFILE_EXT);

	// Default encoder options
	prtx::PRTUtils::AttributeMapBuilderPtr amb(prt::AttributeMapBuilder::create());
	amb->setString(EO_BASE_NAME, L"enc_default_name");
	amb->setBool(EO_ERROR_FALLBACK, prtx::PRTX_TRUE);
	amb->setBool(EO_EMIT_GEOMETRY, prtx::PRTX_TRUE);
	amb->setBool(EO_EMIT_REPORTS, prtx::PRTX_TRUE);
	encoderInfoBuilder.setDefaultOptions(amb->createAttributeMap());

	return new RhinoEncoderFactory(encoderInfoBuilder.create());
}