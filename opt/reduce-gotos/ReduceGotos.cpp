/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

/*
 * This optimizer pass reduces goto instructions.
 *
 * It does so in a few ways:
 * 0) Switches get simplified: dropping useless cases, eliminating empty
 *    switches, or turning them into ifs when beneficial (not strictly reducing
 *    gotos, but similar effects)
 * 1) When a conditional branch would fallthrough to a block that has multiple
 *    sources, and the branch target only one has one, invert condition and
 *    swap branch and goto target. This reduces the need for additional gotos /
 *    maximizes the fallthrough efficiency.
 * 2) It replaces gotos that eventually simply return by return instructions.
 *    Return instructions tend to have a smaller encoding than goto
 *    instructions, and tend to compress better due to less entropy (no offset).
 */

#include "ReduceGotos.h"

#include <vector>

#include "ControlFlow.h"
#include "DexClass.h"
#include "DexUtil.h"
#include "IRCode.h"
#include "IRInstruction.h"
#include "IROpcode.h"
#include "Liveness.h"
#include "Walkers.h"

namespace {

constexpr const char* METRIC_REMOVED_SWITCHES = "num_removed_switches";
constexpr const char* METRIC_REDUCED_SWITCHES = "num_reduced_switches";
constexpr const char* METRIC_REMAINING_TRIVIAL_SWITCHES =
    "num_remaining_trivial_switches";
constexpr const char* METRIC_REMAINING_RANGE_SWITCHES =
    "num_remaining_range_switches";
constexpr const char* METRIC_REMAINING_RANGE_SWITCH_CASES =
    "num_remaining_range_switch_cases";
constexpr const char* METRIC_REMAINING_TWO_CASE_SWITCHES =
    "num_remaining_two_case_switches";
constexpr const char* METRIC_REPLACED_TRIVIAL_SWITCHES =
    "num_replaced_trivial_switches";
constexpr const char* METRIC_REMOVED_SWITCH_CASES = "num_removed_switch_cases";
constexpr const char* METRIC_GOTOS_REPLACED_WITH_RETURNS =
    "num_gotos_replaced_with_returns";
constexpr const char* METRIC_TRAILING_MOVES_REMOVED =
    "num_trailing_moves_removed";
constexpr const char* METRIC_INVERTED_CONDITIONAL_BRANCHES =
    "num_inverted_conditional_branches";

} // namespace

void ReduceGotosPass::shift_registers(cfg::ControlFlowGraph* cfg,
                                      uint16_t* reg) {
  for (auto& mie : cfg::InstructionIterable(*cfg)) {
    auto insn = mie.insn;
    for (uint16_t i = 0; i < insn->srcs_size(); ++i) {
      insn->set_src(i, insn->src(i) + 1);
    }
    if (insn->has_dest()) {
      insn->set_dest(insn->dest() + 1);
    }
  }
  (*reg)++;
}

