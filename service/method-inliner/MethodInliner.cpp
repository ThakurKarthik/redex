/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "MethodInliner.h"

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "ClassHierarchy.h"
#include "Deleter.h"
#include "DexClass.h"
#include "DexUtil.h"
#include "IRCode.h"
#include "IRInstruction.h"
#include "Inliner.h"
#include "MethodOverrideGraph.h"
#include "ReachableClasses.h"
#include "Resolver.h"
#include "Walkers.h"

namespace mog = method_override_graph;

namespace {

static bool can_inline_init(DexMethod* caller, IRCode& code) {
  always_assert(is_init(caller));
  // Check that there is no call to a super constructor, and no assignments to
  // (non-inherited) instance fields before constructor call.
  // (There not being such a super call implies that there must be a call to
  // another constructor in the same class, unless the method doesn't return;
  // calls to other constructors in the same class are inlinable.)
  // The check doesn't take into account data-flow, i.e. whether the super
  // constructor call and the field assignments are actually on the incoming
  // receiver object. In that sense, this function is overly conservative, and
  // there is room for futher improvement.
  DexType* declaring_type = caller->get_class();
  DexType* super_type = type_class(declaring_type)->get_super_class();
  for (auto& mie : InstructionIterable(code)) {
    IRInstruction* insn = mie.insn;
    auto opcode = insn->opcode();

    // give up if there's an assignment to a field of the declaring class
    if (is_iput(opcode) && insn->get_field()->get_class() == declaring_type) {
      return false;
    }

    // give up if there's a call to a constructor of the super class
    if (opcode != OPCODE_INVOKE_DIRECT) {
      continue;
    }
    DexMethod* callee =
        resolve_method(insn->get_method(), MethodSearch::Direct);
    if (callee == nullptr) {
      return false;
    }
    if (!is_init(callee)) {
      continue;
    }
    if (callee->get_class() == super_type) {
      return false;
    }
  }
  return true;
}

/**
 * Collect all non virtual methods and make all small methods candidates
 * for inlining.
 */
std::unordered_set<DexMethod*> gather_non_virtual_methods(Scope& scope,
                                                          bool virtual_inline) {
  // trace counter
  size_t all_methods = 0;
  size_t direct_methods = 0;
  size_t direct_no_code = 0;
  size_t non_virtual_no_code = 0;
  size_t clinit = 0;
  size_t init = 0;
  size_t static_methods = 0;
  size_t private_methods = 0;
  size_t dont_strip = 0;
  size_t non_virt_dont_strip = 0;
  size_t non_virt_methods = 0;

  // collect all non virtual methods (dmethods and vmethods)
  std::unordered_set<DexMethod*> methods;
  walk::methods(scope, [&](DexMethod* method) {
    all_methods++;
    if (method->is_virtual()) return;

    auto code = method->get_code();
    bool dont_inline = code == nullptr;

    direct_methods++;
    if (code == nullptr) direct_no_code++;
    if (is_constructor(method)) {
      (is_static(method)) ? clinit++ : init++;
      if (is_clinit(method) || !can_inline_init(method, *code)) {
        dont_inline = true;
      }
    } else {
      (is_static(method)) ? static_methods++ : private_methods++;
    }

    if (dont_inline) return;

    methods.insert(method);
  });
  if (virtual_inline) {
    auto non_virtual = mog::get_non_true_virtuals(scope);
    non_virt_methods = non_virtual.size();
    for (const auto& vmeth : non_virtual) {
      auto code = vmeth->get_code();
      if (code == nullptr) {
        non_virtual_no_code++;
        continue;
      }
      methods.insert(vmeth);
    }
  }

  TRACE(INLINE, 2, "All methods count: %ld", all_methods);
  TRACE(INLINE, 2, "Direct methods count: %ld", direct_methods);
  TRACE(INLINE, 2, "Virtual methods count: %ld", all_methods - direct_methods);
  TRACE(INLINE, 2, "Direct methods no code: %ld", direct_no_code);
  TRACE(INLINE, 2, "Direct methods with code: %ld",
        direct_methods - direct_no_code);
  TRACE(INLINE, 2, "Constructors with or without code: %ld", init);
  TRACE(INLINE, 2, "Static constructors: %ld", clinit);
  TRACE(INLINE, 2, "Static methods: %ld", static_methods);
  TRACE(INLINE, 2, "Private methods: %ld", private_methods);
  TRACE(INLINE, 2, "Virtual methods non virtual count: %ld", non_virt_methods);
  TRACE(INLINE, 2, "Non virtual no code count: %ld", non_virtual_no_code);
  TRACE(INLINE, 2, "Non virtual no strip count: %ld", non_virt_dont_strip);
  TRACE(INLINE, 2, "Don't strip inlinable methods count: %ld", dont_strip);
  return methods;
}

/**
 * Get a map of method -> implementation method that hold the same
 * implementation as the method would perform at run time.
 * So if a abtract method have multiple implementor but they all have the same
 * implementation, we can have a mapping between the abstract method and
 * one of its implementor.
 */
std::unordered_map<const DexMethod*, DexMethod*> get_same_implementation_map(
    const Scope& scope,
    std::unique_ptr<const mog::Graph>& method_override_graph) {
  std::unordered_map<const DexMethod*, DexMethod*> method_to_implementations;
  walk::methods(scope, [&](DexMethod* method) {
    if (method->is_external() || root(method) || !method->is_virtual() ||
        (!method->get_code() && !is_abstract(method))) {
      return;
    }
    const auto& overriding_methods =
        mog::get_overriding_methods(*method_override_graph, method);
    if (overriding_methods.size() == 0) {
      return;
    }
    // Filter out methods without IRCode.
    std::set<const DexMethod*, dexmethods_comparator> filtered_methods;
    for (auto overriding_method : overriding_methods) {
      if (is_abstract(overriding_method)) {
        continue;
      }
      if (!overriding_method->get_code()) {
        // If the method is not abstract method and it doesn't have
        // implementation, we bail out.
        return;
      }
      filtered_methods.emplace(overriding_method);
    }
    if (filtered_methods.size() == 0) {
      return;
    }
    if (method->get_code()) {
      filtered_methods.emplace(method);
    }

    // If all methods have the same implementation we create mapping between
    // methods and their representative implementation.
    auto* comparing_method = const_cast<DexMethod*>(*filtered_methods.begin());
    auto compare_method_ir = [&](const DexMethod* current) -> bool {
      return const_cast<DexMethod*>(current)->get_code()->structural_equals(
          *(comparing_method->get_code()));
    };
    if (std::all_of(std::next(filtered_methods.begin()), filtered_methods.end(),
                    compare_method_ir)) {
      method_to_implementations[method] = comparing_method;
      for (auto overriding_method : overriding_methods) {
        method_to_implementations[overriding_method] = comparing_method;
      }
    }
  });
  return method_to_implementations;
}

using CallerInsns =
    std::unordered_map<DexMethod*, std::unordered_set<IRInstruction*>>;
/**
 * Gather candidates of true virtual methods that can be inlined and their
 * call site in true_virtual_callers.
 * A true virtual method can be inlined to its callsite if the callsite can
 * be resolved to only one method implementation deterministically.
 * We are currently ruling out candidates that access field/methods or
 * return an object type.
 */
void gather_true_virtual_methods(const Scope& scope,
                                 CalleeCallerInsns* true_virtual_callers,
                                 std::unordered_set<DexMethod*>* methods) {
  auto method_override_graph = mog::build_graph(scope);
  auto non_virtual = mog::get_non_true_virtuals(*method_override_graph, scope);
  auto same_implementation_map =
      get_same_implementation_map(scope, method_override_graph);
  std::unordered_set<DexMethod*> non_virtual_set{non_virtual.begin(),
                                                 non_virtual.end()};
  // Add mapping from callee to monomorphic callsites.
  auto update_monomorphic_callsite =
      [](DexMethod* caller, IRInstruction* callsite, DexMethod* callee,
         ConcurrentMap<DexMethod*, CallerInsns>* meth_caller) {
        if (!callee->get_code()) {
          return;
        }
        meth_caller->update(
            callee, [&](const DexMethod*, CallerInsns& m, bool /* exists */) {
              m[caller].emplace(callsite);
            });
      };

  ConcurrentMap<DexMethod*, CallerInsns> meth_caller;
  walk::parallel::code(scope, [&non_virtual_set, &method_override_graph,
                               &meth_caller, &update_monomorphic_callsite,
                               &same_implementation_map](DexMethod* method,
                                                         IRCode& code) {
    for (auto& mie : InstructionIterable(code)) {
      auto insn = mie.insn;
      if (insn->opcode() != OPCODE_INVOKE_VIRTUAL &&
          insn->opcode() != OPCODE_INVOKE_INTERFACE) {
        continue;
      }
      auto insn_method = insn->get_method();
      auto callee = resolve_method(insn_method, opcode_to_search(insn));
      if (callee == nullptr) {
        // There are some invoke-virtual call on methods whose def are
        // actually in interface.
        callee = resolve_method(insn->get_method(), MethodSearch::Interface);
      }
      if (callee == nullptr) {
        continue;
      }
      if (non_virtual_set.count(callee) != 0) {
        // Not true virtual, no need to continue;
        continue;
      }
      always_assert_log(callee->is_def(), "Resolved method not def %s",
                        SHOW(callee));
      if (same_implementation_map.count(callee)) {
        // We can find the resolved callee in same_implementation_map,
        // just use that piece of info because we know the implementors are all
        // the same
        update_monomorphic_callsite(
            method, insn, same_implementation_map[callee], &meth_caller);
        continue;
      }
      const auto& overriding_methods =
          mog::get_overriding_methods(*method_override_graph, callee);
      if (!callee->is_external()) {
        if (overriding_methods.size() == 0) {
          // There is no override for this method
          update_monomorphic_callsite(
              method, insn, static_cast<DexMethod*>(callee), &meth_caller);
        } else if (is_abstract(callee) && overriding_methods.size() == 1) {
          // The method is an abstract method, the only override is its
          // implementation.
          auto implementing_method =
              const_cast<DexMethod*>(*(overriding_methods.begin()));

          update_monomorphic_callsite(method, insn, implementing_method,
                                      &meth_caller);
        }
      }
    }
  });

  // Post processing candidates.
  std::for_each(meth_caller.begin(), meth_caller.end(), [&](const auto& pair) {
    DexMethod* callee = pair.first;
    auto& caller_to_invocations = pair.second;
    always_assert_log(methods->count(callee) == 0, "%s\n", SHOW(callee));
    always_assert(callee->get_code());
    // Not considering candidates that accessed type in their method body
    // or returning a non primitive type.
    if (!is_primitive(
            get_element_type_if_array(callee->get_proto()->get_rtype()))) {
      return;
    }
    for (auto& mie : InstructionIterable(callee->get_code())) {
      auto insn = mie.insn;
      if (insn->has_type() || insn->has_method() || insn->has_field()) {
        return;
      }
    }
    methods->insert(callee);
    (*true_virtual_callers)[callee] = caller_to_invocations;
  });
}

} // namespace

