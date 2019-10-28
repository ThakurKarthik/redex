/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <boost/optional.hpp>
#include <mutex>
#include <string>

#include "KeepReason.h"
#include "RedexContext.h"

namespace ir_meta_io {
class IRMetaIO;
}

namespace keep_rules {
namespace impl {
class KeepState;
}
} // namespace keep_rules

class ReferencedState {
 private:
  struct InnerStruct {
    // Whether this DexMember is referenced by one of the strings in the native
    // libraries. Note that this doesn't allow us to distinguish
    // native -> Java references from Java -> native refs.
    bool m_by_string{false};
    // This is a superset of m_by_string -- i.e. it's true if m_by_string is
    // true. It also gets set ot true if this DexMember is referenced by one of
    // the "keep_" settings in the Redex config.
    bool m_by_type{false};
    // Whether it is referenced from an XML layout.
    bool m_by_resources{false};
    // Whether it is a json serializer/deserializer class for a reachable class.
    bool m_is_serde{false};

    // ProGuard keep settings
    //
    // Whether any keep rule has matched this. This applies for both `-keep` and
    // `-keepnames`.
    bool m_keep{false};
    // assumenosideeffects allows certain methods to be removed.
    bool m_assumenosideeffects{false};
    // If m_whyareyoukeeping is true then report debugging information
    // about why this class or member is being kept.
    bool m_whyareyoukeeping{false};

    // For keep modifiers: -keep,allowshrinking and -keep,allowobfuscation.
    //
    // Instead of m_allowshrinking and m_allowobfuscation, we need to have
    // set/unset pairs for easier parallelization. The unset has a high
    // priority. See the comments in apply_keep_modifiers.
    bool m_set_allowshrinking{false};
    bool m_unset_allowshrinking{false};
    bool m_set_allowobfuscation{false};
    bool m_unset_allowobfuscation{false};

    bool m_no_optimizations{false};

    bool m_generated{false};

    // For inlining configurations.
    bool m_dont_inline{false};
    bool m_force_inline{false};

    int32_t m_api_level{-1};
  } inner_struct;

  // InterDex subgroup, if any.
  // NOTE: Will be set ONLY for generated classes.
  boost::optional<size_t> m_interdex_subgroup{boost::none};

  std::mutex m_keep_reasons_mtx;
  keep_reason::ReasonPtrSet m_keep_reasons;

 public:
  ReferencedState() = default;

  ReferencedState& operator=(const ReferencedState& other) {
    if (this != &other) {
      this->inner_struct = other.inner_struct;
    }
    return *this;
  }

  void join_with(const ReferencedState& other) {
    if (this != &other) {
      this->inner_struct.m_by_type =
          this->inner_struct.m_by_type | other.inner_struct.m_by_type;
      this->inner_struct.m_by_string =
          this->inner_struct.m_by_string | other.inner_struct.m_by_string;
      this->inner_struct.m_by_resources =
          this->inner_struct.m_by_resources | other.inner_struct.m_by_resources;
      this->inner_struct.m_is_serde =
          this->inner_struct.m_is_serde | other.inner_struct.m_is_serde;

      this->inner_struct.m_keep =
          this->inner_struct.m_keep | other.inner_struct.m_keep;
      this->inner_struct.m_assumenosideeffects =
          this->inner_struct.m_assumenosideeffects &
          other.inner_struct.m_assumenosideeffects;
      this->inner_struct.m_whyareyoukeeping =
          this->inner_struct.m_whyareyoukeeping |
          other.inner_struct.m_whyareyoukeeping;

      this->inner_struct.m_set_allowshrinking =
          this->inner_struct.m_set_allowshrinking &
          other.inner_struct.m_set_allowshrinking;
      this->inner_struct.m_unset_allowshrinking =
          this->inner_struct.m_unset_allowshrinking |
          other.inner_struct.m_unset_allowshrinking;
      this->inner_struct.m_set_allowobfuscation =
          this->inner_struct.m_set_allowobfuscation &
          other.inner_struct.m_set_allowobfuscation;
      this->inner_struct.m_unset_allowobfuscation =
          this->inner_struct.m_unset_allowobfuscation |
          other.inner_struct.m_unset_allowobfuscation;

      this->inner_struct.m_no_optimizations =
          this->inner_struct.m_no_optimizations |
          other.inner_struct.m_no_optimizations;
      this->inner_struct.m_dont_inline =
          this->inner_struct.m_dont_inline | other.inner_struct.m_dont_inline;
      this->inner_struct.m_force_inline =
          this->inner_struct.m_force_inline & other.inner_struct.m_force_inline;
    }
  }

