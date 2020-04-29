// Copyright (C) 2018-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include "myriad_layers_grn_test.hpp"

INSTANTIATE_TEST_CASE_P(accuracy, myriadLayersTestsGRN_nightly,
        ::testing::Combine(
        ::testing::ValuesIn(s_GRNTensors),
        ::testing::ValuesIn(s_GRN_bias),
        ::testing::ValuesIn(s_MVNCustomConfig)));


TEST_F(myriadLayersTests_nightly, GRN_CHW_Input)
{
    std::string model = R"V0G0N(
        <net name="GRN" version="2" batch="1">
            <layers>
                <layer name="data" type="Input" precision="FP16" id="1">
                    <output>
                        <port id="1">
                            <dim>1</dim>
                            <dim>24</dim>
                            <dim>128</dim>
                            <dim>224</dim>
                        </port>
                    </output>
                </layer>
                <layer name="grn" type="GRN" precision="FP16" id="2">
                    <data bias="0.5"/>
                    <input>
                        <port id="2">
                            <dim>1</dim>
                            <dim>24</dim>
                            <dim>128</dim>
                            <dim>224</dim>
                        </port>
                    </input>
                    <output>
                        <port id="3">
                            <dim>1</dim>
                            <dim>24</dim>
                            <dim>128</dim>
                            <dim>224</dim>
                        </port>
                    </output>
                </layer>
            </layers>
            <edges>
                <edge from-layer="1" from-port="1" to-layer="2" to-port="2"/>
            </edges>
        </net>
    )V0G0N";

    StatusCode st;

    ASSERT_NO_THROW(readNetwork(model));

    const auto& network = _cnnNetwork;

    _inputsInfo = network.getInputsInfo();
    _inputsInfo["data"]->setPrecision(Precision::FP16);

    _outputsInfo = network.getOutputsInfo();
    _outputsInfo["grn"]->setPrecision(Precision::FP16);

    ASSERT_NO_THROW(st = _vpuPluginPtr->LoadNetwork(_exeNetwork, network, {}, &_resp));
    ASSERT_EQ(StatusCode::OK, st) << _resp.msg;
    ASSERT_NE(_exeNetwork, nullptr) << _resp.msg;

    ASSERT_NO_THROW(st = _exeNetwork->CreateInferRequest(_inferRequest, &_resp));
    ASSERT_EQ(StatusCode::OK, st) << _resp.msg;

    auto tensorDesc = TensorDesc(Precision::FP16, _inputsInfo["data"]->getTensorDesc().getDims(), Layout::NCHW);
    
    auto inputNCHW = make_shared_blob<ie_fp16>(tensorDesc);
    ASSERT_NO_THROW(inputNCHW->allocate());

    auto outputNCHW = make_shared_blob<ie_fp16>(tensorDesc);
    ASSERT_NO_THROW(outputNCHW->allocate());

    auto output_ref = make_shared_blob<ie_fp16>(tensorDesc);
    ASSERT_NO_THROW(output_ref->allocate());

    ASSERT_NO_THROW(GenRandomData(inputNCHW));

    ASSERT_NO_THROW(st = _inferRequest->SetBlob("data", inputNCHW, &_resp));
    ASSERT_EQ(StatusCode::OK, st) << _resp.msg;

    ASSERT_NO_THROW(st = _inferRequest->SetBlob("grn", outputNCHW, &_resp));
    ASSERT_EQ(StatusCode::OK, st) << _resp.msg;

    ASSERT_NO_THROW(st = _inferRequest->Infer(&_resp));
    ASSERT_EQ(StatusCode::OK, st) << _resp.msg;

    ASSERT_NO_FATAL_FAILURE(refGRN(inputNCHW, output_ref, 0.5f, true));

    CompareCommonAbsolute(outputNCHW, output_ref, 0.003);
}