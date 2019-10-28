/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <string>

namespace keep_rules {
namespace proguard_parser {

std::string form_member_regex(std::string proguard_regex);
std::string form_type_regex(std::string proguard_regex);
std::string convert_wildcard_type(std::string typ);

} // namespace proguard_parser
} // namespace keep_rules