  std::string str() const;

  // ProGuard keep options

  // -keep
  bool can_delete() const {
    return (!inner_struct.m_keep || allowshrinking()) &&
           !inner_struct.m_by_resources;
  }

  // -keepnames
  bool can_rename() const {
    return can_rename_if_also_renaming_xml() && !inner_struct.m_by_resources;
  }

  /*
   * Returns whether :member can be renamed if references to it from XML
   * resources are also updated accordingly. The optimizing pass in question
   * will be responsible for updating the XML resources.
   */
  bool can_rename_if_also_renaming_xml() const {
    return !inner_struct.m_keep || allowobfuscation();
  }

  bool assumenosideeffects() const {
    return inner_struct.m_assumenosideeffects;
  }

  bool report_whyareyoukeeping() const {
    return inner_struct.m_whyareyoukeeping;
  }

  // For example, a classname in a layout, e.g. <com.facebook.MyCustomView /> or
  // Class c = Class.forName("com.facebook.FooBar");
  void ref_by_string() {
    inner_struct.m_by_type = inner_struct.m_by_string = true;
  }

  bool is_referenced_by_string() const { return inner_struct.m_by_string; }

  // A class referenced by resource XML can take the following forms in .xml
  // files under the res/ directory:
  // <com.facebook.FooView />
  // <fragment android:name="com.facebook.BarFragment" />
  //
  // This differs from "by_string" reference since it is possible to rename
  // these string references, and potentially eliminate dead resource .xml files
  void set_referenced_by_resource_xml() {
    inner_struct.m_by_resources = true;
    if (RedexContext::record_keep_reasons()) {
      add_keep_reason(RedexContext::make_keep_reason(keep_reason::XML));
    }
  }

  void unset_referenced_by_resource_xml() {
    inner_struct.m_by_resources = false;
    // TODO: Remove the XML-related keep reasons
  }

  bool is_referenced_by_resource_xml() const {
    return inner_struct.m_by_resources;
  }

  void set_is_serde() { inner_struct.m_is_serde = true; }

  bool is_serde() const { return inner_struct.m_is_serde; }

  // A direct reference from code (not reflection)
  void ref_by_type() { inner_struct.m_by_type = true; }

  void set_root() { set_root(keep_reason::UNKNOWN); }

  /*
   * Mark this DexMember as an entry point that should not be deleted or
   * renamed.
   *
   * The ...args are arguments to the keep_reason::Reason constructor.
   * The typical Redex run does not care to keep the extra diagnostic
   * information of the keep reasons, so it seems worthwhile to forward these
   * arguments to avoid the construction of unused Reason objects when
   * record_keep_reasons() is false.
   */
  template <class... Args>
  void set_root(Args&&... args) {
    inner_struct.m_keep = true;
    unset_allowshrinking();
    unset_allowobfuscation();
    if (RedexContext::record_keep_reasons()) {
      add_keep_reason(
          RedexContext::make_keep_reason(std::forward<Args>(args)...));
    }
  }

  const keep_reason::ReasonPtrSet& keep_reasons() const {
    return m_keep_reasons;
  }

  template <class... Args>
  void set_keepnames(Args&&... args) {
    set_has_keep(std::forward<Args>(args)...);
    set_allowshrinking();
    unset_allowobfuscation();
  }

  void set_keepnames() { set_keepnames(keep_reason::UNKNOWN); }

  // This one should only be used by UnmarkProguardKeepPass to unmark proguard
  // keep rule after proguard file processing is finished. Because
  // ProguardMatcher uses parallel processing, using this will result in race
  // condition.
  void force_unset_allowshrinking() {
    inner_struct.m_set_allowshrinking = true;
    inner_struct.m_unset_allowshrinking = false;
  }

