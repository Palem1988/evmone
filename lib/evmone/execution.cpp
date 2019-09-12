// evmone: Fast Ethereum Virtual Machine implementation
// Copyright 2019 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.

#include "execution.hpp"
#include "analysis.hpp"
#include <memory>

namespace evmone
{
evmc_result execute(evmc_instance* /*unused*/, evmc_context* ctx, evmc_revision rev,
    const evmc_message* msg, const uint8_t* code, size_t code_size) noexcept
{
    auto analysis = analyze(rev, code, code_size);

    auto state = std::make_unique<execution_state>();
    state->analysis = &analysis;
    state->msg = msg;
    state->code = code;
    state->code_size = code_size;
    state->host = evmc::HostContext{ctx};
    state->gas_left = msg->gas;
    state->rev = rev;

    const instr_info* instr = &state->analysis->instrs[0];
    while (instr != nullptr)
        instr = instr->fn(instr, *state);

    evmc_result result{};

    result.status_code = state->status;

    if (result.status_code == EVMC_SUCCESS || result.status_code == EVMC_REVERT)
        result.gas_left = state->gas_left;

    if (state->output_size > 0)
    {
        result.output_size = state->output_size;
        // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
        auto output_data = static_cast<uint8_t*>(std::malloc(result.output_size));
        std::memcpy(output_data, &state->memory[state->output_offset], result.output_size);
        result.output_data = output_data;
        result.release = [](const evmc_result* r) noexcept
        {
            // NOLINTNEXTLINE(cppcoreguidelines-no-malloc, cppcoreguidelines-pro-type-const-cast)
            std::free(const_cast<uint8_t*>(r->output_data));
        };
    }

    return result;
}
}  // namespace evmone
