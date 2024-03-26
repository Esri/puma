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

using System;
using System.Collections.Generic;

using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Grasshopper.Kernel.Data;

using Rhino.Geometry;
using System.Linq;
using PumaGrasshopper.Properties;
using System.Windows.Forms;
using System.Drawing;
using System.Diagnostics;
using Rhino.Runtime.InteropWrappers;
using System.IO;
using GH_IO.Serialization;


namespace PumaGrasshopper
{

    public class ComponentPuma : ComponentPumaShared, IGH_VariableParameterComponent
    {
        public const string COMPONENT_NAME = "Puma";
        public const string COMPONENT_NICK_NAME = "CityEngine Puma";

        enum InputParams
        {
            RPK,
            SHAPES,
            SEEDS
        }

        static readonly ParameterDescriptor[] INPUT_PARAM_DESC = new ParameterDescriptor[]{
            new ParameterDescriptor{ type = ParamType.FILEPATH, name = RPK_INPUT_NAME, nickName = RPK_INPUT_NICK_NAME, desc = RPK_INPUT_DESC },
            new ParameterDescriptor{ type = ParamType.GEOMETRY, name = GEOM_INPUT_NAME, nickName = GEOM_INPUT_NICK_NAME, desc = GEOM_INPUT_DESC },
            new ParameterDescriptor{ type = ParamType.INTEGER, name = SEED_KEY, nickName = SEED_INPUT_NAME, desc = SEED_INPUT_DESC },
        };

        public ComponentPuma()
          : base(COMPONENT_NAME, COMPONENT_NICK_NAME) { }

        protected override void RegisterInputParams(GH_Component.GH_InputParamManager pManager)
        {
            foreach (var param in Enum.GetValues(typeof(InputParams)).Cast<InputParams>())
            {
                var desc = INPUT_PARAM_DESC[(int)param];
                switch (desc.type)
                {
                    case ParamType.GEOMETRY:
                        pManager.AddGeometryParameter(desc.name, desc.nickName, desc.desc, GH_ParamAccess.tree);
                        break;
                    case ParamType.FILEPATH:
                        pManager.AddParameter(new RulePackageParam(), desc.name, desc.nickName, desc.desc, GH_ParamAccess.item);
                        break;
                    case ParamType.INTEGER:
                        pManager.AddIntegerParameter(desc.name, desc.nickName, desc.desc, GH_ParamAccess.tree, 0);
                        break;
                }
            }
        }

        protected override void SolveInstance(IGH_DataAccess DA)
        {
            ClearRuntimeMessages();

            RulePackage rpk = GetRulePackage(DA);
            if (!rpk.IsValid())
                return;

            List<Mesh> inputMeshes = CreateInputMeshes(DA);
            if (inputMeshes == null || inputMeshes.Count == 0)
                return;

            if(mCurrentRpk == null || !mCurrentRpk.IsSame(rpk))
            {
                mCurrentRpk = rpk;
                mRuleAttributes = PRTWrapper.GetRuleAttributes(rpk.path);
                mDefaultValues = PRTWrapper.GetDefaultValues(rpk.path, inputMeshes);
            }

            RuleAttributesMap MM = FillAttributesFromNode(DA, inputMeshes.Count);

            var generatedMeshes = PRTWrapper.Generate(rpk.path, ref MM, inputMeshes);
            OutputGeometry(DA, generatedMeshes.meshes);
            OutputMaterials(DA, generatedMeshes.materials);
            OutputReports(DA, generatedMeshes.reports);
            OutputCGAPrint(DA, generatedMeshes.prints);
            OutputCGAErrors(DA, generatedMeshes.errors);
        }

