#include "yearn_plugin.h"

// Called once to init.
void handle_init_contract(void *parameters) {
    ethPluginInitContract_t *msg = (ethPluginInitContract_t *) parameters;

    if (msg->interfaceVersion != ETH_PLUGIN_INTERFACE_VERSION_LATEST) {
        msg->result = ETH_PLUGIN_RESULT_UNAVAILABLE;
        return;
    }

    if (msg->pluginContextLength < sizeof(context_t)) {
        PRINTF("Plugin parameters structure is bigger than allowed size\n");
        msg->result = ETH_PLUGIN_RESULT_ERROR;
        return;
    }

    context_t *context = (context_t *) msg->pluginContext;
    memset(context, 0, sizeof(*context));

    uint8_t i;
    for (i = 0; i < NUM_SELECTORS; i++) {
        if (memcmp((uint8_t *) PIC(YEARN_SELECTORS[i]), msg->selector, SELECTOR_SIZE) == 0) {
            context->selectorIndex = i;
            break;
        }
    }
    if (i == NUM_SELECTORS) {
        msg->result = ETH_PLUGIN_RESULT_UNAVAILABLE;
    }

    switch (context->selectorIndex) {
        case DEPOSIT_ALL:
        case WITHDRAW_ALL:
        case CLAIM:
        case EXIT:
        case GET_REWARDS:
            break;
        case DEPOSIT:
        case DEPOSIT_TO:
        case WITHDRAW:
        case WITHDRAW_TO:
        case WITHDRAW_TO_SLIPPAGE:
        case IB_MINT:
        case IB_REDEEM:
        case IB_REDEEM_UNDERLYING:
        case IB_BORROW:
        case IB_REPAY_BORROW:
            context->next_param = AMOUNT;
            break;
        case ZAP_IN:
        case ZAP_IN_PICKLE:
            context->next_param = ZAP_TOKEN;
            break;
        default:
            PRINTF("Missing selectorIndex: %d\n", context->selectorIndex);
            msg->result = ETH_PLUGIN_RESULT_ERROR;
            return;
    }
    msg->result = ETH_PLUGIN_RESULT_OK;
}
