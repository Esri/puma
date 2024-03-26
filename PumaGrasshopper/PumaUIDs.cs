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
using System.Text;
using System.Threading.Tasks;

namespace PumaGrasshopper
{
    public static class PumaUIDs
    {
        public static Guid ComponentPumaGuid = new Guid("ad54a111-cdbc-4417-bddd-c2195c9482d8");
        public static Guid ComponentPumaBulkInputGuid = new Guid("03FB28F9-FF2E-4175-B75B-571E4DC26FE9");
        public static Guid ComponentReportsDislayGuid = new Guid("316524f4-1c56-41e7-9315-10f60b35cd61");
        public static Guid ComponentReportsUnpackerGuid = new Guid("23602a6d-1137-4403-867a-082e001ca707");

        public static Guid RulePackageParameterGuid = new Guid("ed287f2d-82c2-417d-9fc4-5b1a5c268032");

        public static Guid AttributeParameterBooleanGuid = new Guid("65bc034c-5340-4d4b-88a0-c60b4c63c667");
        public static Guid AttributeParameterNumberGuid = new Guid("edc1c20d-b8d2-4f2f-af03-c13f0b7f3367");
        public static Guid AttributeParameterStringGuid = new Guid("27d4954d-bce4-4253-91a1-83e286199e5a");
        public static Guid AttributeParameterColourGuid = new Guid("2f6be11a-f0aa-4a20-a81e-ccbf345ba294");
    }
}