        private RuleAttributesMap FillAttributesFromNode(IGH_DataAccess DA, int shapeCount)
        {
            RuleAttributesMap MM = new RuleAttributesMap();

            List<RuleAttribute> attributeList = mRuleAttributes.ToList();

            for(int shapeId = 0; shapeId < shapeCount; ++shapeId)
            {
                MM.StartNewSection();

                for (int idx = (int)InputParams.SEEDS; idx < Params.Input.Count; ++idx)
                {
                    var param = Params.Input[idx];

                    RuleAttribute attribute = attributeList.Find(x => x.mFullName == param.Name);

                    bool expectArray = attribute != null && attribute.IsArray();

                    SetAttributeOfShapes(DA, shapeCount, shapeId, param, expectArray, ref MM);
                }
            }

            return MM;
        }

        private void SetAttributeOfShapes(IGH_DataAccess DA, int shapeCount, int shapeId, IGH_Param attributeParam, bool expectArray, ref RuleAttributesMap MM)
        {
            if (attributeParam.Type == typeof(GH_Number))
            {
                if (DA.GetDataTree(attributeParam.Name, out GH_Structure<GH_Number> tree))
                    ExtractTreeValues(tree, attributeParam, shapeCount, shapeId, expectArray, ref MM);
            }
            else if (attributeParam.Type == typeof(GH_Integer))
            {
                if (DA.GetDataTree(attributeParam.Name, out GH_Structure<GH_Integer> tree))
                    ExtractTreeValues(tree, attributeParam, shapeCount, shapeId, expectArray, ref MM);
            }
            else if (attributeParam.Type == typeof(GH_Boolean))
            {
                if (DA.GetDataTree(attributeParam.Name, out GH_Structure<GH_Boolean> tree))
                    ExtractTreeValues(tree, attributeParam, shapeCount, shapeId, expectArray, ref MM);
            }
            else if (attributeParam.Type == typeof(GH_String))
            {
                if (DA.GetDataTree(attributeParam.Name, out GH_Structure<GH_String> tree))
                    ExtractTreeValues(tree, attributeParam, shapeCount, shapeId, expectArray, ref MM);
            }
            else if (attributeParam.Type == typeof(GH_Colour))
            {
                if (DA.GetDataTree(attributeParam.Name, out GH_Structure<GH_Colour> tree))
                    ExtractTreeValues(tree, attributeParam, shapeCount, shapeId, expectArray, ref MM);
            }
        }

        private void ExtractTreeValues<T>(GH_Structure<T> tree, IGH_Param attributeParam ,int shapeCount, int shapeId, bool expectArray, ref RuleAttributesMap MM) where T : IGH_Goo
        {
            if (tree.IsEmpty || shapeId >= shapeCount)
                return;

            if (expectArray)
            {
                List<T> branch;

                // Grasshopper behaviour: repeat last item to compensate mismatched list lengths
                if (shapeId > tree.Branches.Count) branch = tree.Branches.Last();
                else branch = tree.Branches[shapeId];
              
                SetRuleAttributeArray(attributeParam, branch, ref MM);
            }
            else
            {
                List<T> values = tree.ToList();
                T value;

                // Grasshopper behaviour: repeat last item to compensate mismatched list lengths
                if (shapeId >= values.Count) value = values.Last();
                else value = values[shapeId];
                
                if (value.IsValid)
                {
                    SetRuleAttribute(attributeParam, value, ref MM);
                }
            }
        }

        private void SetRuleAttributeArray<T>(IGH_Param attributeParam, List<T> values, ref RuleAttributesMap MM) where T : IGH_Goo
        {
            var name = attributeParam.Name;

            if (attributeParam.Type == typeof(GH_Number))
            {
                double[] doubleList = values.ConvertAll((x) => { x.CastTo<double>(out double d); return d; }).ToArray();
                MM.AddDoubleArray(name, doubleList);
            }
            else if (attributeParam.Type == typeof(GH_Integer))
            {
                int[] integerList = values.ConvertAll((x) => { x.CastTo<int>(out int i); return i; }).ToArray();
                MM.AddIntegerArray(name, integerList);
            }
            else if (attributeParam.Type == typeof(GH_Boolean))
            {
                bool[] boolList = values.ConvertAll((x) => { x.CastTo<bool>(out bool b); return b; }).ToArray();
                MM.AddBoolArray(name, boolList);
            }
            else if (attributeParam.Type == typeof(GH_String))
            {
                string[] stringList = values.ConvertAll((x) => { x.CastTo<string>(out string s); return s; }).ToArray();
                MM.AddStringArray(name, stringList);
            }
        }

