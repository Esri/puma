/**
 * Puma - CityEngine Plugin for Rhinoceros
 *
 * See https://esri.github.io/cityengine/puma for documentation.
 *
 * Copyright (c) 2021-2023 Esri R&D Center Zurich
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

using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Windows.Forms;

namespace PumaGrasshopper
{
    public abstract class ComponentPumaShared: GH_Component
    {
        public const string RPK_INPUT_NAME = "Path to Rule Package";
        public const string RPK_INPUT_NICK_NAME = "RPK";
        public const string RPK_INPUT_DESC = "Path to a CityEngine rule package (RPK).";

        public const string GEOM_INPUT_NAME = "Input Shapes";
        public const string GEOM_INPUT_NICK_NAME = "Shapes";
        public const string GEOM_INPUT_DESC = "Input shapes on which to execute the rules.";

        public const string SEED_INPUT_NAME = "Seed";
        public const string SEED_KEY = "seed";
        public const string SEED_INPUT_DESC = "A number that will be used to seed the PRT random number generator.";

        public const string GEOM_OUTPUT_NAME = "Generated Models";
        public const string GEOM_OUTPUT_NICK_NAME = "Models";
        public const string GEOM_OUTPUT_DESC = "Generated model geometry per input shape.";

        public const string MATERIAL_OUTPUT_NAME = "Materials";
        public const string MATERIAL_OUTPUT_NICK_NAME = "Materials";
        public const string MATERIAL_OUTPUT_DESC = "Material attributes per input shape.";

        public const string REPORTS_OUTPUT_NAME = "CGA Reports";
        public const string REPORTS_OUTPUT_NICK_NAME = "Reports";
        public const string REPORTS_OUTPUT_DESC = "CGA report values per input shape.";

        public const string CGA_PRINT_OUTPUT_NAME = "CGA Print Output";
        public const string CGA_PRINT_OUTPUT_NICK_NAME = "Prints";
        public const string CGA_PRINT_OUTPUT_DESC = "CGA print output per input shape.";

        public const string CGA_ERROR_OUTPUT_NAME = "CGA and Asset Errors";
        public const string CGA_ERROR_OUTPUT_NICK_NAME = "Errors";
        public const string CGA_ERROR_OUTPUT_DESC = "CGA and asset errors encountered per input shape.";

        public const string CITYENGINE_RESOURCES_URL = "https://doc.arcgis.com/en/cityengine";

        public enum ParamType
        {
            GEOMETRY,
            FILEPATH,
            INTEGER,
            GENERIC
        }

        public struct ParameterDescriptor
        {
            public ParamType type;
            public string name;
            public string nickName;
            public string desc;
        }

        public enum OutputParams
        {
            MODELS,
            MATERIALS,
            REPORTS,
            PRINTS,
            ERRORS
        }

        public static readonly ParameterDescriptor[] OUTPUT_PARAM_DESC = new ParameterDescriptor[]{
            new ParameterDescriptor{ type = ParamType.GEOMETRY, name = GEOM_OUTPUT_NAME, nickName = GEOM_OUTPUT_NICK_NAME, desc = GEOM_OUTPUT_DESC },
            new ParameterDescriptor{ type = ParamType.GENERIC, name = MATERIAL_OUTPUT_NAME, nickName = MATERIAL_OUTPUT_NICK_NAME, desc = MATERIAL_OUTPUT_DESC },
            new ParameterDescriptor{ type = ParamType.GENERIC, name = REPORTS_OUTPUT_NAME, nickName = REPORTS_OUTPUT_NICK_NAME, desc = REPORTS_OUTPUT_DESC },
            new ParameterDescriptor{ type = ParamType.GENERIC, name = CGA_PRINT_OUTPUT_NAME, nickName = CGA_PRINT_OUTPUT_NICK_NAME, desc = CGA_PRINT_OUTPUT_DESC },
            new ParameterDescriptor{ type = ParamType.GENERIC, name = CGA_ERROR_OUTPUT_NAME, nickName = CGA_ERROR_OUTPUT_NICK_NAME, desc = CGA_ERROR_OUTPUT_DESC },
        };

        protected RulePackage mCurrentRpk;

        /// Stores the optional input parameters
        protected RuleAttribute[] mRuleAttributes;

        protected AttributesValuesMap[] mDefaultValues;

        protected bool mDoGenerateMaterials;

        public ComponentPumaShared(string name, string nickname): base(name, nickname, "Puma runs CityEngine CGA rules on input shapes and returns the generated models. (Version " + PRTWrapper.GetVersion() + ")",
            ComponentLibraryInfo.MainCategory, ComponentLibraryInfo.PumaSubCategory)
        {
            bool status = PRTWrapper.InitializeRhinoPRT();
            if (!status) throw new Exception("Fatal Error: PRT initialization failed.");

            mRuleAttributes = new RuleAttribute[0];

            mDoGenerateMaterials = true;

            mCurrentRpk = null;
        }

        protected RulePackage GetRulePackage(IGH_DataAccess dataAccess)
        {
            var result = new RulePackage();

            string rpkPath = "";
            if (!dataAccess.GetData(RPK_INPUT_NAME, ref rpkPath))
                return result;

            try
            {
                result.path = RulePackageParam.GetAbsoluteRulePackagePath(OnPingDocument(), rpkPath);

                FileInfo fi = new FileInfo(result.path);
                result.timestamp = fi.LastWriteTime;
            }
            catch (Exception e)
            {
                AddRuntimeMessage(GH_RuntimeMessageLevel.Error, "Unable to compute absolute path to RPK: " + e.Message);
            }

            return result;
        }

        protected abstract override void RegisterInputParams(GH_InputParamManager pManager);

        /// <summary>
        /// Registers all the output parameters for this component.
        /// </summary>
        protected override void RegisterOutputParams(GH_OutputParamManager pManager)
        {
            foreach (var param in Enum.GetValues(typeof(OutputParams)).Cast<OutputParams>())
            {
                var desc = OUTPUT_PARAM_DESC[(int)param];
                switch (desc.type)
                {
                    case ParamType.GEOMETRY:
                        pManager.AddGeometryParameter(desc.name, desc.nickName, desc.desc, GH_ParamAccess.tree);
                        break;
                    case ParamType.GENERIC:
                        pManager.AddGenericParameter(desc.name, desc.nickName, desc.desc, GH_ParamAccess.tree);
                        break;
                }
            }
        }

        protected List<Mesh> CreateInputMeshes(IGH_DataAccess dataAccess)
        {
            if (!dataAccess.GetDataTree<IGH_GeometricGoo>(GEOM_INPUT_NAME, out GH_Structure<IGH_GeometricGoo> inputShapes))
                return null;

            var meshes = new List<Mesh>();

            int initShapeIdx = 0;
            foreach (IGH_GeometricGoo geom in inputShapes.AllData(true))
            {
                Mesh mesh = ConvertToMesh(geom);
                if (mesh != null)
                {
                    mesh.SetUserString(PRTWrapper.INIT_SHAPE_IDX_KEY, initShapeIdx.ToString());
                    meshes.Add(mesh);
                }
                initShapeIdx++;
            }

            return meshes;
        }

        private Mesh ConvertToMesh(IGH_GeometricGoo shape)
        {
            Mesh mesh = null;

            if (shape is GH_Brep)
            {
                Brep brepShape = null;
                if (!GH_Convert.ToBrep(shape, ref brepShape, GH_Conversion.Both))
                    return null;

                mesh = new Mesh();
                mesh.Append(Mesh.CreateFromBrep(brepShape, MeshingParameters.FastRenderMesh));
                mesh.Compact();
            }
            else if (shape is GH_Surface)
            {
                Surface surf = null;
                if (!GH_Convert.ToSurface(shape, ref surf, GH_Conversion.Both))
                    return null;
                mesh = Mesh.CreateFromSurface(surf, MeshingParameters.FastRenderMesh);
            }
            else
            {
                if (!GH_Convert.ToMesh(shape, ref mesh, GH_Conversion.Both))
                    return null;
            }

            mesh.Vertices.UseDoublePrecisionVertices = true;

            return mesh;
        }

        protected override void AppendAdditionalComponentMenuItems(ToolStripDropDown menu)
        {
            base.AppendAdditionalComponentMenuItems(menu);

            Menu_AppendItem(menu, "Generate Materials", OnMaterialToggleClicked, true, mDoGenerateMaterials);
            Menu_AppendSeparator(menu);
            Menu_AppendItem(menu, "Go to CityEngine Resources", (object sender, EventArgs e) => { Process.Start(CITYENGINE_RESOURCES_URL); });
        }

        private void OnMaterialToggleClicked(object sender, EventArgs e)
        {
            mDoGenerateMaterials = !mDoGenerateMaterials;
            PRTWrapper.SetMaterialGenerationOption(mDoGenerateMaterials);

            ExpireSolution(true);
        }

        protected void OutputGeometry(IGH_DataAccess dataAccess, List<Mesh[]> generatedMeshes)
        {
            var meshStructure = Utils.CreateMeshStructure(generatedMeshes);
            dataAccess.SetDataTree((int)OutputParams.MODELS, meshStructure);
        }

        protected void OutputMaterials(IGH_DataAccess dataAccess, List<GH_Material[]> generatedMaterials)
        {
            if (!mDoGenerateMaterials)
                return;

            GH_Structure<GH_Material> materials = Utils.CreateMaterialStructure(generatedMaterials);
            dataAccess.SetDataTree((int)OutputParams.MATERIALS, materials);
        }

        protected void OutputReports(IGH_DataAccess dataAccess, List<ReportAttribute[]> generatedReports)
        {
            GH_Structure<ReportAttribute> reports = new GH_Structure<ReportAttribute>();

            for (int shapeId = 0; shapeId < generatedReports.Count; shapeId++)
            {
                if (generatedReports[shapeId].Length > 0)
                {
                    GH_Path path = new GH_Path(shapeId);
                    reports.AppendRange(generatedReports[shapeId], path);
                }
            }

            dataAccess.SetDataTree((int)OutputParams.REPORTS, reports);
        }

        protected void OutputCGAPrint(IGH_DataAccess dataAccess, List<GH_String[]> generatedPrints)
        {
            var outputTree = new GH_Structure<GH_String>();

            for (int shapeId = 0; shapeId < generatedPrints.Count; shapeId++)
            {
                var shapePath = new GH_Path(shapeId);
                outputTree.AppendRange(generatedPrints[shapeId], shapePath);
            }

            dataAccess.SetDataTree((int)OutputParams.PRINTS, outputTree);
        }

        protected void OutputCGAErrors(IGH_DataAccess dataAccess, List<GH_String[]> generatedErrors)
        {
            var outputTree = new GH_Structure<GH_String>();

            for (int shapeId = 0; shapeId < generatedErrors.Count; shapeId++)
            {
                var shapePath = new GH_Path(shapeId);
                outputTree.AppendRange(generatedErrors[shapeId], shapePath);
            }

            dataAccess.SetDataTree((int)OutputParams.ERRORS, outputTree);
        }
    }
}
