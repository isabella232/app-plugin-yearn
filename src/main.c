/*******************************************************************************
 *   Ethereum 2 Deposit Application
 *   (c) 2020 Ledger
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ********************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "os.h"
#include "cx.h"

#include "yearn_plugin.h"

static const uint8_t DEPOSIT_ALL_SELECTOR[SELECTOR_SIZE] = {0xd0, 0xe3, 0x0d, 0xb0};
static const uint8_t DEPOSIT_SELECTOR[SELECTOR_SIZE] = {0xb6, 0xb5, 0x5f, 0x25};
static const uint8_t DEPOSIT_TO_SELECTOR[SELECTOR_SIZE] = {0x6e, 0x55, 0x3f, 0x65};
static const uint8_t WITHDRAW_ALL_SELECTOR[SELECTOR_SIZE] = {0x3c, 0xcf, 0xd6, 0x0b};
static const uint8_t WITHDRAW_SELECTOR[SELECTOR_SIZE] = {0x2e, 0x1a, 0x7d, 0x4d};
static const uint8_t WITHDRAW_TO_SELECTOR[SELECTOR_SIZE] = {0x00, 0xf7, 0x14, 0xce};
static const uint8_t WITHDRAW_TO_SLIPPAGE_SELECTOR[SELECTOR_SIZE] = {0xe6, 0x36, 0x97, 0xc8};
static const uint8_t ZAP_IN_SELECTOR[SELECTOR_SIZE] = {0x38, 0xb3, 0x2e, 0x68};
static const uint8_t ZAP_IN_PICKLE_SELECTOR[SELECTOR_SIZE] = {0x28, 0x93, 0x20, 0x94};
static const uint8_t IB_MINT_SELECTOR[SELECTOR_SIZE] = {0xa0, 0x71, 0x2d, 0x68};
static const uint8_t IB_REDEEM_SELECTOR[SELECTOR_SIZE] = {0xdb, 0x00, 0x6a, 0x75};
static const uint8_t IB_REDEEM_UNDERLYING_SELECTOR[SELECTOR_SIZE] = {0x85, 0x2a, 0x12, 0xe3};
static const uint8_t IB_BORROW_SELECTOR[SELECTOR_SIZE] = {0xc5, 0xeb, 0xea, 0xec};
static const uint8_t IB_REPAY_BORROW_SELECTOR[SELECTOR_SIZE] = {0x0e, 0x75, 0x27, 0x02};
static const uint8_t CLAIM_SELECTOR[SELECTOR_SIZE] = {0x4e, 0x71, 0xd9, 0x2d};
static const uint8_t EXIT_SELECTOR[SELECTOR_SIZE] = {0xe9, 0xfa, 0xd8, 0xee};
static const uint8_t GET_REWARDS_SELECTOR[SELECTOR_SIZE] = {0x3d, 0x18, 0xb9, 0x12};

// Array of all the different boilerplate selectors. Make sure this follows the same order as the
// enum defined in `yearn_plugin.h`
const uint8_t *const YEARN_SELECTORS[NUM_SELECTORS] = {DEPOSIT_ALL_SELECTOR,
                                                       DEPOSIT_SELECTOR,
                                                       DEPOSIT_TO_SELECTOR,
                                                       WITHDRAW_ALL_SELECTOR,
                                                       WITHDRAW_SELECTOR,
                                                       WITHDRAW_TO_SELECTOR,
                                                       WITHDRAW_TO_SLIPPAGE_SELECTOR,
                                                       ZAP_IN_SELECTOR,
                                                       ZAP_IN_PICKLE_SELECTOR,
                                                       IB_MINT_SELECTOR,
                                                       IB_REDEEM_SELECTOR,
                                                       IB_REDEEM_UNDERLYING_SELECTOR,
                                                       IB_BORROW_SELECTOR,
                                                       IB_REPAY_BORROW_SELECTOR,
                                                       CLAIM_SELECTOR,
                                                       EXIT_SELECTOR,
                                                       GET_REWARDS_SELECTOR};

const yearnVaultDefinition_t YEARN_VAULTS[NUM_YEARN_VAULTS] = {
    {{0xbf, 0xa4, 0xd8, 0xaa, 0x6d, 0x8a, 0x37, 0x9a, 0xbf, 0xe7,
      0x79, 0x33, 0x99, 0xd3, 0xdd, 0xac, 0xc5, 0xbb, 0xec, 0xbb},
     "DAI",
     "yvDAI",
     18},
    {{0xe2, 0xf6, 0xb9, 0x77, 0x3b, 0xf3, 0xa0, 0x15, 0xe2, 0xaa,
      0x70, 0x74, 0x1b, 0xde, 0x14, 0x98, 0xbd, 0xb9, 0x42, 0x5b},
     "USDC",
     "yvUSDC",
     6},
    {{0xe1, 0x1b, 0xa4, 0x72, 0xf7, 0x48, 0x69, 0x17, 0x66, 0x52,
      0xc3, 0x5d, 0x30, 0xdb, 0x89, 0x85, 0x4b, 0x5a, 0xe8, 0x4d},
     "HEGIC",
     "yvHEGIC",
     18},
    {{0x19, 0xd3, 0x36, 0x4a, 0x39, 0x9d, 0x25, 0x1e, 0x89, 0x4a,
      0xc7, 0x32, 0x65, 0x1b, 0xe8, 0xb0, 0xe4, 0xe8, 0x50, 0x01},
     "DAI",
     "yvDAI",
     18},
    {{0x5f, 0x18, 0xc7, 0x5a, 0xbd, 0xae, 0x57, 0x8b, 0x48, 0x3e,
      0x5f, 0x43, 0xf1, 0x2a, 0x39, 0xcf, 0x75, 0xb9, 0x73, 0xa9},
     "USDC",
     "yvUSDC",
     6},
    {{0xdc, 0xd9, 0x0c, 0x7f, 0x63, 0x24, 0xcf, 0xa4, 0x0d, 0x71,
      0x69, 0xef, 0x80, 0xb1, 0x20, 0x31, 0x77, 0x0b, 0x43, 0x25},
     "stETH",
     "yvCurve-stETH",
     18},
    {{0xcb, 0x55, 0x0a, 0x6d, 0x4c, 0x8e, 0x35, 0x17, 0xa9, 0x39,
      0xbc, 0x79, 0xd0, 0xc7, 0x09, 0x3e, 0xb7, 0xcf, 0x56, 0xb5},
     "WBTC",
     "yvWBTC",
     8},
    {{0x98, 0x6b, 0x4a, 0xff, 0x58, 0x8a, 0x10, 0x9c, 0x09, 0xb5,
      0x0a, 0x03, 0xf4, 0x2e, 0x41, 0x10, 0xe2, 0x9d, 0x35, 0x3f},
     "sETH",
     "yvCurve-sETH",
     18},
    {{0xa9, 0xfe, 0x46, 0x01, 0x81, 0x12, 0x13, 0xc3, 0x40, 0xe8,
      0x50, 0xea, 0x30, 0x54, 0x81, 0xaf, 0xf0, 0x2f, 0x5b, 0x28},
     "WETH",
     "yvWETH",
     18},
    {{0xe1, 0x4d, 0x13, 0xd8, 0xb3, 0xb8, 0x5a, 0xf7, 0x91, 0xb2,
      0xaa, 0xdd, 0x66, 0x1c, 0xdb, 0xd5, 0xe6, 0x09, 0x7d, 0xb1},
     "YFI",
     "yvYFI",
     18},
    {{0xb8, 0xc3, 0xb7, 0xa2, 0xa6, 0x18, 0xc5, 0x52, 0xc2, 0x3b,
      0x1e, 0x47, 0x01, 0x10, 0x9a, 0x9e, 0x75, 0x6b, 0xab, 0x67},
     "1INCH",
     "yv1INCH",
     18},
    {{0x27, 0xb7, 0xb1, 0xad, 0x72, 0x88, 0x07, 0x9a, 0x66, 0xd1,
      0x23, 0x50, 0xc8, 0x28, 0xd3, 0xc0, 0x0a, 0x6f, 0x07, 0xd7},
     "IronBank",
     "yvCurve-IronBank",
     18},
    {{0x62, 0x5b, 0x7d, 0xf2, 0xfa, 0x8a, 0xbe, 0x21, 0xb0, 0xa9,
      0x76, 0x73, 0x6c, 0xda, 0x47, 0x75, 0x52, 0x3a, 0xed, 0x1e},
     "HBTC",
     "yvCurve-HBTC",
     18},
    {{0x9d, 0x40, 0x9a, 0x0a, 0x01, 0x2c, 0xfb, 0xa9, 0xb1, 0x5f,
      0x6d, 0x4b, 0x36, 0xac, 0x57, 0xa4, 0x69, 0x66, 0xab, 0x9a},
     "yveCRV",
     "yvBOOST",
     18},
    {{0x84, 0x14, 0xdb, 0x07, 0xa7, 0xf7, 0x43, 0xde, 0xba, 0xfb,
      0x40, 0x20, 0x70, 0xab, 0x01, 0xa4, 0xe0, 0xd2, 0xe4, 0x5e},
     "sBTC",
     "yvCurve-sBTC",
     18},
    {{0x70, 0x47, 0xf9, 0x02, 0x29, 0xa0, 0x57, 0xc1, 0x3b, 0xf8,
      0x47, 0xc0, 0x74, 0x4d, 0x64, 0x6c, 0xfb, 0x6c, 0x9e, 0x1a},
     "renBTC",
     "yvCurve-renBTC",
     18},
    {{0xb4, 0xd1, 0xbe, 0x44, 0xbf, 0xf4, 0x0a, 0xd6, 0xe5, 0x06,
      0xed, 0xf4, 0x31, 0x56, 0x57, 0x7a, 0x3f, 0x86, 0x72, 0xec},
     "sAave",
     "yvCurve-sAave",
     18},
    {{0xe9, 0xdc, 0x63, 0x08, 0x3c, 0x46, 0x4d, 0x6e, 0xdc, 0xcf,
      0xf2, 0x34, 0x44, 0xff, 0x3c, 0xfc, 0x68, 0x86, 0xf6, 0xfb},
     "oBTC",
     "yvCurve-oBTC",
     18},
    {{0x3c, 0x5d, 0xf3, 0x07, 0x7b, 0xcf, 0x80, 0x06, 0x40, 0xb5,
      0xda, 0xe8, 0xc9, 0x11, 0x06, 0x57, 0x5a, 0x48, 0x26, 0xe6},
     "pBTC",
     "yvCurve-pBTC",
     18},
    {{0x5f, 0xa5, 0xb6, 0x2c, 0x8a, 0xf8, 0x77, 0xcb, 0x37, 0x03,
      0x1e, 0x0a, 0x3b, 0x2f, 0x34, 0xa7, 0x8e, 0x3c, 0x56, 0xa6},
     "LUSD",
     "yvCurve-LUSD",
     18},
    {{0x8f, 0xa3, 0xa9, 0xec, 0xd9, 0xef, 0xb0, 0x7a, 0x8c, 0xe9,
      0x0a, 0x6e, 0xb0, 0x14, 0xcf, 0x3c, 0x0e, 0x3b, 0x32, 0xef},
     "BBTC",
     "yvCurve-BBTC",
     18},
    {{0x23, 0xd3, 0xd0, 0xf1, 0xc6, 0x97, 0x24, 0x7d, 0x5e, 0x0a,
      0x9e, 0xfb, 0x37, 0xd8, 0xb0, 0xed, 0x0c, 0x46, 0x4f, 0x7f},
     "tBTC",
     "yvCurve-tBTC",
     18},
    {{0x7d, 0xa9, 0x6a, 0x38, 0x91, 0xad, 0xd0, 0x58, 0xad, 0xa2,
      0xe8, 0x26, 0x30, 0x6d, 0x81, 0x2c, 0x63, 0x8d, 0x87, 0xa7},
     "USDT",
     "yvUSDT",
     6},
    {{0xa6, 0x96, 0xa6, 0x3c, 0xc7, 0x8d, 0xff, 0xa1, 0xa6, 0x3e,
      0x9e, 0x50, 0x58, 0x7c, 0x19, 0x73, 0x87, 0xff, 0x6c, 0x7e},
     "WBTC",
     "yvWBTC",
     8},
    {{0xb4, 0xad, 0xa6, 0x07, 0xb9, 0xd6, 0xb2, 0xc9, 0xee, 0x07,
      0xa2, 0x75, 0xe9, 0x61, 0x6b, 0x84, 0xac, 0x56, 0x01, 0x39},
     "FRAX",
     "yvCurve-FRAX",
     18},
    {{0xfb, 0xeb, 0x78, 0xa7, 0x23, 0xb8, 0x08, 0x7f, 0xd2, 0xea,
      0x7e, 0xf1, 0xaf, 0xec, 0x93, 0xd3, 0x5e, 0x8b, 0xed, 0x42},
     "UNI",
     "yvUNI",
     18},
    {{0x8e, 0xe5, 0x7c, 0x05, 0x74, 0x1a, 0xa9, 0xdb, 0x94, 0x7a,
      0x74, 0x4e, 0x71, 0x3c, 0x15, 0xd4, 0xd1, 0x9d, 0x88, 0x22},
     "yBUSD",
     "yvCurve-yBUSD",
     18},
    {{0xd6, 0xea, 0x40, 0x59, 0x7b, 0xe0, 0x5c, 0x20, 0x18, 0x45,
      0xc0, 0xbf, 0xd2, 0xe9, 0x6a, 0x60, 0xba, 0xcd, 0xe2, 0x67},
     "COMP",
     "yvCurve-Compound",
     18},
    {{0x2a, 0x38, 0xb9, 0xb0, 0x20, 0x1c, 0xa3, 0x9b, 0x17, 0xb4,
      0x60, 0xed, 0x2f, 0x11, 0xe4, 0x92, 0x95, 0x59, 0x07, 0x1e},
     "GUSD",
     "yvCurve-GUSD",
     18},
    {{0x4b, 0x5b, 0xfd, 0x52, 0x12, 0x47, 0x84, 0x74, 0x5c, 0x10,
      0x71, 0xdc, 0xb2, 0x44, 0xc6, 0x68, 0x8d, 0x25, 0x33, 0xd3},
     "y",
     "yUSD",
     18},
    {{0x84, 0xe1, 0x37, 0x85, 0xb5, 0xa2, 0x78, 0x79, 0x92, 0x1d,
      0x6f, 0x68, 0x5f, 0x04, 0x14, 0x21, 0xc7, 0xf4, 0x82, 0xda},
     "3pool",
     "yvCurve-3pool",
     18},
    {{0xf8, 0x76, 0x88, 0x14, 0xb8, 0x82, 0x81, 0xde, 0x4f, 0x53,
      0x2a, 0x3b, 0xee, 0xfa, 0x5b, 0x85, 0xb6, 0x9b, 0x93, 0x24},
     "TUSD",
     "yvCurve-TUSD",
     18},
    {{0x6e, 0xde, 0x7f, 0x19, 0xdf, 0x5d, 0xf6, 0xef, 0x23, 0xbd,
      0x5b, 0x9c, 0xed, 0xb6, 0x51, 0x58, 0x0b, 0xdf, 0x56, 0xca},
     "BUSD",
     "yvCurve-BUSD",
     18},
    {{0x30, 0xfc, 0xf7, 0xc6, 0xcd, 0xfc, 0x46, 0xec, 0x23, 0x77,
      0x83, 0xd9, 0x4f, 0xc7, 0x85, 0x53, 0xe7, 0x9d, 0x4e, 0x9c},
     "DUSD",
     "yvCurve-DUSD",
     18},
    {{0x1c, 0x6a, 0x97, 0x83, 0xf8, 0x12, 0xb3, 0xaf, 0x3a, 0xbb,
      0xf7, 0xde, 0x64, 0xc3, 0xcd, 0x7c, 0xc7, 0xd1, 0xaf, 0x44},
     "UST",
     "yvCurve-UST",
     18},
    {{0x8c, 0xc9, 0x4c, 0xcd, 0x0f, 0x38, 0x41, 0xa4, 0x68, 0x18,
      0x4a, 0xca, 0x3c, 0xc4, 0x78, 0xd2, 0x14, 0x8e, 0x17, 0x57},
     "mUSD",
     "yvCurve-mUSD",
     18},
    {{0xa5, 0xca, 0x62, 0xd9, 0x5d, 0x24, 0xa4, 0xa3, 0x50, 0x98,
      0x3d, 0x5b, 0x8a, 0xc4, 0xeb, 0x86, 0x38, 0x88, 0x73, 0x96},
     "sUSD",
     "yvsUSD",
     18},
    {{0xf2, 0x9a, 0xe5, 0x08, 0x69, 0x8b, 0xde, 0xf1, 0x69, 0xb8,
      0x98, 0x34, 0xf7, 0x67, 0x04, 0xc3, 0xb2, 0x05, 0xae, 0xdf},
     "SNX",
     "yvSNX",
     18},
    {{0x5a, 0x77, 0x0d, 0xbd, 0x3e, 0xe6, 0xba, 0xf2, 0x80, 0x2d,
      0x29, 0xa9, 0x01, 0xef, 0x11, 0x50, 0x1c, 0x44, 0x79, 0x7a},
     "sUSD",
     "yvCurve-sUSD",
     18},
    {{0xf2, 0xdb, 0x9a, 0x7c, 0x0a, 0xcd, 0x42, 0x7a, 0x68, 0x0d,
      0x64, 0x0f, 0x02, 0xd9, 0x0f, 0x61, 0x86, 0xe7, 0x17, 0x25},
     "LINK",
     "yvCurve-LINK",
     18},
    {{0x3b, 0x96, 0xd4, 0x91, 0xf0, 0x67, 0x91, 0x2d, 0x18, 0x56,
      0x3d, 0x56, 0x85, 0x8b, 0xa7, 0xd6, 0xec, 0x67, 0xa6, 0xfa},
     "USDN",
     "yvCurve-USDN",
     18},
    {{0xc4, 0xda, 0xf3, 0xb5, 0xe2, 0xa9, 0xe9, 0x38, 0x61, 0xc3,
      0xfb, 0xdd, 0x25, 0xf1, 0xe9, 0x43, 0xb8, 0xd8, 0x74, 0x17},
     "USDP",
     "yvCurve-USDP",
     18},
    {{0xa7, 0x4d, 0x4b, 0x67, 0xb3, 0x36, 0x8e, 0x83, 0x79, 0x7a,
      0x35, 0x38, 0x2a, 0xfb, 0x77, 0x6b, 0xaa, 0xe4, 0xf5, 0xc8},
     "alUSD",
     "yvCurve-alUSD",
     18},
    {{0xbf, 0xed, 0xbc, 0xbe, 0x27, 0x17, 0x1c, 0x41, 0x8c, 0xda,
      0xbc, 0x24, 0x77, 0x04, 0x25, 0x54, 0xb1, 0x90, 0x48, 0x57},
     "rETH",
     "yvCurve-rETH",
     18},
    {{0x13, 0x2d, 0x8d, 0x2c, 0x76, 0xdb, 0x38, 0x12, 0x40, 0x34,
      0x31, 0xfa, 0xcb, 0x00, 0xf3, 0x45, 0x3f, 0xc4, 0x21, 0x25},
     "ankrETH",
     "yvCurve-ankrETH",
     18},
    {{0x39, 0xca, 0xf1, 0x3a, 0x10, 0x4f, 0xf5, 0x67, 0xf7, 0x1f,
      0xd2, 0xa4, 0xc6, 0x8c, 0x02, 0x6f, 0xdb, 0x6e, 0x74, 0x0b},
     "Aave",
     "yvCurve-Aave",
     18},
    {{0x05, 0x4a, 0xf2, 0x2e, 0x15, 0x19, 0xb0, 0x20, 0x51, 0x6d,
      0x72, 0xd7, 0x49, 0x22, 0x1c, 0x24, 0x75, 0x63, 0x85, 0xc9},
     "HUSD",
     "yvCurve-HUSD",
     18},
    {{0x25, 0x21, 0x2d, 0xf2, 0x90, 0x73, 0xff, 0xfa, 0x7a, 0x67,
      0x39, 0x9a, 0xce, 0xfc, 0x2d, 0xd7, 0x5a, 0x83, 0x1a, 0x1a},
     "EURS",
     "yvCurve-EURS",
     18},
    {{0x67, 0x1a, 0x91, 0x2c, 0x10, 0xbb, 0xa0, 0xcf, 0xa7, 0x4c,
      0xfc, 0x2d, 0x6f, 0xba, 0x9b, 0xa1, 0xed, 0x95, 0x30, 0xb2},
     "LINK",
     "yvLINK",
     18},
    {{0x87, 0x3f, 0xb5, 0x44, 0x27, 0x7f, 0xd7, 0xb9, 0x77, 0xb1,
      0x96, 0xa8, 0x26, 0x45, 0x9a, 0x69, 0xe2, 0x7e, 0xa4, 0xea},
     "RAI",
     "yvRAI",
     18},
    {{0xa2, 0x58, 0xc4, 0x60, 0x6c, 0xa8, 0x20, 0x6d, 0x8a, 0xa7,
      0x00, 0xce, 0x21, 0x43, 0xd7, 0xdb, 0x85, 0x4d, 0x16, 0x8c},
     "WETH",
     "yvWETH",
     18},
    {{0x3d, 0x98, 0x0e, 0x50, 0x50, 0x8c, 0xfd, 0x41, 0xa1, 0x38,
      0x37, 0xa6, 0x01, 0x49, 0x92, 0x7a, 0x11, 0xc0, 0x37, 0x31},
     "triCrypto",
     "yvCurve-triCrypto",
     18},
    {{0x80, 0xbb, 0xee, 0x2f, 0xa4, 0x60, 0xda, 0x29, 0x1e, 0x79,
      0x6b, 0x90, 0x45, 0xe9, 0x3d, 0x19, 0xef, 0x94, 0x8c, 0x6a},
     "Pax",
     "yvCurve-Pax",
     18},
    {{0x28, 0xa5, 0xb9, 0x5c, 0x10, 0x1d, 0xf3, 0xde, 0xd0, 0xc0,
      0xd9, 0x07, 0x4d, 0xb8, 0x0c, 0x43, 0x87, 0x74, 0xb6, 0xa9},
     "USDT",
     "yvCurve-USDT",
     18},
    {{0x3d, 0x27, 0x70, 0x5c, 0x64, 0x21, 0x3a, 0x5d, 0xcd, 0x9d,
      0x26, 0x88, 0x0c, 0x1b, 0xcf, 0xa7, 0x2d, 0x5b, 0x6b, 0x0e},
     "USDK",
     "yvCurve-USDK",
     18},
    {{0xc1, 0x16, 0xdf, 0x49, 0xc0, 0x2c, 0x5f, 0xd1, 0x47, 0xde,
      0x25, 0xba, 0xa1, 0x05, 0x32, 0x2e, 0xbf, 0x26, 0xbd, 0x97},
     "RSV",
     "yvCurve-RSV",
     18},
    {{0xda, 0x81, 0x64, 0x59, 0xf1, 0xab, 0x56, 0x31, 0x23, 0x2f,
      0xe5, 0xe9, 0x7a, 0x05, 0xbb, 0xbb, 0x94, 0x97, 0x0c, 0x95},
     "DAI",
     "yvDAI",
     18},
    {{0xe5, 0x37, 0xb5, 0xcc, 0x15, 0x8e, 0xb7, 0x10, 0x37, 0xd4,
      0x12, 0x5b, 0xdd, 0x75, 0x38, 0x42, 0x19, 0x81, 0xe6, 0xaa},
     "3Crypto",
     "yvCurve-3Crypto",
     18},
    {{0xd9, 0x78, 0x8f, 0x39, 0x31, 0xed, 0xe4, 0xd5, 0x01, 0x81,
      0x84, 0xe1, 0x98, 0x69, 0x9d, 0xc6, 0xd6, 0x6c, 0x19, 0x15},
     "AAVE",
     "yvAAVE",
     18},
    {{0x4a, 0x3f, 0xe7, 0x57, 0x62, 0x01, 0x7d, 0xb0, 0xed, 0x73,
      0xa7, 0x1c, 0x9a, 0x06, 0xdb, 0x77, 0x68, 0xdb, 0x5e, 0x66},
     "COMP",
     "yvCOMP",
     18},
    {{0x6d, 0x76, 0x5c, 0xbe, 0x5b, 0xc9, 0x22, 0x69, 0x4a, 0xfe,
      0x11, 0x2c, 0x14, 0x0b, 0x88, 0x78, 0xb9, 0xfb, 0x03, 0x90},
     "SUSHI",
     "yvSUSHI",
     18},
    {{0xfd, 0x08, 0x77, 0xd9, 0x09, 0x57, 0x89, 0xca, 0xf2, 0x4c,
      0x98, 0xf7, 0xcc, 0xe0, 0x92, 0xfa, 0x8e, 0x12, 0x07, 0x75},
     "TUSD",
     "yvTUSD",
     18},
    {{0x0d, 0x4e, 0xa8, 0x53, 0x6f, 0x9a, 0x13, 0xe4, 0xfb, 0xa1,
      0x60, 0x42, 0xa4, 0x6c, 0x30, 0xf0, 0x92, 0xb0, 0x6a, 0xa5},
     "EURT",
     "yvCurve-EURT",
     18},
    {{0x2d, 0xfb, 0x14, 0xe3, 0x2e, 0x2f, 0x81, 0x56, 0xec, 0x15,
      0xa2, 0xc2, 0x1c, 0x3a, 0x6c, 0x05, 0x3a, 0xf5, 0x2b, 0xe8},
     "MIM",
     "yvCurve-MIM",
     18},
    {{0x45, 0x60, 0xb9, 0x9c, 0x90, 0x4a, 0xad, 0x03, 0x02, 0x7b,
      0x51, 0x78, 0xcc, 0xa8, 0x15, 0x84, 0x74, 0x4a, 0xc0, 0x1f},
     "cvxCRV",
     "yvCurve-cvxCRV",
     18},
    {{0x67, 0xe0, 0x19, 0xbf, 0xbd, 0x5a, 0x67, 0x20, 0x77, 0x55,
      0xd0, 0x44, 0x67, 0xd6, 0xa7, 0x0c, 0x0b, 0x75, 0xbf, 0x60},
     "ibEUR",
     "yvCurve-ibEUR",
     18},
    {{0x52, 0x8d, 0x50, 0xdc, 0x9a, 0x33, 0x3f, 0x01, 0x54, 0x41,
      0x77, 0xa9, 0x24, 0x89, 0x3f, 0xa1, 0xf5, 0xb9, 0xf7, 0x48},
     "ibKRW",
     "yvCurve-ibKRW",
     18},
    {{0x59, 0x5a, 0x68, 0xa8, 0xc9, 0xd5, 0xc2, 0x30, 0x00, 0x18,
      0x48, 0xb6, 0x9b, 0x19, 0x47, 0xee, 0x2a, 0x60, 0x71, 0x64},
     "ibGBP",
     "yvCurve-ibGBP",
     18},
    {{0x1b, 0x90, 0x53, 0x31, 0xf7, 0xde, 0x27, 0x48, 0xf4, 0xd6,
      0xa0, 0x67, 0x8e, 0x15, 0x21, 0xe2, 0x03, 0x47, 0x64, 0x3f},
     "ibAUD",
     "yvCurve-ibAUD",
     18},
    {{0x49, 0x0b, 0xd0, 0x88, 0x6f, 0x22, 0x1a, 0x5f, 0x79, 0x71,
      0x3d, 0x3e, 0x84, 0x40, 0x43, 0x55, 0xa9, 0x29, 0x3c, 0x50},
     "ibCHF",
     "yvCurve-ibCHF",
     18},
    {{0x59, 0x51, 0x88, 0x84, 0xee, 0xbf, 0xb0, 0x3e, 0x90, 0xa1,
      0x8a, 0xdb, 0xaa, 0xab, 0x77, 0x0d, 0x46, 0x66, 0x47, 0x1e},
     "ibJPY",
     "yvCurve-ibJPY",
     18},
    {{0xdb, 0x25, 0xca, 0x70, 0x31, 0x81, 0xe7, 0x48, 0x4a, 0x15,
      0x5d, 0xd6, 0x12, 0xb0, 0x6f, 0x57, 0xe1, 0x2b, 0xe5, 0xf0},
     "YFI",
     "yvYFI",
     18},
    {{0x6f, 0xaf, 0xca, 0x7f, 0x49, 0xb4, 0xfd, 0x9d, 0xc3, 0x81,
      0x17, 0x46, 0x9c, 0xd3, 0x1a, 0x1e, 0x5a, 0xec, 0x91, 0xf5},
     "USDM",
     "yvCurve-USDM",
     18},
    {{0x71, 0x8a, 0xbe, 0x90, 0x77, 0x7f, 0x5b, 0x77, 0x8b, 0x52,
      0xd5, 0x53, 0xa5, 0xab, 0xaa, 0x14, 0x8d, 0xd0, 0xdc, 0x5d},
     "alETH",
     "yvCurve-alETH",
     18},
    {{0x8b, 0x9c, 0x0c, 0x24, 0x30, 0x73, 0x44, 0xb6, 0xd7, 0x94,
      0x1a, 0xb6, 0x54, 0xb2, 0xae, 0xee, 0x25, 0x34, 0x74, 0x73},
     "EURN",
     "yvCurve-EURN",
     18},
    {{0xd8, 0xc6, 0x20, 0x99, 0x1b, 0x8e, 0x62, 0x6c, 0x09, 0x9e,
      0xaa, 0xb2, 0x9b, 0x1e, 0x3e, 0xea, 0x27, 0x97, 0x63, 0xbb},
     "MIMUST",
     "yvCurve-MIMUST",
     18},
    {{0xc5, 0xbd, 0xdf, 0x98, 0x43, 0x30, 0x83, 0x80, 0x37, 0x5a,
      0x61, 0x1c, 0x18, 0xb5, 0x0f, 0xb9, 0x34, 0x1f, 0x50, 0x2a},
     "CRV",
     "yveCRV",
     18},
    {{0xda, 0x48, 0x1b, 0x27, 0x7d, 0xce, 0x30, 0x5b, 0x97, 0xf4,
      0x09, 0x1b, 0xd6, 0x65, 0x95, 0xd5, 0x7c, 0xf3, 0x16, 0x34},
     "Pickle SLP",
     "pSLPyvBOOST-ETH",
     18},

    {{0xa3, 0x54, 0xf3, 0x58, 0x29, 0xae, 0x97, 0x5e, 0x85, 0x0e,
      0x23, 0xe9, 0x61, 0x5b, 0x11, 0xda, 0x1b, 0x3d, 0xc4, 0xde},
     "USDC",
     "yvUSDC",
     6},
    {{0xf5, 0x9d, 0x66, 0xc1, 0xd5, 0x93, 0xfb, 0x10, 0xe2, 0xf8,
      0xc2, 0xa6, 0xfd, 0x2c, 0x95, 0x87, 0x92, 0x43, 0x4b, 0x9c},
     "OUSD3CRV-f",
     "yvCurve-OUSD",
     18},
    {{0x5e, 0x69, 0xe8, 0xb5, 0x1b, 0x71, 0xc8, 0x59, 0x68, 0x17,
      0xfd, 0x44, 0x28, 0x49, 0xbd, 0x44, 0x21, 0x9b, 0xb0, 0x95},
     "ibBTC",
     "yvCurve-ibBTC",
     18},
    {{0x16, 0x82, 0x50, 0x39, 0xdf, 0xe2, 0xa5, 0xb0, 0x1f, 0x3e,
      0x1e, 0x6a, 0x2b, 0xbf, 0x9a, 0x57, 0x6c, 0x6f, 0x95, 0xc4},
     "D3-f",
     "yvCurve-d3pool",
     18},
    {{0xbc, 0xbb, 0x5b, 0x54, 0xfa, 0x51, 0xe7, 0xb7, 0xdc, 0x92,
      0x03, 0x40, 0x04, 0x3b, 0x20, 0x34, 0x47, 0x84, 0x2a, 0x6b},
     "crvEURTUSD",
     "yvCurve-EURTUSD",
     18},
    {{0x80, 0x1a, 0xb0, 0x61, 0x54, 0xbf, 0x53, 0x9d, 0xea, 0x43,
      0x85, 0xa3, 0x9f, 0x5f, 0xa8, 0x53, 0x4f, 0xb5, 0x30, 0x73},
     "crvEURSUSDC",
     "yvCurve-EURSUSDC",
     18},
    {{0x37, 0x8c, 0xb5, 0x2b, 0x00, 0xf9, 0xd0, 0x92, 0x1c, 0xb4,
      0x6d, 0xfc, 0x09, 0x9c, 0xff, 0x73, 0xb4, 0x24, 0x19, 0xdc},
     "LUSD",
     "yvLUSD",
     18},
};

const yearnVaultDefinition_t IRON_BANK[NUM_IRON_BANK] = {
    {{0x41, 0xc8, 0x4c, 0x0e, 0x2E, 0xE0, 0xb7, 0x40, 0xCf, 0x0d,
      0x31, 0xF6, 0x3f, 0x3B, 0x6F, 0x62, 0x7D, 0xC6, 0xb3, 0x93},
     "WETH",
     "cyWETH",
     18},
    {{0x8e, 0x59, 0x54, 0x70, 0xEd, 0x74, 0x9b, 0x85, 0xC6, 0xF7,
      0x66, 0x9d, 0xe8, 0x3E, 0xAe, 0x30, 0x4C, 0x2e, 0xc6, 0x8F},
     "DAI",
     "cyDAI",
     18},
    {{0x75, 0x89, 0xC9, 0xE1, 0x7B, 0xCF, 0xcE, 0x1C, 0xca, 0xa1,
      0xf9, 0x21, 0x19, 0x6F, 0xDa, 0x17, 0x7F, 0x02, 0x07, 0xFc},
     "Y3CRV",
     "cyY3CRV",
     18},
    {{0xE7, 0xBF, 0xf2, 0xDa, 0x8A, 0x2f, 0x61, 0x9c, 0x25, 0x86,
      0xFB, 0x83, 0x93, 0x8F, 0xa5, 0x6C, 0xE8, 0x03, 0xaA, 0x16},
     "LINK",
     "cyLINK",
     18},
    {{0xFa, 0x34, 0x72, 0xf7, 0x31, 0x94, 0x77, 0xc9, 0xbF, 0xEc,
      0xdD, 0x66, 0xE4, 0xB9, 0x48, 0x56, 0x9E, 0x76, 0x21, 0xb9},
     "YFI",
     "cyYFI",
     18},
    {{0x12, 0xA9, 0xcC, 0x33, 0xA9, 0x80, 0xDA, 0xa7, 0x4E, 0x00,
      0xcc, 0x2d, 0x1A, 0x0E, 0x74, 0xC5, 0x7A, 0x93, 0xd1, 0x2C},
     "SNX",
     "cySNX",
     18},
    {{0x8F, 0xc8, 0xBF, 0xD8, 0x0d, 0x6A, 0x9F, 0x17, 0xFb, 0x98,
      0xA3, 0x73, 0x02, 0x3d, 0x72, 0x53, 0x17, 0x92, 0xB4, 0x31},
     "WBTC",
     "cyWBTC",
     8},
    {{0x48, 0x75, 0x9F, 0x22, 0x0E, 0xD9, 0x83, 0xdB, 0x51, 0xfA,
      0x7A, 0x8C, 0x0D, 0x2A, 0xAb, 0x8f, 0x3c, 0xe4, 0x16, 0x6a},
     "USDT",
     "cyUSDT",
     6},
    {{0x76, 0xEb, 0x2F, 0xE2, 0x8b, 0x36, 0xB3, 0xee, 0x97, 0xF3,
      0xAd, 0xae, 0x0C, 0x69, 0x60, 0x6e, 0xeD, 0xB2, 0xA3, 0x7c},
     "USDC",
     "cyUSDC",
     6},
    {{0xBE, 0x86, 0xe8, 0x91, 0x8D, 0xFc, 0x7d, 0x3C, 0xb1, 0x0d,
      0x29, 0x5f, 0xc2, 0x20, 0xF9, 0x41, 0xA1, 0x47, 0x0C, 0x5c},
     "MUSD",
     "cyMUSD",
     18},
    {{0x29, 0x7d, 0x4D, 0xa7, 0x27, 0xfb, 0xC6, 0x29, 0x25, 0x28,
      0x45, 0xE9, 0x65, 0x38, 0xFC, 0x46, 0x16, 0x7e, 0x45, 0x3A},
     "DUSD",
     "cyDUSD",
     18},
    {{0xA8, 0xca, 0xeA, 0x56, 0x48, 0x11, 0xaf, 0x0e, 0x92, 0xb1,
      0xE0, 0x44, 0xf3, 0xeD, 0xd1, 0x8F, 0xa9, 0xa7, 0x3E, 0x4F},
     "EURS",
     "cyEURS",
     2},
    {{0xCA, 0x55, 0xF9, 0xC4, 0xE7, 0x7f, 0x7B, 0x85, 0x24, 0x17,
      0x85, 0x83, 0xb0, 0xf7, 0xc7, 0x98, 0xDe, 0x17, 0xfD, 0x54},
     "SEUR",
     "cySEUR",
     18},
    {{0x77, 0x36, 0xFf, 0xb0, 0x71, 0x04, 0xc0, 0xC4, 0x00, 0xBb,
      0x0C, 0xC9, 0xA7, 0xC2, 0x28, 0x45, 0x2A, 0x73, 0x29, 0x92},
     "DPI",
     "cyDPI",
     18},
    {{0x09, 0xbD, 0xCC, 0xe2, 0x59, 0x3f, 0x0B, 0xEF, 0x09, 0x91,
      0x18, 0x8c, 0x25, 0xFb, 0x74, 0x48, 0x97, 0xB6, 0x57, 0x2d},
     "BUSD",
     "cyBUSD",
     18},
    {{0x9D, 0x02, 0x9C, 0xD7, 0xCE, 0xdc, 0xB1, 0x94, 0xE2, 0xC3,
      0x61, 0x94, 0x8f, 0x27, 0x9f, 0x17, 0x88, 0x13, 0x5B, 0xB2},
     "CREAM",
     "cyCREAM",
     18},
    {{0x4F, 0x12, 0xc9, 0xDA, 0xBB, 0x53, 0x19, 0xA2, 0x52, 0x46,
      0x3E, 0x60, 0x28, 0xCA, 0x83, 0x3f, 0x11, 0x64, 0xd0, 0x45},
     "CDAI",
     "cyCDAI",
     18},
    {{0xBB, 0x4B, 0x06, 0x7c, 0xc6, 0x12, 0x49, 0x49, 0x14, 0xA9,
      0x02, 0x21, 0x7C, 0xB6, 0x07, 0x8a, 0xB4, 0x72, 0x8E, 0x36},
     "CUSDT",
     "cyCUSDT",
     8},
    {{0x95, 0x00, 0x27, 0x63, 0x2F, 0xbD, 0x6a, 0xDA, 0xdF, 0xe8,
      0x26, 0x44, 0xBf, 0xB6, 0x46, 0x47, 0x64, 0x2B, 0x6C, 0x09},
     "CUSDC",
     "cyCUSDC",
     8},
    {{0xa7, 0xc4, 0x05, 0x4A, 0xFD, 0x3D, 0xbB, 0xbF, 0x5b, 0xFe,
      0x80, 0xf4, 0x18, 0x62, 0xb8, 0x9e, 0xa0, 0x5c, 0x98, 0x06},
     "SUSD",
     "cySUSD",
     18},
    {{0x1b, 0x3E, 0x95, 0xE8, 0xEC, 0xF7, 0xA7, 0xca, 0xB6, 0xc4,
      0xDe, 0x1b, 0x34, 0x4F, 0x94, 0x86, 0x5a, 0xBD, 0x12, 0xd5},
     "CHF",
     "cyCHF",
     18},
    {{0xFE, 0xEB, 0x92, 0x38, 0x6A, 0x05, 0x5E, 0x2e, 0xF7, 0xC2,
      0xB5, 0x98, 0xc8, 0x72, 0xa4, 0x04, 0x7a, 0x7d, 0xB5, 0x9F},
     "UNI",
     "cyUNI",
     18},
    {{0x22, 0x6F, 0x37, 0x38, 0x23, 0x89, 0x32, 0xBA, 0x0d, 0xB2,
      0x31, 0x9a, 0x81, 0x17, 0xD9, 0x55, 0x54, 0x46, 0x10, 0x2f},
     "SUSHI",
     "cySUSHI",
     18},
    {{0x30, 0x19, 0x0a, 0x3B, 0x52, 0xb5, 0xAB, 0x1d, 0xaF, 0x70,
      0xD4, 0x6D, 0x72, 0x53, 0x6F, 0x51, 0x71, 0xf2, 0x23, 0x40},
     "AAVE",
     "cyAAVE",
     18},
    {{0x00, 0xe5, 0xc0, 0x77, 0x4A, 0x5F, 0x06, 0x5c, 0x28, 0x50,
      0x68, 0x17, 0x0b, 0x20, 0x39, 0x39, 0x25, 0xC8, 0x4B, 0xF3},
     "EUR",
     "cyEUR",
     18},
    {{0xB8, 0xc5, 0xaf, 0x54, 0xbb, 0xDC, 0xc6, 0x14, 0x53, 0x14,
      0x4C, 0xF4, 0x72, 0xA9, 0x27, 0x6a, 0xE3, 0x61, 0x09, 0xF9},
     "CRV",
     "cyCRV",
     18},
    {{0x3c, 0x9f, 0x53, 0x85, 0xc2, 0x88, 0xcE, 0x43, 0x8E, 0xd5,
      0x56, 0x20, 0x93, 0x8A, 0x4B, 0x96, 0x7c, 0x08, 0x01, 0x01},
     "KRW",
     "cyKRW",
     18},
    {{0x21, 0x5F, 0x34, 0xaf, 0x65, 0x57, 0xA6, 0x59, 0x8D, 0xbd,
      0xA9, 0xaa, 0x11, 0xcc, 0x55, 0x6F, 0x5A, 0xE2, 0x64, 0xB1},
     "JPY",
     "cyJPY",
     18},
    {{0x86, 0xBB, 0xD9, 0xac, 0x8B, 0x9B, 0x44, 0xC9, 0x5F, 0xFc,
      0x6B, 0xAA, 0xe5, 0x8E, 0x25, 0x03, 0x3B, 0x75, 0x48, 0xAA},
     "AUD",
     "cyAUD",
     18},
    {{0xec, 0xaB, 0x2C, 0x76, 0xf1, 0xA8, 0x35, 0x9A, 0x06, 0xfA,
      0xB5, 0xfA, 0x0C, 0xEe, 0xa5, 0x12, 0x80, 0xA9, 0x7e, 0xCF},
     "GBP",
     "cyGBP",
     18},
    {{0x9e, 0x8E, 0x20, 0x70, 0x83, 0xff, 0xd5, 0xBD, 0xc3, 0xD9,
      0x9A, 0x1F, 0x32, 0xD1, 0xe6, 0x25, 0x08, 0x69, 0xC1, 0xA9},
     "MIM",
     "cyMIM",
     18},
    {{0x67, 0x24, 0x73, 0x90, 0x85, 0x87, 0xb1, 0x0e, 0x65, 0xDA,
      0xB1, 0x77, 0xDb, 0xae, 0xad, 0xcb, 0xb3, 0x0B, 0xF4, 0x0B},
     "ZAR",
     "cyZAR",
     18}};

// Function to dispatch calls from the ethereum app.
void dispatch_plugin_calls(int message, void *parameters) {
    switch (message) {
        case ETH_PLUGIN_INIT_CONTRACT:
            handle_init_contract(parameters);
            break;
        case ETH_PLUGIN_PROVIDE_PARAMETER:
            handle_provide_parameter(parameters);
            break;
        case ETH_PLUGIN_FINALIZE:
            handle_finalize(parameters);
            break;
        case ETH_PLUGIN_PROVIDE_INFO:
            handle_provide_token(parameters);
            break;
        case ETH_PLUGIN_QUERY_CONTRACT_ID:
            handle_query_contract_id(parameters);
            break;
        case ETH_PLUGIN_QUERY_CONTRACT_UI:
            handle_query_contract_ui(parameters);
            break;
        default:
            PRINTF("Unhandled message %d\n", message);
            break;
    }
}

// Calls the ethereum app.
void call_app_ethereum() {
    unsigned int libcall_params[3];
    libcall_params[0] = (unsigned int) "Ethereum";
    libcall_params[1] = 0x100;
    libcall_params[2] = RUN_APPLICATION;
    os_lib_call((unsigned int *) &libcall_params);
}

// Weird low-level black magic. No need to edit this.
__attribute__((section(".boot"))) int main(int arg0) {
    // Exit critical section
    __asm volatile("cpsie i");

    // Ensure exception will work as planned
    os_boot();

    // Try catch block. Please read the docs for more information on how to use those!
    BEGIN_TRY {
        TRY {
            // Low-level black magic.
            check_api_level(CX_COMPAT_APILEVEL);

            // Check if we are called from the dashboard.
            if (!arg0) {
                // Called from dashboard, launch Ethereum app
                call_app_ethereum();
                return 0;
            } else {
                // Not called from dashboard: called from the ethereum app!
                unsigned int *args = (unsigned int *) arg0;

                // If `ETH_PLUGIN_CHECK_PRESENCE` is set, this means the caller is just trying to
                // know whether this app exists or not. We can skip `dispatch_plugin_calls`.
                if (args[0] != ETH_PLUGIN_CHECK_PRESENCE) {
                    dispatch_plugin_calls(args[0], (void *) args[1]);
                }

                // Call `os_lib_end`, go back to the ethereum app.
                os_lib_end();
            }
        }
        FINALLY {
        }
    }
    END_TRY;

    // Will not get reached.
    return 0;
}
