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
using System.Linq;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using PumaGrasshopper.Properties;
using Rhino.Geometry;

namespace PumaGrasshopper
{
    
    public class ComponentPumaBulkInput : ComponentPumaShared
    {
        const string COMPONENT_NAME = "Puma bulk input";
        const string COMPONENT_NICK_NAME = "CityEngine Puma bulk";

        const string BULK_INPUT_NAME = "Rule attributes";
        const string BULK_INPUT_NICK_NAME = "Attributes";
        const string BULK_INPUT_DESC = "Attributes passed to the rule. Expects a tree of key:value pairs.";

        enum InputParams
        {
            RPK,
            SHAPES,
            SEEDS,
            BULK,
        }

        static readonly ParameterDescriptor[] INPUT_PARAM_DESC = new ParameterDescriptor[]{
            new ParameterDescriptor{ type = ParamType.FILEPATH, name = RPK_INPUT_NAME, nickName = RPK_INPUT_NICK_NAME, desc = RPK_INPUT_DESC },
            new ParameterDescriptor{ type = ParamType.GEOMETRY, name = GEOM_INPUT_NAME, nickName = GEOM_INPUT_NICK_NAME, desc = GEOM_INPUT_DESC },
            new ParameterDescriptor{ type = ParamType.INTEGER, name = SEED_KEY, nickName = SEED_INPUT_NAME, desc = SEED_INPUT_DESC },
            new ParameterDescriptor{ type = ParamType.GENERIC, name = BULK_INPUT_NAME, nickName = BULK_INPUT_NICK_NAME, desc = BULK_INPUT_DESC},
        };

        public ComponentPumaBulkInput()
          : base(COMPONENT_NAME, COMPONENT_NICK_NAME) { }

        protected override void RegisterInputParams(GH_Component.GH_InputParamManager pManager)
        {
            foreach( var param in Enum.GetValues(typeof(InputParams)).Cast<InputParams>())
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
                    case ParamType.GENERIC:
                        pManager.AddGenericParameter(desc.name, desc.nickName, desc.desc, GH_ParamAccess.tree);
                        pManager[3].Optional = true;
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

            if (mCurrentRpk == null || !mCurrentRpk.IsSame(rpk))
            {
                mCurrentRpk = rpk;
                mRuleAttributes = PRTWrapper.GetRuleAttributes(rpk.path);
            }

            RuleAttributesMap MM = ParseBulkInputTree(DA, inputMeshes.Count);

            var generatedMeshes = PRTWrapper.Generate(rpk.path, ref MM, inputMeshes);
            OutputGeometry(DA, generatedMeshes.meshes);
            OutputMaterials(DA, generatedMeshes.materials);
            OutputReports(DA, generatedMeshes.reports);
            OutputCGAPrint(DA, generatedMeshes.prints);
            OutputCGAErrors(DA, generatedMeshes.errors);
        }

        private RuleAttributesMap ParseBulkInputTree(IGH_DataAccess DA, int shapeCount)
        {
            RuleAttributesMap MM = new RuleAttributesMap();

            GH_Structure<IGH_Goo> inputTree;
            if (!DA.GetDataTree(BULK_INPUT_NAME, out inputTree))
                return null;

            List<RuleAttribute> attributesList = mRuleAttributes.ToList();

            for(int shapeId = 0; shapeId < shapeCount; ++shapeId)
            {
                // We need to initialize the buffers even if there are no attributes provided.
                MM.StartNewSection();

                // Only continue if actual attributes are provided.
                if (inputTree.PathCount > 0)
                {
                    // Repeat the last attribute branch in case there is more shape than attribute branch.
                    var branchId = shapeId < inputTree.PathCount ? shapeId : inputTree.PathCount - 1;

                    List<IGH_Goo> branch = (List<IGH_Goo>)inputTree.get_Branch(branchId);

                    branch.ForEach((input) => {
                        var pair = Utils.ParseInputPair(input);
                        var attribute = attributesList.Find(attr => attr.mFullName.Equals(pair.Key) || attr.mNickname.Equals(pair.Key));

                        if(attribute != null)
                        {
                            if (attribute.IsArray())
                            {
                                SetRuleAttributeArray(attribute, pair.Value, ref MM);
                            }
                            else if(pair.Value.Length > 0)
                            {
                                SetRuleAttribute(attribute, pair.Value[0], ref MM);
                            } 
                            else
                            {
                                throw new Exception("Attribute key " + pair.Key + " has no corresponding value");
                            }
                        }
                    });
                }
            }

            return MM;
        }

        private void SetRuleAttribute(RuleAttribute attribute, string value, ref RuleAttributesMap MM)
        {
            var name = attribute.mFullName;
            switch(attribute.mAttribType)
            {
                case Annotations.AnnotationArgumentType.AAT_BOOL:
                    try
                    {
                        MM.AddBoolean(name, Convert.ToBoolean(value));
                    } catch(Exception)
                    {
                        throw new Exception(Utils.GetCastErrorMessage(name, "boolean"));
                    }
                    break;
                case Annotations.AnnotationArgumentType.AAT_INT:
                    try
                    {
                        MM.AddInteger(name, Convert.ToInt32(value));
                    } catch(Exception)
                    {
                        throw new Exception(Utils.GetCastErrorMessage(name, "integer"));
                    }
                    break;
                case Annotations.AnnotationArgumentType.AAT_FLOAT:
                    try
                    {
                        MM.AddDouble(name, Convert.ToDouble(value));
                    } catch (Exception)
                    {
                        throw new Exception(Utils.GetCastErrorMessage(name, "double"));
                    }                        
                    break;
                case Annotations.AnnotationArgumentType.AAT_STR:
                    MM.AddString(name, value);
                    break;
                default:
                    return;
            }
        }

        private void SetRuleAttributeArray(RuleAttribute attribute, string[] value, ref RuleAttributesMap MM)
        {
            var name = attribute.mFullName;

            switch(attribute.mAttribType)
            {
                case Annotations.AnnotationArgumentType.AAT_BOOL_ARRAY:
                    try { 
                        bool[] boolList = Array.ConvertAll(value, (x) => Convert.ToBoolean(x));
                        MM.AddBoolArray(name, boolList);
                    } catch(Exception)
                    {
                        throw new Exception(Utils.GetCastErrorMessage(name, "bool array"));
                    }
                    break;
                case Annotations.AnnotationArgumentType.AAT_FLOAT_ARRAY:
                    try {
                        double[] doubleList = Array.ConvertAll(value, (x) => Convert.ToDouble(x));
                        MM.AddDoubleArray(name, doubleList);
                    } catch(Exception)  {
                        throw new Exception(Utils.GetCastErrorMessage(name, "double array"));
                    }
                    break;
                case Annotations.AnnotationArgumentType.AAT_STR_ARRAY:
                    MM.AddStringArray(name, value);
                    break;
                case Annotations.AnnotationArgumentType.AAT_INT_ARRAY:
                    try
                    {
                        int[] intList = Array.ConvertAll(value, x => Convert.ToInt32(x));
                        MM.AddIntegerArray(name, intList);
                    } catch(Exception)
                    {
                        throw new Exception(Utils.GetCastErrorMessage(name, "integer array"));
                    }
                    break;
                default:
                    return;
            }
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
            get { return PumaUIDs.ComponentPumaBulkInputGuid; }
        }
    }
}