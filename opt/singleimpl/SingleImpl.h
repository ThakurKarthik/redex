/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include "Pass.h"

struct SingleImplConfig {
  std::vector<std::string> white_list;
  std::vector<std::string> package_white_list;
  std::vector<std::string> black_list;
  std::vector<std::string> package_black_list;
  std::vector<std::string> anno_black_list;
  bool intf_anno;
  bool meth_anno;
  bool field_anno;
  bool rename_on_collision;
  bool filter_proguard_special_interfaces;
};

class SingleImplPass : public Pass {
 public:
  SingleImplPass() : Pass("SingleImplPass") {}

  void bind_config() override {
    bind("white_list", {}, m_pass_config.white_list);
    bind("package_white_list", {}, m_pass_config.package_white_list);
    bind("black_list", {}, m_pass_config.black_list);
    bind("package_black_list", {}, m_pass_config.package_black_list);
    bind("anno_black_list", {}, m_pass_config.anno_black_list);
    bind("type_annotations", true, m_pass_config.intf_anno);
    bind("method_annotations", true, m_pass_config.meth_anno);
    bind("field_annotations", true, m_pass_config.field_anno);
    bind("rename_on_collision", false, m_pass_config.rename_on_collision);
    bind("filter_proguard_special_interfaces",
         false,
         m_pass_config.filter_proguard_special_interfaces);
  }

  void run_pass(DexStoresVector&, ConfigFiles&, PassManager&) override;

  // count of removed interfaces
  size_t removed_count{0};

  // count of invoke-interface changed to invoke-virtual
  static size_t s_invoke_intf_count;

 private:
  SingleImplConfig m_pass_config;
};