void ReduceGotosPass::process_code_switches(cfg::ControlFlowGraph& cfg,
                                            Stats& stats) {
  auto blocks = cfg.blocks();
  std::vector<cfg::Block*> switch_blocks;
  std::copy_if(blocks.begin(), blocks.end(), std::back_inserter(switch_blocks),
               [](cfg::Block* b) {
                 return b->branchingness() == opcode::BRANCH_SWITCH;
               });
  if (switch_blocks.size() == 0) {
    // Let's skip computing the liveness
    return;
  }

  std::unique_ptr<LivenessFixpointIterator> liveness_iter;

  boost::optional<uint16_t> const_reg;

  // Optimization #0: Remove switches...
  // - turning them into gotos when all cases branch to the same block
  // - removing cases that branch to the same block as the "default case"
  // - this might turn packed switches into sparse switches (but don't worry,
  //   there's an optimization IRCode that turns them into packed switches at
  //   the last moment during lowering if that's beneficial for code size)
  // - try turn trivial switches with just one case into ifs; respect the
  //   limitations of the DEX instructions in terms of register and literal
  //   bitness, assuming that this pass runs after register allocation.
  //   (We explored running a remove-switches pass before register allocation
  //   and the subsequent dedup-blocks pass, but that lead to a dramatic
  //   decrease in the number of cases that were identified to be unnecessary;
  //   thus, it seems beneficial to run after register allocation and
  //   dedup-blocks, and then we have to watch the registers.)
  for (cfg::Block* b : switch_blocks) {
    auto it = b->get_last_insn();
    always_assert(it != b->end());
    auto insn = it->insn;
    auto opcode = insn->opcode();
    cfg::Block* goto_target = b->goes_to();

    std::unordered_set<cfg::Edge*> fallthrough_edges;
    auto branch_edges = cfg.get_succ_edges_of_type(b, cfg::EDGE_BRANCH);
    for (cfg::Edge* branch_edge : branch_edges) {
      if (branch_edge->target() == goto_target) {
        fallthrough_edges.emplace(branch_edge);
      }
    }

    stats.removed_switch_cases += fallthrough_edges.size();

    if (fallthrough_edges.size() == branch_edges.size()) {
      // all branches fall through; just remove switch...
      stats.removed_switches++;
      b->remove_insn(it);
      continue;
    }

    if (fallthrough_edges.size() + 1 == branch_edges.size()) {
      // We have exactly one relevant branch (that isn't effectively falling
      // through)
      if (!liveness_iter) {
        liveness_iter.reset(new LivenessFixpointIterator(cfg));
        liveness_iter->run(LivenessDomain(cfg.get_registers_size()));
      }
      auto live_out_vars = liveness_iter->get_live_out_vars_at(b);
      auto single_non_fallthrough_edge_it = std::find_if(
          branch_edges.begin(), branch_edges.end(),
          [goto_target](cfg::Edge* e) { return e->target() != goto_target; });
      always_assert(single_non_fallthrough_edge_it != branch_edges.end());
      cfg::Edge* branch_edge = *single_non_fallthrough_edge_it;
      cfg::Block* branch_target = branch_edge->target();
      auto case_key = *branch_edge->case_key();
      auto reg = insn->src(0);
      always_assert(reg < 256);
      // We will try to replace
      //   switch reg
      //     case_key => branch_target
      // with...
      // 1) if case_key = 0:
      //      if-eqz reg, branch_target
      // 2) if reg is not live-out, reg fits in 8 bits and case_key in 8 bits:
      //      rsub-int/lit8 reg, reg, case_key
      //      if-eqz reg, branch_target
      // 3) if reg is not live-out, reg fits in 4 bits and case_key in 16 bits:
      //      rsub-int reg, reg, case_key
      //      if-eqz reg, branch_target
      // 4) if we can allocate another temp register creg with 4 bits:
      //      const creg, case_key
      //      if-eq creg, reg, branch_target

      boost::optional<IROpcode> transform_opcode;
      if (case_key == 0) {
        // Case 1
        transform_opcode = OPCODE_NOP;
      } else if (!live_out_vars.contains(reg)) {
        if (reg < 256 && (int8_t)case_key == case_key) {
          // Case 2
          transform_opcode = OPCODE_RSUB_INT_LIT8;
        } else if (reg < 16 && (int16_t)case_key == case_key) {
          // Case 3
          transform_opcode = OPCODE_RSUB_INT;
        }
      }

      if (transform_opcode) {
        // Case 1, 2, 3
        stats.replaced_trivial_switches++;
        stats.reduced_switches++;
        b->remove_insn(it);
        if (*transform_opcode != OPCODE_NOP) {
          IRInstruction* transform_insn = new IRInstruction(*transform_opcode);
          transform_insn->set_src(0, reg);
          transform_insn->set_literal(case_key);
          transform_insn->set_dest(reg);
          b->push_back(transform_insn);
        }
        IRInstruction* if_insn = new IRInstruction(OPCODE_IF_EQZ);
        if_insn->set_src(0, reg);
        cfg.create_branch(b, if_insn, goto_target, branch_target);
        continue;
      } else if (reg < (const_reg ? 16 : 15)) {
        if (!const_reg && cfg.get_registers_size() < 16) {
          // We'll use the register 0, and shift all other registers up by one.
          // This is because we assume that we run after the register allocation
          // pass, and all incoming parameters have been assigned to the highest
          // registers. We maintain this invariant.
          // Since we didn't use 16 registers before, we know that even after
          // allocating one more and shifting everything, all registers will
          // still fit into 4 bits, and thus all instruction register size
          // requirements will still be met.
          cfg.allocate_temp();
          shift_registers(&cfg, &reg);
          // Reset liveness, so that it gets re-computed if needed with
          // shifted register numberss
          liveness_iter.reset();
          const_reg = 0;
        }
        if (const_reg) {
          // Case 4
          stats.replaced_trivial_switches++;
          stats.reduced_switches++;
          b->remove_insn(it);
          IRInstruction* const_insn = new IRInstruction(OPCODE_CONST);
          const_insn->set_literal(case_key);
          const_insn->set_dest(*const_reg);
          b->push_back(const_insn);
          IRInstruction* if_insn = new IRInstruction(OPCODE_IF_EQ);
          if_insn->set_src(0, *const_reg);
          if_insn->set_src(1, reg);
          cfg.create_branch(b, if_insn, goto_target, branch_target);
          continue;
        }
      }

      TRACE(RG, 3,
            "[reduce gotos] Found irreducible trivial switch with register %u, "
            "case key %d, live out: %d",
            reg, case_key, live_out_vars.contains(insn->src(0)));
      stats.remaining_trivial_switches++;
    }

    if (fallthrough_edges.size() == 0) {
      // Nothing to optimize here
      continue;
    }

    stats.reduced_switches++;
    std::vector<std::pair<int32_t, cfg::Block*>> cases;
    for (cfg::Edge* branch_edge :
         cfg.get_succ_edges_of_type(b, cfg::EDGE_BRANCH)) {
      if (!fallthrough_edges.count(branch_edge)) {
        cases.emplace_back(*branch_edge->case_key(), branch_edge->target());
      }
    }
    always_assert(cases.size() > 0);

    // Sort, to make things tidy and deterministic, and ensure we can rely on
    // the front and back case keys being ordered properly
    std::sort(
        cases.begin(), cases.end(),
        [](std::pair<int32_t, cfg::Block*> a,
           std::pair<int32_t, cfg::Block*> b) { return a.first < b.first; });

    IRInstruction* new_switch = new IRInstruction(OPCODE_SWITCH);
    new_switch->set_src(0, insn->src(0));
    b->remove_insn(it);
    cfg.create_branch(b, new_switch, goto_target, cases);

    if (cases.size() == 2) {
      // If there's a significant amount of switches with just two cases, it
      // might be worthwhile to turn those into two ifs.
      stats.remaining_two_case_switches++;
    } else if (std::all_of(cases.begin(), cases.end(),
                           [&cases](const std::pair<int32_t, cfg::Block*>& c) {
                             return c.second == cases[0].second;
                           })) {
      // We found a switch with a contigious range where all cases point to the
      // same block. If there's a significant amount of switches of this kind,
      // it might be worthwhile to turn them into two ifs that check whether the
      // selector is in the range.
      stats.remaining_range_switches++;
      stats.remaining_range_switch_cases += cases.size();
    }
  }
}

