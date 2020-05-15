// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "xls/passes/dce_pass.h"

#include "xls/common/logging/logging.h"
#include "xls/common/status/status_macros.h"
#include "xls/ir/dfs_visitor.h"
#include "xls/ir/node_iterator.h"

namespace xls {

xabsl::StatusOr<bool> DeadCodeEliminationPass::RunOnFunction(
    Function* f, const PassOptions& options, PassResults* results) const {
  std::deque<Node*> worklist;
  for (Node* n : f->nodes()) {
    if (n->users().empty() && n != f->return_value() && !n->Is<Param>()) {
      worklist.push_back(n);
    }
  }
  int64 removed_count = 0;
  absl::flat_hash_set<Node*> unique_operands;
  while (!worklist.empty()) {
    Node* node = worklist.front();
    worklist.pop_front();

    // A node may appear more than once as an operand of 'node'. Keep track of
    // which operands have been handled in a set.
    unique_operands.clear();
    for (Node* operand : node->operands()) {
      if (unique_operands.insert(operand).second) {
        if (operand->users().size() == 1 && operand != f->return_value() &&
            !operand->Is<Param>()) {
          worklist.push_back(operand);
        }
      }
    }
    XLS_RETURN_IF_ERROR(f->RemoveNode(node));
    removed_count++;
  }

  XLS_VLOG(2) << "Removed " << removed_count << " dead nodes";
  return removed_count > 0;
}

}  // namespace xls