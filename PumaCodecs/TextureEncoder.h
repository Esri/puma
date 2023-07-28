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

#pragma once

#include "prtx/NamePreparator.h"
#include "prtx/Texture.h"

#include "prt/Callbacks.h"

#include <string>

namespace TextureEncoder {

enum class Format : uint8_t { AUTO, JPG, PNG, TIF };

std::wstring encode(const prtx::TexturePtr& tex, prt::SimpleOutputCallbacks* soh, prtx::NamePreparator& namePreparator,
                    const prtx::NamePreparator::NamespacePtr& namespaceFilenames,
                    const std::wstring& memTexFileNamePrefix, const Format& targetFormat = Format::AUTO);

} // namespace TextureEncoder