void ReduceGotosPass::process_code_ifs(cfg::ControlFlowGraph& cfg,
                                       Stats& stats) {
  // Optimization #1: Invert conditional branch conditions and swap goto/branch
  // targets if this may lead to more fallthrough cases where no additional
  // goto instruction is needed
  for (cfg::Block* b : cfg.blocks()) {
    auto br = b->branchingness();
    if (br != opcode::BRANCH_IF) {
      continue;
    }

    // So we have a block that ends with a conditional branch
    // Let's find the (unique) branch and goto targets
    auto it = b->get_last_insn();
    always_assert(it != b->end());
    auto insn = it->insn;
    auto opcode = insn->opcode();
    always_assert(is_conditional_branch(opcode));
    cfg::Edge* goto_edge = cfg.get_succ_edge_of_type(b, cfg::EDGE_GOTO);
    cfg::Edge* branch_edge = cfg.get_succ_edge_of_type(b, cfg::EDGE_BRANCH);
    always_assert(goto_edge != nullptr);
    always_assert(branch_edge != nullptr);

    // If beneficial, invert condition and swap targets
    if (goto_edge->target()->preds().size() > 1 &&
        branch_edge->target()->preds().size() == 1) {
      stats.inverted_conditional_branches++;
      // insert condition
      insn->set_opcode(opcode::invert_conditional_branch(opcode));
      // swap goto and branch target
      cfg::Block* branch_target = branch_edge->target();
      cfg::Block* goto_target = goto_edge->target();
      cfg.set_edge_target(branch_edge, goto_target);
      cfg.set_edge_target(goto_edge, branch_target);
    }
  }

  // Optimization #2:
  // Inline all blocks that just contain a single return instruction and are
  // reached via a goto edge; this may leave behind some unreachable blocks
  // which will get cleaned up via simplify() eventually.
  // Small bonus optimization: Also eliminate move instructions that only exist
  // to faciliate shared return instructions.

  bool rerun;
  do {
    auto order = cfg.order();
    rerun = false;
    for (auto block_it = order.begin(); block_it != order.end(); ++block_it) {
      cfg::Block* b = *block_it;
      auto last_mie_it = b->get_last_insn();
      if (last_mie_it == b->end()) {
        continue;
      }
      auto first_mie_it = b->get_first_insn();
      if (first_mie_it != last_mie_it) {
        continue;
      }
      MethodItemEntry* mie = &*last_mie_it;
      if (!is_return(mie->insn->opcode())) {
        continue;
      }

      std::vector<std::pair<cfg::Block*, IRInstruction*>> returns_to_add;
      for (cfg::Edge* e : cfg.get_pred_edges_of_type(b, cfg::EDGE_GOTO)) {
        cfg::Block* src = e->src();

        IRInstruction* cloned_return = new IRInstruction(*mie->insn);

        bool removed_trailing_move = false;
        if (cloned_return->srcs_size()) {
          // eliminate trailing move instruction by specializing return
          // instruction
          auto src_last_mie_it = src->get_last_insn();
          if (src_last_mie_it != src->end()) {
            // We are looking for an instruction of the form
            //   move $dest, $source
            // matching an
            //   return $dest
            // instruction we found earlier.
            IRInstruction* src_last_insn = src_last_mie_it->insn;
            if (is_move(src_last_insn->opcode()) &&
                src_last_insn->dest() == cloned_return->src(0) &&
                src_last_insn->is_wide() == cloned_return->is_wide()) {
              // Found a matching move! We'll rewrite the (cloned) return
              // instruction to
              //   return $source
              removed_trailing_move = true;
              cloned_return->set_src(0, src_last_insn->src(0));
              src->remove_insn(src_last_mie_it);
              stats.removed_trailing_moves++;
            }
          }
        }

        if (removed_trailing_move) {
          // Let's remember to run the optimization one more time, as removing
          // this move instruction may have unlocked further potential as it may
          // create a block with just a return in it.
          rerun = true;
        } else if (block_it != order.begin() && *std::prev(block_it) == src) {
          // Don't put in a return instruction if we would just fall through
          // anyway, i.e. if linearization won't insert a goto here.
          continue;
        }

        const auto& non_gotos =
            cfg.get_succ_edges_if(src, [](const cfg::Edge* e) {
              return e->type() == cfg::EDGE_BRANCH ||
                     e->type() == cfg::EDGE_THROW;
            });
        if (!non_gotos.empty()) {
          // It's not safe to add a return because `src` has outgoing edges of
          // another type.
          //
          // Create a new return block that only `src` is the predecessor of.
          // This way, when the CFG chooses an order, it may choose this block
          // as the fallthrough predecessor, which means we don't need a goto.
          //
          // Effectively, we are duplicating this return block for each of its
          // goto predecessors. Notice that this optimization is the opposite of
          // DedupBlocksPass. This optimization should always occur after
          // DedupBlocks because DedupBlocks doesn't check if deduplicating the
          // blocks is worth the extra goto.
          auto new_return_block = cfg.create_block();
          new_return_block->push_back(cloned_return);
          cfg.set_edge_target(e, new_return_block);
        } else {
          // `src` has no other outgoing edges, we will add a return to this
          // block directly. However, we can't do it yet because we're iterating
          // through a copy of the predecessor edges of `b` and adding a return
          // deletes the outgoing edges of `src`. If we deleted this edge now we
          // might reach a stale Edge pointer that has been deleted.
          returns_to_add.emplace_back(src, cloned_return);
        }

        stats.replaced_gotos_with_returns++;
      }

      for (const auto& pair : returns_to_add) {
        // `src` has no other outgoing edges, we can just stick the return
        // instruction on the end
        cfg::Block* src = pair.first;
        IRInstruction* cloned_return = pair.second;
        src->push_back(cloned_return);
      }
    }

  } while (rerun);
}

