/*
  COPYRIGHT (c) 2012-2020 Esri R&D Center Zurich
  TRADE SECRETS: ESRI PROPRIETARY AND CONFIDENTIAL
  Unpublished material - all rights reserved under the
  Copyright Laws of the United States and applicable international
  laws, treaties, and conventions.

  For additional information, contact:
  Environmental Systems Research Institute, Inc.
  Attn: Contracts and Legal Services Department
  380 New York Street
  Redlands, California, 92373
  USA

  email: contracts@esri.com
*/

// !! DO NOT MODIFY !! - this file is automatically generated by CGAC
// Sat May 09 12:07:07 CEST 2020

#ifndef BUILTINSHAPEATTRIBUTES_H_
#define BUILTINSHAPEATTRIBUTES_H_

#include "prtx/prtx.h"
#include "prtx/BuiltinAttributes.h"

#include <string>

#ifdef _MSC_VER
#	pragma warning(push)
#	pragma warning (disable : 4251 4231 4660)
#endif


namespace prtx {

/**
* Interface for direct getters of the @cgaref{cgaindex.html#shapeattributes,CGA Shape Attributes}.
* @sa Shape
*/
class PRTX_EXPORTS_API BuiltinShapeAttributes : public BuiltinAttributesGetters {
public:

	virtual bool trim_horizontal() const = 0;
	virtual bool trim_vertical() const = 0;
	virtual double comp_index() const = 0;
	virtual double comp_total() const = 0;
	virtual double initialShape_origin_ox() const = 0;
	virtual double initialShape_origin_oy() const = 0;
	virtual double initialShape_origin_oz() const = 0;
	virtual double initialShape_origin_px() const = 0;
	virtual double initialShape_origin_py() const = 0;
	virtual double initialShape_origin_pz() const = 0;
	virtual double material_ambient_b() const = 0;
	virtual double material_ambient_g() const = 0;
	virtual double material_ambient_r() const = 0;
	virtual double material_bumpValue() const = 0;
	virtual double material_bumpmap_rw() const = 0;
	virtual double material_bumpmap_su() const = 0;
	virtual double material_bumpmap_sv() const = 0;
	virtual double material_bumpmap_tu() const = 0;
	virtual double material_bumpmap_tv() const = 0;
	virtual double material_color_b() const = 0;
	virtual double material_color_g() const = 0;
	virtual double material_color_r() const = 0;
	virtual double material_colormap_rw() const = 0;
	virtual double material_colormap_su() const = 0;
	virtual double material_colormap_sv() const = 0;
	virtual double material_colormap_tu() const = 0;
	virtual double material_colormap_tv() const = 0;
	virtual double material_dirtmap_rw() const = 0;
	virtual double material_dirtmap_su() const = 0;
	virtual double material_dirtmap_sv() const = 0;
	virtual double material_dirtmap_tu() const = 0;
	virtual double material_dirtmap_tv() const = 0;
	virtual double material_emissive_b() const = 0;
	virtual double material_emissive_g() const = 0;
	virtual double material_emissive_r() const = 0;
	virtual double material_emissivemap_rw() const = 0;
	virtual double material_emissivemap_su() const = 0;
	virtual double material_emissivemap_sv() const = 0;
	virtual double material_emissivemap_tu() const = 0;
	virtual double material_emissivemap_tv() const = 0;
	virtual double material_metallic() const = 0;
	virtual double material_metallicmap_rw() const = 0;
	virtual double material_metallicmap_su() const = 0;
	virtual double material_metallicmap_sv() const = 0;
	virtual double material_metallicmap_tu() const = 0;
	virtual double material_metallicmap_tv() const = 0;
	virtual double material_normalmap_rw() const = 0;
	virtual double material_normalmap_su() const = 0;
	virtual double material_normalmap_sv() const = 0;
	virtual double material_normalmap_tu() const = 0;
	virtual double material_normalmap_tv() const = 0;
	virtual double material_occlusionmap_rw() const = 0;
	virtual double material_occlusionmap_su() const = 0;
	virtual double material_occlusionmap_sv() const = 0;
	virtual double material_occlusionmap_tu() const = 0;
	virtual double material_occlusionmap_tv() const = 0;
	virtual double material_opacity() const = 0;
	virtual double material_opacitymap_cutoff() const = 0;
	virtual double material_opacitymap_rw() const = 0;
	virtual double material_opacitymap_su() const = 0;
	virtual double material_opacitymap_sv() const = 0;
	virtual double material_opacitymap_tu() const = 0;
	virtual double material_opacitymap_tv() const = 0;
	virtual double material_reflectivity() const = 0;
	virtual double material_roughness() const = 0;
	virtual double material_roughnessmap_rw() const = 0;
	virtual double material_roughnessmap_su() const = 0;
	virtual double material_roughnessmap_sv() const = 0;
	virtual double material_roughnessmap_tu() const = 0;
	virtual double material_roughnessmap_tv() const = 0;
	virtual double material_shininess() const = 0;
	virtual double material_specular_b() const = 0;
	virtual double material_specular_g() const = 0;
	virtual double material_specular_r() const = 0;
	virtual double material_specularmap_rw() const = 0;
	virtual double material_specularmap_su() const = 0;
	virtual double material_specularmap_sv() const = 0;
	virtual double material_specularmap_tu() const = 0;
	virtual double material_specularmap_tv() const = 0;
	virtual double pivot_ox() const = 0;
	virtual double pivot_oy() const = 0;
	virtual double pivot_oz() const = 0;
	virtual double pivot_px() const = 0;
	virtual double pivot_py() const = 0;
	virtual double pivot_pz() const = 0;
	virtual double scope_rx() const = 0;
	virtual double scope_ry() const = 0;
	virtual double scope_rz() const = 0;
	virtual double scope_sx() const = 0;
	virtual double scope_sy() const = 0;
	virtual double scope_sz() const = 0;
	virtual double scope_tx() const = 0;
	virtual double scope_ty() const = 0;
	virtual double scope_tz() const = 0;
	virtual double seedian() const = 0;
	virtual double split_index() const = 0;
	virtual double split_total() const = 0;
	virtual const std::wstring& comp_sel() const = 0;
	virtual const std::wstring& initialShape_name() const = 0;
	virtual const std::wstring& initialShape_startRule() const = 0;
	virtual const std::wstring& material_bumpmap() const = 0;
	virtual const std::wstring& material_color_rgb() const = 0;
	virtual const std::wstring& material_colormap() const = 0;
	virtual const std::wstring& material_dirtmap() const = 0;
	virtual const std::wstring& material_emissivemap() const = 0;
	virtual const std::wstring& material_metallicmap() const = 0;
	virtual const std::wstring& material_name() const = 0;
	virtual const std::wstring& material_normalmap() const = 0;
	virtual const std::wstring& material_occlusionmap() const = 0;
	virtual const std::wstring& material_opacitymap() const = 0;
	virtual const std::wstring& material_opacitymap_mode() const = 0;
	virtual const std::wstring& material_roughnessmap() const = 0;
	virtual const std::wstring& material_shader() const = 0;
	virtual const std::wstring& material_specularmap() const = 0;

protected:
	virtual void fillBuiltinKeys(WStringVector& keys) const override;
	virtual prt::Attributable::PrimitiveType getBuiltinType(const std::wstring& k) const override;

	virtual bool getBuiltinBool(const std::wstring& k, bool& stat) const override;
	virtual double getBuiltinFloat(const std::wstring& k, bool& stat) const override;
	virtual const std::wstring& getBuiltinString(const std::wstring& k, bool& val) const override;

	BuiltinShapeAttributes() {};
	virtual ~BuiltinShapeAttributes() {};
};

}	// namespace
#endif 