        private void SetRuleAttribute<T>(IGH_Param attributeParam, T value, ref RuleAttributesMap MM) where T : IGH_Goo
        {
            var name = attributeParam.Name;
            if (attributeParam.Type == typeof(GH_Number))
            {
                if (value.CastTo(out double number))
                    MM.AddDouble(name, number);
                else
                    throw new Exception(Utils.GetCastErrorMessage(name, "double"));
            }
            else if (attributeParam.Type == typeof(GH_Integer))
            {
                if (value.CastTo(out int integer))
                    MM.AddInteger(name, integer);
                else
                    throw new Exception(Utils.GetCastErrorMessage(name, "integer"));
            }
            else if (attributeParam.Type == typeof(GH_Boolean))
            {
                if (value.CastTo(out bool boolean))
                    MM.AddBoolean(name, boolean);
                else
                    throw new Exception(Utils.GetCastErrorMessage(name, "bool"));
            }
            else if (attributeParam.Type == typeof(GH_String))
            {
                if (value.CastTo(out string text))
                    MM.AddString(name, text);
                else
                    throw new Exception(Utils.GetCastErrorMessage(name, "string"));
            }
            else if (attributeParam.Type == typeof(GH_Colour))
            {
                if (value.CastTo(out Color color))
                {
                    string text = Utils.HexColor(color);
                    MM.AddString(name, text);
                }
                else
                    throw new Exception(Utils.GetCastErrorMessage(name, "Colour"));
            }
        }

        private List<RuleAttribute> GetEligibleAttributes()
        {
            var eligibleAttributes = new List<RuleAttribute>();
            foreach (var attr in mRuleAttributes)
            {
                if (!Params.Input.Exists(x => x.NickName == attr.mNickname))
                {
                    eligibleAttributes.Add(attr);
                }
            }
            return eligibleAttributes;
        }

        public bool CanInsertParameter(GH_ParameterSide side, int index)
        {
            if ((side != GH_ParameterSide.Input) || (index <= 2))
                return false;

            List<RuleAttribute> eligibleAttributes = GetEligibleAttributes();
            return (eligibleAttributes.Count > 0);
        }

        public bool CanRemoveParameter(GH_ParameterSide side, int index)
        {
            return (side == GH_ParameterSide.Input) && (index > 2);
        }

        public IGH_Param CreateParameter(GH_ParameterSide side, int index)
        {
            Debug.Assert(mRuleAttributes.Length > 0); // ensured by CanInsertParameter

            List<RuleAttribute> eligibleAttributes = GetEligibleAttributes();

            var form = new AttributeForm(eligibleAttributes,
                                         mDefaultValues,
                                         new System.Drawing.Point(Cursor.Position.X, Cursor.Position.Y));
            var result = form.ShowDialog();
            if (result == DialogResult.OK)
            {
                if (form.SelectedIndex >= 0 && form.SelectedIndex < mRuleAttributes.Length)
                {
                    var attr = eligibleAttributes[form.SelectedIndex];
                    return attr.CreateInputParameter(mDefaultValues);
                }
            }

            return null;
        }

        public bool DestroyParameter(GH_ParameterSide side, int index)
        {
            return true;
        }

        public void VariableParameterMaintenance()
        {
            return;
        }

        protected override System.Drawing.Bitmap Icon
        {
            get
            {
                return Resources.gh_prt_main_component;
            }
        }

        public override Guid ComponentGuid
        {
            get { return PumaUIDs.ComponentPumaGuid; }
        }
    }
}