ReduceGotosPass::Stats ReduceGotosPass::process_code(IRCode* code) {
  Stats stats;

  code->build_cfg(/* editable = true*/);
  code->cfg().calculate_exit_block();
  auto& cfg = code->cfg();
  process_code_switches(cfg, stats);
  process_code_ifs(cfg, stats);
  code->clear_cfg();

  return stats;
}

void ReduceGotosPass::run_pass(DexStoresVector& stores,
                               ConfigFiles& /* unused */,
                               PassManager& mgr) {
  auto scope = build_class_scope(stores);

  Stats stats = walk::parallel::reduce_methods<Stats>(
      scope,
      [](DexMethod* method) -> Stats {
        const auto code = method->get_code();
        if (!code) {
          return Stats{};
        }

        Stats stats = ReduceGotosPass::process_code(code);
        if (stats.replaced_gotos_with_returns ||
            stats.inverted_conditional_branches) {
          TRACE(RG, 3,
                "[reduce gotos] Replaced %u gotos with returns, "
                "removed %u trailing moves, "
                "inverted %u conditional branches in {%s}",
                stats.replaced_gotos_with_returns, stats.removed_trailing_moves,
                stats.inverted_conditional_branches, SHOW(method));
        }
        return stats;
      },
      [](Stats a, Stats b) {
        Stats c;
        c.removed_switches = a.removed_switches + b.removed_switches;
        c.reduced_switches = a.reduced_switches + b.reduced_switches;
        c.replaced_trivial_switches =
            a.replaced_trivial_switches + b.replaced_trivial_switches;
        c.remaining_trivial_switches =
            a.remaining_trivial_switches + b.remaining_trivial_switches;
        c.removed_switch_cases =
            a.removed_switch_cases + b.removed_switch_cases;
        c.replaced_gotos_with_returns =
            a.replaced_gotos_with_returns + b.replaced_gotos_with_returns;
        c.removed_trailing_moves =
            a.removed_trailing_moves + b.removed_trailing_moves;
        c.inverted_conditional_branches =
            a.inverted_conditional_branches + b.inverted_conditional_branches;
        c.remaining_two_case_switches =
            a.remaining_two_case_switches + b.remaining_two_case_switches;
        c.remaining_range_switches =
            a.remaining_range_switches + b.remaining_range_switches;
        c.remaining_range_switch_cases =
            a.remaining_range_switch_cases + b.remaining_range_switch_cases;
        return c;
      });

  mgr.incr_metric(METRIC_REMOVED_SWITCHES, stats.removed_switches);
  mgr.incr_metric(METRIC_REDUCED_SWITCHES, stats.reduced_switches);
  mgr.incr_metric(METRIC_REMAINING_TRIVIAL_SWITCHES,
                  stats.remaining_trivial_switches);
  mgr.incr_metric(METRIC_REPLACED_TRIVIAL_SWITCHES,
                  stats.replaced_trivial_switches);
  mgr.incr_metric(METRIC_REMAINING_RANGE_SWITCHES,
                  stats.remaining_range_switches);
  mgr.incr_metric(METRIC_REMAINING_RANGE_SWITCH_CASES,
                  stats.remaining_range_switch_cases);
  mgr.incr_metric(METRIC_REMAINING_TWO_CASE_SWITCHES,
                  stats.remaining_two_case_switches);
  mgr.incr_metric(METRIC_REMOVED_SWITCH_CASES, stats.removed_switch_cases);
  mgr.incr_metric(METRIC_GOTOS_REPLACED_WITH_RETURNS,
                  stats.replaced_gotos_with_returns);
  mgr.incr_metric(METRIC_TRAILING_MOVES_REMOVED, stats.removed_trailing_moves);
  mgr.incr_metric(METRIC_INVERTED_CONDITIONAL_BRANCHES,
                  stats.inverted_conditional_branches);
  TRACE(RG, 1,
        "[reduce gotos] Replaced %u gotos with returns, inverted %u "
        "conditional brnaches in total",
        stats.replaced_gotos_with_returns, stats.inverted_conditional_branches);
}

static ReduceGotosPass s_pass;
