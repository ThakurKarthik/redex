/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "RemoveUnusedFields.h"

#include "DexClass.h"
#include "FieldOpTracker.h"
#include "IRCode.h"
#include "Resolver.h"
#include "Walkers.h"

using namespace remove_unused_fields;

namespace {

bool can_remove(DexField* field) {
  // XXX(jezng): why is can_rename not a subset of can_delete?
  return field != nullptr && !field->is_external() &&
         can_delete_DEPRECATED(field) && can_rename_DEPRECATED(field);
}

bool has_non_zero_static_value(DexField* field) {
  auto ev = field->get_static_value();
  return ev != nullptr && !ev->is_zero();
}

class RemoveUnusedFields final {
 public:
  RemoveUnusedFields(const Config& config, const Scope& scope)
      : m_config(config), m_scope(scope) {
    analyze();
    transform();
  }

  const std::unordered_set<const DexField*>& unread_fields() const {
    return m_unread_fields;
  }

  const std::unordered_set<const DexField*>& unwritten_fields() const {
    return m_unwritten_fields;
  }

  const std::unordered_set<const DexField*>& zero_written_fields() const {
    return m_zero_written_fields;
  }

 private:
  bool is_blacklisted(const DexType* type) const {
    return m_config.blacklist_types.count(type) != 0;
  }

  bool is_whitelisted(DexField* field) const {
    return !m_config.whitelist || m_config.whitelist->count(field) != 0;
  }

  void analyze() {
    field_op_tracker::FieldStatsMap field_stats =
        field_op_tracker::analyze(m_scope);

    // analyze_non_zero_writes and the later transform() need (editable) cfg
    walk::parallel::code(m_scope, [&](const DexMethod*, IRCode& code) {
      code.build_cfg(/* editable = true*/);
    });

    boost::optional<field_op_tracker::NonZeroWrittenFields> non_zero_writes;
    if (m_config.remove_zero_written_fields) {
      non_zero_writes = field_op_tracker::analyze_non_zero_writes(m_scope);
    }

    for (auto& pair : field_stats) {
      auto* field = pair.first;
      auto& stats = pair.second;
      TRACE(RMUF,
            3,
            "%s: %lu %lu %lu %d",
            SHOW(field),
            stats.reads,
            stats.reads_outside_init,
            stats.writes,
            is_synthetic(field));
      if (can_remove(field) && !is_blacklisted(field->get_type()) &&
          is_whitelisted(field)) {
        if (m_config.remove_unread_fields && stats.reads == 0) {
          m_unread_fields.emplace(field);
        } else if (m_config.remove_unwritten_fields && stats.writes == 0 &&
                   !has_non_zero_static_value(field)) {
          m_unwritten_fields.emplace(field);
        } else if (m_config.remove_zero_written_fields &&
                   !non_zero_writes->count(field) &&
                   !has_non_zero_static_value(field)) {
          m_zero_written_fields.emplace(field);
        }
      }
    }
    TRACE(RMUF, 2, "unread_fields %u", m_unread_fields.size());
    TRACE(RMUF, 2, "unwritten_fields %u", m_unwritten_fields.size());
    TRACE(RMUF, 2, "zero written_fields %u", m_zero_written_fields.size());
  }

  void transform() const {
    // Replace reads to unwritten fields with appropriate const-0 instructions,
    // and remove the writes to unread fields.
    walk::parallel::code(m_scope, [&](const DexMethod*, IRCode& code) {
      auto& cfg = code.cfg();
      auto iterable = cfg::InstructionIterable(cfg);
      std::vector<cfg::InstructionIterator> to_replace;
      std::vector<cfg::InstructionIterator> to_remove;
      for (auto insn_it = iterable.begin(); insn_it != iterable.end();
           ++insn_it) {
        auto* insn = insn_it->insn;
        if (!insn->has_field()) {
          continue;
        }
        auto field = resolve_field(insn->get_field());
        if (m_unread_fields.count(field)) {
          always_assert(is_iput(insn->opcode()) || is_sput(insn->opcode()));
          TRACE(RMUF, 5, "Removing %s", SHOW(insn));
          to_remove.push_back(insn_it);
        } else if (m_unwritten_fields.count(field)) {
          always_assert(is_iget(insn->opcode()) || is_sget(insn->opcode()));
          TRACE(RMUF, 5, "Replacing %s with const 0", SHOW(insn));
          to_replace.push_back(insn_it);
        } else if (m_zero_written_fields.count(field)) {
          if (is_iput(insn->opcode()) || is_sput(insn->opcode())) {
            TRACE(RMUF, 5, "Removing %s", SHOW(insn));
            to_remove.push_back(insn_it);
          } else {
            always_assert(is_iget(insn->opcode()) || is_sget(insn->opcode()));
            TRACE(RMUF, 5, "Replacing %s with const 0", SHOW(insn));
            to_replace.push_back(insn_it);
          }
        }
      }
      for (auto insn_it : to_replace) {
        auto move_result = cfg.move_result_of(insn_it)->insn;
        IRInstruction* const0 = new IRInstruction(
            move_result->dest_is_wide() ? OPCODE_CONST_WIDE : OPCODE_CONST);
        const0->set_dest(move_result->dest())->set_literal(0);
        auto invalidated = cfg.replace_insn(insn_it, const0);
        always_assert(!invalidated);
      }
      for (auto insn_it : to_remove) {
        cfg.remove_insn(insn_it);
      }
      code.clear_cfg();
    });
  }

  const Config& m_config;
  const Scope& m_scope;
  std::unordered_set<const DexField*> m_unread_fields;
  std::unordered_set<const DexField*> m_unwritten_fields;
  std::unordered_set<const DexField*> m_zero_written_fields;
};

} // namespace

namespace remove_unused_fields {

void PassImpl::run_pass(DexStoresVector& stores,
                        ConfigFiles& conf,
                        PassManager& mgr) {
  auto scope = build_class_scope(stores);
  RemoveUnusedFields rmuf(m_config, scope);
  mgr.set_metric("unread_fields", rmuf.unread_fields().size());
  mgr.set_metric("unwritten_fields", rmuf.unwritten_fields().size());
  mgr.set_metric("zero written_fields", rmuf.zero_written_fields().size());

  if (m_export_removed) {
    std::vector<const DexField*> removed_fields(rmuf.unread_fields().begin(),
                                                rmuf.unread_fields().end());
    removed_fields.insert(removed_fields.end(),
                          rmuf.unwritten_fields().begin(),
                          rmuf.unwritten_fields().end());
    sort_unique(removed_fields, compare_dexfields);
    auto path = conf.metafile(REMOVED_FIELDS_FILENAME);
    std::ofstream ofs(path, std::ofstream::out | std::ofstream::trunc);
    for (auto* field : removed_fields) {
      ofs << show_deobfuscated(field) << "\n";
    }
  }
}

static PassImpl s_pass;

} // namespace remove_unused_fields
