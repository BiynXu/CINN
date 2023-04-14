// Copyright (c) 2023 CINN Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <memory>
#include <vector>

#include "cinn/common/common.h"
#include "cinn/common/macros.h"
#include "cinn/hlir/framework/node.h"
#include "cinn/hlir/framework/op.h"
#include "cinn/hlir/framework/op_strategy.h"
#include "cinn/hlir/op/op_util.h"
#include "cinn/hlir/pe/elementwise.h"
#include "cinn/hlir/pe/ir_schedule_pe.h"
#include "cinn/ir/ir.h"
#include "cinn/ir/ir_base.h"
#include "cinn/ir/tensor.h"
#include "cinn/lang/builtin.h"
#include "cinn/lang/compute.h"

namespace cinn {
namespace hlir {
namespace op {

using common::CINNValue;
using common::CINNValuePack;

std::shared_ptr<framework::OpStrategy> StrategyForAssertTrue(const framework::NodeAttr &attrs,
                                                             const std::vector<ir::Tensor> &inputs,
                                                             const std::vector<Type> &out_type,
                                                             const std::vector<std::vector<int>> &output_shapes,
                                                             const Target &target) {
  framework::CINNCompute assert_true_compute([=](lang::Args args, lang::RetValue *ret) {
    CHECK(!args.empty()) << "The input argument of assert_true is empty! Please check.";
    CINNValuePack pack_args = args[0];
    CHECK_GE(pack_args.size(), 2U) << "Two input tensors are required for the computation of assert_true.";
    Expr a_expr             = pack_args[0];
    Expr b_expr             = pack_args[1];
    ir::Tensor a            = a_expr.as_tensor_ref();
    ir::Tensor b            = b_expr.as_tensor_ref();
    std::string tensor_name = "assert_true_out";
    auto out                = pe::Identity(b, tensor_name).front();
    auto stages             = CreateStages({out});
    std::vector<CINNValue> res{CINNValue(out), CINNValue(stages)};
    *ret = CINNValuePack{res};
  });
  auto strategy = std::make_shared<framework::OpStrategy>();
  strategy->AddImpl(
      assert_true_compute, GetElementwiseScheduleFunc(output_shapes, target), "strategy.assert_true.x86", 1);
  return strategy;
}

std::vector<framework::shape_t> InferShapeForAssertTrue(const std::vector<framework::shape_t> &inputs_shape,
                                                        const framework::AttrMapType &attrs) {
  return inputs_shape;
}

std::vector<Type> InferDtypeForAssertTrue(const std::vector<Type> &inputs_type, const framework::AttrMapType &attrs) {
  return inputs_type;
}

}  // namespace op
}  // namespace hlir
}  // namespace cinn

CINN_REGISTER_HELPER(assert_true_ops) {
  CINN_REGISTER_OP(assert_true)
      .describe("AssertTrue")
      .set_num_inputs(1)
      .set_num_outputs(1)
      .set_attr<cinn::hlir::framework::StrategyFunction>("CINNStrategy", cinn::hlir::op::StrategyForAssertTrue)
      .set_attr("infershape", MakeOpFunction(cinn::hlir::op::InferShapeForAssertTrue))
      .set_attr("inferdtype", MakeOpFunction(cinn::hlir::op::InferDtypeForAssertTrue))
      .set_attr<cinn::hlir::framework::OpPatternKind>("OpPattern", cinn::hlir::framework::OpPatternKind::kNonFusible)
      .set_support_level(4);

  return true;
}