  void set_assumenosideeffects() { inner_struct.m_assumenosideeffects = true; }

  void set_whyareyoukeeping() { inner_struct.m_whyareyoukeeping = true; }

  void set_interdex_subgroup(const boost::optional<size_t>& interdex_subgroup) {
    m_interdex_subgroup = interdex_subgroup;
  }
  size_t get_interdex_subgroup() const { return m_interdex_subgroup.get(); }
  bool has_interdex_subgroup() const {
    return m_interdex_subgroup != boost::none;
  }

  // -1 means unknown, e.g. for a method created by Redex
  int32_t get_api_level() const { return inner_struct.m_api_level; }
  void set_api_level(int api_level) { inner_struct.m_api_level = api_level; }

  bool no_optimizations() const { return inner_struct.m_no_optimizations; }
  void set_no_optimizations() { inner_struct.m_no_optimizations = true; }

  // Methods and classes marked as "generated" tend to not have stable names,
  // and don't properly participate in coldstart tracking.
  bool is_generated() const { return inner_struct.m_generated; }
  void set_generated() { inner_struct.m_generated = true; }

  bool force_inline() const { return inner_struct.m_force_inline; }
  void set_force_inline() { inner_struct.m_force_inline = true; }
  bool dont_inline() const { return inner_struct.m_dont_inline; }
  void set_dont_inline() { inner_struct.m_dont_inline = true; }

  /*** YOU PROBABLY SHOULDN'T USE THIS ***/
  // This is a conservative estimate about what cannot be deleted. Not all
  // passes respect this -- most critically, RMU doesn't. Use can_delete()
  // instead, which ignores our over-conservative native libraries analysis.
  bool can_delete_DEPRECATED() const {
    return !inner_struct.m_by_type && !inner_struct.m_by_resources &&
           (!inner_struct.m_keep || allowshrinking());
  }

  // Like can_delete_DEPRECATED(), this is also over-conservative. Weirdly, it
  // also excludes things that are marked with allowshrinking. Use can_rename()
  // instead.
  bool can_rename_DEPRECATED() const {
    return !inner_struct.m_by_string &&
           (!inner_struct.m_keep || allowobfuscation()) && !allowshrinking();
  }

 private:
  // Does any keep rule (whether -keep or -keepnames) match this DexMember?
  bool has_keep() const { return inner_struct.m_keep; }

  /*
   * This should only be called from ProguardMatcher, and is used whenever we
   * encounter a keep rule (regardless of whether it's `-keep` or `-keepnames`).
   */
  template <class... Args>
  void set_has_keep(Args&&... args) {
    inner_struct.m_keep = true;
    if (RedexContext::record_keep_reasons()) {
      add_keep_reason(
          RedexContext::make_keep_reason(std::forward<Args>(args)...));
    }
  }

  // There's generally no need to call this; use can_delete() instead.
  bool allowshrinking() const {
    return !inner_struct.m_unset_allowshrinking &&
           inner_struct.m_set_allowshrinking;
  }

  void set_allowshrinking() { inner_struct.m_set_allowshrinking = true; }

  void unset_allowshrinking() { inner_struct.m_unset_allowshrinking = true; }

  // There's generally no need to call this; use can_rename() instead.
  bool allowobfuscation() const {
    return !inner_struct.m_unset_allowobfuscation &&
           inner_struct.m_set_allowobfuscation;
  }

  void set_allowobfuscation() { inner_struct.m_set_allowobfuscation = true; }

  void unset_allowobfuscation() {
    inner_struct.m_unset_allowobfuscation = true;
  }

  void add_keep_reason(const keep_reason::Reason* reason) {
    always_assert(RedexContext::record_keep_reasons());
    std::lock_guard<std::mutex> lock(m_keep_reasons_mtx);
    m_keep_reasons.emplace(reason);
  }

  friend class keep_rules::impl::KeepState;

  // IR serialization class
  friend class ir_meta_io::IRMetaIO;
};
