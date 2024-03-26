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

namespace PumaGrasshopper
{
    public class RulePackage
    {
        public string path { get; set; }
        public DateTime timestamp { get; set; }

        public bool IsValid()
        {
            return (path != null) && (path.Length > 0) && (timestamp != null);
        }

        public bool IsSame(RulePackage other)
        {
            if (path == null || other.path == null)
                return false; // default-initialized rule package paths are always different
            return (string.Compare(path, other.path) == 0) && (timestamp == other.timestamp);
        }
    };
}
