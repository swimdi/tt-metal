# SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.

# SPDX-License-Identifier: Apache-2.0

import torch
import pytest
from loguru import logger
import transformers
import ttnn
from models.demos.ttnn_falcon7b.tt.falcon_causallm import TtFalconCausalLM
from models.demos.ttnn_falcon7b.tt.model_config import (
    get_model_config,
    get_tt_cache_path,
)
from models.demos.ttnn_falcon7b.tt.common import create_custom_preprocessor
from ttnn.model_preprocessing import preprocess_model_parameters
from tests.ttnn.utils_for_testing import assert_with_pcc

from models.demos.ttnn_falcon7b.tt.common import (
    create_custom_preprocessor,
    create_kv_cache,
)

from loguru import logger
from ttnn import ShardTensorToMesh, ReplicateTensorToMesh, ConcatMeshToTensor


PRETRAINED_MODEL_NAME = f"tiiuae/falcon-7b-instruct"


@pytest.mark.parametrize(
    "llm_mode, device_batch_size, seq_len, kv_cache_len",
    (
        ("prefill", 1, 128, 0),
        ("decode", 32, 1, 128),
    ),
    ids=["prefill_seq128_batch32", "decode_batch32"],
)
@pytest.mark.parametrize(
    "num_layers, expected_pcc",
    (
        (1, 0.98),
        (2, 0.98),
        (32, 0.60),
    ),
    ids=[
        "layers_1",
        "layers_2",
        "layers_32",
    ],
)
@pytest.mark.parametrize(
    "model_version",
    ("tiiuae/falcon-7b-instruct",),
    ids=["falcon_7b"],
)
@pytest.mark.parametrize("model_config_str", ("BFLOAT16-DRAM", "BFLOAT16-L1"))
@pytest.mark.parametrize(
    "device_mesh",
    [
        2,
    ],
    indirect=True,
)
@pytest.mark.parametrize(
    "enable_async, num_loops",
    ((True, 20), (False, 1)),
)
def test_falcon_causal_lm(
    device_mesh,
    use_program_cache,
    model_version,
    llm_mode,
    device_batch_size,
    seq_len,
    kv_cache_len,
    num_layers,
    expected_pcc,
    model_config_str,
    enable_async,
    num_loops,
):
    for device in device_mesh.get_device_ids():
        device_mesh.get_device(device).enable_async(enable_async)

    torch.manual_seed(0)
    batch = device_batch_size * device_mesh.get_num_devices()
    if llm_mode == "decode":
        shard_dim = 2
    else:
        shard_dim = 0

    configuration = transformers.FalconConfig.from_pretrained(model_version)
    configuration.num_hidden_layers = num_layers
    model = transformers.models.falcon.modeling_falcon.FalconForCausalLM.from_pretrained(
        model_version, config=configuration
    ).eval()
    model_config = get_model_config(model_config_str)
    dtype = model_config["DEFAULT_DTYPE"]
    kv_len = seq_len if llm_mode == "prefill" else kv_cache_len + 1

    model_input = torch.arange(seq_len * batch).reshape(batch, seq_len)

    if llm_mode == "prefill":
        past_key_values = None
        tt_layer_past = ()
        for i in range(num_layers):
            _, tt_current_layer_past = create_kv_cache(
                llm_mode,
                dtype,
                batch,
                kv_cache_len,
                configuration,
                device_mesh,
                mesh_mapper=ShardTensorToMesh(device_mesh, dim=0),
            )
            tt_layer_past += (tt_current_layer_past,)
        attention_mask = None

    elif llm_mode == "decode":
        past_key_values = ()
        tt_layer_past = ()
        for i in range(num_layers):
            current_layer_past, tt_current_layer_past = create_kv_cache(
                llm_mode,
                dtype,
                batch,
                kv_cache_len,
                configuration,
                device_mesh,
                mesh_mapper=ShardTensorToMesh(device_mesh, dim=0),
            )
            past_key_values += (current_layer_past,)
            tt_layer_past += (tt_current_layer_past,)

    else:
        raise NotImplementedError(f"Llm mode {llm_mode} is not supported! Must be one of prefill or decode.")

    pytorch_out, pytorch_layer_present = model(
        input_ids=model_input,
        attention_mask=None,  # when attention_mask is None, a causal mask is created under the hood
        past_key_values=past_key_values,
        use_cache=True,
        return_dict=False,
    )

    def convert_to_ttnn(model, name):
        return not isinstance(model, torch.nn.Embedding)

    parameters = preprocess_model_parameters(
        initialize_model=lambda: model,
        device=device_mesh,
        custom_preprocessor=create_custom_preprocessor(
            model_config,
            tt_cache_path=get_tt_cache_path(f"{model_version}"),
            device=device_mesh,
            weights_mesh_mapper=ReplicateTensorToMesh(device_mesh),
        ),
        convert_to_ttnn=convert_to_ttnn,
    )
    tt_FalconCausalLM = TtFalconCausalLM(
        device_mesh,
        num_layers,
        configuration,
        configuration.max_position_embeddings,
        model_config,
        parameters,
    )
    # TODO: Generate embeddings and attention_mask on device
    if llm_mode == "prefill":
        for loop in range(num_loops):
            tt_outs = []
            tt_embeddings, tt_attention_mask = tt_FalconCausalLM.model_preprocessing(
                llm_mode, model_input, kv_cache_len, num_input_tokens=seq_len
            )
            tt_out, tt_layer_present = tt_FalconCausalLM(
                input_embeddings=tt_embeddings,
                llm_mode=llm_mode,
                attention_mask=tt_attention_mask,
                user_id=0,
                layer_past=tt_layer_past,
                layer_past_len=kv_cache_len,
                use_cache=True,
            )
            # Explicitly move tensor to host ... in async mode this is faster than calling from torch directly,
            # due to parallelization of tensor shards
            tt_out = ttnn.from_device(tt_out)
            tt_out = ttnn.to_torch(
                tt_out, mesh_composer=ConcatMeshToTensor(device_mesh, dim=shard_dim), device=device_mesh
            ).squeeze(1)

    elif llm_mode == "decode":
        for loop in range(num_loops):
            tt_embeddings, tt_attention_mask = tt_FalconCausalLM.model_preprocessing(
                llm_mode, model_input, kv_cache_len, num_input_tokens=kv_len
            )
            tt_out, tt_layer_present = tt_FalconCausalLM(
                input_embeddings=tt_embeddings,
                llm_mode=llm_mode,
                attention_mask=tt_attention_mask,
                layer_past=tt_layer_past,
                layer_past_len=kv_cache_len,
                use_cache=True,
            )
            tt_out = ttnn.from_device(tt_out)
            tt_out = ttnn.to_torch(
                tt_out, mesh_composer=ConcatMeshToTensor(device_mesh, dim=shard_dim), device=device_mesh
            ).squeeze(1)
            tt_out = tt_out.transpose(0, 1)

    passed, pcc = assert_with_pcc(pytorch_out, tt_out.to(pytorch_out.dtype), expected_pcc)
    logger.success(f"Passed: pcc: {pcc}, expected: {expected_pcc}")

    for i in range(num_layers):
        tt_layer_pres = (
            ttnn.to_torch(
                tt_layer_present[i][0], mesh_composer=ConcatMeshToTensor(device_mesh, dim=0), device=device_mesh
            ),
            ttnn.to_torch(
                tt_layer_present[i][1], mesh_composer=ConcatMeshToTensor(device_mesh, dim=0), device=device_mesh
            ),
        )
        if llm_mode == "prefill":
            pytorch_layer_pres = pytorch_layer_present[i]
            tt_layer_pres = (
                tt_layer_pres[0][:, :, :kv_len, :],
                tt_layer_pres[1][:, :, :kv_len, :],
            )
        elif llm_mode == "decode":
            pytorch_layer_pres = (
                pytorch_layer_present[i][0][:, :, kv_cache_len, :],
                pytorch_layer_present[i][1][:, :, kv_cache_len, :],
            )
            tt_layer_pres = (
                tt_layer_pres[0][:, :, kv_cache_len, :],
                tt_layer_pres[1][:, :, kv_cache_len, :],
            )

        passed, pcc = assert_with_pcc(
            pytorch_layer_pres[0], tt_layer_pres[0].to(pytorch_layer_pres[0].dtype), expected_pcc
        )
        logger.success(f"Passed: pcc: {pcc}, expected: {expected_pcc}")
        passed, pcc = assert_with_pcc(
            pytorch_layer_pres[1], tt_layer_pres[1].to(pytorch_layer_pres[1].dtype), expected_pcc
        )
        logger.success(f"Passed: pcc: {pcc}, expected: {expected_pcc}")

    logger.info("Falcon CausalLM Passed!")

    for device in device_mesh.get_device_ids():
        device_mesh.get_device(device).enable_async(False)