namespace inliner {
void run_inliner(DexStoresVector& stores,
                 PassManager& mgr,
                 const InlinerConfig& inliner_config,
                 bool intra_dex /*false*/) {
  if (mgr.no_proguard_rules()) {
    TRACE(INLINE, 1,
          "MethodInlinePass not run because no ProGuard configuration was "
          "provided.");
    return;
  }
  auto scope = build_class_scope(stores);
  CalleeCallerInsns true_virtual_callers;
  // Gather all inlinable candidates.
  auto methods =
      gather_non_virtual_methods(scope, inliner_config.virtual_inline);

  if (inliner_config.virtual_inline && inliner_config.true_virtual_inline) {
    gather_true_virtual_methods(scope, &true_virtual_callers, &methods);
  }
  // keep a map from refs to defs or nullptr if no method was found
  MethodRefCache resolved_refs;
  auto resolver = [&resolved_refs](DexMethodRef* method, MethodSearch search) {
    return resolve_method(method, search, resolved_refs);
  };
  if (inliner_config.use_cfg_inliner) {
    walk::parallel::code(scope, [](DexMethod*, IRCode& code) {
      code.build_cfg(/* editable */ true);
    });
  }

  // inline candidates
  MultiMethodInliner inliner(scope, stores, methods, resolver, inliner_config,
                             intra_dex ? IntraDex : InterDex,
                             true_virtual_callers);
  inliner.inline_methods();

  if (inliner_config.use_cfg_inliner) {
    walk::parallel::code(scope,
                         [](DexMethod*, IRCode& code) { code.clear_cfg(); });
  }

  // delete all methods that can be deleted
  auto inlined = inliner.get_inlined();
  size_t inlined_count = inlined.size();

  // Do not erase true virtual methods that are inlined because we are only
  // inlining callsites that are monomorphic, for polymorphic callsite we
  // didn't inline, but in run time the callsite may still be resolved to
  // those methods that are inlined. We are relying on RMU to clean up
  // true virtual methods that are not referenced.
  for (const auto& pair : true_virtual_callers) {
    inlined.erase(pair.first);
  }
  size_t deleted = delete_methods(scope, inlined, resolver);

  TRACE(INLINE, 3, "recursive %ld", inliner.get_info().recursive);
  TRACE(INLINE, 3, "blacklisted meths %ld", inliner.get_info().blacklisted);
  TRACE(INLINE, 3, "virtualizing methods %ld", inliner.get_info().need_vmethod);
  TRACE(INLINE, 3, "invoke super %ld", inliner.get_info().invoke_super);
  TRACE(INLINE, 3, "override inputs %ld", inliner.get_info().write_over_ins);
  TRACE(INLINE, 3, "escaped virtual %ld", inliner.get_info().escaped_virtual);
  TRACE(INLINE, 3, "known non public virtual %ld",
        inliner.get_info().non_pub_virtual);
  TRACE(INLINE, 3, "non public ctor %ld", inliner.get_info().non_pub_ctor);
  TRACE(INLINE, 3, "unknown field %ld", inliner.get_info().escaped_field);
  TRACE(INLINE, 3, "non public field %ld", inliner.get_info().non_pub_field);
  TRACE(INLINE, 3, "throws %ld", inliner.get_info().throws);
  TRACE(INLINE, 3, "multiple returns %ld", inliner.get_info().multi_ret);
  TRACE(INLINE, 3, "references cross stores %ld",
        inliner.get_info().cross_store);
  TRACE(INLINE, 3, "not found %ld", inliner.get_info().not_found);
  TRACE(INLINE, 3, "caller too large %ld", inliner.get_info().caller_too_large);
  TRACE(INLINE, 1, "%ld inlined calls over %ld methods and %ld methods removed",
        inliner.get_info().calls_inlined, inlined_count, deleted);

  mgr.incr_metric("calls_inlined", inliner.get_info().calls_inlined);
  mgr.incr_metric("methods_removed", deleted);
  mgr.incr_metric("escaped_virtual", inliner.get_info().escaped_virtual);
  mgr.incr_metric("unresolved_methods", inliner.get_info().unresolved_methods);
  mgr.incr_metric("known_public_methods",
                  inliner.get_info().known_public_methods);
  mgr.incr_metric("constant_invoke_callers_analyzed",
                  inliner.get_info().constant_invoke_callers_analyzed);
  mgr.incr_metric(
      "constant_invoke_callers_unreachable_blocks",
      inliner.get_info().constant_invoke_callers_unreachable_blocks);
  mgr.incr_metric("constant_invoke_callees_analyzed",
                  inliner.get_info().constant_invoke_callees_analyzed);
  mgr.incr_metric(
      "constant_invoke_callees_unreachable_blocks",
      inliner.get_info().constant_invoke_callees_unreachable_blocks);
}
} // namespace inliner
