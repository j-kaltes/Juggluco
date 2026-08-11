/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2(+), Libre 3(+), Dexcom G7/ONE+,        */
/*      Sibionics GS1Sb and GS3, Accu-Chek SmartGuide, CareSens Air and              */
/*      Aidex X sensors.                                                             */
/*                                                                                   */
/*      Copyright (C) 2021 Jaap Korthals Altes <jaapkorthalsaltes@gmail.com>         */
/*                                                                                   */
/*      Juggluco is free software: you can redistribute it and/or modify             */
/*      it under the terms of the GNU General Public License as published            */
/*      by the Free Software Foundation, either version 3 of the License, or         */
/*      (at your option) any later version.                                          */
/*                                                                                   */
/*      Juggluco is distributed in the hope that it will be useful, but              */
/*      WITHOUT ANY WARRANTY; without even the implied warranty of                   */
/*      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                         */
/*      See the GNU General Public License for more details.                         */
/*                                                                                   */
/*      You should have received a copy of the GNU General Public License            */
/*      along with Juggluco. If not, see <https://www.gnu.org/licenses/>.            */
/*                                                                                   */
/*      Tue Aug 11 16:33:40 CEST 2026                                                */
#include "authorization_round_secondary.h"

#include <stddef.h>
#include <string.h>

static uint32_t u32_add_carry(uint32_t a, uint32_t b) {
    return (uint32_t)(((uint64_t)a + (uint64_t)b) >> 32);
}

static uint32_t u32_mulhi(uint32_t a, uint32_t b) {
    return (uint32_t)(((uint64_t)a * (uint64_t)b) >> 32);
}

static uint32_t u32_mullo(uint32_t a, uint32_t b) {
    return (uint32_t)((uint64_t)a * (uint64_t)b);
}

static uint32_t u32_mla(uint32_t x, uint32_t mul, uint32_t add) {
    return (uint32_t)(x * mul + add);
}

static const l3_authorization_secondary_pair32 k_f41a15f0[16] = {
    {0xec24d5b4u, 0xc1063d2eu}, {0xc8e87e2fu, 0x96b054cbu},
    {0xa5ac26aau, 0x6c5a6c68u}, {0x1c1e4e0cu, 0x351204e8u},
    {0xf8e1f687u, 0x0abc1c84u}, {0x6f541de9u, 0xd373b504u},
    {0x4c17c664u, 0xa91dcca1u}, {0xc289edc6u, 0x71d56520u},
    {0x9f4d9641u, 0x477f7cbdu}, {0x7c113ebcu, 0x1d29945au},
    {0xf283661eu, 0xe5e12cd9u}, {0xcf470e99u, 0xbb8b4476u},
    {0x45b935fbu, 0x8442dcf6u}, {0x227cde76u, 0x59ecf493u},
    {0x98ef05d8u, 0x22a48d12u}, {0x75b2ae53u, 0xf84ea4afu}
};

static const l3_authorization_secondary_pair32 k_f41a1670[16] = {
    {0x01f76432u, 0x1559dc14u}, {0xad2f5282u, 0x0cb26806u},
    {0x586740d2u, 0xf40af3f9u}, {0x7c43e9d9u, 0xece2400au},
    {0x277bd829u, 0xd43acbfdu}, {0x4b588130u, 0xcd12180eu},
    {0xf6906f80u, 0xb46aa400u}, {0x1a6d1887u, 0xad41f012u},
    {0xc5a506d7u, 0x949a7c04u}, {0x70dcf527u, 0x8bf307f7u},
    {0x94b99e2eu, 0x74ca5408u}, {0x3ff18c7eu, 0x6c22dffbu},
    {0x63ce3585u, 0x54fa2c0cu}, {0x0f0623d5u, 0x4c52b7ffu},
    {0x32e2ccdcu, 0x352a0410u}, {0xde1abb2cu, 0x2c829002u}
};

static const uint32_t k_f4190bb0[8] = {
    0x9cec262du, 0x03878413u, 0x0bcabfd7u, 0x3dd5cf75u,
    0xca67dc13u, 0x05955ccbu, 0x1ff0209fu, 0xd22d0657u
};

static const uint32_t k_f4190bd0[8] = {
    0xcabb0cb4u, 0xcab2e0fbu, 0x1fa8cf05u, 0xab2dfd30u,
    0xde4795d9u, 0x97e10eceu, 0xe42f61e0u, 0xff802e35u
};

static const uint32_t k_f4190bf0[8] = {
    0x23890d39u, 0x2fbd2407u, 0x31f71bbbu, 0x063f3761u,
    0xc2ee1c07u, 0xbcedf3dfu, 0x8b08bf63u, 0x7f14283bu
};

static const uint32_t k_f4190c10[8] = {
    0xea669113u, 0x5917637eu, 0x1cdefce0u, 0x25b8c49fu,
    0x9071ff64u, 0x92365a45u, 0x8d573a8fu, 0x1c7e6350u
};

static const l3_authorization_secondary_pair32 k_f41a1970[16] = {
    {0xefd81bc6u, 0x51127cc8u}, {0x340ac0b1u, 0xa0b7529eu},
    {0x783d659cu, 0xf05c2873u}, {0xbc700a87u, 0x4000fe48u},
    {0x00a2af72u, 0x9fa5d41eu}, {0x44d5545du, 0xef4aa9f3u},
    {0x45ddaa99u, 0x34a22274u}, {0x8a104f84u, 0x8446f849u},
    {0xce42f46fu, 0xd3ebce1eu}, {0x1275995au, 0x2390a3f4u},
    {0x56a83e45u, 0x733579c9u}, {0x9adae330u, 0xc2da4f9eu},
    {0xdf0d881bu, 0x127f2573u}, {0x23402d06u, 0x6223fb49u},
    {0x6772d1f1u, 0xb1c8d11eu}, {0xaba576dcu, 0x016da6f3u}
};

static const l3_authorization_secondary_pair32 k_f41a19f0[16] = {
    {0xbd435e30u, 0x81b5da57u}, {0x4dbebcf3u, 0xf17b65cau},
    {0x8809733fu, 0x653715c6u}, {0xc254298bu, 0xd8f2c5c2u},
    {0x52cf884eu, 0x48b85135u}, {0x8d1a3e9au, 0xbc740131u},
    {0x1d959d5du, 0x2c398ca4u}, {0x57e053a9u, 0x9ff53ca0u},
    {0xe85bb26cu, 0x0fbac812u}, {0x22a668b8u, 0x7376780fu},
    {0x5cf11f04u, 0xe732280bu}, {0xed6c7dc7u, 0x56f7b37du},
    {0x27b73413u, 0xcab3637au}, {0xb83292d6u, 0x3a78eeecu},
    {0xf27d4922u, 0xae349ee8u}, {0x82f8a7e5u, 0x1dfa2a5bu}
};

static const uint32_t k_f4190cb0[8] = {
    0xea69e419u, 0xa6d8571fu, 0xf272769bu, 0x1fae71e7u,
    0x3d58d6edu, 0xc5510229u, 0xf9310c9bu, 0x945c2e6du
};

static const uint32_t k_f4190cd0[8] = {
    0xd32ca002u, 0xd4bbca9bu, 0x49b3ee18u, 0x48532051u,
    0xac5484ddu, 0xa677f32cu, 0xdb0358d6u, 0x8a14e36fu
};

static const uint32_t k_f4190cf0[8] = {
    0xbfbac9e1u, 0x2fb8d475u, 0x1a6b9131u, 0xd5cff04du,
    0x945838b7u, 0xf982e9dbu, 0xf6304f19u, 0x2ab19ed5u
};

static const uint32_t k_f4190d10[8] = {
    0xb2d86ff2u, 0xc99c1299u, 0x6affe521u, 0x9888f1ceu,
    0xfe0d9c0bu, 0x1014c2e6u, 0x545a021au, 0xd6f771c4u
};

static const l3_authorization_secondary_pair32 k_f41a1ab0[16] = {
    {0x2b83f32fu, 0x269ca479u}, {0x7dd78f11u, 0xfa8b172eu},
    {0x215da40cu, 0xc3ee134cu}, {0x73b13feeu, 0x97dc8601u},
    {0x173754e9u, 0x613f821fu}, {0xbabd69e4u, 0x3aa27e3cu},
    {0x0d1105c6u, 0x0e90f0f2u}, {0xb0971ac1u, 0xd7f3ed0fu},
    {0x02eab6a3u, 0xabe25fc5u}, {0xa670cb9eu, 0x75455be2u},
    {0xf8c46780u, 0x4933ce97u}, {0x9c4a7c7bu, 0x1296cab5u},
    {0x3fd09176u, 0xebf9c6d3u}, {0x92242d58u, 0xbfe83988u},
    {0x35aa4253u, 0x894b35a6u}, {0x87fdde35u, 0x5d39a85bu}
};

static const l3_authorization_secondary_pair32 k_f41a1b30[16] = {
    {0xd911e73du, 0x97b76cc7u}, {0xc3c53280u, 0x06014e29u},
    {0xc8af8398u, 0x766668ebu}, {0xcd99d4b0u, 0xe6cb83adu},
    {0xd28425c8u, 0x57309e6fu}, {0xd76e76e0u, 0xc795b931u},
    {0xdc58c7f8u, 0x37fad3f3u}, {0xc70c133bu, 0xa644b555u},
    {0xcbf66453u, 0x16a9d017u}, {0xd0e0b56bu, 0x870eead9u},
    {0xd5cb0683u, 0xf774059bu}, {0xdab5579bu, 0x67d9205du},
    {0xc568a2deu, 0xd62301bfu}, {0xca52f3f6u, 0x46881c81u},
    {0xcf3d450eu, 0xb6ed3743u}, {0xd4279626u, 0x27525205u}
};

static const uint32_t k_f4190d70[8] = {
    0x2c554271u, 0x1dcc24cfu, 0x76f18ae3u, 0x1177b359u,
    0xb9bd5ccfu, 0x36cedde7u, 0xa03e294bu, 0x2185b363u
};

static const uint32_t k_f4190d90[8] = {
    0x1c4b60e7u, 0x026abcfau, 0xef7e594cu, 0xbbd38c13u,
    0xf31523b0u, 0x87d95e89u, 0x858ee283u, 0xe30e9c3cu
};

static const uint32_t k_f4190db0[8] = {
    0x307a8ca7u, 0x498eecb3u, 0x35a1d7d7u, 0x21cc039bu,
    0xb4bc7981u, 0xb9319f7du, 0x6eecd52fu, 0x0cc44553u
};

static const uint32_t k_f4190dd0[8] = {
    0xa7bda41fu, 0x93358b30u, 0x6f8ac8e8u, 0x51156323u,
    0x0776044eu, 0x6f3ececbu, 0x5ce64f37u, 0xba5d37ddu
};

static const l3_authorization_secondary_pair32 k_f41a1cf0[16] = {
    {0x8205ef73u, 0xe7d4975du}, {0xf816cc24u, 0xd533901du},
    {0xc26c3948u, 0xc20864ddu}, {0x8cc1a66cu, 0xbedd399du},
    {0x02d2831du, 0xac3c325eu}, {0xcd27f041u, 0x9911071du},
    {0x977d5d65u, 0x85e5dbddu}, {0x0d8e3a16u, 0x7344d49eu},
    {0xd7e3a73au, 0x6019a95du}, {0xa239145eu, 0x5cee7e1du},
    {0x1849f10fu, 0x4a4d76deu}, {0xe29f5e33u, 0x37224b9du},
    {0xacf4cb57u, 0x23f7205du}, {0x774a387bu, 0x10cbf51du},
    {0xed5b152cu, 0x0e2aedddu}, {0xb7b08250u, 0xfaffc29du}
};

static const l3_authorization_secondary_pair32 k_f41a1d70[16] = {
    {0x2fa06374u, 0x393a190du}, {0x95473338u, 0xce71da52u},
    {0x214131dfu, 0x54ccdf90u}, {0xad3b3086u, 0xeb27e4cdu},
    {0x39352f2du, 0x7182ea0bu}, {0x9edbfef1u, 0x06baab50u},
    {0x2ad5fd98u, 0x9d15b08eu}, {0xb6cffc3fu, 0x2370b5cbu},
    {0x1c76cc03u, 0xb8a87711u}, {0xa870caaau, 0x4f037c4eu},
    {0x346ac951u, 0xd55e818cu}, {0x9a119915u, 0x6a9642d1u},
    {0x260b97bcu, 0xf0f1480fu}, {0xb2059663u, 0x874c4d4cu},
    {0x17ac6627u, 0x1c840e92u}, {0xa3a664ceu, 0xa2df13cfu}
};


static const l3_authorization_secondary_pair32 k_f41a1df0[16] = {
    {0x6019e505u, 0x89aed55eu}, {0x9211f8dau, 0x748d4a4fu},
    {0xbdbd006cu, 0x630a4909u}, {0xefb51441u, 0x5de8bdfau},
    {0x21ad2816u, 0x48c732ecu}, {0x4d582fa8u, 0x374431a6u},
    {0x7f50437du, 0x2222a697u}, {0xb1485752u, 0x1d011b88u},
    {0xdcf35ee4u, 0x0b7e1a42u}, {0x0eeb72b9u, 0xf65c8f34u},
    {0x40e3868eu, 0xe13b0425u}, {0x6c8e8e20u, 0xdfb802dfu},
    {0x9e86a1f5u, 0xca9677d0u}, {0xd07eb5cau, 0xb574ecc1u},
    {0x0276c99fu, 0xa05361b3u}, {0x2e21d131u, 0x9ed0606du}
};

static const l3_authorization_secondary_pair32 k_f41a1e70[16] = {
    {0xfe6371f0u, 0xa47770c0u}, {0xb377d4a8u, 0xf0f39ae5u},
    {0xef390c97u, 0x4e9d1b91u}, {0xa44d6f4fu, 0x9b1945b6u},
    {0xe00ea73eu, 0xe8c2c662u}, {0x952309f6u, 0x353ef087u},
    {0xd0e441e5u, 0x82e87133u}, {0x85f8a49du, 0xdf649b58u},
    {0x3b0d0755u, 0x2be0c57du}, {0x76ce3f44u, 0x798a4629u},
    {0x2be2a1fcu, 0xc606704eu}, {0x67a3d9ebu, 0x13aff0fau},
    {0x1cb83ca3u, 0x602c1b1fu}, {0x58797492u, 0xbdd59bcbu},
    {0x0d8dd74au, 0x0a51c5f0u}, {0x494f0f39u, 0x57fb469cu}
};


static const l3_authorization_secondary_pair32 k_f41a1ef0[16] = {
    {0x19a5b417u, 0x49c59d84u}, {0xaba0b24cu, 0x7d58a3bdu},
    {0x3d9bb081u, 0xa0eba9f7u}, {0xafe6cb67u, 0xdb4e4c97u},
    {0x41e1c99cu, 0x0ee152d1u}, {0xd3dcc7d1u, 0x3274590au},
    {0x65d7c606u, 0x66075f44u}, {0xf7d2c43bu, 0x999a657du},
    {0x89cdc270u, 0xcd2d6bb7u}, {0x1bc8c0a5u, 0xf0c071f1u},
    {0xadc3bedau, 0x2453782au}, {0x3fbebd0fu, 0x57e67e64u},
    {0xd1b9bb44u, 0x8b79849du}, {0x63b4b979u, 0xbf0c8ad7u},
    {0xf5afb7aeu, 0xe29f9110u}, {0x87aab5e3u, 0x1632974au}
};

static const l3_authorization_secondary_pair32 k_f41a1f70[16] = {
    {0xef5a19a4u, 0xde05e933u}, {0xe8c88738u, 0x455d8b58u},
    {0xe236f4ccu, 0xbcb52d7du}, {0xa9586f4bu, 0x22396eddu},
    {0xa2c6dcdfu, 0x99911102u}, {0x9c354a73u, 0x00e8b327u},
    {0x95a3b807u, 0x7840554cu}, {0x8f12259bu, 0xef97f771u},
    {0x5633a01au, 0x551c38d1u}, {0x4fa20daeu, 0xcc73daf6u},
    {0x49107b42u, 0x33cb7d1bu}, {0x427ee8d6u, 0xab231f40u},
    {0x3bed566au, 0x127ac165u}, {0x355bc3feu, 0x89d2638au},
    {0xfc7d3e7du, 0xff56a4e9u}, {0xf5ebac11u, 0x66ae470eu}
};

static const uint32_t k_f4190eb0[8] = {
    0x79853a1du, 0xa827cf55u, 0x8de25f37u, 0x059cedf1u,
    0xff0a15dfu, 0xc1655cbfu, 0x473b81a9u, 0x667d2d67u
};

static const uint32_t k_f4190ed0[8] = {
    0x0f11ece5u, 0x900edfcbu, 0xe85e8476u, 0x836ac467u,
    0xae7eab10u, 0x24db2994u, 0x0c057ea5u, 0x5e651177u
};

static const uint32_t k_f4190ef0[8] = {
    0x9e934817u, 0x133d51bfu, 0x853012e5u, 0xc66cd293u,
    0x637354ddu, 0x62abd17du, 0xdbcbb7bbu, 0x965ae175u
};

static const uint32_t k_f4190f10[8] = {
    0xb9ac5158u, 0x728fce8au, 0x865fd8cbu, 0x8080215eu,
    0xd5c4c819u, 0x145e76a5u, 0x35e75698u, 0x127a148eu
};

/* The previously omitted native bridge between the third and fourth
 * FUN_f3fdc45c calls.  Its first pair stream is derived from caller param_6;
 * the second is derived from the third call's 13-word export. */
static const uint32_t k_f4190df0[8] = {
    0x60ef7f17u, 0xd02a6c31u, 0xdb4769f5u, 0x789f4ae9u,
    0x0878a603u, 0x1f9f2c07u, 0xcefbf3f5u, 0x19c07683u
};

static const uint32_t k_f4190e10[8] = {
    0x20ee3d38u, 0x44d5edcfu, 0xd3330a42u, 0xc4db3139u,
    0x2d3084edu, 0x77202aeeu, 0x1eb3be24u, 0xe77b0ebbu
};

static const uint32_t k_f4190e30[8] = {
    0x1a909129u, 0xeca3dffbu, 0xeba6bda7u, 0xc0bff62fu,
    0x6582db41u, 0x360eaed9u, 0x328c714du, 0xb3d8bd7bu
};

static const uint32_t k_f4190e50[8] = {
    0xf5083fb5u, 0x02bb6502u, 0x86c619c0u, 0x91206593u,
    0x371e97d8u, 0x822f59e7u, 0x194506e0u, 0x7eb1b593u
};

static const l3_authorization_secondary_pair32 k_f41a1bb0[16] = {
    {0x726dd4bdu, 0x702427e8u}, {0xa5419081u, 0xe01b8b57u},
    {0xd8154c45u, 0x5012eec6u}, {0x01dd1596u, 0xccf2ad9fu},
    {0x34b0d15au, 0x3cea110eu}, {0x67848d1eu, 0xace1747du},
    {0x914c566fu, 0x19c13355u}, {0xc4201233u, 0x89b896c4u},
    {0xf6f3cdf7u, 0xf9affa33u}, {0x20bb9748u, 0x668fb90cu},
    {0x538f530cu, 0xd6871c7bu}, {0x86630ed0u, 0x467e7feau},
    {0xb02ad821u, 0xb35e3ec2u}, {0xe2fe93e5u, 0x2355a231u},
    {0x15d24fa9u, 0x934d05a1u}, {0x3f9a18fau, 0x002cc479u}
};

static const l3_authorization_secondary_pair32 k_f41a1c30[16] = {
    {0x9856027au, 0xeac35845u}, {0x38dfeea3u, 0x3f88688eu},
    {0xd969daccu, 0x844d78d6u}, {0x79f3c6f5u, 0xd912891fu},
    {0x1a7db31eu, 0x2dd79968u}, {0x62d2b3c2u, 0x7e81a82du},
    {0x035c9febu, 0xc346b876u}, {0xa3e68c14u, 0x180bc8beu},
    {0x4470783du, 0x6cd0d907u}, {0xe4fa6466u, 0xb195e94fu},
    {0x2d4f650au, 0x023ff815u}, {0xcdd95133u, 0x5705085du},
    {0x6e633d5cu, 0xabca18a6u}, {0x0eed2985u, 0xf08f28efu},
    {0xaf7715aeu, 0x45543937u}, {0x500101d7u, 0x9a194980u}
};


static const uint32_t k_f4190f30[8] = {
    0x1407f941u, 0xe6cb359fu, 0x8009c497u, 0xd0160009u,
    0x2b7f33f5u, 0x98cb017bu, 0xeabb14d1u, 0xf66dd825u
};

static const uint32_t k_f4190f50[8] = {
    0xbedfe14du, 0x3d894ce8u, 0xc07e7e23u, 0x7d2713edu,
    0xe3d622bcu, 0x16980fd7u, 0x3291775bu, 0x361217f7u
};

static const uint32_t k_f4190f70[8] = {
    0xf035398fu, 0x249b8277u, 0x7000844du, 0xef6802ebu,
    0x594bec05u, 0x796b95a5u, 0x04c66353u, 0x80eabb5du
};

static const uint32_t k_f4190f90[8] = {
    0xbde8079cu, 0x595944deu, 0xb738fe67u, 0x36cba752u,
    0xe3b131a5u, 0xd1fb1391u, 0xf960c6dcu, 0xee537002u
};


static const uint32_t k_f4190fb0[8] = {
    0x2c24946du, 0xdcde5b8bu, 0xf99c9241u, 0x3fbeb431u,
    0xcd5fc749u, 0x7dc81335u, 0x5e8a4603u, 0xca0bd685u
};

static const uint32_t k_f4190fd0[8] = {
    0xda910d33u, 0x74637eecu, 0x70497f63u, 0xce673e3bu,
    0x309bd453u, 0x0c34bd93u, 0x985d6ec5u, 0x9bb7574eu
};

static const uint32_t k_f4190ff0[8] = {
    0xd889c961u, 0xa572417fu, 0xa4d56f77u, 0xbe4be929u,
    0x222e7a95u, 0xc773e8dbu, 0x16b6d6f1u, 0xe3b124c5u
};

static const uint32_t k_f4191010[8] = {
    0x30a86a72u, 0x6d6d016du, 0x17a71208u, 0xc70ef112u,
    0x1c67b1c1u, 0x8d50fa3cu, 0xcc25f240u, 0xaf61065cu
};


static const l3_authorization_secondary_pair32 k_f41a1ff0[16] = {
    {0x8fc89aa4u, 0x44512c6bu}, {0x69d9eb6du, 0x50343599u},
    {0x8a1a39b3u, 0x6c73ee34u}, {0x642b8a7cu, 0x7856f762u},
    {0x3e3cdb45u, 0x843a0090u}, {0x5e7d298bu, 0x9079b92bu},
    {0x388e7a54u, 0xac5cc259u}, {0x129fcb1du, 0xb83fcb87u},
    {0x32e01963u, 0xc47f8422u}, {0x0cf16a2cu, 0xd0628d50u},
    {0xe702baf5u, 0xec45967du}, {0x0743093bu, 0xf8854f19u},
    {0xe1545a04u, 0x04685846u}, {0xbb65aacdu, 0x104b6174u},
    {0x9576fb96u, 0x2c2e6aa2u}, {0xb5b749dcu, 0x386e233du}
};

static const l3_authorization_secondary_pair32 k_f41a2070[16] = {
    {0xa8bef333u, 0x4fbbb127u}, {0x46542941u, 0x10999a6fu},
    {0xe3e95f4fu, 0xe17783b6u}, {0x3de062f4u, 0xb05a3336u},
    {0xdb759902u, 0x81381c7du}, {0x356c9ca7u, 0x501acbfdu},
    {0xd301d2b5u, 0x20f8b544u}, {0x2cf8d65au, 0xffdb64c4u},
    {0xca8e0c68u, 0xc0b94e0bu}, {0x68234276u, 0x91973753u},
    {0xc21a461bu, 0x6079e6d2u}, {0x5faf7c29u, 0x3157d01au},
    {0xb9a67fceu, 0x003a7f99u}, {0x573bb5dcu, 0xd11868e1u},
    {0xb132b981u, 0xaffb1860u}, {0x4ec7ef8fu, 0x70d901a8u}
};

static const uint32_t k_f4191030[8] = {
    0x76c685bdu, 0xdad437f3u, 0x68dfd075u, 0x7724e49bu,
    0x3c98ada1u, 0xe57e117bu, 0xd0eee817u, 0xcdb83847u
};
static const uint32_t k_f4191050[8] = {
    0xde55f86fu, 0x1579323cu, 0x38940d1cu, 0x70f54a40u,
    0x954b421cu, 0x615c5c15u, 0xe14e07fbu, 0xd1011b6eu
};
static const uint32_t k_f4191070[8] = {
    0xcdcafd57u, 0xa72369b9u, 0x014fa07fu, 0x58ae88b1u,
    0x4cfedb83u, 0xdc3a5b51u, 0x1e5988a5u, 0x95b23735u
};
static const uint32_t k_f4191090[8] = {
    0x3d865a2du, 0x33c8ddb4u, 0xbb2a7a54u, 0x0c826040u,
    0x50373954u, 0x8993e0dfu, 0x1fc3e851u, 0xbe0e148au
};


static const l3_authorization_secondary_pair32 k_f41a2170[16] = {
    {0xe4648bb6u, 0x3bb17855u}, {0xc59b6dc3u, 0x0564fe33u},
    {0x3c89160du, 0xdcda0ae5u}, {0x1dbff81au, 0xa68d90c3u},
    {0xfef6da27u, 0x704116a0u}, {0x75e48271u, 0x47b62352u},
    {0x571b647eu, 0x1169a930u}, {0x3852468bu, 0xeb1d2f0eu},
    {0xaf3feed5u, 0xb2923bbfu}, {0x9076d0e2u, 0x8c45c19du},
    {0x71adb2efu, 0x55f9477bu}, {0xe89b5b39u, 0x2d6e542cu},
    {0xc9d23d46u, 0xf721da0au}, {0xab091f53u, 0xc0d55fe8u},
    {0x21f6c79du, 0x984a6c9au}, {0x032da9aau, 0x61fdf278u}
};

static const l3_authorization_secondary_pair32 k_f41a21f0[16] = {
    {0x5f730a4du, 0x18fe079au}, {0x32e3d072u, 0x4cd881c9u},
    {0x06549697u, 0x70b2fbf8u}, {0xd9c55cbcu, 0xa48d7626u},
    {0xad3622e1u, 0xd867f055u}, {0xc3a2c841u, 0x0d0a898au},
    {0x97138e66u, 0x30e503b9u}, {0x6a84548bu, 0x64bf7de8u},
    {0x3df51ab0u, 0x9899f817u}, {0x1165e0d5u, 0xcc747246u},
    {0x27d28635u, 0xf1170b7bu}, {0xfb434c5au, 0x24f185a9u},
    {0xceb4127fu, 0x58cbffd8u}, {0xa224d8a4u, 0x8ca67a07u},
    {0x75959ec9u, 0xb080f436u}, {0x490664eeu, 0xe45b6e65u}
};


static const l3_authorization_secondary_pair32 k_f41a2270[16] = {
    {0x4400cfdau, 0xe04eeedfu}, {0x785b7ceau, 0x96995380u},
    {0x107930b1u, 0x47a76468u}, {0x44d3ddc1u, 0xfdf1c909u},
    {0x792e8ad1u, 0xa43c2daau}, {0x114c3e98u, 0x554a3e92u},
    {0x45a6eba8u, 0x0b94a333u}, {0xddc49f6fu, 0xbca2b41au},
    {0x121f4c7fu, 0x62ed18bcu}, {0xaa3d0046u, 0x13fb29a3u},
    {0xde97ad56u, 0xca458e44u}, {0x12f25a66u, 0x708ff2e6u},
    {0xab100e2du, 0x219e03cdu}, {0xdf6abb3du, 0xd7e8686eu},
    {0x77886f04u, 0x88f67956u}, {0xabe31c14u, 0x3f40ddf7u}
};

static const uint32_t k_f41911b0[8] = {
    0xecfa3273u, 0x2704c605u, 0x6205ec87u, 0x1b083f01u,
    0xc8d2f19fu, 0x1fafb13du, 0xc2f97387u, 0xbe1a5e83u
};

static const uint32_t k_f41911d0[8] = {
    0x24ef51abu, 0xd1c51f80u, 0xf7807c65u, 0x6be3ca31u,
    0x745432cau, 0xd846e456u, 0x08418c2bu, 0xdb2e781cu
};


static const l3_authorization_secondary_pair32 k_f41a22f0[16] = {
    {0x55958d09u, 0x39028b73u}, {0x9a5a1195u, 0x6b9e6750u},
    {0xdf1e9621u, 0x9e3a432du}, {0x23e31aadu, 0xc0d61f0bu},
    {0x68a79f39u, 0xf371fae8u}, {0xad6c23c5u, 0x260dd6c5u},
    {0xf230a851u, 0x58a9b2a2u}, {0xeaace41eu, 0x8187d0abu},
    {0x2f7168aau, 0xb423ac89u}, {0x7435ed36u, 0xe6bf8866u},
    {0xb8fa71c2u, 0x195b6443u}, {0xfdbef64eu, 0x4bf74020u},
    {0x42837adau, 0x7e931bfeu}, {0x8747ff66u, 0xa12ef7dbu},
    {0xcc0c83f2u, 0xd3cad3b8u}, {0x10d1087eu, 0x0666af96u}
};

static const l3_authorization_secondary_pair32 k_f41a2370[16] = {
    {0xca6d6c15u, 0x5a391a68u}, {0x6ee11b6bu, 0xee7b8e65u},
    {0x1354cac1u, 0x72be0262u}, {0xb7c87a17u, 0x0700765eu},
    {0x5c3c296du, 0x9b42ea5bu}, {0x00afd8c3u, 0x2f855e58u},
    {0xa5238819u, 0xb3c7d254u}, {0x4997376fu, 0x480a4651u},
    {0xee0ae6c5u, 0xdc4cba4du}, {0x927e961bu, 0x608f2e4au},
    {0x36f24571u, 0xf4d1a247u}, {0xdb65f4c7u, 0x89141643u},
    {0x389eaebeu, 0x192f4a76u}, {0xdd125e14u, 0xad71be72u},
    {0x81860d6au, 0x31b4326fu}, {0x25f9bcc0u, 0xc5f6a66cu}
};

static const l3_authorization_secondary_pair32 k_f41a23f0[16] = {
    {0x09554bc2u, 0x301291beu}, {0x9565c53eu, 0xe5eb0ffau},
    {0x21763ebau, 0x9bc38e37u}, {0xad86b836u, 0x419c0c73u},
    {0x399731b2u, 0xf7748ab0u}, {0xc5a7ab2eu, 0xad4d08ecu},
    {0x51b824aau, 0x53258729u}, {0xddc89e26u, 0x08fe0565u},
    {0x69d917a2u, 0xbed683a2u}, {0xf5e9911eu, 0x64af01deu},
    {0x81fa0a9au, 0x1a87801bu}, {0x0e0a8416u, 0xc05ffe58u},
    {0x9a1afd92u, 0x76387c94u}, {0x262b770eu, 0x2c10fad1u},
    {0xf13458cbu, 0xd4619544u}, {0x7d44d247u, 0x8a3a1381u}
};

static const uint32_t k_f4191270[8] = {
    0x8cd4c01fu, 0xa1de1a89u, 0x431b3c51u, 0x8bb9bb6bu,
    0xca40aee3u, 0x4268b7ffu, 0x248083b7u, 0x3e7b20b9u
};

static const uint32_t k_f4191290[8] = {
    0xe92d8321u, 0xf0f2ff24u, 0x3d64fe6cu, 0x7f6ebcf1u,
    0x1478d54au, 0x30cd315bu, 0x799c3cf2u, 0x4be69191u
};

static const uint32_t k_f41911f0[8] = {
    0x871b9761u, 0x241ca735u, 0xdaca8187u, 0x05c66f49u,
    0x552b3c81u, 0x1bdbc119u, 0x00fddd25u, 0xb7d4f385u
};


static const uint32_t k_f41912b0[8] = {
    0x3df25bd7u, 0x5763b851u, 0x44793603u, 0x198c9495u,
    0x430fca4bu, 0x784c7d39u, 0x15c33303u, 0xd8e08e07u
};

static const uint32_t k_f41912d0[8] = {
    0x9f5f339fu, 0x72fa49f1u, 0xc3d7cef1u, 0xcf75e683u,
    0x92539651u, 0xf9109dcdu, 0xe0a3210bu, 0xfaa91eabu
};

static const uint32_t k_f41912f0[8] = {
    0x15db50c9u, 0xebd653dfu, 0x56dfd827u, 0x7f4fadc5u,
    0x031d26fdu, 0x6e4fefa9u, 0xe3956061u, 0x7fa9c98fu
};

static const uint32_t k_f41a2470[16] = {
    0x7933fd11u, 0x08334d9fu, 0x97329e2du, 0x2631eebbu,
    0xb5313f49u, 0x44308fd7u, 0xd32fe065u, 0x622f30f3u,
    0xf13978a2u, 0x8038c930u, 0x1f3819beu, 0xae376a4cu,
    0x3d36badau, 0xcc360b68u, 0x5b355bf6u, 0xea34ac84u
};

static const uint32_t k_f4191310[8] = {
    0x571b2b4du, 0x97237831u, 0x2645752du, 0x4f0469a3u,
    0xe406417bu, 0x68e3463fu, 0xacf08e57u, 0x4d1cfb85u
};

static const uint32_t k_f4191330[8] = {
    0xdae3571bu, 0xc43b5d8eu, 0x4266081fu, 0x87b54291u,
    0x1abfd9f4u, 0xe96f2d43u, 0x3a18f43bu, 0xc2fd4077u
};

static const uint32_t k_f4191350[8] = {
    0x15808845u, 0xdee48091u, 0xba275d65u, 0x9530eb4bu,
    0xc56084f3u, 0x6f56a5ffu, 0x53e96fa7u, 0xabd5f60du
};

static const uint32_t k_f4191370[8] = {
    0xd8353d2bu, 0x17631204u, 0x5c549837u, 0x5bdc73f7u,
    0x92445cd6u, 0xf2d5cab5u, 0x3f6227f5u, 0xff706f67u
};

static const l3_authorization_secondary_pair32 k_f41a24b0[16] = {
    {0xa460273au, 0x90d2a225u}, {0x8f369422u, 0x436b9c8fu},
    {0xdea6912du, 0xf0c8fc30u}, {0x2e168e38u, 0xae265bd2u},
    {0x18ecfb20u, 0x50bf563cu}, {0x685cf82bu, 0x0e1cb5ddu},
    {0xb7ccf536u, 0xbb7a157eu}, {0xa2a3621eu, 0x6e130fe8u},
    {0xf2135f29u, 0x1b706f89u}, {0x41835c34u, 0xc8cdcf2bu},
    {0x2c59c91cu, 0x7b66c995u}, {0x7bc9c627u, 0x28c42936u},
    {0xcb39c332u, 0xd62188d7u}, {0xb610301au, 0x88ba8341u},
    {0x05802d25u, 0x3617e2e3u}, {0x54f02a30u, 0xe3754284u}
};

static const uint32_t k_f4191390[8] = {
    0x51e420efu, 0x03426fd1u, 0xdc2c053du, 0x2e682f07u,
    0xe8be37d1u, 0xce02f7b9u, 0x69b075d5u, 0x2bd38cbdu
};

static const uint32_t k_f41913b0[8] = {
    0xfe2dab24u, 0xaaad0131u, 0x88dfe59fu, 0xb6e83c78u,
    0xaa2c84bbu, 0x2d7e7c42u, 0x6c8f4008u, 0x381b2cafu
};

static const uint32_t k_f41913d0[8] = {
    0xdd504167u, 0xf4fd928fu, 0xe493dcd5u, 0xf8d9f723u,
    0x63d6e54du, 0x960e9fedu, 0xd64fcbcbu, 0xe2be1665u
};

static const uint32_t k_f41913f0[8] = {
    0x25e564bfu, 0x3d879151u, 0x16ec9ac2u, 0xa46227e5u,
    0xb054b2b0u, 0x26567f7cu, 0x1fe525ffu, 0x7b124415u
};

static const l3_authorization_secondary_pair32 k_f41a2530[16] = {
    {0xb9e33175u, 0xcd6cf121u}, {0xf7adffb3u, 0xbf3f0beau},
    {0x5f16f688u, 0xa60c5731u}, {0x9ce1c4c6u, 0x97de71fau},
    {0x044abb9bu, 0x8eabbd41u}, {0x421589d9u, 0x707dd80au},
    {0xa97e80aeu, 0x674b2350u}, {0xe7494eecu, 0x591d3e19u},
    {0x4eb245c1u, 0x4fea8960u}, {0x8c7d13ffu, 0x31bca429u},
    {0xca47e23du, 0x238ebef2u}, {0x31b0d912u, 0x1a5c0a39u},
    {0x6f7ba750u, 0x0c2e2502u}, {0xd6e49e25u, 0xf2fb7048u},
    {0x14af6c63u, 0xe4cd8b12u}, {0x7c186338u, 0xdb9ad658u}
};

static const l3_authorization_secondary_pair32 k_f41a25b0[16] = {
    {0x532934c8u, 0xd2309cfdu}, {0x7f133ddfu, 0xcf5ee657u},
    {0x04ae4e55u, 0xb3d347dau}, {0x8a495ecbu, 0xa847a95cu},
    {0x0fe46f41u, 0x9cbc0adfu}, {0x957f7fb7u, 0x81306c61u},
    {0x1b1a902du, 0x75a4cde4u}, {0xa0b5a0a3u, 0x6a192f66u},
    {0x2650b119u, 0x5e8d90e9u}, {0xabebc18fu, 0x4301f26bu},
    {0x3186d205u, 0x377653eeu}, {0xb721e27bu, 0x2beab570u},
    {0x3cbcf2f1u, 0x105f16f3u}, {0xc2580367u, 0x04d37875u},
    {0x47f313ddu, 0xf947d9f8u}, {0xcd8e2453u, 0xedbc3b7au}
};

static const uint32_t k_f4191210[8] = {
    0x01140237u, 0x3965ed05u, 0x4d578ab0u, 0xa52da840u,
    0x4c2e4648u, 0x731e5da1u, 0x6105f25fu, 0x08ea2338u
};

static const uint32_t k_f4191230[8] = {
    0x9bbca135u, 0xefab78a7u, 0xb9d547b9u, 0xa6c90c05u,
    0xc21ef5adu, 0x58164b3du, 0x8666bba9u, 0xf26bd705u
};

static const uint32_t k_f4191250[8] = {
    0x4e09faf4u, 0x3f369fa0u, 0x2b849aa7u, 0x5d9adff2u,
    0xb2816f3cu, 0x5afa15bcu, 0x405c62c1u, 0x754fd186u
};

static const uint32_t k_f4191130[8] = {
    0x457f6779u, 0x8c30fc77u, 0xe6bd47d1u, 0x77f4afffu,
    0xf9bf5b8du, 0x732d0b5fu, 0x0a6e73cbu, 0x3786383bu
};
static const uint32_t k_f4191150[8] = {
    0x662be096u, 0xe526d2dfu, 0x671a443fu, 0x4337c493u,
    0xa921d53fu, 0xf4bb2244u, 0x8d13a732u, 0x10591929u
};
static const uint32_t k_f4191170[8] = {
    0xd5c4defbu, 0xa72d3825u, 0xcb1b0f7bu, 0xf1707b97u,
    0x7b6f3bbdu, 0x664ece75u, 0xd3706529u, 0x595d5973u
};
static const uint32_t k_f4191190[8] = {
    0x20374405u, 0xc0f70bb8u, 0xc8e4c0c8u, 0xe5edd3fau,
    0xdcdc4bc3u, 0xb97e3b97u, 0x302ef238u, 0x9a46975fu
};

static l3_authorization_secondary_pair32 fold_step(l3_authorization_secondary_pair32 v,
                                        const l3_authorization_secondary_pair32 table[16]) {
    const unsigned idx = v.lo & 0x0fu;
    const uint32_t shifted_lo = (v.lo >> 4) | (v.hi << 28);
    const uint32_t shifted_hi = v.hi >> 4;
    l3_authorization_secondary_pair32 out;
    out.lo = shifted_lo + table[idx].lo;
    out.hi = shifted_hi + table[idx].hi + u32_add_carry(shifted_lo, table[idx].lo);
    return out;
}

static l3_authorization_secondary_pair32 fold_rounds(l3_authorization_secondary_pair32 seed,
                                          const l3_authorization_secondary_pair32 table[16],
                                          unsigned rounds) {
    for (unsigned i = 0; i < rounds; ++i) seed = fold_step(seed, table);
    return seed;
}

static l3_authorization_secondary_pair32 make_15f0_pair(uint32_t x, uint32_t *fold8_low) {
    const uint32_t lo_mul0 = u32_mullo(x, 0x8b2a6e39u);
    l3_authorization_secondary_pair32 seed = {
        lo_mul0 + 0x3f0d2391u,
        (uint32_t)(x * 0xc15768dau + u32_mulhi(x, 0x8b2a6e39u) +
                   0x9c0a83b4u + u32_add_carry(lo_mul0, 0x3f0d2391u))
    };
    const uint32_t folded_low = fold_rounds(seed, k_f41a15f0, 8u).lo;
    if (fold8_low) *fold8_low = folded_low;

    const uint32_t lo_mul1 = u32_mullo(x, 0xf118792du);
    l3_authorization_secondary_pair32 out = {
        lo_mul1 + 0x26005d44u,
        (uint32_t)(x * 0x15e3df18u + u32_mulhi(x, 0xf118792du) +
                   folded_low * 0xa3271d6bu + 0x00ceef33u +
                   u32_add_carry(lo_mul1, 0x26005d44u))
    };
    return out;
}

static l3_authorization_secondary_pair32 make_1670_pair(uint32_t x, uint32_t *fold8_low) {
    const uint32_t lo_mul0 = u32_mullo(x, 0xefd4ed73u);
    l3_authorization_secondary_pair32 seed = {
        lo_mul0 + 0xa348fe85u,
        (uint32_t)(x * 0x978ae60fu + u32_mulhi(x, 0xefd4ed73u) +
                   0x699aafecu + u32_add_carry(lo_mul0, 0xa348fe85u))
    };
    const uint32_t folded_low = fold_rounds(seed, k_f41a1670, 8u).lo;
    if (fold8_low) *fold8_low = folded_low;

    const uint32_t lo_mul1 = u32_mullo(x, 0x1a2252c7u);
    l3_authorization_secondary_pair32 out = {
        lo_mul1 + 0x047455a7u,
        (uint32_t)(x * 0x07705e94u + u32_mulhi(x, 0x1a2252c7u) +
                   folded_low * 0x92ec4fa3u + 0x13363c4du +
                   u32_add_carry(lo_mul1, 0x047455a7u))
    };
    return out;
}


static l3_authorization_secondary_pair32 make_1970_pair(uint32_t x, uint32_t *fold8_low) {
    const uint32_t lo_mul0 = u32_mullo(x, 0x172fcd11u);
    l3_authorization_secondary_pair32 seed = {
        lo_mul0 + 0x1458f204u,
        (uint32_t)(x * 0x58128a00u + u32_mulhi(x, 0x172fcd11u) +
                   0x1a8090dcu + u32_add_carry(lo_mul0, 0x1458f204u))
    };
    const uint32_t folded_low = fold_rounds(seed, k_f41a1970, 8u).lo;
    if (fold8_low) *fold8_low = folded_low;

    const uint32_t lo_mul1 = u32_mullo(x, 0x57bb4967u);
    l3_authorization_secondary_pair32 out = {
        lo_mul1 + 0xfd754d91u,
        (uint32_t)(x * 0xea41ba7au + u32_mulhi(x, 0x57bb4967u) +
                   folded_low * 0xa4f27109u + 0x5ee25f71u +
                   u32_add_carry(lo_mul1, 0xfd754d91u))
    };
    return out;
}

static l3_authorization_secondary_pair32 make_19f0_pair(uint32_t x, uint32_t *fold8_low) {
    const uint32_t lo_mul0 = u32_mullo(x, 0x2354809du);
    l3_authorization_secondary_pair32 seed = {
        lo_mul0 + 0x82fc41f5u,
        (uint32_t)(x * 0x79e68e0fu + u32_mulhi(x, 0x2354809du) +
                   0x8fe27365u + u32_add_carry(lo_mul0, 0x82fc41f5u))
    };
    const uint32_t folded_low = fold_rounds(seed, k_f41a19f0, 8u).lo;
    if (fold8_low) *fold8_low = folded_low;

    const uint32_t lo_mul1 = u32_mullo(x, 0xb7e4ddd1u);
    l3_authorization_secondary_pair32 out = {
        lo_mul1 + 0xcfcae893u,
        (uint32_t)(x * 0xb4f9838du + u32_mulhi(x, 0xb7e4ddd1u) +
                   folded_low * 0xaca4163bu + 0x23359017u +
                   u32_add_carry(lo_mul1, 0xcfcae893u))
    };
    return out;
}

static l3_authorization_secondary_pair32 make_1ab0_pair(uint32_t x, uint32_t *fold8_low) {
    const uint32_t lo_mul0 = u32_mullo(x, 0x1e60763fu);
    l3_authorization_secondary_pair32 seed = {
        lo_mul0 + 0xc244847au,
        (uint32_t)(x * 0x8c2dd594u + u32_mulhi(x, 0x1e60763fu) +
                   0x97658790u + u32_add_carry(lo_mul0, 0xc244847au))
    };
    const uint32_t folded_low = fold_rounds(seed, k_f41a1ab0, 8u).lo;
    if (fold8_low) *fold8_low = folded_low;

    const uint32_t lo_mul1 = u32_mullo(x, 0x9c8e6ddbu);
    l3_authorization_secondary_pair32 out = {
        lo_mul1 + 0x1b18d6d2u,
        (uint32_t)(x * 0xb9689594u + u32_mulhi(x, 0x9c8e6ddbu) +
                   folded_low * 0xd79b869bu + 0x68d8b14cu +
                   u32_add_carry(lo_mul1, 0x1b18d6d2u))
    };
    return out;
}

static l3_authorization_secondary_pair32 make_1b30_pair(uint32_t x, uint32_t *fold8_low) {
    const uint32_t lo_mul0 = u32_mullo(x, 0x3d7983a1u);
    l3_authorization_secondary_pair32 seed = {
        lo_mul0 + 0x1b726f71u,
        (uint32_t)(x * 0x9c4b0281u + u32_mulhi(x, 0x3d7983a1u) +
                   0x9566813cu + u32_add_carry(lo_mul0, 0x1b726f71u))
    };
    const uint32_t folded_low = fold_rounds(seed, k_f41a1b30, 8u).lo;
    if (fold8_low) *fold8_low = folded_low;

    const uint32_t lo_mul1 = u32_mullo(x, 0x25b8c31fu);
    l3_authorization_secondary_pair32 out = {
        lo_mul1 + 0x70b042d6u,
        (uint32_t)(x * 0x8fa40b8bu + u32_mulhi(x, 0x25b8c31fu) +
                   folded_low * 0xf25b3141u + 0x02f0ebf7u +
                   u32_add_carry(lo_mul1, 0x70b042d6u))
    };
    return out;
}

static l3_authorization_secondary_pair32 make_1cf0_pair(uint32_t x, uint32_t *fold8_low) {
    const uint32_t lo_mul0 = u32_mullo(x, 0xeabad461u);
    l3_authorization_secondary_pair32 seed = {
        lo_mul0 + 0xc3eb233du,
        (uint32_t)(x * 0xfe2f1990u + u32_mulhi(x, 0xeabad461u) +
                   0xf11bf09bu + u32_add_carry(lo_mul0, 0xc3eb233du))
    };
    const uint32_t folded_low = fold_rounds(seed, k_f41a1cf0, 8u).lo;
    if (fold8_low) *fold8_low = folded_low;

    const uint32_t lo_mul1 = u32_mullo(x, 0xb47c84e7u);
    l3_authorization_secondary_pair32 out = {
        lo_mul1 + 0xe6186359u,
        (uint32_t)(x * 0x1e86c5d2u + u32_mulhi(x, 0xb47c84e7u) +
                   folded_low * 0x414fa1b9u + 0xae58011fu +
                   u32_add_carry(lo_mul1, 0xe6186359u))
    };
    return out;
}

static l3_authorization_secondary_pair32 make_1d70_pair(uint32_t x, uint32_t *fold8_low) {
    const uint32_t lo_mul0 = u32_mullo(x, 0x6d7b0ac7u);
    l3_authorization_secondary_pair32 seed = {
        lo_mul0 + 0x02a75a29u,
        (uint32_t)(x * 0xcfbe6ef8u + u32_mulhi(x, 0x6d7b0ac7u) +
                   0x91b6ad6eu + u32_add_carry(lo_mul0, 0x02a75a29u))
    };
    const uint32_t folded_low = fold_rounds(seed, k_f41a1d70, 8u).lo;
    if (fold8_low) *fold8_low = folded_low;

    const uint32_t lo_mul1 = u32_mullo(x, 0x69ca541fu);
    l3_authorization_secondary_pair32 out = {
        lo_mul1 + 0x30f32c1eu,
        (uint32_t)(x * 0xf4b8d9e2u + u32_mulhi(x, 0x69ca541fu) +
                   folded_low * 0x49c6ac17u + 0x901c0a1du +
                   u32_add_carry(lo_mul1, 0x30f32c1eu))
    };
    return out;
}

static l3_authorization_secondary_pair32 make_1bb0_pair(
    uint32_t x, uint32_t *fold8_low) {
    const uint32_t lo_mul0 = u32_mullo(x, 0xe66e766fu);
    l3_authorization_secondary_pair32 seed = {
        lo_mul0 + 0x04e52a30u,
        (uint32_t)(x * 0x811a9afcu + u32_mulhi(x, 0xe66e766fu) +
                   0x0ba56751u + u32_add_carry(lo_mul0, 0x04e52a30u))
    };
    const uint32_t folded_low =
        fold_rounds(seed, k_f41a1bb0, 8u).lo;
    if (fold8_low) *fold8_low = folded_low;

    const uint32_t lo_mul1 = u32_mullo(x, 0xc7b2f29du);
    l3_authorization_secondary_pair32 out = {
        lo_mul1 + 0xead5aa6eu,
        (uint32_t)(x * 0xa9a99ecbu + u32_mulhi(x, 0xc7b2f29du) +
                   folded_low * 0x680a724du + 0x5bbd9e57u +
                   u32_add_carry(lo_mul1, 0xead5aa6eu))
    };
    return out;
}

static l3_authorization_secondary_pair32 make_1c30_pair(
    uint32_t x, uint32_t *fold8_low) {
    const uint32_t lo_mul0 = u32_mullo(x, 0xc7ad30a5u);
    l3_authorization_secondary_pair32 seed = {
        lo_mul0 + 0x6530b13bu,
        (uint32_t)(x * 0xe401dad7u + u32_mulhi(x, 0xc7ad30a5u) +
                   0x985cc198u + u32_add_carry(lo_mul0, 0x6530b13bu))
    };
    const uint32_t folded_low =
        fold_rounds(seed, k_f41a1c30, 8u).lo;
    if (fold8_low) *fold8_low = folded_low;

    const uint32_t lo_mul1 = u32_mullo(x, 0xb9633237u);
    l3_authorization_secondary_pair32 out = {
        lo_mul1 + 0xc4c6771du,
        (uint32_t)(x * 0xf8ee4330u + u32_mulhi(x, 0xb9633237u) +
                   folded_low * 0x80bd5b55u + 0x04b3b44fu +
                   u32_add_carry(lo_mul1, 0xc4c6771du))
    };
    return out;
}


static l3_authorization_secondary_pair32 make_1df0_pair(uint32_t x, uint32_t *fold8_low) {
    const uint32_t lo_mul0 = u32_mullo(x, 0x8df61709u);
    l3_authorization_secondary_pair32 seed = {
        lo_mul0 + 0xaba42e63u,
        (uint32_t)(x * 0x7b2208ceu + u32_mulhi(x, 0x8df61709u) +
                   0xabe74f92u + u32_add_carry(lo_mul0, 0xaba42e63u))
    };
    const uint32_t folded_low = fold_rounds(seed, k_f41a1df0, 8u).lo;
    if (fold8_low) *fold8_low = folded_low;

    const uint32_t lo_mul1 = u32_mullo(x, 0x73118a8fu);
    l3_authorization_secondary_pair32 out = {
        lo_mul1 + 0xd165f84fu,
        (uint32_t)(x * 0xfc1a3a86u + u32_mulhi(x, 0x73118a8fu) +
                   folded_low * 0xc012dd29u + 0x85765455u +
                   u32_add_carry(lo_mul1, 0xd165f84fu))
    };
    return out;
}

static l3_authorization_secondary_pair32 make_1e70_pair(uint32_t x, uint32_t *fold8_low) {
    const uint32_t lo_mul0 = u32_mullo(x, 0x09a5a7fdu);
    l3_authorization_secondary_pair32 seed = {
        lo_mul0 + 0xdb691cb9u,
        (uint32_t)(x * 0x11714fb4u + u32_mulhi(x, 0x09a5a7fdu) +
                   0xc55b20aau + u32_add_carry(lo_mul0, 0xdb691cb9u))
    };
    const uint32_t folded_low = fold_rounds(seed, k_f41a1e70, 8u).lo;
    if (fold8_low) *fold8_low = folded_low;

    const uint32_t lo_mul1 = u32_mullo(x, 0x69316553u);
    l3_authorization_secondary_pair32 out = {
        lo_mul1 + 0x2a18dc52u,
        (uint32_t)(x * 0x449efe33u + u32_mulhi(x, 0x69316553u) +
                   folded_low * 0x74a58471u + 0x7ceae7c5u +
                   u32_add_carry(lo_mul1, 0x2a18dc52u))
    };
    return out;
}


static l3_authorization_secondary_pair32 make_1ef0_pair(uint32_t x, uint32_t *fold8_low) {
    const uint32_t lo_mul0 = u32_mullo(x, 0x4fed7f55u);
    l3_authorization_secondary_pair32 seed = {
        lo_mul0 + 0xbd3b401eu,
        (uint32_t)(x * 0x1dd5c836u + u32_mulhi(x, 0x4fed7f55u) +
                   0x24d59ce5u + u32_add_carry(lo_mul0, 0xbd3b401eu))
    };
    const uint32_t folded_low = fold_rounds(seed, k_f41a1ef0, 8u).lo;
    if (fold8_low) *fold8_low = folded_low;

    const uint32_t lo_mul1 = u32_mullo(x, 0xd79ff45bu);
    l3_authorization_secondary_pair32 out = {
        lo_mul1 + 0xcb0f806du,
        (uint32_t)(x * 0x7e4768e1u + u32_mulhi(x, 0xd79ff45bu) +
                   folded_low * 0x343a3b11u + 0xfa81954cu +
                   u32_add_carry(lo_mul1, 0xcb0f806du))
    };
    return out;
}

static l3_authorization_secondary_pair32 make_1f70_pair(uint32_t x, uint32_t *fold8_low) {
    const uint32_t lo_mul0 = u32_mullo(x, 0x90a2435bu);
    l3_authorization_secondary_pair32 seed = {
        lo_mul0 + 0xb53b2209u,
        (uint32_t)(x * 0xf9f84fe3u + u32_mulhi(x, 0x90a2435bu) +
                   0xbf0562b6u + u32_add_carry(lo_mul0, 0xb53b2209u))
    };
    const uint32_t folded_low = fold_rounds(seed, k_f41a1f70, 8u).lo;
    if (fold8_low) *fold8_low = folded_low;

    const uint32_t lo_mul1 = u32_mullo(x, 0x2e1548d9u);
    l3_authorization_secondary_pair32 out = {
        lo_mul1 + 0x32d3e9b9u,
        (uint32_t)(x * 0xfb03cea4u + u32_mulhi(x, 0x2e1548d9u) +
                   folded_low * 0x7f0ce125u + 0x5388c1ddu +
                   u32_add_carry(lo_mul1, 0x32d3e9b9u))
    };
    return out;
}


static l3_authorization_secondary_pair32 make_1ff0_pair(uint32_t x, uint32_t *fold8_low) {
    const uint32_t lo_mul0 = u32_mullo(x, 0xacdf8b5fu);
    l3_authorization_secondary_pair32 seed = {
        lo_mul0 + 0x8bcf77b6u,
        (uint32_t)(x * 0xe84f87bau + u32_mulhi(x, 0xacdf8b5fu) +
                   0xa8068fa3u + u32_add_carry(lo_mul0, 0x8bcf77b6u))
    };
    const uint32_t folded_low = fold_rounds(seed, k_f41a1ff0, 8u).lo;
    if (fold8_low) *fold8_low = folded_low;

    const uint32_t lo_mul1 = u32_mullo(x, 0x88b93e4fu);
    l3_authorization_secondary_pair32 out = {
        lo_mul1 + 0x2be35d3bu,
        (uint32_t)(x * 0x225ae2d2u + u32_mulhi(x, 0x88b93e4fu) +
                   folded_low * 0x29f2dcefu + 0xad952681u +
                   u32_add_carry(lo_mul1, 0x2be35d3bu))
    };
    return out;
}


static l3_authorization_secondary_pair32 make_22f0_pair(uint32_t x, uint32_t *fold8_low) {
    const uint32_t lo_mul0 = u32_mullo(x, 0x153f9addu);
    l3_authorization_secondary_pair32 seed = {
        lo_mul0 + 0xe7576e4au,
        (uint32_t)(x * 0x909ea467u + u32_mulhi(x, 0x153f9addu) +
                   0x01bdaa7au + u32_add_carry(lo_mul0, 0xe7576e4au))
    };
    const uint32_t folded_low = fold_rounds(seed, k_f41a22f0, 8u).lo;
    if (fold8_low) *fold8_low = folded_low;

    const uint32_t lo_mul1 = u32_mullo(x, 0x0cd827f5u);
    l3_authorization_secondary_pair32 out = {
        lo_mul1 + 0x6fa79717u,
        (uint32_t)(x * 0x79a82e21u + u32_mulhi(x, 0x0cd827f5u) +
                   folded_low * 0x9ca54c07u + 0x544a4cfdu +
                   u32_add_carry(lo_mul1, 0x6fa79717u))
    };
    return out;
}

static l3_authorization_secondary_pair32 make_2370_pair(uint32_t x, uint32_t *fold8_low) {
    const uint32_t lo_mul0 = u32_mullo(x, 0x5d7baf51u);
    l3_authorization_secondary_pair32 seed = {
        lo_mul0 + 0x42e0315fu,
        (uint32_t)(x * 0x0de8d349u + u32_mulhi(x, 0x5d7baf51u) +
                   0x5e1a911cu + u32_add_carry(lo_mul0, 0x42e0315fu))
    };
    const uint32_t folded_low = fold_rounds(seed, k_f41a2370, 8u).lo;
    if (fold8_low) *fold8_low = folded_low;

    const uint32_t lo_mul1 = u32_mullo(x, 0x2585877du);
    l3_authorization_secondary_pair32 out = {
        lo_mul1 + 0xaa83f1dfu,
        (uint32_t)(x * 0x648a7c29u + u32_mulhi(x, 0x2585877du) +
                   folded_low * 0xf626bd93u + 0xde04dc75u +
                   u32_add_carry(lo_mul1, 0xaa83f1dfu))
    };
    return out;
}

static l3_authorization_secondary_pair32 make_2070_pair(uint32_t x, uint32_t *fold8_low) {
    const uint32_t lo_mul0 = u32_mullo(x, 0x52e6243fu);
    l3_authorization_secondary_pair32 seed = {
        lo_mul0 + 0x35692a46u,
        (uint32_t)(x * 0xd2d169abu + u32_mulhi(x, 0x52e6243fu) +
                   0xd3178ad5u + u32_add_carry(lo_mul0, 0x35692a46u))
    };
    const uint32_t folded_low = fold_rounds(seed, k_f41a2070, 8u).lo;
    if (fold8_low) *fold8_low = folded_low;

    const uint32_t lo_mul1 = u32_mullo(x, 0xa7837953u);
    l3_authorization_secondary_pair32 out = {
        lo_mul1 + 0x771357d3u,
        (uint32_t)(x * 0x1f507bb4u + u32_mulhi(x, 0xa7837953u) +
                   folded_low * 0xd52aaa13u + 0xdb242426u +
                   u32_add_carry(lo_mul1, 0x771357d3u))
    };
    return out;
}


static l3_authorization_secondary_pair32 make_2170_pair(uint32_t x, uint32_t *fold8_low) {
    const uint32_t lo_mul0 = u32_mullo(x, 0xef73849du);
    l3_authorization_secondary_pair32 seed = {
        lo_mul0 + 0x99ed4eb4u,
        (uint32_t)(x * 0x680558a0u + u32_mulhi(x, 0xef73849du) +
                   0xca92d3d3u + u32_add_carry(lo_mul0, 0x99ed4eb4u))
    };
    const uint32_t folded_low = fold_rounds(seed, k_f41a2170, 8u).lo;
    if (fold8_low) *fold8_low = folded_low;

    const uint32_t lo_mul1 = u32_mullo(x, 0x063d56dfu);
    l3_authorization_secondary_pair32 out = {
        lo_mul1 + 0xcaaa982cu,
        (uint32_t)(x * 0x51370f06u + u32_mulhi(x, 0x063d56dfu) +
                   folded_low * 0x9405d555u + 0x06fb4520u +
                   u32_add_carry(lo_mul1, 0xcaaa982cu))
    };
    return out;
}

static l3_authorization_secondary_pair32 make_21f0_pair(uint32_t x, uint32_t *fold8_low) {
    const uint32_t lo_mul0 = u32_mullo(x, 0x916ba08bu);
    l3_authorization_secondary_pair32 seed = {
        lo_mul0 + 0x4917267bu,
        (uint32_t)(x * 0xe6cfff98u + u32_mulhi(x, 0x916ba08bu) +
                   0x1dd53fb9u + u32_add_carry(lo_mul0, 0x4917267bu))
    };
    const uint32_t folded_low = fold_rounds(seed, k_f41a21f0, 8u).lo;
    if (fold8_low) *fold8_low = folded_low;

    const uint32_t lo_mul1 = u32_mullo(x, 0xc2243445u);
    l3_authorization_secondary_pair32 out = {
        lo_mul1 + 0x2d16c1fau,
        (uint32_t)(x * 0xce0c9a40u + u32_mulhi(x, 0xc2243445u) +
                   folded_low * 0x26383791u + 0xbacb5a53u +
                   u32_add_carry(lo_mul1, 0x2d16c1fau))
    };
    return out;
}


static l3_authorization_secondary_pair32 make_24b0_pair(uint32_t x, uint32_t *fold8_low) {
    const uint32_t lo_mul0 = u32_mullo(x, 0x1d86a409u);
    l3_authorization_secondary_pair32 seed = {
        lo_mul0 + 0xad30fb7fu,
        (uint32_t)(x * 0x278a2de8u + u32_mulhi(x, 0x1d86a409u) +
                   0xff28517bu + u32_add_carry(lo_mul0, 0xad30fb7fu))
    };
    const uint32_t folded_low = fold_rounds(seed, k_f41a24b0, 8u).lo;
    if (fold8_low) *fold8_low = folded_low;

    const uint32_t lo_mul1 = u32_mullo(x, 0x4102159du);
    l3_authorization_secondary_pair32 out = {
        lo_mul1 + 0x1d3cc9dfu,
        (uint32_t)(x * 0x1a9d299fu + u32_mulhi(x, 0x4102159du) +
                   folded_low * 0xea316e0bu + 0xbba3dea2u +
                   u32_add_carry(lo_mul1, 0x1d3cc9dfu))
    };
    return out;
}

static l3_authorization_secondary_pair32 make_2530_pair(uint32_t x, uint32_t *fold8_low) {
    const uint32_t lo_mul0 = u32_mullo(x, 0x466ed31du);
    l3_authorization_secondary_pair32 seed = {
        lo_mul0 + 0x3d42730cu,
        (uint32_t)(x * 0x1277b2d7u + u32_mulhi(x, 0x466ed31du) +
                   0x7a5630a8u + u32_add_carry(lo_mul0, 0x3d42730cu))
    };
    const uint32_t folded_low = fold_rounds(seed, k_f41a2530, 8u).lo;
    if (fold8_low) *fold8_low = folded_low;

    const uint32_t lo_mul1 = u32_mullo(x, 0xef73dd91u);
    l3_authorization_secondary_pair32 out = {
        lo_mul1 + 0xbbe5c77du,
        (uint32_t)(x * 0x5e110204u + u32_mulhi(x, 0xef73dd91u) +
                   folded_low * 0xf689a9fbu + 0x7e69b068u +
                   u32_add_carry(lo_mul1, 0xbbe5c77du))
    };
    return out;
}

static l3_authorization_secondary_pair32 make_25b0_pair(uint32_t x, uint32_t *fold8_low) {
    const uint32_t lo_mul0 = u32_mullo(x, 0x8a8c7ba1u);
    l3_authorization_secondary_pair32 seed = {
        lo_mul0 + 0x0125fdd6u,
        (uint32_t)(x * 0xd1818d7du + u32_mulhi(x, 0x8a8c7ba1u) +
                   0x7d6be4e0u + u32_add_carry(lo_mul0, 0x0125fdd6u))
    };
    const uint32_t folded_low = fold_rounds(seed, k_f41a25b0, 8u).lo;
    if (fold8_low) *fold8_low = folded_low;

    const uint32_t lo_mul1 = u32_mullo(x, 0xc0d0f1bdu);
    l3_authorization_secondary_pair32 out = {
        lo_mul1 + 0x8937f0f5u,
        (uint32_t)(x * 0x34345ea0u + u32_mulhi(x, 0xc0d0f1bdu) +
                   folded_low * 0x0aeadf63u + 0xdf97db68u +
                   u32_add_carry(lo_mul1, 0x8937f0f5u))
    };
    return out;
}

int l3_authorization_secondary_param3_accum15_pairs13(
    const int32_t param3_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_param3_accum15_trace *out) {
    if (!param3_words13 || !out_pairs13 || !out_cumulative13) return -1;

    l3_authorization_secondary_param3_accum15_trace tmp;
    l3_authorization_secondary_param3_accum15_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    uint32_t x = u32_mla((uint32_t)param3_words13[0], 0xbf82a11fu, 0x8827b29eu);
    dst->transformed_words13[0] = x;
    out_pairs13[0] = make_15f0_pair(x, &dst->final_fold_lows13[0]);
    out_cumulative13[0] = out_pairs13[0];
    dst->generated_pairs13[0] = out_pairs13[0];
    dst->cumulative_pairs13[0] = out_cumulative13[0];

    for (unsigned i = 1; i < L3_AUTH_ROUND_SECONDARY_WORDS13; ++i) {
        const unsigned j = i & 7u;
        uint32_t pre = u32_mla((uint32_t)param3_words13[i], k_f4190bb0[j], k_f4190bd0[j]);
        x = u32_mla(pre, 0x1498dffbu, 0x37d17222u);
        dst->transformed_words13[i] = x;
        out_pairs13[i] = make_15f0_pair(x, &dst->final_fold_lows13[i]);
        out_cumulative13[i].lo = out_cumulative13[i - 1u].lo + out_pairs13[i].lo;
        out_cumulative13[i].hi = out_cumulative13[i - 1u].hi + out_pairs13[i].hi +
                                 u32_add_carry(out_cumulative13[i - 1u].lo, out_pairs13[i].lo);
        dst->generated_pairs13[i] = out_pairs13[i];
        dst->cumulative_pairs13[i] = out_cumulative13[i];
    }
    return 0;
}



int l3_authorization_secondary_param3_accum1670_pairs13(
    const int32_t param3_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_param3_accum1670_trace *out) {
    if (!param3_words13 || !out_pairs13 || !out_cumulative13) return -1;

    l3_authorization_secondary_param3_accum1670_trace tmp;
    l3_authorization_secondary_param3_accum1670_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    uint32_t x = u32_mla((uint32_t)param3_words13[0], 0xab8ae253u, 0xdf349417u);
    dst->transformed_words13[0] = x;
    out_pairs13[0] = make_1670_pair(x, &dst->final_fold_lows13[0]);
    out_cumulative13[0] = out_pairs13[0];
    dst->generated_pairs13[0] = out_pairs13[0];
    dst->cumulative_pairs13[0] = out_cumulative13[0];

    for (unsigned i = 1; i < L3_AUTH_ROUND_SECONDARY_WORDS13; ++i) {
        const unsigned j = i & 7u;
        uint32_t pre = u32_mla((uint32_t)param3_words13[i], k_f4190bf0[j], k_f4190c10[j]);
        x = u32_mla(pre, 0x7e88b7ebu, 0xf540d2a6u);
        dst->transformed_words13[i] = x;
        out_pairs13[i] = make_1670_pair(x, &dst->final_fold_lows13[i]);
        out_cumulative13[i].lo = out_cumulative13[i - 1u].lo + out_pairs13[i].lo;
        out_cumulative13[i].hi = out_cumulative13[i - 1u].hi + out_pairs13[i].hi +
                                 u32_add_carry(out_cumulative13[i - 1u].lo, out_pairs13[i].lo);
        dst->generated_pairs13[i] = out_pairs13[i];
        dst->cumulative_pairs13[i] = out_cumulative13[i];
    }
    return 0;
}

static void secondary_pair_range_sum(
    const l3_authorization_secondary_pair32 prefix13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    unsigned first,
    unsigned last,
    l3_authorization_secondary_pair32 *out) {
    out->lo = prefix13[last].lo;
    out->hi = prefix13[last].hi;
    if (first != 0u) {
        const uint32_t borrow = (out->lo < prefix13[first - 1u].lo);
        out->lo -= prefix13[first - 1u].lo;
        out->hi -= prefix13[first - 1u].hi + borrow;
    }
}

int l3_authorization_secondary_param3_convolution26(
    const l3_authorization_secondary_pair32 raw15_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix15_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 raw1670_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix1670_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_param3_convolution26_trace *out) {
    if (!raw15_13 || !prefix15_13 || !raw1670_13 || !prefix1670_13 || !out_pairs26) return -1;

    l3_authorization_secondary_param3_convolution26_trace tmp;
    l3_authorization_secondary_param3_convolution26_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    for (unsigned idx = 0; idx < L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26; ++idx) {
        unsigned first = idx - 12u;
        unsigned last;
        if (idx < 12u || first == 0u) {
            first = 0u;
            last = idx;
        } else {
            last = 12u;
        }

        uint32_t u = 0x69505203u;
        uint32_t v = 0xb7edb599u; /* -0x48124a67 */
        l3_authorization_secondary_pair32 conv = {0u, 0u};
        l3_authorization_secondary_pair32 sum15 = {0u, 0u};
        l3_authorization_secondary_pair32 sum1670 = {0u, 0u};

        dst->first_last_count26[idx][0] = first;
        dst->first_last_count26[idx][1] = last;
        dst->first_last_count26[idx][2] = (first <= last) ? ((last - first) + 1u) : 0u;

        if (first <= last) {
            for (unsigned k = first; k <= last; ++k) {
                const l3_authorization_secondary_pair32 *a15 = &raw15_13[k];
                const l3_authorization_secondary_pair32 *b1670 = &raw1670_13[idx - k];
                const uint64_t prod = (uint64_t)a15->lo * (uint64_t)b1670->lo;
                const uint32_t old = conv.lo;
                conv.lo += (uint32_t)prod;
                conv.hi += b1670->hi * a15->lo + b1670->lo * a15->hi +
                           (uint32_t)(prod >> 32) + (uint32_t)(conv.lo < old);
            }

            secondary_pair_range_sum(prefix15_13, first, last, &sum15);
            secondary_pair_range_sum(prefix1670_13, idx - last, idx - first, &sum1670);

            const uint32_t count = (last - first) + 1u;
            const uint64_t count_prod = (uint64_t)count * 0xddc883e5u;
            const uint32_t count_lo = (uint32_t)count_prod;
            const uint32_t count_hi = (uint32_t)(count_prod >> 32);
            const uint64_t conv_scaled = (uint64_t)conv.lo * 0xb4962b41u;
            const uint32_t conv_scaled_lo = (uint32_t)conv_scaled;
            const uint64_t sum15_scaled = (uint64_t)sum15.lo * 0x8d16ea05u;
            const uint32_t sum15_scaled_lo = (uint32_t)sum15_scaled;
            const uint64_t sum1670_scaled = (uint64_t)sum1670.lo * 0xc5ee5ba1u;
            const uint32_t sum1670_scaled_lo = (uint32_t)sum1670_scaled;

            const uint32_t t0 = count_lo + 0x69505203u;
            const uint32_t t1 = t0 + conv_scaled_lo;
            const uint32_t t2 = t1 + sum15_scaled_lo;
            u = t2 + sum1670_scaled_lo;

            v = count * 0x5496ee9au + count_hi + 0xb7edb599u +
                u32_add_carry(count_lo, 0x69505203u) +
                conv.hi * 0xb4962b41u + conv.lo * 0x43028508u +
                (uint32_t)(conv_scaled >> 32) + u32_add_carry(t0, conv_scaled_lo) +
                sum15.hi * 0x8d16ea05u + sum15.lo * 0x90e0b606u +
                (uint32_t)(sum15_scaled >> 32) + u32_add_carry(t1, sum15_scaled_lo) +
                sum1670.hi * 0xc5ee5ba1u + sum1670.lo * 0xe53795dcu +
                (uint32_t)(sum1670_scaled >> 32) + u32_add_carry(t2, sum1670_scaled_lo);

        }

        const uint32_t out_lo_mul = u * 0x5ff18f01u;
        out_pairs26[idx].lo = out_lo_mul + 0xdddf4131u;
        out_pairs26[idx].hi = v * 0x5ff18f01u + u * 0xd62a80e4u +
                              u32_mulhi(u, 0x5ff18f01u) + 0x53c21b10u +
                              u32_add_carry(out_lo_mul, 0xdddf4131u);
        dst->convolution26[idx] = conv;
        dst->range15_26[idx] = sum15;
        dst->range1670_26[idx] = sum1670;
        dst->pre_reduce26[idx].lo = u;
        dst->pre_reduce26[idx].hi = v;
        dst->output26[idx] = out_pairs26[idx];
    }
    return 0;
}

int l3_authorization_secondary_param3_accum_convolution26(
    const int32_t param3_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_param3_convolution26_trace *out) {
    if (!param3_words13 || !out_pairs26) return -1;

    l3_authorization_secondary_pair32 raw15[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 prefix15[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 raw1670[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 prefix1670[L3_AUTH_ROUND_SECONDARY_PAIRS13];

    int rc = l3_authorization_secondary_param3_accum15_pairs13(param3_words13, raw15, prefix15, NULL);
    if (rc != 0) return rc;
    rc = l3_authorization_secondary_param3_accum1670_pairs13(param3_words13, raw1670, prefix1670, NULL);
    if (rc != 0) return rc;
    return l3_authorization_secondary_param3_convolution26(raw15, prefix15, raw1670, prefix1670, out_pairs26, out);
}

/* ---- v268: FUN_f3fdc45c standalone/update-export helper family ---- */

static const uint32_t k_f4190c30[8] = {
    0xa07211fdu, 0xf89277a5u, 0x7da289bbu, 0x0d97420du,
    0x00906117u, 0xad8dac3fu, 0x57c5a0c3u, 0xbaee3eb7u
};

static const uint32_t k_f4190c50[8] = {
    0xea92566fu, 0x8c752a09u, 0x05fb47b3u, 0x32e8f3ffu,
    0xc2019ba9u, 0xaf60f3a9u, 0x05019c7au, 0x98aa54c5u
};

static const uint32_t k_f4190c70[8] = {
    0x0dba7887u, 0xf355b8adu, 0x1fe2d049u, 0x91dcec91u,
    0x5a9607dfu, 0x98035437u, 0xafa8579bu, 0xa540822du
};

static const uint32_t k_f4190c90[8] = {
    0x719c685du, 0x7da9c048u, 0x0d858696u, 0xc22e97a9u,
    0x7f0e4542u, 0x3e41bd6bu, 0x7d3b92a2u, 0xf40e9de5u
};

static const uint32_t k_f41a16f0[16][2] = {
    {0xf9170d5cu, 0x662d8328u}, {0x03e36cacu, 0xff43b878u},
    {0x0eafcbfcu, 0x8859edc7u}, {0x197c2b4cu, 0x11702316u},
    {0x24488a9cu, 0xaa865865u}, {0x2f14e9ecu, 0x339c8db4u},
    {0x39e1493cu, 0xccb2c303u}, {0x44ada88cu, 0x55c8f852u},
    {0xa2b412ddu, 0xed7bd8b0u}, {0xad80722du, 0x76920dffu},
    {0xb84cd17du, 0x0fa8434eu}, {0xc31930cdu, 0x98be789du},
    {0xcde5901du, 0x21d4adecu}, {0xd8b1ef6du, 0xbaeae33bu},
    {0xe37e4ebdu, 0x4401188au}, {0xee4aae0du, 0xdd174dd9u}
};

static const uint32_t k_f41a1770[16][2] = {
    {0x037c0c6eu, 0x34dcaf87u}, {0xa8dd92b0u, 0x05bcc478u},
    {0xafa953c9u, 0xd92ef2afu}, {0x550ada0bu, 0xaa0f07a1u},
    {0x5bd69b24u, 0x7d8135d8u}, {0x01382166u, 0x4e614acau},
    {0x0803e27fu, 0x11d37901u}, {0xad6568c1u, 0xe2b38df2u},
    {0x52c6ef03u, 0xb393a2e4u}, {0x5992b01cu, 0x8705d11bu},
    {0xfef4365eu, 0x57e5e60cu}, {0x05bff777u, 0x2b581444u},
    {0xab217db9u, 0xfc382935u}, {0xb1ed3ed2u, 0xcfaa576cu},
    {0x574ec514u, 0x908a6c5eu}, {0x5e1a862du, 0x63fc9a95u}
};

static const uint32_t k_f41a17f0[16][2] = {
    {0x84fd77b4u, 0x4d8c87aeu}, {0x9b4810cau, 0xbdc4e1e7u},
    {0xb192a9e0u, 0x2dfd3c20u}, {0x808825e3u, 0x971adc09u},
    {0x96d2bef9u, 0x07533642u}, {0xad1d580fu, 0x778b907bu},
    {0x7c12d412u, 0xe0a93064u}, {0x925d6d28u, 0x50e18a9du},
    {0xa8a8063u, 0xc119e4d6u}, {0xbef29f54u, 0x31523f0fu},
    {0x8de81b57u, 0xaa6fdef8u}, {0xa432b46du, 0x1aa83931u},
    {0xba7d4d83u, 0x8ae0936au}, {0x8972c986u, 0xf3fe3353u},
    {0x9fbd629cu, 0x64368d8cu}, {0xb607fbb2u, 0xd46ee7c5u}
};

static const uint32_t k_f41a1870[16][2] = {
    {0xb83dd7d7u, 0xe2c5d85du}, {0x5ac2a70cu, 0x347b2190u},
    {0x6dfaa5f4u, 0x8afa2ed6u}, {0x8132a4dcu, 0xd1793c1cu},
    {0x23b77411u, 0x232e854fu}, {0x36ef72f9u, 0x79ad9295u},
    {0x4a2771e1u, 0xc02c9fdbu}, {0x5d5f70c9u, 0x16abad21u},
    {0xffe43ffeu, 0x6860f653u}, {0x131c3ee6u, 0xbee0039au},
    {0x26543dceu, 0x055f10e0u}, {0xc8d90d03u, 0x57145a12u},
    {0xdc110bebu, 0xad936758u}, {0xef490ad3u, 0xf412749eu},
    {0x91cdda08u, 0x45c7bdd1u}, {0xa505d8f0u, 0x9c46cb17u}
};

static const uint32_t k_f41a18f0[16][2] = {
    {0x9d3c4d3bu, 0x5399eff1u}, {0x8b7a9999u, 0x288a9c21u},
    {0x79b8e5f7u, 0xfd7b4851u}, {0x3a630ac2u, 0xc903671bu},
    {0x28a15720u, 0x9df4134bu}, {0x16dfa37eu, 0x62e4bf7bu},
    {0x051defdcu, 0x37d56babu}, {0xc5c814a7u, 0x035d8a74u},
    {0xb4066105u, 0xd84e36a4u}, {0xa244ad63u, 0xad3ee2d4u},
    {0x62eed22eu, 0x78c7019eu}, {0x512d1e8cu, 0x4db7adceu},
    {0x3f6b6aeau, 0x12a859feu}, {0x00158fb5u, 0xee3078c8u},
    {0xee53dc13u, 0xb32124f7u}, {0xdc922871u, 0x8811d127u}
};

static void nibble_ladder64_run7(const uint32_t tab[16][2], uint32_t *lo, uint32_t *hi) {
    uint32_t xlo = *lo;
    uint32_t xhi = *hi;
    for (unsigned i = 0; i < 7u; ++i) {
        const unsigned nib = xlo & 0x0fu;
        const uint32_t shifted = (xlo >> 4) | (xhi << 28);
        const uint32_t tlo = tab[nib][0];
        const uint32_t nlo = shifted + tlo;
        const uint32_t nhi = (xhi >> 4) + tab[nib][1] + u32_add_carry(shifted, tlo);
        xlo = nlo;
        xhi = nhi;
    }
    *lo = xlo;
    *hi = xhi;
}

static uint32_t nibble_ladder64_run8_low(const uint32_t tab[16][2], uint32_t lo, uint32_t hi) {
    for (unsigned i = 0; i < 8u; ++i) {
        const unsigned nib = lo & 0x0fu;
        const uint32_t shifted = (lo >> 4) | (hi << 28);
        const uint32_t tlo = tab[nib][0];
        const uint32_t nlo = shifted + tlo;
        const uint32_t nhi = (hi >> 4) + tab[nib][1] + u32_add_carry(shifted, tlo);
        lo = nlo;
        hi = nhi;
    }
    return lo;
}

static uint32_t secondary_stage_ladder_mix(uint32_t lo, uint32_t hi) {
    uint32_t xlo = lo;
    uint32_t xhi = hi;
    nibble_ladder64_run7(k_f41a1870, &xlo, &xhi);
    const unsigned nib = xlo & 0x0fu;
    const uint32_t step8_low = ((xlo >> 4) | (xhi << 28)) + k_f41a1870[nib][0];
    return (step8_low >> 4) + k_f41a1870[step8_low & 0x0fu][0];
}

static void pair64_add(uint32_t acc[2], const uint32_t addend[2]) {
    const uint32_t old = acc[0];
    acc[0] = old + addend[0];
    acc[1] = acc[1] + addend[1] + u32_add_carry(old, addend[0]);
}

static void pair64_multiply(const uint32_t a[2], const uint32_t b[2], uint32_t out[2]) {
    const uint64_t p = (uint64_t)a[0] * (uint64_t)b[0];
    out[0] = (uint32_t)p;
    out[1] = (uint32_t)(a[1] * b[0] + a[0] * b[1] + (uint32_t)(p >> 32));
}

static void secondary_vector_reduce_seed_pair_from_token(uint32_t token, uint32_t out_pair[2]) {
    const uint32_t lo0 = u32_mullo(token, 0x8a84ad25u);
    const uint32_t lo = lo0 + 0x48adc2bbu;
    const uint32_t hi = token * 0xedbce81fu + u32_mulhi(token, 0x8a84ad25u) +
                        0x1909bd49u + (uint32_t)(0xb7523d44u < lo0);
    const uint32_t ladder_low = nibble_ladder64_run8_low(k_f41a16f0, lo, hi);

    const uint32_t lo1 = u32_mullo(token, 0xf46f119du);
    out_pair[0] = lo1 + 0xe78a1a03u;
    out_pair[1] = token * 0x7432d9acu + u32_mulhi(token, 0xf46f119du) +
                  ladder_low * 0xc8dc4ae7u + 0xf63b337eu +
                  (uint32_t)(0x1875e5fcu < lo1);
}

int l3_authorization_secondary_vector_reduce_seed_pairs13(
    const int32_t param1_13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_seed_pairs13[L3_AUTH_ROUND_SECONDARY_WORDS13][2]) {
    if (!param1_13 || !out_seed_pairs13) return -1;
    for (unsigned i = 0; i < L3_AUTH_ROUND_SECONDARY_WORDS13; ++i) {
        const unsigned lane = i & 7u;
        const uint32_t token = ((uint32_t)param1_13[i] * k_f4190c30[lane] +
                                k_f4190c50[lane]) * 0x2f489b01u + 0x49c3feeau;
        secondary_vector_reduce_seed_pair_from_token(token, out_seed_pairs13[i]);
    }
    return 0;
}

int l3_authorization_secondary_vector_reduce_lane_scalar_core(
    uint32_t param3,
    uint32_t param4,
    const uint32_t rolling_pair[2],
    l3_authorization_secondary_vector_reduce_lane_scalars *out) {
    if (!rolling_pair || !out) return -1;

    const uint32_t p3_mul3d0 = u32_mullo(param3, 0x3d0ca0b4u);
    const uint32_t p3_mul3a2 = u32_mullo(param3, 0x3a2d2f5fu);
    const uint32_t base_lo = p3_mul3d0 + 0x27d67a11u;
    const uint32_t base_hi = p3_mul3a2 + 0x16579516u;

    const uint32_t cur_lo = rolling_pair[0];
    const uint32_t cur_hi = rolling_pair[1];
    const uint64_t prod_cur_base_hi = (uint64_t)cur_lo * (uint64_t)base_hi;
    const uint32_t prod_cur_base_hi_lo = (uint32_t)prod_cur_base_hi;
    const uint32_t lane_lo = base_lo + prod_cur_base_hi_lo;

    const uint32_t base_hi_affine =
        param4 * 0x3a2d2f5fu +
        param3 * 0x960e9440u +
        u32_mulhi(param3, 0x3a2d2f5fu) +
        0xf58eef37u +
        (uint32_t)(0xe9a86ae9u < p3_mul3a2);

    const uint32_t lane_hi =
        param4 * 0x3d0ca0b4u +
        param3 * 0xcfd4b978u +
        u32_mulhi(param3, 0x3d0ca0b4u) +
        0x585bd669u +
        (uint32_t)(0xd82985eeu < p3_mul3d0) +
        base_hi_affine * cur_lo +
        base_hi * cur_hi +
        (uint32_t)(prod_cur_base_hi >> 32) +
        u32_add_carry(base_lo, prod_cur_base_hi_lo);

    const uint32_t lane_mul_e210 = u32_mullo(lane_lo, 0xe2105dbfu);
    uint32_t ladder_lo = lane_mul_e210 + 0x488bfc21u;
    uint32_t ladder_hi =
        lane_hi * 0xe2105dbfu +
        lane_lo * 0x83a3966bu +
        u32_mulhi(lane_lo, 0xe2105dbfu) +
        0x76ec41beu +
        (uint32_t)(0xb77403deu < lane_mul_e210);
    nibble_ladder64_run7(k_f41a1770, &ladder_lo, &ladder_hi);

    const uint64_t prod_ladder_700 = (uint64_t)ladder_lo * 0x70000000ull;
    const uint32_t prod_ladder_700_lo = (uint32_t)prod_ladder_700;
    const uint32_t prod_lane_89b = u32_mullo(lane_lo, 0x89b07747u);
    const uint32_t mid_sum = prod_lane_89b + prod_ladder_700_lo;
    const uint32_t second_lo = mid_sum + 0x98dec3a8u;
    const uint32_t mul8cec_low = u32_mullo(second_lo, 0x8cec7849u);
    const uint32_t second_hi =
        lane_hi * 0x89b07747u +
        lane_lo * 0xd7218c02u +
        u32_mulhi(lane_lo, 0x89b07747u) +
        ladder_lo * 0x7c41a278u +
        (uint32_t)(prod_ladder_700 >> 32) +
        ladder_hi * 0x70000000u +
        u32_add_carry(prod_lane_89b, prod_ladder_700_lo) +
        0x594716edu +
        (uint32_t)(0x67213c57u < mid_sum);

    const uint32_t scale = mul8cec_low + 0xaf1ff261u;
    const uint32_t mul19f4_low = u32_mullo(second_lo, 0x19f4bda9u);
    const uint32_t vector_bias = mul19f4_low + 0x93e670c1u;

    out->base_pair[0] = base_lo;
    out->base_pair[1] = base_hi;
    out->lane_pair[0] = lane_lo;
    out->lane_pair[1] = lane_hi;
    out->ladder1770_pair[0] = ladder_lo;
    out->ladder1770_pair[1] = ladder_hi;
    out->second_pair[0] = second_lo;
    out->second_pair[1] = second_hi;
    out->scale = scale;
    out->vector_bias = vector_bias;
    out->mul8cec_low = mul8cec_low;
    out->mul19f4_low = mul19f4_low;
    return 0;
}

static void secondary_vector_reduce_scale_pair(const l3_authorization_secondary_vector_reduce_lane_scalars *s, uint32_t out_pair[2]) {
    out_pair[0] = s->scale;
    out_pair[1] =
        s->second_pair[1] * 0x8cec7849u +
        s->second_pair[0] * 0x3a4d5fcfu +
        u32_mulhi(s->second_pair[0], 0x8cec7849u) +
        0x46a130d5u +
        (uint32_t)(0x50e00d9eu < s->mul8cec_low);
}

static void secondary_vector_reduce_vector_bias_pair(const l3_authorization_secondary_vector_reduce_lane_scalars *s, uint32_t out_pair[2]) {
    out_pair[0] = s->vector_bias;
    out_pair[1] =
        s->second_pair[1] * 0x19f4bda9u +
        s->second_pair[0] * 0x0c620766u +
        u32_mulhi(s->second_pair[0], 0x19f4bda9u) +
        0x78f261b1u +
        (uint32_t)(0x6c198f3eu < s->mul19f4_low);
}

int l3_authorization_secondary_vector_reduce_vector_feedback_stores(
    const uint32_t seed_pairs13[L3_AUTH_ROUND_SECONDARY_WORDS13][2],
    const l3_authorization_secondary_vector_reduce_lane_scalars *lane_scalars,
    unsigned lane_index,
    uint32_t state_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2],
    l3_authorization_secondary_vector_reduce_vector_feedback_result *out) {
    if (!seed_pairs13 || !lane_scalars || !state_pairs26) return -1;
    if (lane_index >= L3_AUTH_ROUND_SECONDARY_WORDS13 || lane_index + 11u >= L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26) return -2;

    uint32_t scale_pair[2];
    uint32_t bias_pair[2];
    secondary_vector_reduce_scale_pair(lane_scalars, scale_pair);
    secondary_vector_reduce_vector_bias_pair(lane_scalars, bias_pair);
    if (out) {
        out->scale_pair[0] = scale_pair[0];
        out->scale_pair[1] = scale_pair[1];
        out->vector_bias_pair[0] = bias_pair[0];
        out->vector_bias_pair[1] = bias_pair[1];
    }

    for (unsigned j = 0; j < 12u; ++j) {
        uint32_t prod[2];
        pair64_multiply(seed_pairs13[j], scale_pair, prod);
        if (out) {
            out->product_pairs12[j][0] = prod[0];
            out->product_pairs12[j][1] = prod[1];
        }
        pair64_add(state_pairs26[lane_index + j], bias_pair);
        pair64_add(state_pairs26[lane_index + j], prod);
    }
    return 0;
}

int l3_authorization_secondary_vector_reduce_lane_scalar_tail(
    const uint32_t seed_tail_pair[2],
    const l3_authorization_secondary_vector_reduce_lane_scalars *lane_scalars,
    unsigned lane_index,
    uint32_t state_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2],
    l3_authorization_secondary_vector_reduce_lane_tail_result *out) {
    if (!seed_tail_pair || !lane_scalars || !state_pairs26) return -1;
    if (lane_index + 12u >= L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26) return -2;

    uint32_t scale_pair[2];
    uint32_t bias_pair[2];
    secondary_vector_reduce_scale_pair(lane_scalars, scale_pair);
    secondary_vector_reduce_vector_bias_pair(lane_scalars, bias_pair);

    uint32_t tail_product[2];
    pair64_multiply(seed_tail_pair, scale_pair, tail_product);
    pair64_add(state_pairs26[lane_index + 12u], bias_pair);
    pair64_add(state_pairs26[lane_index + 12u], tail_product);

    const uint32_t cur_lo = state_pairs26[lane_index][0];
    const uint32_t cur_hi = state_pairs26[lane_index][1];
    const uint32_t next_lo_old = state_pairs26[lane_index + 1u][0];
    const uint32_t next_hi_old = state_pairs26[lane_index + 1u][1];

    const uint64_t p_cur = (uint64_t)cur_lo * 0x1f12f293ull;
    const uint32_t p_cur_lo = (uint32_t)p_cur;
    const uint32_t aff_lo = p_cur_lo + 0x22dded6au;
    const uint32_t aff_hi =
        cur_hi * 0x1f12f293u +
        cur_lo * 0x275217d2u +
        (uint32_t)(p_cur >> 32) +
        0xd47ba3cau +
        (uint32_t)(0xdd221295u < p_cur_lo);

    uint32_t lad17_lo = aff_lo;
    uint32_t lad17_hi = aff_hi;
    nibble_ladder64_run7(k_f41a17f0, &lad17_lo, &lad17_hi);

    const uint64_t p17 = (uint64_t)lad17_lo * 0xec43f26bull;
    const uint32_t p17_lo = (uint32_t)p17;
    const uint32_t post17_lo = p17_lo + 0xd9158a28u;
    const uint32_t post17_hi =
        lad17_hi * 0xec43f26bu +
        lad17_lo * 0x6d0ca6edu +
        (uint32_t)(p17 >> 32) +
        0x8ca8cc2du +
        (uint32_t)(0x26ea75d7u < p17_lo);

    const uint64_t p_post_9ad = (uint64_t)post17_lo * 0x9ad26cd5ull;
    const uint32_t p_post_9ad_lo = (uint32_t)p_post_9ad;
    const uint64_t p_corr = (uint64_t)p_post_9ad_lo * 0x72eb1dcdull;
    const uint32_t corr_lo = (uint32_t)p_corr;
    const uint32_t sum_next_lo = corr_lo + next_lo_old;
    const uint32_t final_next_lo = sum_next_lo + 0x7efa70c7u;

    const uint32_t lad1870_mix = secondary_stage_ladder_mix(
        u32_mullo(post17_lo, 0x266e191du) + 0xf8340742u,
        post17_hi * 0x266e191du + post17_lo * 0x883686a7u +
        u32_mulhi(post17_lo, 0x266e191du) + 0x2b52ded2u +
        (uint32_t)(0x07cbf8bdu < u32_mullo(post17_lo, 0x266e191du)));

    const uint32_t corr_inner =
        post17_hi * 0x9ad26cd5u +
        post17_lo * 0xd1d07dd7u +
        (uint32_t)(p_post_9ad >> 32) +
        lad1870_mix * 0x85e72e70u;
    const uint32_t final_next_hi =
        corr_inner * 0x72eb1dcdu +
        p_post_9ad_lo * 0x929db29cu +
        (uint32_t)(p_corr >> 32) +
        next_hi_old +
        u32_add_carry(corr_lo, next_lo_old) +
        0x8a1600e8u +
        (uint32_t)(0x81058f38u < sum_next_lo);

    state_pairs26[lane_index + 1u][0] = final_next_lo;
    state_pairs26[lane_index + 1u][1] = final_next_hi;

    if (out) {
        out->current_affine_pair[0] = aff_lo;
        out->current_affine_pair[1] = aff_hi;
        out->ladder17f0_pair[0] = lad17_lo;
        out->ladder17f0_pair[1] = lad17_hi;
        out->post17f0_pair[0] = post17_lo;
        out->post17f0_pair[1] = post17_hi;
        out->ladder1870_mix = lad1870_mix;
        out->tail_product_pair[0] = tail_product[0];
        out->tail_product_pair[1] = tail_product[1];
        out->final_lane1_pair[0] = final_next_lo;
        out->final_lane1_pair[1] = final_next_hi;
        out->final_lane12_pair[0] = state_pairs26[lane_index + 12u][0];
        out->final_lane12_pair[1] = state_pairs26[lane_index + 12u][1];
    }
    return 0;
}

int l3_authorization_secondary_vector_reduce_run13_update(
    const int32_t param1_13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t param3,
    uint32_t param4,
    uint32_t state_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2],
    l3_authorization_secondary_vector_reduce_run13_trace *out) {
    if (!param1_13 || !state_pairs26) return -1;
    uint32_t seed_pairs13[L3_AUTH_ROUND_SECONDARY_WORDS13][2];
    int rc = l3_authorization_secondary_vector_reduce_seed_pairs13(param1_13, seed_pairs13);
    if (rc != 0) return rc;
    if (out) memcpy(out->seed_pairs13, seed_pairs13, sizeof(seed_pairs13));

    for (unsigned lane = 0; lane < L3_AUTH_ROUND_SECONDARY_WORDS13; ++lane) {
        l3_authorization_secondary_vector_reduce_lane_scalars scalars;
        rc = l3_authorization_secondary_vector_reduce_lane_scalar_core(param3, param4, state_pairs26[lane], &scalars);
        if (rc != 0) return rc;
        if (out) out->lane_scalars[lane] = scalars;
        rc = l3_authorization_secondary_vector_reduce_vector_feedback_stores(seed_pairs13, &scalars, lane, state_pairs26, NULL);
        if (rc != 0) return rc;
        rc = l3_authorization_secondary_vector_reduce_lane_scalar_tail(seed_pairs13[12], &scalars, lane, state_pairs26,
                                             out ? &out->lane_tails[lane] : NULL);
        if (rc != 0) return rc;
    }
    return 0;
}

static void secondary_vector_reduce_ladder18f0_step(uint32_t *lo, uint32_t *hi) {
    const uint32_t xlo = *lo;
    const uint32_t xhi = *hi;
    const unsigned nib = xlo & 0x0fu;
    const uint32_t shifted = (xlo >> 4) | (xhi << 28);
    const uint32_t tlo = k_f41a18f0[nib][0];
    *lo = tlo + shifted;
    *hi = k_f41a18f0[nib][1] + (xhi >> 4) + u32_add_carry(tlo, shifted);
}

int l3_authorization_secondary_vector_reduce_export_words13(
    const uint32_t state_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2],
    uint32_t out_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_vector_reduce_export_trace *out) {
    if (!state_pairs26 || !out_words13) return -1;

    uint32_t rolling_lo = 0x46e5e60cu;
    uint32_t rolling_hi = 0x8dd273afu;

    for (unsigned lane = 0; lane < L3_AUTH_ROUND_SECONDARY_WORDS13; ++lane) {
        const uint32_t cur_lo = state_pairs26[13u + lane][0];
        const uint32_t cur_hi = state_pairs26[13u + lane][1];

        const uint64_t p_cur = (uint64_t)cur_lo * 0xefcaedcbull;
        const uint32_t p_cur_lo = (uint32_t)p_cur;
        const uint32_t p_roll_lo = u32_mullo(rolling_lo, 0xfb5571d5u);
        const uint32_t sum0 = p_roll_lo + p_cur_lo;
        const uint32_t base = sum0 + 0x4e44b5e0u;
        const uint32_t p_base = u32_mullo(base, 0x098384d9u);
        uint32_t ladder_lo = p_base + 0x63d751bbu;

        uint32_t ladder_hi =
            rolling_hi * 0xfb5571d5u +
            rolling_lo * 0xa681cbd4u +
            u32_mulhi(rolling_lo, 0xfb5571d5u) +
            cur_hi * 0xefcaedcbu +
            cur_lo * 0xd4ec87fau +
            (uint32_t)(p_cur >> 32) +
            u32_add_carry(p_roll_lo, p_cur_lo) +
            0x30597857u +
            (uint32_t)(0xb1bb4a1fu < sum0);
        ladder_hi =
            ladder_hi * 0x098384d9u +
            base * 0x03b96e16u +
            u32_mulhi(base, 0x098384d9u) +
            0x126bbe4bu +
            (uint32_t)(0x9c28ae44u < p_base);

        for (unsigned i = 0; i < 7u; ++i) secondary_vector_reduce_ladder18f0_step(&ladder_lo, &ladder_hi);

        const uint32_t ladder7_lo = ladder_lo;
        const uint32_t ladder7_hi = ladder_hi;
        const uint32_t roll_mul_lo = u32_mullo(ladder7_lo, 0x6ce25945u);
        const uint32_t pre_export = base * 0x1502560du + ladder7_lo * 0xb0000000u + 0xdbb8e349u;
        out_words13[lane] = pre_export * k_f4190c70[lane & 7u] + k_f4190c90[lane & 7u];

        for (unsigned i = 0; i < 7u; ++i) secondary_vector_reduce_ladder18f0_step(&ladder_lo, &ladder_hi);
        const unsigned nib = ladder_lo & 0x0fu;
        const uint32_t shifted = (ladder_lo >> 4) | (ladder_hi << 28);
        const uint32_t low_after15 = k_f41a18f0[nib][0] + shifted;
        const uint32_t final_low_lookup = k_f41a18f0[low_after15 & 0x0fu][0];

        rolling_lo = roll_mul_lo + 0xd142ccedu;
        rolling_hi =
            ladder7_hi * 0x6ce25945u +
            ladder7_lo * 0x8ff3b16du +
            u32_mulhi(ladder7_lo, 0x6ce25945u) +
            (final_low_lookup + (low_after15 >> 4)) * 0x31da6bb0u +
            0x1c124f47u +
            (uint32_t)(0x2ebd3312u < roll_mul_lo);

        if (out) {
            out->export_inputs13[lane][0] = cur_lo;
            out->export_inputs13[lane][1] = cur_hi;
            out->pre_export_word13[lane] = pre_export;
            out->ladder7_pair13[lane][0] = ladder7_lo;
            out->ladder7_pair13[lane][1] = ladder7_hi;
            out->low_after15_13[lane] = low_after15;
            out->rolling_pair_after13[lane][0] = rolling_lo;
            out->rolling_pair_after13[lane][1] = rolling_hi;
        }
    }
    return 0;
}

int l3_authorization_secondary_vector_reduce_run13_update_export(
    const int32_t param1_13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t param3,
    uint32_t param4,
    uint32_t state_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2],
    uint32_t out_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_vector_reduce_full_export_trace *out) {
    if (!param1_13 || !state_pairs26 || !out_words13) return -1;
    int rc = l3_authorization_secondary_vector_reduce_run13_update(param1_13, param3, param4, state_pairs26,
                                          out ? &out->run13 : NULL);
    if (rc != 0) return rc;
    return l3_authorization_secondary_vector_reduce_export_words13(state_pairs26, out_words13,
                                          out ? &out->export_trace : NULL);
}

/* ---- v269: FUN_f3fcc2d0 caller-bound f3fdc45c affine materialization ---- */

int l3_authorization_secondary_post_secondary_vector_reduce_affine_words13(
    const uint32_t secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_post_secondary_vector_reduce_affine_trace *out) {
    if (!secondary_vector_reduce_words13 || !out_words13) return -1;

    static const uint32_t k_group0_or_2[4] = {
        0x4057a567u, 0x3954f421u, 0x8caa51f9u, 0xdb695da5u
    };
    static const uint32_t add_group0_or_2[4] = {
        0x5654fb9cu, 0x05846138u, 0xe2973d92u, 0x516e7b0au
    };
    static const uint32_t k_group1[4] = {
        0x379b3989u, 0x399aaeb5u, 0x0d775099bu, 0x40d307c1u
    };
    static const uint32_t add_group1[4] = {
        0x91bd5910u, 0xef875be2u, 0x74a70dc1u, 0x44d847dcu
    };

    l3_authorization_secondary_post_secondary_vector_reduce_affine_trace tmp;
    l3_authorization_secondary_post_secondary_vector_reduce_affine_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));
    memcpy(dst->input_words13, secondary_vector_reduce_words13, sizeof(dst->input_words13));

    for (unsigned i = 0; i < 4u; ++i) {
        out_words13[i] =
            secondary_vector_reduce_words13[i] * k_group0_or_2[i] + add_group0_or_2[i];
        out_words13[4u + i] =
            secondary_vector_reduce_words13[4u + i] * k_group1[i] + add_group1[i];
        out_words13[8u + i] =
            secondary_vector_reduce_words13[8u + i] * k_group0_or_2[i] +
            add_group0_or_2[i];
        dst->affine_group0_4[i] = out_words13[i];
        dst->affine_group1_4[i] = out_words13[4u + i];
        dst->affine_group2_4[i] = out_words13[8u + i];
    }
    out_words13[12] = secondary_vector_reduce_words13[12] * 0x379b3989u + 0x91bd5910u;
    dst->scalar_tail_word12 = out_words13[12];
    memcpy(dst->output_words13, out_words13, sizeof(dst->output_words13));
    return 0;
}

int l3_authorization_secondary_param3_bound_secondary_vector_reduce_affine_words13(
    const int32_t param3_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t caller_seed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t caller_scalar0,
    uint32_t caller_scalar1,
    uint32_t out_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_bound_secondary_vector_reduce_affine_trace *out) {
    if (!param3_words13 || !caller_seed_words13 || !out_words13) return -1;

    l3_authorization_secondary_bound_secondary_vector_reduce_affine_trace tmp;
    l3_authorization_secondary_bound_secondary_vector_reduce_affine_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    l3_authorization_secondary_pair32 conv_pairs[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    int rc = l3_authorization_secondary_param3_accum_convolution26(
        param3_words13, conv_pairs, &dst->convolution);
    if (rc != 0) return rc;

    uint32_t state_pairs[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    for (unsigned i = 0; i < L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26; ++i) {
        state_pairs[i][0] = conv_pairs[i].lo;
        state_pairs[i][1] = conv_pairs[i].hi;
    }
    memcpy(dst->state_before_f3fdc45c26, state_pairs, sizeof(state_pairs));

    uint32_t secondary_vector_reduce_words[L3_AUTH_ROUND_SECONDARY_WORDS13];
    rc = l3_authorization_secondary_vector_reduce_run13_update_export(
        caller_seed_words13, caller_scalar0, caller_scalar1, state_pairs,
        secondary_vector_reduce_words, &dst->f3fdc45c);
    if (rc != 0) return rc;
    memcpy(dst->state_after_f3fdc45c26, state_pairs, sizeof(state_pairs));
    memcpy(dst->secondary_vector_reduce_words13, secondary_vector_reduce_words, sizeof(secondary_vector_reduce_words));

    return l3_authorization_secondary_post_secondary_vector_reduce_affine_words13(
        secondary_vector_reduce_words, out_words13, &dst->affine);
}

/* ---- v270: FUN_f3fcc2d0 param_5 / post-affine generated-pair accumulators ---- */

int l3_authorization_secondary_param5_accum1970_pairs13(
    const int32_t param5_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_param5_accum1970_trace *out) {
    if (!param5_words13 || !out_pairs13 || !out_cumulative13) return -1;

    l3_authorization_secondary_param5_accum1970_trace tmp;
    l3_authorization_secondary_param5_accum1970_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    uint32_t x = u32_mla((uint32_t)param5_words13[0], 0x2a8401b3u, 0x0971606bu);
    dst->transformed_words13[0] = x;
    out_pairs13[0] = make_1970_pair(x, &dst->final_fold_lows13[0]);
    out_cumulative13[0] = out_pairs13[0];
    dst->generated_pairs13[0] = out_pairs13[0];
    dst->cumulative_pairs13[0] = out_cumulative13[0];

    for (unsigned i = 1; i < L3_AUTH_ROUND_SECONDARY_WORDS13; ++i) {
        const unsigned j = i & 7u;
        uint32_t pre = u32_mla((uint32_t)param5_words13[i], k_f4190cb0[j], k_f4190cd0[j]);
        x = u32_mla(pre, 0xa23a6dabu, 0x760da515u);
        dst->transformed_words13[i] = x;
        out_pairs13[i] = make_1970_pair(x, &dst->final_fold_lows13[i]);
        out_cumulative13[i].lo = out_cumulative13[i - 1u].lo + out_pairs13[i].lo;
        out_cumulative13[i].hi = out_cumulative13[i - 1u].hi + out_pairs13[i].hi +
                                 u32_add_carry(out_cumulative13[i - 1u].lo, out_pairs13[i].lo);
        dst->generated_pairs13[i] = out_pairs13[i];
        dst->cumulative_pairs13[i] = out_cumulative13[i];
    }
    return 0;
}

int l3_authorization_secondary_affine_accum19f0_pairs13(
    const uint32_t affine_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_affine_accum19f0_trace *out) {
    if (!affine_words13 || !out_pairs13 || !out_cumulative13) return -1;

    l3_authorization_secondary_affine_accum19f0_trace tmp;
    l3_authorization_secondary_affine_accum19f0_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    uint32_t x = u32_mla(affine_words13[0], 0x878fe3a3u, 0xcb9d3754u);
    dst->transformed_words13[0] = x;
    out_pairs13[0] = make_19f0_pair(x, &dst->final_fold_lows13[0]);
    out_cumulative13[0] = out_pairs13[0];
    dst->generated_pairs13[0] = out_pairs13[0];
    dst->cumulative_pairs13[0] = out_cumulative13[0];

    for (unsigned i = 1; i < L3_AUTH_ROUND_SECONDARY_WORDS13; ++i) {
        const unsigned j = i & 7u;
        uint32_t pre = u32_mla(affine_words13[i], k_f4190cf0[j], k_f4190d10[j]);
        x = u32_mla(pre, 0xd2ac4603u, 0xc9dfbb7eu);
        dst->transformed_words13[i] = x;
        out_pairs13[i] = make_19f0_pair(x, &dst->final_fold_lows13[i]);
        out_cumulative13[i].lo = out_cumulative13[i - 1u].lo + out_pairs13[i].lo;
        out_cumulative13[i].hi = out_cumulative13[i - 1u].hi + out_pairs13[i].hi +
                                 u32_add_carry(out_cumulative13[i - 1u].lo, out_pairs13[i].lo);
        dst->generated_pairs13[i] = out_pairs13[i];
        dst->cumulative_pairs13[i] = out_cumulative13[i];
    }
    return 0;
}

int l3_authorization_secondary_param5_affine_accum_pairs13(
    const int32_t param5_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t affine_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 raw1970_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix1970_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 raw19f0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix19f0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_param5_affine_accum_trace *out) {
    if (!param5_words13 || !affine_words13 || !raw1970_13 || !prefix1970_13 ||
        !raw19f0_13 || !prefix19f0_13) return -1;

    l3_authorization_secondary_param5_affine_accum_trace tmp;
    l3_authorization_secondary_param5_affine_accum_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    int rc = l3_authorization_secondary_param5_accum1970_pairs13(
        param5_words13, raw1970_13, prefix1970_13, &dst->accum1970);
    if (rc != 0) return rc;
    return l3_authorization_secondary_affine_accum19f0_pairs13(
        affine_words13, raw19f0_13, prefix19f0_13, &dst->accum19f0);
}


/* ---- v271: FUN_f3fcc2d0 param_5/post-affine convolution-reduction ---- */

int l3_authorization_secondary_param5_convolution26(
    const l3_authorization_secondary_pair32 raw1970_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix1970_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 raw19f0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix19f0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_param5_convolution26_trace *out) {
    if (!raw1970_13 || !prefix1970_13 || !raw19f0_13 || !prefix19f0_13 || !out_pairs26) return -1;

    l3_authorization_secondary_param5_convolution26_trace tmp;
    l3_authorization_secondary_param5_convolution26_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    for (unsigned idx = 0; idx < L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26; ++idx) {
        unsigned first = idx - 12u;
        unsigned last;
        if (idx < 12u || first == 0u) {
            first = 0u;
            last = idx;
        } else {
            last = 12u;
        }

        uint32_t u = 0x59025044u;
        uint32_t v = 0x1c78e2a7u;
        l3_authorization_secondary_pair32 conv = {0u, 0u};
        l3_authorization_secondary_pair32 sum1970 = {0u, 0u};
        l3_authorization_secondary_pair32 sum19f0 = {0u, 0u};

        dst->first_last_count26[idx][0] = first;
        dst->first_last_count26[idx][1] = last;
        dst->first_last_count26[idx][2] = (first <= last) ? ((last - first) + 1u) : 0u;

        if (first <= last) {
            for (unsigned k = first; k <= last; ++k) {
                const l3_authorization_secondary_pair32 *a1970 = &raw1970_13[k];
                const l3_authorization_secondary_pair32 *b19f0 = &raw19f0_13[idx - k];
                const uint64_t prod = (uint64_t)a1970->lo * (uint64_t)b19f0->lo;
                const uint32_t old = conv.lo;
                conv.lo += (uint32_t)prod;
                conv.hi += b19f0->hi * a1970->lo + b19f0->lo * a1970->hi +
                           (uint32_t)(prod >> 32) + (uint32_t)(conv.lo < old);
            }

            secondary_pair_range_sum(prefix1970_13, first, last, &sum1970);
            secondary_pair_range_sum(prefix19f0_13, idx - last, idx - first, &sum19f0);

            const uint32_t count = (last - first) + 1u;
            const uint64_t count_prod = (uint64_t)count * 0x19280839u;
            const uint32_t count_lo = (uint32_t)count_prod;
            const uint32_t count_hi = (uint32_t)(count_prod >> 32);
            const uint64_t conv_scaled = (uint64_t)conv.lo * 0xa23ac999u;
            const uint32_t conv_scaled_lo = (uint32_t)conv_scaled;
            const uint64_t sum1970_scaled = (uint64_t)sum1970.lo * 0x6f9a36e9u;
            const uint32_t sum1970_scaled_lo = (uint32_t)sum1970_scaled;
            const uint64_t sum19f0_scaled = (uint64_t)sum19f0.lo * 0xd10589e9u;
            const uint32_t sum19f0_scaled_lo = (uint32_t)sum19f0_scaled;

            const uint32_t t0 = count_lo + 0x59025044u;
            const uint32_t t1 = t0 + conv_scaled_lo;
            const uint32_t t2 = t1 + sum1970_scaled_lo;
            u = t2 + sum19f0_scaled_lo;

            v = count * 0x86614a77u + count_hi + 0x1c78e2a7u +
                u32_add_carry(count_lo, 0x59025044u) +
                conv.hi * 0xa23ac999u + conv.lo * 0x79965fbfu +
                (uint32_t)(conv_scaled >> 32) + u32_add_carry(t0, conv_scaled_lo) +
                sum1970.hi * 0x6f9a36e9u + sum1970.lo * 0xc455cb08u +
                (uint32_t)(sum1970_scaled >> 32) + u32_add_carry(t1, sum1970_scaled_lo) +
                sum19f0.hi * 0xd10589e9u + sum19f0.lo * 0x4eb2ca75u +
                (uint32_t)(sum19f0_scaled >> 32) + u32_add_carry(t2, sum19f0_scaled_lo);
        }

        const uint32_t out_lo_mul = u * 0x1b7c388bu;
        out_pairs26[idx].lo = out_lo_mul + 0x9411cb48u;
        out_pairs26[idx].hi = v * 0x1b7c388bu + u * 0xf979cb9du +
                              u32_mulhi(u, 0x1b7c388bu) + 0xcbcad296u +
                              u32_add_carry(out_lo_mul, 0x9411cb48u);
        dst->convolution26[idx] = conv;
        dst->range1970_26[idx] = sum1970;
        dst->range19f0_26[idx] = sum19f0;
        dst->pre_reduce26[idx].lo = u;
        dst->pre_reduce26[idx].hi = v;
        dst->output26[idx] = out_pairs26[idx];
    }
    return 0;
}

int l3_authorization_secondary_param5_affine_accum_convolution26(
    const int32_t param5_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t affine_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_param5_convolution26_trace *out) {
    if (!param5_words13 || !affine_words13 || !out_pairs26) return -1;

    l3_authorization_secondary_pair32 raw1970[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 prefix1970[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 raw19f0[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 prefix19f0[L3_AUTH_ROUND_SECONDARY_PAIRS13];

    int rc = l3_authorization_secondary_param5_affine_accum_pairs13(
        param5_words13, affine_words13, raw1970, prefix1970, raw19f0, prefix19f0, NULL);
    if (rc != 0) return rc;
    return l3_authorization_secondary_param5_convolution26(raw1970, prefix1970, raw19f0, prefix19f0,
                                               out_pairs26, out);
}


/* ---- v272: second f3fdc45c call binding plus mixed local_390 export ---- */

static const uint32_t k_f41a1a70[16] = {
    0xaf2675eeu, 0x12ce64dcu, 0x867653cau, 0xfa1e42b8u,
    0x6dc631a6u, 0xddee7b9fu, 0x41966a8du, 0xb53e597bu,
    0x28e64869u, 0x9c8e3757u, 0x0cb68150u, 0x705e703eu,
    0xe4065f2cu, 0x57ae4e1au, 0xcb563d08u, 0x3b7e8701u
};

static const uint32_t k_f4190d30[8] = {
    0xe28c7eddu, 0xf92512e5u, 0x952c51bfu, 0x20d6fc99u,
    0x9939c617u, 0x2cba7c37u, 0xbd74d061u, 0x0e98f80fu
};

static const uint32_t k_f4190d50[8] = {
    0xd628c474u, 0x253f5146u, 0x34805685u, 0x429871c2u,
    0x0c644a27u, 0xccc87fcbu, 0x80a762c4u, 0x911b499eu
};

static uint32_t secondary_mix_ladder1a70_steps(uint32_t x, unsigned steps) {
    for (unsigned i = 0; i < steps; ++i) {
        x = k_f41a1a70[x & 0x0fu] + (x >> 4);
    }
    return x;
}

int l3_authorization_secondary_post_second_secondary_vector_reduce_mix390_words13(
    const uint32_t secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t param1_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t caller_mix_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_post_second_secondary_vector_reduce_mix390_trace *out) {
    if (!secondary_vector_reduce_words13 || !param1_words13 || !caller_mix_words13 || !out_words13) return -1;

    static const uint32_t k_out0_8[4] = {
        0xdc0c7029u, 0xfc2efcfbu, 0xc5fe8ea7u, 0xd5717f2fu
    };
    static const uint32_t k_out4[4] = {
        0x0b4f6241u, 0x2d8e5dd9u, 0x5f584c4du, 0x73125a7bu
    };
    static const uint32_t k_mix0_8[4] = {
        0xd38e9a5du, 0xaee8c001u, 0xf1cefe89u, 0x74056bedu
    };
    static const uint32_t k_p1_0_2[4] = {
        0xfa121827u, 0xec8ee4c1u, 0x67ab4923u, 0xbacec8f3u
    };
    static const uint32_t k_mix4[4] = {
        0xa1507cabu, 0x1c362219u, 0x9ae3fdedu, 0x0b7615527u
    };
    static const uint32_t k_p1_1[4] = {
        0x320c9b3bu, 0x1be56f7fu, 0xdb49c129u, 0xe71a176fu
    };
    static const uint32_t k_bias0_8[4] = {
        0x7e3bd7ddu, 0x00092834u, 0x11b1422du, 0x56b0787du
    };
    static const uint32_t k_bias4[4] = {
        0x3d56173au, 0x6dbaa6d9u, 0xb05324d9u, 0x220e77c4u
    };

    l3_authorization_secondary_post_second_secondary_vector_reduce_mix390_trace tmp;
    l3_authorization_secondary_post_second_secondary_vector_reduce_mix390_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));
    memcpy(dst->input_words13, secondary_vector_reduce_words13, sizeof(dst->input_words13));
    for (unsigned i = 0; i < L3_AUTH_ROUND_SECONDARY_WORDS13; ++i) {
        dst->param1_words13[i] = (uint32_t)param1_words13[i];
        dst->caller_mix_words13[i] = (uint32_t)caller_mix_words13[i];
    }

    for (unsigned i = 0; i < 4u; ++i) {
        dst->mixed_words13[i] =
            secondary_vector_reduce_words13[i] * k_out0_8[i] +
            (uint32_t)caller_mix_words13[i] * k_mix0_8[i] +
            (uint32_t)param1_words13[i] * k_p1_0_2[i] +
            k_bias0_8[i];
        dst->mixed_words13[4u + i] =
            secondary_vector_reduce_words13[4u + i] * k_out4[i] +
            (uint32_t)caller_mix_words13[4u + i] * k_mix4[i] +
            (uint32_t)param1_words13[4u + i] * k_p1_1[i] +
            k_bias4[i];
        dst->mixed_words13[8u + i] =
            secondary_vector_reduce_words13[8u + i] * k_out0_8[i] +
            (uint32_t)caller_mix_words13[8u + i] * k_mix0_8[i] +
            (uint32_t)param1_words13[8u + i] * k_p1_0_2[i] +
            k_bias0_8[i];
    }
    dst->mixed_words13[12] =
        (uint32_t)param1_words13[12] * k_p1_1[0] +
        secondary_vector_reduce_words13[12] * k_out4[0] +
        (uint32_t)caller_mix_words13[12] * k_mix4[0] +
        k_bias4[0];

    uint32_t rolling = 0xe778a56eu; /* -0x18875a92 */
    for (unsigned lane = 0; lane < L3_AUTH_ROUND_SECONDARY_WORDS13; ++lane) {
        dst->rolling_before13[lane] = rolling;
        const uint32_t x = rolling * 0x03080cebu + dst->mixed_words13[lane];
        const uint32_t seed = x * 0x8acff625u + 0x44e1b533u;
        const uint32_t ladder7 = secondary_mix_ladder1a70_steps(seed, 7u);
        const uint32_t ladder8 = secondary_mix_ladder1a70_steps(ladder7, 1u);
        const uint32_t pre = x * 0xab5ab7bfu + ladder7 * 0xd0000000u + 0x8ac691cdu;
        out_words13[lane] = pre * k_f4190d30[lane & 7u] + k_f4190d50[lane & 7u];

        dst->loop_x13[lane] = x;
        dst->ladder7_13[lane] = ladder7;
        dst->ladder8_rolling13[lane] = ladder8;
        dst->output_words13[lane] = out_words13[lane];
        rolling = ladder7 * 0x0f0ac1c7u + ladder8 * 0x0f53e390u + 0x09e1f2cdu;
    }
    return 0;
}

int l3_authorization_secondary_derive_state_stage1(
    const int32_t param5_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t affine_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t caller_seed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t caller_scalar0,
    uint32_t caller_scalar1,
    const int32_t param1_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t caller_mix_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_bound_second_secondary_vector_reduce_mix390_trace *out) {
    if (!param5_words13 || !affine_words13 || !caller_seed_words13 ||
        !param1_words13 || !caller_mix_words13 || !out_words13) return -1;

    l3_authorization_secondary_bound_second_secondary_vector_reduce_mix390_trace tmp;
    l3_authorization_secondary_bound_second_secondary_vector_reduce_mix390_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    l3_authorization_secondary_pair32 conv[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    int rc = l3_authorization_secondary_param5_affine_accum_convolution26(
        param5_words13, affine_words13, conv, &dst->convolution);
    if (rc != 0) return rc;

    uint32_t state_pairs[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    for (unsigned i = 0; i < L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26; ++i) {
        state_pairs[i][0] = conv[i].lo;
        state_pairs[i][1] = conv[i].hi;
    }
    memcpy(dst->state_before_f3fdc45c26, state_pairs, sizeof(state_pairs));

    uint32_t secondary_vector_reduce_words[L3_AUTH_ROUND_SECONDARY_WORDS13];
    rc = l3_authorization_secondary_vector_reduce_run13_update_export(
        caller_seed_words13, caller_scalar0, caller_scalar1, state_pairs,
        secondary_vector_reduce_words, &dst->f3fdc45c);
    if (rc != 0) return rc;
    memcpy(dst->state_after_f3fdc45c26, state_pairs, sizeof(state_pairs));
    memcpy(dst->secondary_vector_reduce_words13, secondary_vector_reduce_words, sizeof(secondary_vector_reduce_words));

    return l3_authorization_secondary_post_second_secondary_vector_reduce_mix390_words13(
        secondary_vector_reduce_words, param1_words13, caller_mix_words13, out_words13,
        &dst->mix390);
}

/* ---- v273: FUN_f3fcc2d0 param_3 / V269-affine generated-pair accumulators ---- */

int l3_authorization_secondary_param3_accum1ab0_pairs13(
    const int32_t param3_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_param3_accum1ab0_trace *out) {
    if (!param3_words13 || !out_pairs13 || !out_cumulative13) return -1;

    l3_authorization_secondary_param3_accum1ab0_trace tmp;
    l3_authorization_secondary_param3_accum1ab0_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    uint32_t x = u32_mla((uint32_t)param3_words13[0], 0x5da09da7u, 0x19781a4du);
    dst->transformed_words13[0] = x;
    out_pairs13[0] = make_1ab0_pair(x, &dst->final_fold_lows13[0]);
    out_cumulative13[0] = out_pairs13[0];
    dst->generated_pairs13[0] = out_pairs13[0];
    dst->cumulative_pairs13[0] = out_cumulative13[0];

    for (unsigned i = 1; i < L3_AUTH_ROUND_SECONDARY_WORDS13; ++i) {
        const unsigned j = i & 7u;
        uint32_t pre = u32_mla((uint32_t)param3_words13[i], k_f4190d70[j], k_f4190d90[j]);
        x = u32_mla(pre, 0xec59bd97u, 0x1e28670cu);
        dst->transformed_words13[i] = x;
        out_pairs13[i] = make_1ab0_pair(x, &dst->final_fold_lows13[i]);
        out_cumulative13[i].lo = out_cumulative13[i - 1u].lo + out_pairs13[i].lo;
        out_cumulative13[i].hi = out_cumulative13[i - 1u].hi + out_pairs13[i].hi +
                                 u32_add_carry(out_cumulative13[i - 1u].lo, out_pairs13[i].lo);
        dst->generated_pairs13[i] = out_pairs13[i];
        dst->cumulative_pairs13[i] = out_cumulative13[i];
    }
    return 0;
}

int l3_authorization_secondary_affine_accum1b30_pairs13(
    const uint32_t affine_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_affine_accum1b30_trace *out) {
    if (!affine_words13 || !out_pairs13 || !out_cumulative13) return -1;

    l3_authorization_secondary_affine_accum1b30_trace tmp;
    l3_authorization_secondary_affine_accum1b30_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    uint32_t x = u32_mla(affine_words13[0], 0x60965acbu, 0xe2552458u);
    dst->transformed_words13[0] = x;
    out_pairs13[0] = make_1b30_pair(x, &dst->final_fold_lows13[0]);
    out_cumulative13[0] = out_pairs13[0];
    dst->generated_pairs13[0] = out_pairs13[0];
    dst->cumulative_pairs13[0] = out_cumulative13[0];

    for (unsigned i = 1; i < L3_AUTH_ROUND_SECONDARY_WORDS13; ++i) {
        const unsigned j = i & 7u;
        uint32_t pre = u32_mla(affine_words13[i], k_f4190db0[j], k_f4190dd0[j]);
        x = u32_mla(pre, 0x7e4e513du, 0xa5c539f5u);
        dst->transformed_words13[i] = x;
        out_pairs13[i] = make_1b30_pair(x, &dst->final_fold_lows13[i]);
        out_cumulative13[i].lo = out_cumulative13[i - 1u].lo + out_pairs13[i].lo;
        out_cumulative13[i].hi = out_cumulative13[i - 1u].hi + out_pairs13[i].hi +
                                 u32_add_carry(out_cumulative13[i - 1u].lo, out_pairs13[i].lo);
        dst->generated_pairs13[i] = out_pairs13[i];
        dst->cumulative_pairs13[i] = out_cumulative13[i];
    }
    return 0;
}

int l3_authorization_secondary_param3_affine_accum1ab0_1b30_pairs13(
    const int32_t param3_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t affine_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 raw1ab0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix1ab0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 raw1b30_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix1b30_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_param3_affine_accum1ab0_1b30_trace *out) {
    if (!param3_words13 || !affine_words13 || !raw1ab0_13 || !prefix1ab0_13 ||
        !raw1b30_13 || !prefix1b30_13) return -1;

    l3_authorization_secondary_param3_affine_accum1ab0_1b30_trace tmp;
    l3_authorization_secondary_param3_affine_accum1ab0_1b30_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    int rc = l3_authorization_secondary_param3_accum1ab0_pairs13(
        param3_words13, raw1ab0_13, prefix1ab0_13, &dst->accum1ab0);
    if (rc != 0) return rc;
    return l3_authorization_secondary_affine_accum1b30_pairs13(
        affine_words13, raw1b30_13, prefix1b30_13, &dst->accum1b30);
}

/* ---- v274: FUN_f3fcc2d0 param_3/V269-affine convolution-reduction ---- */

int l3_authorization_secondary_param3_affine_convolution26(
    const l3_authorization_secondary_pair32 raw1ab0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix1ab0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 raw1b30_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix1b30_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_param3_affine_convolution26_trace *out) {
    if (!raw1ab0_13 || !prefix1ab0_13 || !raw1b30_13 || !prefix1b30_13 || !out_pairs26) return -1;

    l3_authorization_secondary_param3_affine_convolution26_trace tmp;
    l3_authorization_secondary_param3_affine_convolution26_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    for (unsigned idx = 0; idx < L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26; ++idx) {
        unsigned first = idx - 12u;
        unsigned last;
        if (idx < 12u || first == 0u) {
            first = 0u;
            last = idx;
        } else {
            last = 12u;
        }

        uint32_t u = 0x7f0c8421u;
        uint32_t v = 0x9f49243du; /* -0x60b6dbc3 */
        l3_authorization_secondary_pair32 conv = {0u, 0u};
        l3_authorization_secondary_pair32 sum1ab0 = {0u, 0u};
        l3_authorization_secondary_pair32 sum1b30 = {0u, 0u};

        dst->first_last_count26[idx][0] = first;
        dst->first_last_count26[idx][1] = last;
        dst->first_last_count26[idx][2] = (first <= last) ? ((last - first) + 1u) : 0u;

        if (first <= last) {
            for (unsigned k = first; k <= last; ++k) {
                const l3_authorization_secondary_pair32 *a1ab0 = &raw1ab0_13[k];
                const l3_authorization_secondary_pair32 *b1b30 = &raw1b30_13[idx - k];
                const uint64_t prod = (uint64_t)a1ab0->lo * (uint64_t)b1b30->lo;
                const uint32_t old = conv.lo;
                conv.lo += (uint32_t)prod;
                conv.hi += b1b30->hi * a1ab0->lo + b1b30->lo * a1ab0->hi +
                           (uint32_t)(prod >> 32) + (uint32_t)(conv.lo < old);
            }

            secondary_pair_range_sum(prefix1ab0_13, first, last, &sum1ab0);
            secondary_pair_range_sum(prefix1b30_13, idx - last, idx - first, &sum1b30);

            const uint32_t count = (last - first) + 1u;
            const uint64_t count_prod = (uint64_t)count * 0x14f6f2e8u;
            const uint32_t count_lo = (uint32_t)count_prod;
            const uint32_t count_hi = (uint32_t)(count_prod >> 32);
            const uint64_t conv_scaled = (uint64_t)conv.lo * 0x551f8f5du;
            const uint32_t conv_scaled_lo = (uint32_t)conv_scaled;
            const uint64_t sum1ab0_scaled = (uint64_t)sum1ab0.lo * 0xc69c2562u;
            const uint32_t sum1ab0_scaled_lo = (uint32_t)sum1ab0_scaled;
            const uint64_t sum1b30_scaled = (uint64_t)sum1b30.lo * 0x9ff4fe64u;
            const uint32_t sum1b30_scaled_lo = (uint32_t)sum1b30_scaled;

            const uint32_t t0 = count_lo + 0x7f0c8421u;
            const uint32_t t1 = t0 + conv_scaled_lo;
            const uint32_t t2 = t1 + sum1ab0_scaled_lo;
            u = t2 + sum1b30_scaled_lo;

            v = count * 0x7e1e5ef6u + count_hi + 0x9f49243du +
                u32_add_carry(count_lo, 0x7f0c8421u) +
                conv.hi * 0x551f8f5du + conv.lo * 0x901ea469u +
                (uint32_t)(conv_scaled >> 32) + u32_add_carry(t0, conv_scaled_lo) +
                sum1ab0.hi * 0xc69c2562u + sum1ab0.lo * 0x4ed01793u +
                (uint32_t)(sum1ab0_scaled >> 32) + u32_add_carry(t1, sum1ab0_scaled_lo) +
                sum1b30.hi * 0x9ff4fe64u + sum1b30.lo * 0x7dcdaaafu +
                (uint32_t)(sum1b30_scaled >> 32) + u32_add_carry(t2, sum1b30_scaled_lo);
        }

        const uint32_t out_lo_mul = u * 0x5f29c52bu;
        out_pairs26[idx].lo = out_lo_mul + 0x15c1a9a9u;
        out_pairs26[idx].hi = v * 0x5f29c52bu + u * 0xb5f184b3u +
                              u32_mulhi(u, 0x5f29c52bu) + 0xf68c3201u +
                              u32_add_carry(out_lo_mul, 0x15c1a9a9u);
        dst->convolution26[idx] = conv;
        dst->range1ab0_26[idx] = sum1ab0;
        dst->range1b30_26[idx] = sum1b30;
        dst->pre_reduce26[idx].lo = u;
        dst->pre_reduce26[idx].hi = v;
        dst->output26[idx] = out_pairs26[idx];
    }
    return 0;
}

int l3_authorization_secondary_param3_affine_accum_convolution26(
    const int32_t param3_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t affine_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_param3_affine_convolution26_trace *out) {
    if (!param3_words13 || !affine_words13 || !out_pairs26) return -1;

    l3_authorization_secondary_pair32 raw1ab0[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 prefix1ab0[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 raw1b30[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 prefix1b30[L3_AUTH_ROUND_SECONDARY_PAIRS13];

    int rc = l3_authorization_secondary_param3_affine_accum1ab0_1b30_pairs13(
        param3_words13, affine_words13, raw1ab0, prefix1ab0, raw1b30, prefix1b30, NULL);
    if (rc != 0) return rc;
    return l3_authorization_secondary_param3_affine_convolution26(raw1ab0, prefix1ab0, raw1b30, prefix1b30,
                                                       out_pairs26, out);
}

/* ---- v275: third f3fdc45c binding plus mixed local_3c4 export ---- */

static const uint32_t k_f41a1cb0[16] = {
    0x4ca57816u, 0xb7ce3d39u, 0x22f7025cu, 0x9e1fc77fu,
    0x09488ca2u, 0x747151c5u, 0xef9a16e8u, 0x5ac2dc0bu,
    0xc5eba12eu, 0x31146651u, 0xac3d2b74u, 0x1765f097u,
    0x828eb5bau, 0xfb2b28aeu, 0x6653edd1u, 0xd17cb2f4u
};

static const uint32_t k_f4190e70[8] = {
    0x25c1c247u, 0x4cec96a9u, 0x0e9c5e5fu, 0x80994b71u,
    0x38b52b8bu, 0x76df64d1u, 0x19b9a1cdu, 0x625593bdu
};

static const uint32_t k_f4190e90[8] = {
    0xef68f763u, 0x78f08e0eu, 0x53483782u, 0xda438132u,
    0x49102d2au, 0x6c7f0103u, 0x7f64b4adu, 0x652b3328u
};

static uint32_t secondary_mix_ladder1cb0_steps(uint32_t x, unsigned steps) {
    for (unsigned i = 0; i < steps; ++i) {
        x = k_f41a1cb0[x & 0x0fu] + (x >> 4);
    }
    return x;
}

int l3_authorization_secondary_post_third_secondary_vector_reduce_mix3c4_words13(
    const uint32_t secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t param2_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t caller_mix_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_post_third_secondary_vector_reduce_mix3c4_trace *out) {
    if (!secondary_vector_reduce_words13 || !local390_words13 || !param2_words13 || !caller_mix_words13 || !out_words13) return -1;

    static const uint32_t k_out0_8[4] = {
        0xd4edd1f5u, 0x955c285fu, 0x79d6d49bu, 0x0b9a6283u
    };
    static const uint32_t k_mix0_8[4] = {
        0x223ed855u, 0xc761b345u, 0x846ee525u, 0x77177febu
    };
    static const uint32_t k_p2_0_2[4] = {
        0xdc31b49bu, 0x21360377u, 0xed16eaddu, 0x77291913u
    };
    static const uint32_t k_out4[4] = {
        0x8346922du, 0x003e1de5u, 0xe8128f49u, 0xec53b7dfu
    };
    static const uint32_t k_mix4[4] = {
        0x15ce8d53u, 0x99a1c99du, 0x97119abfu, 0x36016f39u
    };
    static const uint32_t k_p2_1[4] = {
        0x25462afbu, 0x26fcb803u, 0x07375dc7u, 0xf78e7de7u
    };
    static const uint32_t k_bias0_8[4] = {
        0x6bba7324u, 0x001311e3u, 0x87d8374bu, 0x2b8630f3u
    };
    static const uint32_t k_bias4[4] = {
        0xdd189bc1u, 0xd7db33f7u, 0xa8d47ebcu, 0xfe9bdb02u
    };

    l3_authorization_secondary_post_third_secondary_vector_reduce_mix3c4_trace tmp;
    l3_authorization_secondary_post_third_secondary_vector_reduce_mix3c4_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));
    memcpy(dst->input_words13, secondary_vector_reduce_words13, sizeof(dst->input_words13));
    memcpy(dst->local390_words13, local390_words13, sizeof(dst->local390_words13));
    for (unsigned i = 0; i < L3_AUTH_ROUND_SECONDARY_WORDS13; ++i) {
        dst->param2_words13[i] = (uint32_t)param2_words13[i];
        dst->caller_mix_words13[i] = (uint32_t)caller_mix_words13[i];
    }

    for (unsigned i = 0; i < 4u; ++i) {
        dst->mixed_words13[i] =
            secondary_vector_reduce_words13[i] * k_out0_8[i] +
            (uint32_t)caller_mix_words13[i] * k_mix0_8[i] +
            (uint32_t)param2_words13[i] * k_p2_0_2[i] +
            k_bias0_8[i];
        dst->mixed_words13[4u + i] =
            secondary_vector_reduce_words13[4u + i] * k_out4[i] +
            (uint32_t)caller_mix_words13[4u + i] * k_mix4[i] +
            (uint32_t)param2_words13[4u + i] * k_p2_1[i] +
            k_bias4[i];
        dst->mixed_words13[8u + i] =
            secondary_vector_reduce_words13[8u + i] * k_out0_8[i] +
            (uint32_t)caller_mix_words13[8u + i] * k_mix0_8[i] +
            (uint32_t)param2_words13[8u + i] * k_p2_0_2[i] +
            k_bias0_8[i];
    }
    dst->mixed_words13[12] =
        (uint32_t)param2_words13[12] * k_p2_1[0] +
        secondary_vector_reduce_words13[12] * k_out4[0] +
        (uint32_t)caller_mix_words13[12] * k_mix4[0] +
        k_bias4[0];

    /* Native keeps the previous local_390[0] live across the call and uses it
     * as the first rolling scalar for this DAT_f41a1cb0 export ladder. */
    uint32_t rolling = 0x439db9b8u;
    for (unsigned lane = 0; lane < L3_AUTH_ROUND_SECONDARY_WORDS13; ++lane) {
        uint32_t input = dst->mixed_words13[lane];
        if (lane == 0u) {
            /* Keep the carried dependency explicit in the trace.  The value is
             * not added directly to mixed_words13; it is the value read before
             * the third FUN_f3fdc45c call in the native basic block. */
            dst->local390_words13[0] = local390_words13[0];
        }
        dst->rolling_before13[lane] = rolling;
        const uint32_t x = rolling * 0xfcb052ebu + input;
        const uint32_t seed = x * 0x1fdffd25u + 0x1f32ef20u;
        const uint32_t ladder7 = secondary_mix_ladder1cb0_steps(seed, 7u);
        const uint32_t ladder8 = secondary_mix_ladder1cb0_steps(ladder7, 1u);
        const uint32_t pre = x * 0xb6148d45u + ladder7 * 0xf0000000u + 0xa2b28478u;
        out_words13[lane] = pre * k_f4190e70[lane & 7u] + k_f4190e90[lane & 7u];

        dst->loop_x13[lane] = x;
        dst->ladder7_13[lane] = ladder7;
        dst->ladder8_rolling13[lane] = ladder8;
        dst->output_words13[lane] = out_words13[lane];
        rolling = ladder7 * 0x3e67a6c7u + ladder8 * 0x19859390u + 0x8644fd7cu;
    }
    return 0;
}

int l3_authorization_secondary_derive_state_stage2(
    const int32_t param3_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t affine_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t caller_seed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t caller_scalar0,
    uint32_t caller_scalar1,
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t param2_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t caller_mix_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_bound_third_secondary_vector_reduce_mix3c4_trace *out) {
    if (!param3_words13 || !affine_words13 || !caller_seed_words13 || !local390_words13 ||
        !param2_words13 || !caller_mix_words13 || !out_words13) return -1;

    l3_authorization_secondary_bound_third_secondary_vector_reduce_mix3c4_trace tmp;
    l3_authorization_secondary_bound_third_secondary_vector_reduce_mix3c4_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    l3_authorization_secondary_pair32 conv[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    int rc = l3_authorization_secondary_param3_affine_accum_convolution26(
        param3_words13, affine_words13, conv, &dst->convolution);
    if (rc < 0) return rc;

    uint32_t state_pairs[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    for (unsigned i = 0; i < L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26; ++i) {
        state_pairs[i][0] = conv[i].lo;
        state_pairs[i][1] = conv[i].hi;
    }
    memcpy(dst->state_before_f3fdc45c26, state_pairs, sizeof(state_pairs));

    uint32_t secondary_vector_reduce_words[L3_AUTH_ROUND_SECONDARY_WORDS13];
    rc = l3_authorization_secondary_vector_reduce_run13_update_export(
        caller_seed_words13, caller_scalar0, caller_scalar1,
        state_pairs, secondary_vector_reduce_words, &dst->f3fdc45c);
    if (rc < 0) return rc;

    memcpy(dst->state_after_f3fdc45c26, state_pairs, sizeof(state_pairs));
    memcpy(dst->secondary_vector_reduce_words13, secondary_vector_reduce_words, sizeof(secondary_vector_reduce_words));

    return l3_authorization_secondary_post_third_secondary_vector_reduce_mix3c4_words13(
        secondary_vector_reduce_words, local390_words13, param2_words13, caller_mix_words13,
        out_words13, &dst->mix3c4);
}

int l3_authorization_secondary_param6_third_export_convolution26(
    const int32_t caller_param6_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t third_secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32
        out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26]) {
    if (!caller_param6_words13 || !third_secondary_vector_reduce_words13 ||
        !out_pairs26) {
        return -1;
    }

    l3_authorization_secondary_pair32 raw_param6[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 prefix_param6[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 raw_export[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 prefix_export[L3_AUTH_ROUND_SECONDARY_PAIRS13];

    for (unsigned lane = 0; lane < L3_AUTH_ROUND_SECONDARY_WORDS13; ++lane) {
        uint32_t x_param6;
        uint32_t x_export;
        if (lane == 0u) {
            x_param6 =
                (uint32_t)caller_param6_words13[0] * 0x3ea4dfe7u +
                0x10f8c053u;
            x_export = third_secondary_vector_reduce_words13[0] * 0x61192587u +
                       0xce46c908u;
        } else {
            const unsigned slot = lane & 7u;
            const uint32_t pre_param6 =
                (uint32_t)caller_param6_words13[lane] *
                    k_f4190df0[slot] +
                k_f4190e10[slot];
            const uint32_t pre_export =
                third_secondary_vector_reduce_words13[lane] *
                    k_f4190e30[slot] +
                k_f4190e50[slot];
            x_param6 = pre_param6 * 0x13caa7b1u + 0x6c20e49bu;
            x_export = pre_export * 0x06cf672fu + 0xbec643cdu;
        }

        raw_param6[lane] = make_1bb0_pair(x_param6, NULL);
        raw_export[lane] = make_1c30_pair(x_export, NULL);
        if (lane == 0u) {
            prefix_param6[lane] = raw_param6[lane];
            prefix_export[lane] = raw_export[lane];
        } else {
            prefix_param6[lane].lo =
                prefix_param6[lane - 1u].lo + raw_param6[lane].lo;
            prefix_param6[lane].hi =
                prefix_param6[lane - 1u].hi + raw_param6[lane].hi +
                u32_add_carry(
                    prefix_param6[lane - 1u].lo, raw_param6[lane].lo);
            prefix_export[lane].lo =
                prefix_export[lane - 1u].lo + raw_export[lane].lo;
            prefix_export[lane].hi =
                prefix_export[lane - 1u].hi + raw_export[lane].hi +
                u32_add_carry(
                    prefix_export[lane - 1u].lo, raw_export[lane].lo);
        }
    }

    for (unsigned idx = 0; idx < L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26; ++idx) {
        unsigned first = idx - 12u;
        unsigned last;
        if (idx < 12u || first == 0u) {
            first = 0u;
            last = idx;
        } else {
            last = 12u;
        }

        uint32_t u = 0x395287bau;
        uint32_t v = 0x0bd467b4u;
        if (first <= last) {
            l3_authorization_secondary_pair32 conv = {0u, 0u};
            for (unsigned k = first; k <= last; ++k) {
                const l3_authorization_secondary_pair32 *a = &raw_export[k];
                const l3_authorization_secondary_pair32 *b =
                    &raw_param6[idx - k];
                const uint64_t product =
                    (uint64_t)a->lo * (uint64_t)b->lo;
                const uint32_t old_lo = conv.lo;
                conv.lo += (uint32_t)product;
                conv.hi +=
                    b->hi * a->lo + b->lo * a->hi +
                    (uint32_t)(product >> 32) +
                    (uint32_t)(conv.lo < old_lo);
            }

            l3_authorization_secondary_pair32 sum_param6;
            l3_authorization_secondary_pair32 sum_export;
            secondary_pair_range_sum(
                prefix_param6, idx - last, idx - first, &sum_param6);
            secondary_pair_range_sum(
                prefix_export, first, last, &sum_export);

            const uint32_t count = (last - first) + 1u;
            const uint64_t count_product =
                (uint64_t)count * 0x56be390cu;
            const uint32_t count_lo = (uint32_t)count_product;
            const uint32_t count_hi = (uint32_t)(count_product >> 32);
            const uint64_t conv_scaled =
                (uint64_t)conv.lo * 0x895dab67u;
            const uint32_t conv_scaled_lo = (uint32_t)conv_scaled;
            const uint64_t param6_scaled =
                (uint64_t)sum_param6.lo * 0x43535b11u;
            const uint32_t param6_scaled_lo =
                (uint32_t)param6_scaled;
            const uint64_t export_scaled =
                (uint64_t)sum_export.lo * 0x2ba63294u;
            const uint32_t export_scaled_lo =
                (uint32_t)export_scaled;

            const uint32_t t0 = count_lo + 0x395287bau;
            const uint32_t t1 = t0 + conv_scaled_lo;
            const uint32_t t2 = t1 + param6_scaled_lo;
            u = t2 + export_scaled_lo;

            v = count * 0x1c5ae5bcu + count_hi + 0x0bd467b4u +
                u32_add_carry(count_lo, 0x395287bau) +
                conv.hi * 0x895dab67u + conv.lo * 0xf72003eeu +
                (uint32_t)(conv_scaled >> 32) +
                u32_add_carry(t0, conv_scaled_lo) +
                sum_param6.hi * 0x43535b11u +
                sum_param6.lo * 0xebc14d6bu +
                (uint32_t)(param6_scaled >> 32) +
                u32_add_carry(t1, param6_scaled_lo) +
                sum_export.hi * 0x2ba63294u +
                sum_export.lo * 0x0a289c00u +
                (uint32_t)(export_scaled >> 32) +
                u32_add_carry(t2, export_scaled_lo);
        }

        const uint32_t out_lo_mul = u * 0x95a10027u;
        out_pairs26[idx].lo = out_lo_mul + 0x1d4592deu;
        out_pairs26[idx].hi =
            v * 0x95a10027u + u * 0x62edd591u +
            u32_mulhi(u, 0x95a10027u) + 0xb9ba7b81u +
            u32_add_carry(out_lo_mul, 0x1d4592deu);
    }
    return 0;
}



/* ---- v276: FUN_f3fcc2d0 local_390 generated-pair accumulators ---- */

int l3_authorization_secondary_local390_accum1cf0_pairs13(
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_local390_accum1cf0_trace *out) {
    if (!local390_words13 || !out_pairs13 || !out_cumulative13) return -1;

    l3_authorization_secondary_local390_accum1cf0_trace tmp;
    l3_authorization_secondary_local390_accum1cf0_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    uint32_t x = u32_mla(local390_words13[0], 0xa0afecbfu, 0x5710f441u);
    dst->transformed_words13[0] = x;
    out_pairs13[0] = make_1cf0_pair(x, &dst->final_fold_lows13[0]);
    out_cumulative13[0] = out_pairs13[0];
    dst->generated_pairs13[0] = out_pairs13[0];
    dst->cumulative_pairs13[0] = out_cumulative13[0];

    for (unsigned i = 1; i < L3_AUTH_ROUND_SECONDARY_WORDS13; ++i) {
        const unsigned j = i & 7u;
        uint32_t pre = u32_mla(local390_words13[i], k_f4190eb0[j], k_f4190ed0[j]);
        x = u32_mla(pre, 0x6f28ab8bu, 0xe0505ceau);
        dst->transformed_words13[i] = x;
        out_pairs13[i] = make_1cf0_pair(x, &dst->final_fold_lows13[i]);
        out_cumulative13[i].lo = out_cumulative13[i - 1u].lo + out_pairs13[i].lo;
        out_cumulative13[i].hi = out_cumulative13[i - 1u].hi + out_pairs13[i].hi +
                                 u32_add_carry(out_cumulative13[i - 1u].lo, out_pairs13[i].lo);
        dst->generated_pairs13[i] = out_pairs13[i];
        dst->cumulative_pairs13[i] = out_cumulative13[i];
    }
    return 0;
}

int l3_authorization_secondary_local390_accum1d70_pairs13(
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_local390_accum1d70_trace *out) {
    if (!local390_words13 || !out_pairs13 || !out_cumulative13) return -1;

    l3_authorization_secondary_local390_accum1d70_trace tmp;
    l3_authorization_secondary_local390_accum1d70_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    uint32_t x = u32_mla(local390_words13[0], 0x13a70e07u, 0x8caa29e1u);
    dst->transformed_words13[0] = x;
    out_pairs13[0] = make_1d70_pair(x, &dst->final_fold_lows13[0]);
    out_cumulative13[0] = out_pairs13[0];
    dst->generated_pairs13[0] = out_pairs13[0];
    dst->cumulative_pairs13[0] = out_cumulative13[0];

    for (unsigned i = 1; i < L3_AUTH_ROUND_SECONDARY_WORDS13; ++i) {
        const unsigned j = i & 7u;
        uint32_t pre = u32_mla(local390_words13[i], k_f4190ef0[j], k_f4190f10[j]);
        x = u32_mla(pre, 0x18592f91u, 0x3788ef09u);
        dst->transformed_words13[i] = x;
        out_pairs13[i] = make_1d70_pair(x, &dst->final_fold_lows13[i]);
        out_cumulative13[i].lo = out_cumulative13[i - 1u].lo + out_pairs13[i].lo;
        out_cumulative13[i].hi = out_cumulative13[i - 1u].hi + out_pairs13[i].hi +
                                 u32_add_carry(out_cumulative13[i - 1u].lo, out_pairs13[i].lo);
        dst->generated_pairs13[i] = out_pairs13[i];
        dst->cumulative_pairs13[i] = out_cumulative13[i];
    }
    return 0;
}

int l3_authorization_secondary_local390_accum1cf0_1d70_pairs13(
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 raw1cf0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix1cf0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 raw1d70_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix1d70_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_local390_accum1cf0_1d70_trace *out) {
    if (!local390_words13 || !raw1cf0_13 || !prefix1cf0_13 ||
        !raw1d70_13 || !prefix1d70_13) return -1;

    l3_authorization_secondary_local390_accum1cf0_1d70_trace tmp;
    l3_authorization_secondary_local390_accum1cf0_1d70_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    int rc = l3_authorization_secondary_local390_accum1cf0_pairs13(
        local390_words13, raw1cf0_13, prefix1cf0_13, &dst->accum1cf0);
    if (rc != 0) return rc;
    return l3_authorization_secondary_local390_accum1d70_pairs13(
        local390_words13, raw1d70_13, prefix1d70_13, &dst->accum1d70);
}

/* ---- v277: FUN_f3fcc2d0 local_390 convolution/reduction ---- */

int l3_authorization_secondary_local390_convolution26(
    const l3_authorization_secondary_pair32 raw1cf0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix1cf0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 raw1d70_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix1d70_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_local390_convolution26_trace *out) {
    if (!raw1cf0_13 || !prefix1cf0_13 || !raw1d70_13 || !prefix1d70_13 || !out_pairs26) return -1;

    l3_authorization_secondary_local390_convolution26_trace tmp;
    l3_authorization_secondary_local390_convolution26_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    for (unsigned idx = 0; idx < L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26; ++idx) {
        unsigned first = idx - 12u;
        unsigned last;
        if (idx < 12u || first == 0u) {
            first = 0u;
            last = idx;
        } else {
            last = 12u;
        }

        uint32_t u = 0xa084b402u;
        uint32_t v = 0x49fb0908u;
        l3_authorization_secondary_pair32 conv = {0u, 0u};
        l3_authorization_secondary_pair32 sum1cf0 = {0u, 0u};
        l3_authorization_secondary_pair32 sum1d70 = {0u, 0u};

        dst->first_last_count26[idx][0] = first;
        dst->first_last_count26[idx][1] = last;
        dst->first_last_count26[idx][2] = (first <= last) ? ((last - first) + 1u) : 0u;

        if (first <= last) {
            for (unsigned k = first; k <= last; ++k) {
                const l3_authorization_secondary_pair32 *a1cf0 = &raw1cf0_13[k];
                const l3_authorization_secondary_pair32 *b1d70 = &raw1d70_13[idx - k];
                const uint64_t prod = (uint64_t)a1cf0->lo * (uint64_t)b1d70->lo;
                const uint32_t old = conv.lo;
                conv.lo += (uint32_t)prod;
                conv.hi += b1d70->hi * a1cf0->lo + b1d70->lo * a1cf0->hi +
                           (uint32_t)(prod >> 32) + (uint32_t)(conv.lo < old);
            }

            secondary_pair_range_sum(prefix1cf0_13, first, last, &sum1cf0);
            secondary_pair_range_sum(prefix1d70_13, idx - last, idx - first, &sum1d70);

            const uint32_t count = (last - first) + 1u;
            const uint64_t count_prod = (uint64_t)count * 0x5349f9e6u;
            const uint32_t count_lo = (uint32_t)count_prod;
            const uint32_t count_hi = (uint32_t)(count_prod >> 32);
            const uint64_t conv_scaled = (uint64_t)conv.lo * 0x36374889u;
            const uint32_t conv_scaled_lo = (uint32_t)conv_scaled;
            const uint64_t sum1cf0_scaled = (uint64_t)sum1cf0.lo * 0x57ad390au;
            const uint32_t sum1cf0_scaled_lo = (uint32_t)sum1cf0_scaled;
            const uint64_t sum1d70_scaled = (uint64_t)sum1d70.lo * 0x665d954fu;
            const uint32_t sum1d70_scaled_lo = (uint32_t)sum1d70_scaled;

            const uint32_t t0 = count_lo + 0xa084b402u;
            const uint32_t t1 = t0 + conv_scaled_lo;
            const uint32_t t2 = t1 + sum1cf0_scaled_lo;
            u = t2 + sum1d70_scaled_lo;

            v = count * 0x29060469u + count_hi + 0x49fb0908u +
                u32_add_carry(count_lo, 0xa084b402u) +
                conv.hi * 0x36374889u + conv.lo * 0x502342a9u +
                (uint32_t)(conv_scaled >> 32) + u32_add_carry(t0, conv_scaled_lo) +
                sum1cf0.hi * 0x57ad390au + sum1cf0.lo * 0x9d156fbdu +
                (uint32_t)(sum1cf0_scaled >> 32) + u32_add_carry(t1, sum1cf0_scaled_lo) +
                sum1d70.hi * 0x665d954fu + sum1d70.lo * 0xd3abcd83u +
                (uint32_t)(sum1d70_scaled >> 32) + u32_add_carry(t2, sum1d70_scaled_lo);
        }

        const uint32_t out_lo_mul = u * 0x631f991fu;
        out_pairs26[idx].lo = out_lo_mul + 0x24ed41f6u;
        out_pairs26[idx].hi = v * 0x631f991fu + u * 0x058fca69u +
                              u32_mulhi(u, 0x631f991fu) + 0x132995bbu +
                              u32_add_carry(out_lo_mul, 0x24ed41f6u);
        dst->convolution26[idx] = conv;
        dst->range1cf0_26[idx] = sum1cf0;
        dst->range1d70_26[idx] = sum1d70;
        dst->pre_reduce26[idx].lo = u;
        dst->pre_reduce26[idx].hi = v;
        dst->output26[idx] = out_pairs26[idx];
    }
    return 0;
}

int l3_authorization_secondary_local390_accum_convolution26(
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_local390_convolution26_trace *out) {
    if (!local390_words13 || !out_pairs26) return -1;

    l3_authorization_secondary_pair32 raw1cf0[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 prefix1cf0[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 raw1d70[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 prefix1d70[L3_AUTH_ROUND_SECONDARY_PAIRS13];

    int rc = l3_authorization_secondary_local390_accum1cf0_1d70_pairs13(
        local390_words13, raw1cf0, prefix1cf0, raw1d70, prefix1d70, NULL);
    if (rc != 0) return rc;
    return l3_authorization_secondary_local390_convolution26(
        raw1cf0, prefix1cf0, raw1d70, prefix1d70, out_pairs26, out);
}

/* ---- v278: fourth f3fdc45c binding plus affine local_3f8 export ---- */

int l3_authorization_secondary_post_fourth_secondary_vector_reduce_mix3f8_words13(
    const uint32_t secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_post_fourth_secondary_vector_reduce_mix3f8_trace *out) {
    if (!secondary_vector_reduce_words13 || !out_words13) return -1;

    static const uint32_t k_group0_or_2[4] = {
        0x1c4b6ae9u, 0xa3a66125u, 0x1aa27e71u, 0xa790cd77u
    };
    static const uint32_t k_group1[4] = {
        0xf0d9df9du, 0xc8978dbbu, 0x0636dcbdu, 0x70fd701fu
    };
    static const uint32_t k_add_group0_or_2[4] = {
        0x41495607u, 0x936419e7u, 0xf2133864u, 0xf2ba671du
    };
    static const uint32_t k_add_group1[4] = {
        0x65e3e0afu, 0x64151c9du, 0xcb6f41a4u, 0x04e3e99fu
    };

    l3_authorization_secondary_post_fourth_secondary_vector_reduce_mix3f8_trace tmp;
    l3_authorization_secondary_post_fourth_secondary_vector_reduce_mix3f8_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));
    memcpy(dst->input_words13, secondary_vector_reduce_words13, sizeof(dst->input_words13));

    for (unsigned i = 0; i < 4u; ++i) {
        out_words13[i] =
            secondary_vector_reduce_words13[i] * k_group0_or_2[i] +
            k_add_group0_or_2[i];
        out_words13[4u + i] =
            secondary_vector_reduce_words13[4u + i] * k_group1[i] +
            k_add_group1[i];
        out_words13[8u + i] =
            secondary_vector_reduce_words13[8u + i] * k_group0_or_2[i] +
            k_add_group0_or_2[i];
        dst->affine_group0_4[i] = out_words13[i];
        dst->affine_group1_4[i] = out_words13[4u + i];
        dst->affine_group2_4[i] = out_words13[8u + i];
    }
    out_words13[12] = secondary_vector_reduce_words13[12] * 0xf0d9df9du + 0x65e3e0afu;
    dst->scalar_tail_word12 = out_words13[12];
    memcpy(dst->output_words13, out_words13, sizeof(dst->output_words13));
    return 0;
}

int l3_authorization_secondary_derive_state_stage3(
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t caller_seed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t caller_scalar0,
    uint32_t caller_scalar1,
    uint32_t out_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_bound_fourth_secondary_vector_reduce_mix3f8_trace *out) {
    if (!local390_words13 || !caller_seed_words13 || !out_words13) return -1;

    l3_authorization_secondary_bound_fourth_secondary_vector_reduce_mix3f8_trace tmp;
    l3_authorization_secondary_bound_fourth_secondary_vector_reduce_mix3f8_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    l3_authorization_secondary_pair32 conv[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    int rc = l3_authorization_secondary_local390_accum_convolution26(
        local390_words13, conv, &dst->convolution);
    if (rc != 0) return rc;

    uint32_t state_pairs[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    for (unsigned i = 0; i < L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26; ++i) {
        state_pairs[i][0] = conv[i].lo;
        state_pairs[i][1] = conv[i].hi;
    }
    memcpy(dst->state_before_f3fdc45c26, state_pairs, sizeof(state_pairs));

    uint32_t secondary_vector_reduce_words[L3_AUTH_ROUND_SECONDARY_WORDS13];
    rc = l3_authorization_secondary_vector_reduce_run13_update_export(
        caller_seed_words13, caller_scalar0, caller_scalar1,
        state_pairs, secondary_vector_reduce_words, &dst->f3fdc45c);
    if (rc != 0) return rc;

    memcpy(dst->state_after_f3fdc45c26, state_pairs, sizeof(state_pairs));
    memcpy(dst->secondary_vector_reduce_words13, secondary_vector_reduce_words, sizeof(secondary_vector_reduce_words));

    return l3_authorization_secondary_post_fourth_secondary_vector_reduce_mix3f8_words13(
        secondary_vector_reduce_words, out_words13, &dst->mix3f8);
}

/* ---- v279: FUN_f3fcc2d0 local_3f8/local_390 generated-pair accumulators ---- */

int l3_authorization_secondary_local3f8_accum1df0_pairs13(
    const uint32_t local3f8_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_local3f8_accum1df0_trace *out) {
    if (!local3f8_words13 || !out_pairs13 || !out_cumulative13) return -1;

    l3_authorization_secondary_local3f8_accum1df0_trace tmp;
    l3_authorization_secondary_local3f8_accum1df0_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    uint32_t x = u32_mla(local3f8_words13[0], 0x9b89907bu, 0x9a232f4fu);
    dst->transformed_words13[0] = x;
    out_pairs13[0] = make_1df0_pair(x, &dst->final_fold_lows13[0]);
    out_cumulative13[0] = out_pairs13[0];
    dst->generated_pairs13[0] = out_pairs13[0];
    dst->cumulative_pairs13[0] = out_cumulative13[0];

    for (unsigned i = 1; i < L3_AUTH_ROUND_SECONDARY_WORDS13; ++i) {
        const unsigned j = i & 7u;
        uint32_t pre = u32_mla(local3f8_words13[i], k_f4190f30[j], k_f4190f50[j]);
        x = u32_mla(pre, 0x4e68febbu, 0x0dc73610u);
        dst->transformed_words13[i] = x;
        out_pairs13[i] = make_1df0_pair(x, &dst->final_fold_lows13[i]);
        out_cumulative13[i].lo = out_cumulative13[i - 1u].lo + out_pairs13[i].lo;
        out_cumulative13[i].hi = out_cumulative13[i - 1u].hi + out_pairs13[i].hi +
                                 u32_add_carry(out_cumulative13[i - 1u].lo, out_pairs13[i].lo);
        dst->generated_pairs13[i] = out_pairs13[i];
        dst->cumulative_pairs13[i] = out_cumulative13[i];
    }
    return 0;
}

int l3_authorization_secondary_local390_accum1e70_pairs13(
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t caller_scalar1,
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_local390_accum1e70_trace *out) {
    if (!local390_words13 || !out_pairs13 || !out_cumulative13) return -1;

    l3_authorization_secondary_local390_accum1e70_trace tmp;
    l3_authorization_secondary_local390_accum1e70_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    uint32_t x = u32_mla(caller_scalar1, 0x5d7a9b6fu, 0xde0d550fu);
    dst->transformed_words13[0] = x;
    out_pairs13[0] = make_1e70_pair(x, &dst->final_fold_lows13[0]);
    out_cumulative13[0] = out_pairs13[0];
    dst->generated_pairs13[0] = out_pairs13[0];
    dst->cumulative_pairs13[0] = out_cumulative13[0];

    for (unsigned i = 1; i < L3_AUTH_ROUND_SECONDARY_WORDS13; ++i) {
        const unsigned j = i & 7u;
        uint32_t pre = u32_mla(local390_words13[i], k_f4190f70[j], k_f4190f90[j]);
        x = u32_mla(pre, 0x0986d021u, 0x654d99f3u);
        dst->transformed_words13[i] = x;
        out_pairs13[i] = make_1e70_pair(x, &dst->final_fold_lows13[i]);
        out_cumulative13[i].lo = out_cumulative13[i - 1u].lo + out_pairs13[i].lo;
        out_cumulative13[i].hi = out_cumulative13[i - 1u].hi + out_pairs13[i].hi +
                                 u32_add_carry(out_cumulative13[i - 1u].lo, out_pairs13[i].lo);
        dst->generated_pairs13[i] = out_pairs13[i];
        dst->cumulative_pairs13[i] = out_cumulative13[i];
    }
    return 0;
}

int l3_authorization_secondary_local3f8_local390_accum1df0_1e70_pairs13(
    const uint32_t local3f8_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t caller_scalar1,
    l3_authorization_secondary_pair32 raw1df0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix1df0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 raw1e70_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix1e70_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_local3f8_local390_accum1df0_1e70_trace *out) {
    if (!local3f8_words13 || !local390_words13 || !raw1df0_13 || !prefix1df0_13 ||
        !raw1e70_13 || !prefix1e70_13) return -1;

    l3_authorization_secondary_local3f8_local390_accum1df0_1e70_trace tmp;
    l3_authorization_secondary_local3f8_local390_accum1df0_1e70_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    int rc = l3_authorization_secondary_local3f8_accum1df0_pairs13(
        local3f8_words13, raw1df0_13, prefix1df0_13, &dst->accum1df0);
    if (rc != 0) return rc;
    return l3_authorization_secondary_local390_accum1e70_pairs13(
        local390_words13, caller_scalar1, raw1e70_13, prefix1e70_13, &dst->accum1e70);
}


/* ---- v280: FUN_f3fcc2d0 local_3f8/local_390 convolution/reduction ---- */

int l3_authorization_secondary_local3f8_local390_convolution26(
    const l3_authorization_secondary_pair32 raw1df0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix1df0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 raw1e70_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix1e70_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_local3f8_local390_convolution26_trace *out) {
    if (!raw1df0_13 || !prefix1df0_13 || !raw1e70_13 || !prefix1e70_13 || !out_pairs26) return -1;

    l3_authorization_secondary_local3f8_local390_convolution26_trace tmp;
    l3_authorization_secondary_local3f8_local390_convolution26_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    for (unsigned idx = 0; idx < L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26; ++idx) {
        unsigned first = idx - 12u;
        unsigned last;
        if (idx < 12u || first == 0u) {
            first = 0u;
            last = idx;
        } else {
            last = 12u;
        }

        uint32_t u = 0x466a8fabu;
        uint32_t v = 0x5006f266u;
        l3_authorization_secondary_pair32 conv = {0u, 0u};
        l3_authorization_secondary_pair32 sum1df0 = {0u, 0u};
        l3_authorization_secondary_pair32 sum1e70 = {0u, 0u};

        dst->first_last_count26[idx][0] = first;
        dst->first_last_count26[idx][1] = last;
        dst->first_last_count26[idx][2] = (first <= last) ? ((last - first) + 1u) : 0u;

        if (first <= last) {
            for (unsigned k = first; k <= last; ++k) {
                const l3_authorization_secondary_pair32 *a1df0 = &raw1df0_13[k];
                const l3_authorization_secondary_pair32 *b1e70 = &raw1e70_13[idx - k];
                const uint64_t prod = (uint64_t)a1df0->lo * (uint64_t)b1e70->lo;
                const uint32_t old = conv.lo;
                conv.lo += (uint32_t)prod;
                conv.hi += b1e70->hi * a1df0->lo + b1e70->lo * a1df0->hi +
                           (uint32_t)(prod >> 32) + (uint32_t)(conv.lo < old);
            }

            secondary_pair_range_sum(prefix1df0_13, first, last, &sum1df0);
            secondary_pair_range_sum(prefix1e70_13, idx - last, idx - first, &sum1e70);

            const uint32_t count = (last - first) + 1u;
            const uint64_t count_prod = (uint64_t)count * 0xe2d29c74u;
            const uint32_t count_lo = (uint32_t)count_prod;
            const uint32_t count_hi = (uint32_t)(count_prod >> 32);
            const uint64_t conv_scaled = (uint64_t)conv.lo * 0x997dd335u;
            const uint32_t conv_scaled_lo = (uint32_t)conv_scaled;
            const uint64_t sum1df0_scaled = (uint64_t)sum1df0.lo * 0x493f06d4u;
            const uint32_t sum1df0_scaled_lo = (uint32_t)sum1df0_scaled;
            const uint64_t sum1e70_scaled = (uint64_t)sum1e70.lo * 0x86d8649du;
            const uint32_t sum1e70_scaled_lo = (uint32_t)sum1e70_scaled;

            const uint32_t t0 = count_lo + 0x466a8fabu;
            const uint32_t t1 = t0 + conv_scaled_lo;
            const uint32_t t2 = t1 + sum1df0_scaled_lo;
            u = t2 + sum1e70_scaled_lo;

            v = count * 0xec7de385u + count_hi + 0x5006f266u +
                u32_add_carry(count_lo, 0x466a8fabu) +
                conv.hi * 0x997dd335u + conv.lo * 0xd2bee43eu +
                (uint32_t)(conv_scaled >> 32) + u32_add_carry(t0, conv_scaled_lo) +
                sum1df0.hi * 0x493f06d4u + sum1df0.lo * 0x8498b691u +
                (uint32_t)(sum1df0_scaled >> 32) + u32_add_carry(t1, sum1df0_scaled_lo) +
                sum1e70.hi * 0x86d8649du + sum1e70.lo * 0x381078a5u +
                (uint32_t)(sum1e70_scaled >> 32) + u32_add_carry(t2, sum1e70_scaled_lo);
        }

        const uint32_t out_lo_mul = u * 0xd4b971c1u;
        out_pairs26[idx].lo = out_lo_mul + 0xe87e7549u;
        out_pairs26[idx].hi = v * 0xd4b971c1u + u * 0x3f397035u +
                              u32_mulhi(u, 0xd4b971c1u) + 0x19376ba3u +
                              u32_add_carry(out_lo_mul, 0xe87e7549u);
        dst->convolution26[idx] = conv;
        dst->range1df0_26[idx] = sum1df0;
        dst->range1e70_26[idx] = sum1e70;
        dst->pre_reduce26[idx].lo = u;
        dst->pre_reduce26[idx].hi = v;
        dst->output26[idx] = out_pairs26[idx];
    }
    return 0;
}

int l3_authorization_secondary_local3f8_local390_accum_convolution26(
    const uint32_t local3f8_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t caller_scalar1,
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_local3f8_local390_convolution26_trace *out) {
    if (!local3f8_words13 || !local390_words13 || !out_pairs26) return -1;

    l3_authorization_secondary_pair32 raw1df0[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 prefix1df0[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 raw1e70[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 prefix1e70[L3_AUTH_ROUND_SECONDARY_PAIRS13];

    int rc = l3_authorization_secondary_local3f8_local390_accum1df0_1e70_pairs13(
        local3f8_words13, local390_words13, caller_scalar1,
        raw1df0, prefix1df0, raw1e70, prefix1e70, NULL);
    if (rc != 0) return rc;
    return l3_authorization_secondary_local3f8_local390_convolution26(
        raw1df0, prefix1df0, raw1e70, prefix1e70, out_pairs26, out);
}


/* ---- v281: fifth f3fdc45c binding plus affine local_42c export ---- */

int l3_authorization_secondary_post_fifth_secondary_vector_reduce_mix42c_words13(
    const uint32_t secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_post_fifth_secondary_vector_reduce_mix42c_trace *out) {
    if (!secondary_vector_reduce_words13 || !out_words13) return -1;

    static const uint32_t k_group0[4] = {
        0xc6c50b8du, 0xce503ab5u, 0x47891297u, 0xd8be22fbu
    };
    static const uint32_t k_group7[4] = {
        0xa1f945b7u, 0xc6c50b8du, 0xce503ab5u, 0x47891297u
    };
    static const uint32_t k_bias_group0[4] = {
        0x860e0c89u, 0xb8f0ef6au, 0x94e6db35u, 0x233ae589u
    };
    static const uint32_t k_bias_group7[4] = {
        0xf6e9b705u, 0x860e0c89u, 0xb8f0ef6au, 0x94e6db35u
    };

    l3_authorization_secondary_post_fifth_secondary_vector_reduce_mix42c_trace tmp;
    l3_authorization_secondary_post_fifth_secondary_vector_reduce_mix42c_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));
    memcpy(dst->input_words13, secondary_vector_reduce_words13, sizeof(dst->input_words13));

    for (unsigned i = 0; i < 4u; ++i) {
        out_words13[i] =
            secondary_vector_reduce_words13[i] * k_group0[i] + k_bias_group0[i];
        out_words13[7u + i] =
            secondary_vector_reduce_words13[7u + i] * k_group7[i] + k_bias_group7[i];
        dst->affine_group0_4[i] = out_words13[i];
        dst->affine_group7_10[i] = out_words13[7u + i];
    }
    out_words13[4] = secondary_vector_reduce_words13[4] * 0x1edf1d4du + 0x05751cd0u;
    out_words13[5] = secondary_vector_reduce_words13[5] * 0xb4544bf5u + 0x8af580a3u;
    out_words13[6] = secondary_vector_reduce_words13[6] * 0x9a2462edu + 0x9f47ab9bu;
    out_words13[11] = secondary_vector_reduce_words13[11] * 0xd8be22fbu + 0x233ae589u;
    out_words13[12] = secondary_vector_reduce_words13[12] * 0x1edf1d4du + 0x05751cd0u;

    dst->affine_middle3[0] = out_words13[4];
    dst->affine_middle3[1] = out_words13[5];
    dst->affine_middle3[2] = out_words13[6];
    dst->affine_tail11_12[0] = out_words13[11];
    dst->affine_tail11_12[1] = out_words13[12];
    memcpy(dst->output_words13, out_words13, sizeof(dst->output_words13));
    return 0;
}

int l3_authorization_secondary_derive_state_stage4(
    const uint32_t local3f8_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t local390_word0_for_accum,
    const int32_t caller_seed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t caller_scalar0,
    uint32_t caller_scalar1,
    uint32_t out_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_bound_fifth_secondary_vector_reduce_mix42c_trace *out) {
    if (!local3f8_words13 || !local390_words13 || !caller_seed_words13 || !out_words13) return -1;

    l3_authorization_secondary_bound_fifth_secondary_vector_reduce_mix42c_trace tmp;
    l3_authorization_secondary_bound_fifth_secondary_vector_reduce_mix42c_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    l3_authorization_secondary_pair32 conv[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    int rc = l3_authorization_secondary_local3f8_local390_accum_convolution26(
        local3f8_words13, local390_words13, local390_word0_for_accum,
        conv, &dst->convolution);
    if (rc != 0) return rc;

    uint32_t state_pairs[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    for (unsigned i = 0; i < L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26; ++i) {
        state_pairs[i][0] = conv[i].lo;
        state_pairs[i][1] = conv[i].hi;
    }
    memcpy(dst->state_before_f3fdc45c26, state_pairs, sizeof(state_pairs));

    uint32_t secondary_vector_reduce_words[L3_AUTH_ROUND_SECONDARY_WORDS13];
    rc = l3_authorization_secondary_vector_reduce_run13_update_export(
        caller_seed_words13, caller_scalar0, caller_scalar1,
        state_pairs, secondary_vector_reduce_words, &dst->f3fdc45c);
    if (rc != 0) return rc;

    memcpy(dst->state_after_f3fdc45c26, state_pairs, sizeof(state_pairs));
    memcpy(dst->secondary_vector_reduce_words13, secondary_vector_reduce_words, sizeof(secondary_vector_reduce_words));

    return l3_authorization_secondary_post_fifth_secondary_vector_reduce_mix42c_words13(
        secondary_vector_reduce_words, out_words13, &dst->mix42c);
}

/* ---- v282: FUN_f3fcc2d0 param_1/local_3f8 generated-pair accumulators ---- */

int l3_authorization_secondary_param1_accum1ef0_pairs13(
    const int32_t param1_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_param1_accum1ef0_trace *out) {
    if (!param1_words13 || !out_pairs13 || !out_cumulative13) return -1;

    l3_authorization_secondary_param1_accum1ef0_trace tmp;
    l3_authorization_secondary_param1_accum1ef0_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    uint32_t x = u32_mla((uint32_t)param1_words13[0], 0xd0cca515u, 0x482d494bu);
    dst->transformed_words13[0] = x;
    out_pairs13[0] = make_1ef0_pair(x, &dst->final_fold_lows13[0]);
    out_cumulative13[0] = out_pairs13[0];
    dst->generated_pairs13[0] = out_pairs13[0];
    dst->cumulative_pairs13[0] = out_cumulative13[0];

    for (unsigned i = 1; i < L3_AUTH_ROUND_SECONDARY_WORDS13; ++i) {
        const unsigned j = i & 7u;
        uint32_t pre = u32_mla((uint32_t)param1_words13[i], k_f4190fb0[j], k_f4190fd0[j]);
        x = u32_mla(pre, 0x18f05a49u, 0xce5c97c0u);
        dst->transformed_words13[i] = x;
        out_pairs13[i] = make_1ef0_pair(x, &dst->final_fold_lows13[i]);
        out_cumulative13[i].lo = out_cumulative13[i - 1u].lo + out_pairs13[i].lo;
        out_cumulative13[i].hi = out_cumulative13[i - 1u].hi + out_pairs13[i].hi +
                                 u32_add_carry(out_cumulative13[i - 1u].lo, out_pairs13[i].lo);
        dst->generated_pairs13[i] = out_pairs13[i];
        dst->cumulative_pairs13[i] = out_cumulative13[i];
    }
    return 0;
}

int l3_authorization_secondary_local3f8_accum1f70_pairs13(
    const uint32_t local3f8_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_local3f8_accum1f70_trace *out) {
    if (!local3f8_words13 || !out_pairs13 || !out_cumulative13) return -1;

    l3_authorization_secondary_local3f8_accum1f70_trace tmp;
    l3_authorization_secondary_local3f8_accum1f70_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    uint32_t x = u32_mla(local3f8_words13[0], 0xa3394861u, 0x50e665f1u);
    dst->transformed_words13[0] = x;
    out_pairs13[0] = make_1f70_pair(x, &dst->final_fold_lows13[0]);
    out_cumulative13[0] = out_pairs13[0];
    dst->generated_pairs13[0] = out_pairs13[0];
    dst->cumulative_pairs13[0] = out_cumulative13[0];

    for (unsigned i = 1; i < L3_AUTH_ROUND_SECONDARY_WORDS13; ++i) {
        const unsigned j = i & 7u;
        uint32_t pre = u32_mla(local3f8_words13[i], k_f4190ff0[j], k_f4191010[j]);
        x = u32_mla(pre, 0x56c4df01u, 0xa03cad7fu);
        dst->transformed_words13[i] = x;
        out_pairs13[i] = make_1f70_pair(x, &dst->final_fold_lows13[i]);
        out_cumulative13[i].lo = out_cumulative13[i - 1u].lo + out_pairs13[i].lo;
        out_cumulative13[i].hi = out_cumulative13[i - 1u].hi + out_pairs13[i].hi +
                                 u32_add_carry(out_cumulative13[i - 1u].lo, out_pairs13[i].lo);
        dst->generated_pairs13[i] = out_pairs13[i];
        dst->cumulative_pairs13[i] = out_cumulative13[i];
    }
    return 0;
}

int l3_authorization_secondary_param1_local3f8_accum1ef0_1f70_pairs13(
    const int32_t param1_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local3f8_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 raw1ef0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix1ef0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 raw1f70_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix1f70_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_param1_local3f8_accum1ef0_1f70_trace *out) {
    if (!param1_words13 || !local3f8_words13 || !raw1ef0_13 || !prefix1ef0_13 ||
        !raw1f70_13 || !prefix1f70_13) return -1;

    l3_authorization_secondary_param1_local3f8_accum1ef0_1f70_trace tmp;
    l3_authorization_secondary_param1_local3f8_accum1ef0_1f70_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    int rc = l3_authorization_secondary_param1_accum1ef0_pairs13(
        param1_words13, raw1ef0_13, prefix1ef0_13, &dst->accum1ef0);
    if (rc != 0) return rc;
    return l3_authorization_secondary_local3f8_accum1f70_pairs13(
        local3f8_words13, raw1f70_13, prefix1f70_13, &dst->accum1f70);
}

/* ---- v283: FUN_f3fcc2d0 param_1/local_3f8 convolution/reduction ---- */

int l3_authorization_secondary_param1_local3f8_convolution26(
    const l3_authorization_secondary_pair32 raw1ef0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix1ef0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 raw1f70_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix1f70_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_param1_local3f8_convolution26_trace *out) {
    if (!raw1ef0_13 || !prefix1ef0_13 || !raw1f70_13 || !prefix1f70_13 || !out_pairs26) return -1;

    l3_authorization_secondary_param1_local3f8_convolution26_trace tmp;
    l3_authorization_secondary_param1_local3f8_convolution26_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    for (unsigned idx = 0; idx < L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26; ++idx) {
        unsigned first = idx - 12u;
        unsigned last;
        if (idx < 12u || first == 0u) {
            first = 0u;
            last = idx;
        } else {
            last = 12u;
        }

        uint32_t u = 0x65464175u;
        uint32_t v = 0xa623eab0u;
        l3_authorization_secondary_pair32 conv = {0u, 0u};
        l3_authorization_secondary_pair32 sum1ef0 = {0u, 0u};
        l3_authorization_secondary_pair32 sum1f70 = {0u, 0u};

        dst->first_last_count26[idx][0] = first;
        dst->first_last_count26[idx][1] = last;
        dst->first_last_count26[idx][2] = (first <= last) ? ((last - first) + 1u) : 0u;

        if (first <= last) {
            for (unsigned k = first; k <= last; ++k) {
                const l3_authorization_secondary_pair32 *a1ef0 = &raw1ef0_13[k];
                const l3_authorization_secondary_pair32 *b1f70 = &raw1f70_13[idx - k];
                const uint64_t prod = (uint64_t)a1ef0->lo * (uint64_t)b1f70->lo;
                const uint32_t old = conv.lo;
                conv.lo += (uint32_t)prod;
                conv.hi += b1f70->hi * a1ef0->lo + b1f70->lo * a1ef0->hi +
                           (uint32_t)(prod >> 32) + (uint32_t)(conv.lo < old);
            }

            secondary_pair_range_sum(prefix1ef0_13, first, last, &sum1ef0);
            secondary_pair_range_sum(prefix1f70_13, idx - last, idx - first, &sum1f70);

            const uint32_t count = (last - first) + 1u;
            const uint64_t count_prod = (uint64_t)count * 0x46a803bfu;
            const uint32_t count_lo = (uint32_t)count_prod;
            const uint32_t count_hi = (uint32_t)(count_prod >> 32);
            const uint64_t conv_scaled = (uint64_t)conv.lo * 0x54120afbu;
            const uint32_t conv_scaled_lo = (uint32_t)conv_scaled;
            const uint64_t sum1ef0_scaled = (uint64_t)sum1ef0.lo * 0x02522049u;
            const uint32_t sum1ef0_scaled_lo = (uint32_t)sum1ef0_scaled;
            const uint64_t sum1f70_scaled = (uint64_t)sum1f70.lo * 0x1f09aa1du;
            const uint32_t sum1f70_scaled_lo = (uint32_t)sum1f70_scaled;

            const uint32_t t0 = count_lo + 0x65464175u;
            const uint32_t t1 = t0 + conv_scaled_lo;
            const uint32_t t2 = t1 + sum1ef0_scaled_lo;
            u = t2 + sum1f70_scaled_lo;

            v = count * 0x7679776du + count_hi + 0xa623eab0u +
                u32_add_carry(count_lo, 0x65464175u) +
                conv.hi * 0x54120afbu + conv.lo * 0x4f982030u +
                (uint32_t)(conv_scaled >> 32) + u32_add_carry(t0, conv_scaled_lo) +
                sum1ef0.hi * 0x02522049u + sum1ef0.lo * 0x159e6bdau +
                (uint32_t)(sum1ef0_scaled >> 32) + u32_add_carry(t1, sum1ef0_scaled_lo) +
                sum1f70.hi * 0x1f09aa1du + sum1f70.lo * 0x23d299f1u +
                (uint32_t)(sum1f70_scaled >> 32) + u32_add_carry(t2, sum1f70_scaled_lo);
        }

        const uint32_t out_lo_mul = u * 0x12f3ac9bu;
        out_pairs26[idx].lo = out_lo_mul + 0x953f025du;
        out_pairs26[idx].hi = v * 0x12f3ac9bu + u * 0x30b2d602u +
                              u32_mulhi(u, 0x12f3ac9bu) + 0xc40f0029u +
                              u32_add_carry(out_lo_mul, 0x953f025du);
        dst->convolution26[idx] = conv;
        dst->range1ef0_26[idx] = sum1ef0;
        dst->range1f70_26[idx] = sum1f70;
        dst->pre_reduce26[idx].lo = u;
        dst->pre_reduce26[idx].hi = v;
        dst->output26[idx] = out_pairs26[idx];
    }
    return 0;
}

int l3_authorization_secondary_param1_local3f8_accum_convolution26(
    const int32_t param1_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local3f8_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_param1_local3f8_convolution26_trace *out) {
    if (!param1_words13 || !local3f8_words13 || !out_pairs26) return -1;

    l3_authorization_secondary_pair32 raw1ef0[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 prefix1ef0[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 raw1f70[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 prefix1f70[L3_AUTH_ROUND_SECONDARY_PAIRS13];

    int rc = l3_authorization_secondary_param1_local3f8_accum1ef0_1f70_pairs13(
        param1_words13, local3f8_words13,
        raw1ef0, prefix1ef0, raw1f70, prefix1f70, NULL);
    if (rc != 0) return rc;
    return l3_authorization_secondary_param1_local3f8_convolution26(
        raw1ef0, prefix1ef0, raw1f70, prefix1f70, out_pairs26, out);
}


/* ---- v284: sixth f3fdc45c call binding / export boundary ---- */

int l3_authorization_secondary_derive_feedback_state(
    const int32_t param1_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local3f8_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t caller_seed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t caller_scalar0,
    uint32_t caller_scalar1,
    uint32_t out_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_bound_sixth_secondary_vector_reduce_export_trace *out) {
    if (!param1_words13 || !local3f8_words13 || !caller_seed_words13 || !out_words13) return -1;

    l3_authorization_secondary_bound_sixth_secondary_vector_reduce_export_trace tmp;
    l3_authorization_secondary_bound_sixth_secondary_vector_reduce_export_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    l3_authorization_secondary_pair32 conv[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    int rc = l3_authorization_secondary_param1_local3f8_accum_convolution26(
        param1_words13, local3f8_words13, conv, &dst->convolution);
    if (rc != 0) return rc;

    uint32_t state_pairs[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    for (unsigned i = 0; i < L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26; ++i) {
        state_pairs[i][0] = conv[i].lo;
        state_pairs[i][1] = conv[i].hi;
    }
    memcpy(dst->state_before_f3fdc45c26, state_pairs, sizeof(state_pairs));

    rc = l3_authorization_secondary_vector_reduce_run13_update_export(
        caller_seed_words13, caller_scalar0, caller_scalar1,
        state_pairs, out_words13, &dst->f3fdc45c);
    if (rc != 0) return rc;

    memcpy(dst->state_after_f3fdc45c26, state_pairs, sizeof(state_pairs));
    memcpy(dst->secondary_vector_reduce_words13, out_words13, sizeof(dst->secondary_vector_reduce_words13));
    return 0;
}

/* ---- v285: post-sixth local_3c4 accumulator streams plus side-vector sources ---- */

int l3_authorization_secondary_local3c4_accum2070_pairs13(
    const uint32_t local3c4_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_local3c4_accum2070_trace *out) {
    if (!local3c4_words13 || !out_pairs13 || !out_cumulative13) return -1;

    l3_authorization_secondary_local3c4_accum2070_trace tmp;
    l3_authorization_secondary_local3c4_accum2070_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    uint32_t x = u32_mla(local3c4_words13[0], 0xa43fbd01u, 0x1b0c0277u);
    dst->transformed_words13[0] = x;
    out_pairs13[0] = make_2070_pair(x, &dst->final_fold_lows13[0]);
    out_cumulative13[0] = out_pairs13[0];
    dst->generated_pairs13[0] = out_pairs13[0];
    dst->cumulative_pairs13[0] = out_cumulative13[0];

    for (unsigned i = 1; i < L3_AUTH_ROUND_SECONDARY_WORDS13; ++i) {
        const unsigned j = i & 7u;
        uint32_t pre = u32_mla(local3c4_words13[i], k_f4191070[j], k_f4191090[j]);
        x = u32_mla(pre, 0x899a4967u, 0xb634e55cu);
        dst->transformed_words13[i] = x;
        out_pairs13[i] = make_2070_pair(x, &dst->final_fold_lows13[i]);
        out_cumulative13[i].lo = out_cumulative13[i - 1u].lo + out_pairs13[i].lo;
        out_cumulative13[i].hi = out_cumulative13[i - 1u].hi + out_pairs13[i].hi +
                                 u32_add_carry(out_cumulative13[i - 1u].lo, out_pairs13[i].lo);
        dst->generated_pairs13[i] = out_pairs13[i];
        dst->cumulative_pairs13[i] = out_cumulative13[i];
    }
    return 0;
}

int l3_authorization_secondary_local3c4_accum1ff0_pairs13(
    const uint32_t local3c4_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_local3c4_accum1ff0_trace *out) {
    if (!local3c4_words13 || !out_pairs13 || !out_cumulative13) return -1;

    l3_authorization_secondary_local3c4_accum1ff0_trace tmp;
    l3_authorization_secondary_local3c4_accum1ff0_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    uint32_t x = u32_mla(local3c4_words13[0], 0x6660896bu, 0xb41083b6u);
    dst->transformed_words13[0] = x;
    out_pairs13[0] = make_1ff0_pair(x, &dst->final_fold_lows13[0]);
    out_cumulative13[0] = out_pairs13[0];
    dst->generated_pairs13[0] = out_pairs13[0];
    dst->cumulative_pairs13[0] = out_cumulative13[0];

    for (unsigned i = 1; i < L3_AUTH_ROUND_SECONDARY_WORDS13; ++i) {
        const unsigned j = i & 7u;
        uint32_t pre = u32_mla(local3c4_words13[i], k_f4191030[j], k_f4191050[j]);
        x = u32_mla(pre, 0x97d15a47u, 0x854296edu);
        dst->transformed_words13[i] = x;
        out_pairs13[i] = make_1ff0_pair(x, &dst->final_fold_lows13[i]);
        out_cumulative13[i].lo = out_cumulative13[i - 1u].lo + out_pairs13[i].lo;
        out_cumulative13[i].hi = out_cumulative13[i - 1u].hi + out_pairs13[i].hi +
                                 u32_add_carry(out_cumulative13[i - 1u].lo, out_pairs13[i].lo);
        dst->generated_pairs13[i] = out_pairs13[i];
        dst->cumulative_pairs13[i] = out_cumulative13[i];
    }
    return 0;
}

int l3_authorization_secondary_sixth_export_local3c4_accum2070_1ff0_sources(
    const uint32_t sixth_secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local3c4_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 raw2070_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix2070_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 raw1ff0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix1ff0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_sixth_local3c4_accum_sources_trace *out) {
    if (!sixth_secondary_vector_reduce_words13 || !local3c4_words13 || !raw2070_13 || !prefix2070_13 ||
        !raw1ff0_13 || !prefix1ff0_13) return -1;

    l3_authorization_secondary_sixth_local3c4_accum_sources_trace tmp;
    l3_authorization_secondary_sixth_local3c4_accum_sources_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));
    memcpy(dst->sixth_secondary_vector_reduce_words13, sixth_secondary_vector_reduce_words13, sizeof(dst->sixth_secondary_vector_reduce_words13));
    memcpy(dst->local3c4_words13, local3c4_words13, sizeof(dst->local3c4_words13));

    int rc = l3_authorization_secondary_local3c4_accum2070_pairs13(
        local3c4_words13, raw2070_13, prefix2070_13, &dst->accum2070);
    if (rc != 0) return rc;
    rc = l3_authorization_secondary_local3c4_accum1ff0_pairs13(
        local3c4_words13, raw1ff0_13, prefix1ff0_13, &dst->accum1ff0);
    if (rc != 0) return rc;

    static const uint32_t k_mul72[4] = {0xf56a8679u, 0x1d52c56bu, 0x5d616a0du, 0xb1e50707u};
    static const uint32_t k_add94[4] = {0xba8ebf25u, 0x86e3d431u, 0x7e292bb8u, 0xf4e09419u};
    static const uint32_t k_mul87[4] = {0xbe661f1fu, 0x7ce0ef19u, 0x4fda35dbu, 0xed76e7f5u};
    static const uint32_t k_add88[4] = {0x67612004u, 0x5430d4f7u, 0xcb858845u, 0xd5f28e8du};

    for (unsigned i = 0; i < 4u; ++i) {
        dst->side_vec60_0_3[i] = sixth_secondary_vector_reduce_words13[i] * k_mul72[i] + k_add94[i];
        dst->side_vec99_4_7[i] = sixth_secondary_vector_reduce_words13[4u + i] * k_mul87[i] + k_add88[i];
        dst->side_vec81_8_11[i] = sixth_secondary_vector_reduce_words13[8u + i] * k_mul72[i] + k_add94[i];
    }
    dst->side_word12 = sixth_secondary_vector_reduce_words13[12] * 0xbe661f1fu + 0x67612004u;
    return 0;
}

/* ---- v286: FUN_f3fcc2d0 local_3c4 convolution/reduction ---- */

int l3_authorization_secondary_local3c4_convolution26(
    const l3_authorization_secondary_pair32 raw2070_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix2070_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 raw1ff0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix1ff0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_local3c4_convolution26_trace *out) {
    if (!raw2070_13 || !prefix2070_13 || !raw1ff0_13 || !prefix1ff0_13 || !out_pairs26) return -1;

    l3_authorization_secondary_local3c4_convolution26_trace tmp;
    l3_authorization_secondary_local3c4_convolution26_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    for (unsigned idx = 0; idx < L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26; ++idx) {
        unsigned first = idx - 12u;
        unsigned last;
        if (idx < 12u || first == 0u) {
            first = 0u;
            last = idx;
        } else {
            last = 12u;
        }

        uint32_t u = 0x235b23e9u;
        uint32_t v = 0xbae8341du;
        l3_authorization_secondary_pair32 conv = {0u, 0u};
        l3_authorization_secondary_pair32 sum2070 = {0u, 0u};
        l3_authorization_secondary_pair32 sum1ff0 = {0u, 0u};

        dst->first_last_count26[idx][0] = first;
        dst->first_last_count26[idx][1] = last;
        dst->first_last_count26[idx][2] = (first <= last) ? ((last - first) + 1u) : 0u;

        if (first <= last) {
            for (unsigned k = first; k <= last; ++k) {
                const l3_authorization_secondary_pair32 *a2070 = &raw2070_13[k];
                const l3_authorization_secondary_pair32 *b1ff0 = &raw1ff0_13[idx - k];
                const uint64_t prod = (uint64_t)a2070->lo * (uint64_t)b1ff0->lo;
                const uint32_t old = conv.lo;
                conv.lo += (uint32_t)prod;
                conv.hi += b1ff0->hi * a2070->lo + b1ff0->lo * a2070->hi +
                           (uint32_t)(prod >> 32) + (uint32_t)(conv.lo < old);
            }

            /* Native's first correction range follows the local_e0 stream,
             * which is the 1ff0 prefix, while the second follows 2070. */
            secondary_pair_range_sum(
                prefix1ff0_13, first, last, &sum2070);
            secondary_pair_range_sum(
                prefix2070_13, idx - last, idx - first, &sum1ff0);

            const uint32_t count = (last - first) + 1u;
            const uint64_t count_prod = (uint64_t)count * 0xfe0b5220u;
            const uint32_t count_lo = (uint32_t)count_prod;
            const uint32_t count_hi = (uint32_t)(count_prod >> 32);
            const uint64_t conv_scaled = (uint64_t)conv.lo * 0x1f81ce5bu;
            const uint32_t conv_scaled_lo = (uint32_t)conv_scaled;
            const uint64_t sum2070_scaled = (uint64_t)sum2070.lo * 0x66075109u;
            const uint32_t sum2070_scaled_lo = (uint32_t)sum2070_scaled;
            const uint64_t sum1ff0_scaled = (uint64_t)sum1ff0.lo * 0x4a2a9e60u;
            const uint32_t sum1ff0_scaled_lo = (uint32_t)sum1ff0_scaled;

            const uint32_t t0 = count_lo + 0x235b23e9u;
            const uint32_t t1 = t0 + conv_scaled_lo;
            const uint32_t t2 = t1 + sum2070_scaled_lo;
            u = t2 + sum1ff0_scaled_lo;

            v = count * 0x87c78ad2u + count_hi + 0xbae8341du +
                u32_add_carry(count_lo, 0x235b23e9u) +
                conv.hi * 0x1f81ce5bu + conv.lo * 0x13aaf5e6u +
                (uint32_t)(conv_scaled >> 32) + u32_add_carry(t0, conv_scaled_lo) +
                sum2070.hi * 0x66075109u + sum2070.lo * 0xadf241cfu +
                (uint32_t)(sum2070_scaled >> 32) + u32_add_carry(t1, sum2070_scaled_lo) +
                sum1ff0.hi * 0x4a2a9e60u + sum1ff0.lo * 0x77a8960du +
                (uint32_t)(sum1ff0_scaled >> 32) + u32_add_carry(t2, sum1ff0_scaled_lo);
        }

        const uint32_t out_lo_mul = u * 0xf8624883u;
        out_pairs26[idx].lo = out_lo_mul + 0x45e357f9u;
        out_pairs26[idx].hi = v * 0xf8624883u + u * 0x83d8c20au +
                              u32_mulhi(u, 0xf8624883u) + 0x87695694u +
                              u32_add_carry(out_lo_mul, 0x45e357f9u);
        dst->convolution26[idx] = conv;
        dst->range2070_26[idx] = sum2070;
        dst->range1ff0_26[idx] = sum1ff0;
        dst->pre_reduce26[idx].lo = u;
        dst->pre_reduce26[idx].hi = v;
        dst->output26[idx] = out_pairs26[idx];
    }
    return 0;
}



/* ---- v287: bind V286 local_328 into seventh f3fdc45c plus local_390 export ---- */




/* ---- v321: FUN_f3fcc2d0 param_7/local_5b0 and local_460 rolling export loops ---- */

static const uint32_t k_f41a20f0[16] = {
    0x8a324609u, 0x1d718bf7u, 0xaaeb003eu, 0x38647485u,
    0xcba3ba73u, 0x591d2ebau, 0xec5c74a8u, 0x79d5e8efu,
    0x0d152eddu, 0x9a8ea324u, 0x2808176bu, 0xbb475d59u,
    0x48c0d1a0u, 0xdc00178eu, 0x69798bd5u, 0xfcb8d1c3u
};

static const uint32_t k_f41910b0[8] = {
    0x0d9f60a7u, 0x6e0f2c2du, 0x850350ddu, 0x599c962bu,
    0x4829856du, 0x2f0d2773u, 0x593ae749u, 0x5dd85651u
};

static const uint32_t k_f41910d0[8] = {
    0xdc98e391u, 0xb5eea875u, 0x45421323u, 0x24a5aadbu,
    0xff55174eu, 0xa0c07c6eu, 0xfb700b19u, 0x0c277814u
};

static const uint32_t k_f41a2130[16] = {
    0x0f7aeb48u, 0x576024e1u, 0xaf455e7au, 0xf9b0efdau,
    0x41962973u, 0x9c01bad3u, 0xe3e6f46cu, 0x3e5285ccu,
    0x8637bf65u, 0xde1cf8feu, 0x28888a5eu, 0x706dc3f7u,
    0xcad95557u, 0x12be8ef0u, 0x6d2a2050u, 0xb50f59e9u
};

static const uint32_t k_f41910f0[8] = {
    0xc8b21869u, 0xa5182937u, 0x38bbd2e9u, 0x43886505u,
    0x2360ce6fu, 0x85f080c7u, 0xb98558bbu, 0x059eee81u
};

static const uint32_t k_f4191110[8] = {
    0x0034aca9u, 0xf3eed000u, 0x6a8725d0u, 0x7af585f2u,
    0x12084c99u, 0x18e9ad7du, 0x2b128d80u, 0x5733a803u
};

static uint32_t secondary_mix_fold20f0(uint32_t x, unsigned rounds) {
    for (unsigned i = 0; i < rounds; ++i) x = k_f41a20f0[x & 0x0fu] + (x >> 4);
    return x;
}

static uint32_t secondary_mix_fold2130(uint32_t x, unsigned rounds) {
    for (unsigned i = 0; i < rounds; ++i) x = k_f41a2130[x & 0x0fu] + (x >> 4);
    return x;
}

int l3_authorization_secondary_param7_export20f0_words13(
    const uint32_t pre_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_param7_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_param7_export20f0_trace *out) {
    if (!pre_words13 || !out_param7_words13) return -1;

    l3_authorization_secondary_param7_export20f0_trace tmp;
    l3_authorization_secondary_param7_export20f0_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));
    memcpy(dst->pre_words13, pre_words13, sizeof(dst->pre_words13));

    uint32_t rolling = 0x5bfe0a54u;
    for (unsigned lane = 0; lane < L3_AUTH_ROUND_SECONDARY_WORDS13; ++lane) {
        const unsigned j = lane & 7u;
        dst->rolling_before13[lane] = rolling;
        const uint32_t x = rolling * 0x9fefeb29u + pre_words13[lane];
        const uint32_t seed = x * 0x69f2a171u + 0xb3bec809u;
        const uint32_t ladder7 = secondary_mix_fold20f0(seed, 7u);
        const uint32_t ladder8 = secondary_mix_fold20f0(ladder7, 1u);
        const uint32_t pre = x * 0xf8e346a3u + ladder7 * 0xd0000000u + 0xbdfc4602u;
        out_param7_words13[lane] = pre * k_f41910b0[j] + k_f41910d0[j];
        dst->loop_x13[lane] = x;
        dst->ladder7_13[lane] = ladder7;
        dst->ladder8_rolling13[lane] = ladder8;
        dst->output_words13[lane] = out_param7_words13[lane];
        rolling = ladder7 * 0xc14d6629u + ladder8 * 0xeb299d70u + 0xff7f1ce2u;
    }
    return 0;
}

int l3_authorization_secondary_local460_export2130_words13(
    const uint32_t pre_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_local460_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_local460_export2130_trace *out) {
    if (!pre_words13 || !out_local460_words13) return -1;

    l3_authorization_secondary_local460_export2130_trace tmp;
    l3_authorization_secondary_local460_export2130_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));
    memcpy(dst->pre_words13, pre_words13, sizeof(dst->pre_words13));

    uint32_t rolling = 0x0d5d6910u;
    for (unsigned lane = 0; lane < L3_AUTH_ROUND_SECONDARY_WORDS13; ++lane) {
        const unsigned j = lane & 7u;
        dst->rolling_before13[lane] = rolling;
        const uint32_t x = rolling * 0x62dbba73u + pre_words13[lane];
        const uint32_t seed = x * 0x1742a02du + 0x9084aa92u;
        const uint32_t ladder7 = secondary_mix_fold2130(seed, 7u);
        const uint32_t ladder8 = secondary_mix_fold2130(ladder7, 1u);
        const uint32_t pre = x * 0xdcc43fd7u + ladder7 * 0xd0000000u + 0xae225ef9u;
        out_local460_words13[lane] = pre * k_f41910f0[j] + k_f4191110[j];
        dst->loop_x13[lane] = x;
        dst->ladder7_13[lane] = ladder7;
        dst->ladder8_rolling13[lane] = ladder8;
        dst->output_words13[lane] = out_local460_words13[lane];
        rolling = ladder7 * 0x3aa0ef87u + ladder8 * 0x55f10790u + 0x9ed493f9u;
    }
    return 0;
}


/* ---- v325: native vector preimage materialization for V321 ladders ---- */

static uint32_t secondary_mix_ctx_u32(const int32_t *ctx_words, uint32_t byte_offset) {
    return (uint32_t)ctx_words[byte_offset / 4u];
}

static uint32_t secondary_mix_madd4(uint32_t a, uint32_t ka,
                                     uint32_t b, uint32_t kb,
                                     uint32_t c, uint32_t kc,
                                     uint32_t d, uint32_t kd,
                                     uint32_t bias) {
    return a * ka + b * kb + c * kc + d * kd + bias;
}

int l3_authorization_secondary_vector_preimage_param7_local460_words13(
    const uint32_t sixth_secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local3c4_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local42c_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t *param4_ctx_words,
    const int32_t caller_seed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t caller_scalar0,
    uint32_t caller_scalar1,
    uint32_t out_param7_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_local460_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_vector_preimage_param7_local460_trace *out) {
    if (!sixth_secondary_vector_reduce_words13 || !local3c4_words13 || !local42c_words13 ||
        !param4_ctx_words || !caller_seed_words13 || !out_param7_words13 ||
        !out_local460_words13) {
        return -1;
    }

    l3_authorization_secondary_vector_preimage_param7_local460_trace tmp;
    l3_authorization_secondary_vector_preimage_param7_local460_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));
    memcpy(dst->sixth_secondary_vector_reduce_words13, sixth_secondary_vector_reduce_words13, sizeof(dst->sixth_secondary_vector_reduce_words13));
    memcpy(dst->local3c4_words13, local3c4_words13, sizeof(dst->local3c4_words13));
    memcpy(dst->local42c_words13, local42c_words13, sizeof(dst->local42c_words13));
    for (unsigned i = 0; i < L3_AUTH_ROUND_SECONDARY_WORDS13; ++i) {
        dst->caller_seed_words13[i] = (uint32_t)caller_seed_words13[i];
    }
    dst->caller_scalar0 = caller_scalar0;
    dst->caller_scalar1 = caller_scalar1;

    l3_authorization_secondary_pair32 raw2070[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 prefix2070[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 raw1ff0[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 prefix1ff0[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    int rc = l3_authorization_secondary_sixth_export_local3c4_accum2070_1ff0_sources(
        sixth_secondary_vector_reduce_words13, local3c4_words13,
        raw2070, prefix2070, raw1ff0, prefix1ff0,
        &dst->source_trace);
    if (rc != 0) return rc;

    l3_authorization_secondary_pair32 conv_pairs[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    rc = l3_authorization_secondary_local3c4_convolution26(
        raw2070, prefix2070, raw1ff0, prefix1ff0,
        conv_pairs, &dst->convolution);
    if (rc != 0) return rc;

    uint32_t state_pairs[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    for (unsigned i = 0; i < L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26; ++i) {
        state_pairs[i][0] = conv_pairs[i].lo;
        state_pairs[i][1] = conv_pairs[i].hi;
    }
    memcpy(dst->state_before_f3fdc45c26, state_pairs, sizeof(state_pairs));

    rc = l3_authorization_secondary_vector_reduce_run13_update_export(
        caller_seed_words13, caller_scalar0, caller_scalar1,
        state_pairs, dst->secondary_vector_reduce_words13, &dst->f3fdc45c);
    if (rc != 0) return rc;
    memcpy(dst->state_after_f3fdc45c26, state_pairs, sizeof(state_pairs));

    const uint32_t *side60 = dst->source_trace.side_vec60_0_3;
    const uint32_t *side99 = dst->source_trace.side_vec99_4_7;
    const uint32_t *side81 = dst->source_trace.side_vec81_8_11;
    const uint32_t side12 = dst->source_trace.side_word12;
    const uint32_t *seventh = dst->secondary_vector_reduce_words13;
    uint32_t *pre7 = dst->param7_pre_words13;

    static const uint32_t k64[4] = {0x91832e12u, 0x885fb512u, 0x27257116u, 0x4989a862u};
    static const uint32_t k89[4] = {0xa0b6e6bfu, 0xa7f23bbdu, 0x557812f1u, 0x6bfbdba9u};
    static const uint32_t k90[4] = {0x78a71eabu, 0x26289ca7u, 0x86ec475fu, 0xbb29939bu};
    static const uint32_t k82[4] = {0xacafbd85u, 0x237f4617u, 0x626f6049u, 0x4348e555u};
    static const uint32_t bias74[4] = {0xcbab106cu, 0xae78c79eu, 0x7262b2ffu, 0x7c9f3366u};
    for (unsigned i = 0; i < 4u; ++i) {
        pre7[i] = secondary_mix_madd4(side60[i], k64[i],
                                      seventh[i], k89[i],
                                      secondary_mix_ctx_u32(param4_ctx_words, 0x12cu + i * 4u), k90[i],
                                      local42c_words13[i], k82[i],
                                      bias74[i]);
    }

    pre7[4] = local42c_words13[4] * 0x11a5b17du +
              secondary_mix_ctx_u32(param4_ctx_words, 0x13cu) * 0xa0bfc78du +
              side99[0] * 0xd985ac8eu +
              seventh[4] * 0xaa2b7467u + 0xb14125f7u;
    pre7[5] = local42c_words13[5] * 0xffae300du +
              secondary_mix_ctx_u32(param4_ctx_words, 0x140u) * 0xb8ac3a4fu +
              side99[1] * 0x07e08432u +
              seventh[5] * 0x5df4348fu + 0x23534f58u;
    pre7[6] = local42c_words13[6] * 0xa5ebc339u +
              secondary_mix_ctx_u32(param4_ctx_words, 0x144u) * 0x68d2d19bu +
              side99[2] * 0x122e59beu +
              seventh[6] * 0x65fc723bu + 0x0bd2febdu;

    static const uint32_t k95[4] = {0x9fc377aeu, 0x91832e12u, 0x885fb512u, 0x27257116u};
    static const uint32_t k100[4] = {0x740f6055u, 0xacafbd85u, 0x237f4617u, 0x626f6049u};
    static const uint32_t k83[4] = {0xbc5e3a3du, 0xa0b6e6bfu, 0xa7f23bbdu, 0x557812f1u};
    static const uint32_t k73[4] = {0x27bd9071u, 0x78a71eabu, 0x26289ca7u, 0x86ec475fu};
    static const uint32_t bias75[4] = {0xb8084238u, 0xcbab106cu, 0xae78c79eu, 0x7262b2ffu};
    const uint32_t shifted_side[4] = {side99[3], side81[0], side81[1], side81[2]};
    for (unsigned j = 0; j < 4u; ++j) {
        const unsigned lane = 7u + j;
        pre7[lane] = secondary_mix_madd4(local42c_words13[lane], k100[j],
                                         shifted_side[j], k95[j],
                                         seventh[lane], k83[j],
                                         secondary_mix_ctx_u32(param4_ctx_words, 0x148u + j * 4u), k73[j],
                                         bias75[j]);
    }
    pre7[11] = secondary_mix_madd4(local42c_words13[11], k82[3],
                                   secondary_mix_ctx_u32(param4_ctx_words, 0x158u), k90[3],
                                   side81[3], k64[3],
                                   seventh[11], k89[3],
                                   bias74[3]);
    pre7[12] = local42c_words13[12] * 0x11a5b17du +
               secondary_mix_ctx_u32(param4_ctx_words, 0x15cu) * 0xa0bfc78du +
               side12 * 0xd985ac8eu +
               seventh[12] * 0xaa2b7467u + 0xb14125f7u;

    rc = l3_authorization_secondary_param7_export20f0_words13(pre7, out_param7_words13, &dst->param7_export);
    if (rc != 0) return rc;
    memcpy(dst->param7_words13, out_param7_words13, sizeof(dst->param7_words13));

    uint32_t *pre460 = dst->local460_pre_words13;
    static const uint32_t k91[4] = {0x71b96023u, 0x85bae6a3u, 0xe0026a39u, 0x52143edbu};
    static const uint32_t k65[4] = {0x6de40c27u, 0xecb7847fu, 0x117e6187u, 0xd718431du};
    static const uint32_t k84[4] = {0xd4500e4du, 0x681ac5d3u, 0x26e858d5u, 0xcf25d77du};
    static const uint32_t k76[4] = {0xd0a04c9fu, 0x4921893du, 0xe07d3a8du, 0xb6a167abu};
    static const uint32_t k107[4] = {0xe9b26df3u, 0x66ed100bu, 0x3038cdd3u, 0x4279141fu};
    static const uint32_t bias102[4] = {0xbbebe515u, 0xbfd182d5u, 0xd435cff8u, 0xb4e60910u};
    for (unsigned i = 0; i < 4u; ++i) {
        pre460[i] = side60[i] * k91[i] +
                    secondary_mix_ctx_u32(param4_ctx_words, 0x160u + i * 4u) * k65[i] +
                    out_param7_words13[i] * k76[i] +
                    bias102[i];
    }

    static const uint32_t k6[4] = {0x72f85bfdu, 0x2ba12d23u, 0xcb63e0f1u, 0xb9084ce9u};
    static const uint32_t bias7[4] = {0x74e1569eu, 0xab9fe406u, 0x972e4cdau, 0xa048d046u};
    for (unsigned i = 0; i < 4u; ++i) {
        pre460[4u + i] = side99[i] * k84[i] +
                         secondary_mix_ctx_u32(param4_ctx_words, 0x170u + i * 4u) * k107[i] +
                         out_param7_words13[4u + i] * k6[i] +
                         bias7[i];
    }
    for (unsigned i = 0; i < 4u; ++i) {
        pre460[8u + i] = side81[i] * k91[i] +
                         secondary_mix_ctx_u32(param4_ctx_words, 0x180u + i * 4u) * k65[i] +
                         out_param7_words13[8u + i] * k76[i] +
                         bias102[i];
    }
    pre460[12] = out_param7_words13[12] * 0x72f85bfdu +
                 side12 * 0xd4500e4du +
                 secondary_mix_ctx_u32(param4_ctx_words, 0x190u) * 0xe9b26df3u +
                 0x74e1569eu;

    rc = l3_authorization_secondary_local460_export2130_words13(pre460, out_local460_words13, &dst->local460_export);
    if (rc != 0) return rc;
    memcpy(dst->local460_words13, out_local460_words13, sizeof(dst->local460_words13));
    return 0;
}


/* ---- v288: FUN_f3fcc2d0 local_3c4/local_460 generated-pair accumulators ---- */

int l3_authorization_secondary_local3c4_accum2170_pairs13(
    const uint32_t local3c4_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_local3c4_accum2170_trace *out) {
    if (!local3c4_words13 || !out_pairs13 || !out_cumulative13) return -1;

    l3_authorization_secondary_local3c4_accum2170_trace tmp;
    l3_authorization_secondary_local3c4_accum2170_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    for (unsigned i = 0; i < L3_AUTH_ROUND_SECONDARY_WORDS13; ++i) {
        const unsigned j = i & 7u;
        uint32_t pre = u32_mla(local3c4_words13[i], k_f4191130[j], k_f4191150[j]);
        uint32_t x = u32_mla(pre, 0xada1bc41u, 0x64a94915u);
        dst->transformed_words13[i] = x;
        out_pairs13[i] = make_2170_pair(x, &dst->final_fold_lows13[i]);
        if (i == 0u) {
            out_cumulative13[i] = out_pairs13[i];
        } else {
            out_cumulative13[i].lo = out_cumulative13[i - 1u].lo + out_pairs13[i].lo;
            out_cumulative13[i].hi = out_cumulative13[i - 1u].hi + out_pairs13[i].hi +
                                     u32_add_carry(out_cumulative13[i - 1u].lo, out_pairs13[i].lo);
        }
        dst->generated_pairs13[i] = out_pairs13[i];
        dst->cumulative_pairs13[i] = out_cumulative13[i];
    }
    return 0;
}

int l3_authorization_secondary_local460_accum21f0_pairs13(
    const uint32_t local460_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_local460_accum21f0_trace *out) {
    if (!local460_words13 || !out_pairs13 || !out_cumulative13) return -1;

    l3_authorization_secondary_local460_accum21f0_trace tmp;
    l3_authorization_secondary_local460_accum21f0_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    for (unsigned i = 0; i < L3_AUTH_ROUND_SECONDARY_WORDS13; ++i) {
        const unsigned j = i & 7u;
        uint32_t pre = u32_mla(local460_words13[i], k_f4191170[j], k_f4191190[j]);
        uint32_t x = u32_mla(pre, 0x0484f291u, 0xfa2b116bu);
        dst->transformed_words13[i] = x;
        out_pairs13[i] = make_21f0_pair(x, &dst->final_fold_lows13[i]);
        if (i == 0u) {
            out_cumulative13[i] = out_pairs13[i];
        } else {
            out_cumulative13[i].lo = out_cumulative13[i - 1u].lo + out_pairs13[i].lo;
            out_cumulative13[i].hi = out_cumulative13[i - 1u].hi + out_pairs13[i].hi +
                                     u32_add_carry(out_cumulative13[i - 1u].lo, out_pairs13[i].lo);
        }
        dst->generated_pairs13[i] = out_pairs13[i];
        dst->cumulative_pairs13[i] = out_cumulative13[i];
    }
    return 0;
}

int l3_authorization_secondary_local3c4_local460_accum2170_21f0_pairs13(
    const uint32_t local3c4_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local460_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 raw2170_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix2170_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 raw21f0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix21f0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_local3c4_local460_accum2170_21f0_trace *out) {
    if (!local3c4_words13 || !local460_words13 || !raw2170_13 || !prefix2170_13 ||
        !raw21f0_13 || !prefix21f0_13) return -1;

    l3_authorization_secondary_local3c4_local460_accum2170_21f0_trace tmp;
    l3_authorization_secondary_local3c4_local460_accum2170_21f0_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    int rc = l3_authorization_secondary_local3c4_accum2170_pairs13(
        local3c4_words13, raw2170_13, prefix2170_13, &dst->accum2170);
    if (rc != 0) return rc;
    return l3_authorization_secondary_local460_accum21f0_pairs13(
        local460_words13, raw21f0_13, prefix21f0_13, &dst->accum21f0);
}

/* ---- v289: FUN_f3fcc2d0 local_3c4/local_460 convolution plus local_4c8 materialization ---- */

static uint32_t materialize_local4c8_word(
    unsigned idx,
    l3_authorization_secondary_pair32 pre,
    l3_authorization_secondary_pair32 *next_pre) {
    const uint32_t lo_mul0 = u32_mullo(pre.lo, 0xeeb4aaf5u);
    l3_authorization_secondary_pair32 seed = {
        lo_mul0 + 0x18055b23u,
        (uint32_t)(pre.hi * 0xeeb4aaf5u + pre.lo * 0x6d13eb15u +
                   u32_mulhi(pre.lo, 0xeeb4aaf5u) + 0x842a3a48u +
                   u32_add_carry(lo_mul0, 0x18055b23u))
    };
    const l3_authorization_secondary_pair32 folded7 = fold_rounds(seed, k_f41a2270, 7u);
    const l3_authorization_secondary_pair32 folded15 =
        fold_rounds(folded7, k_f41a2270, 8u);
    const uint32_t folded16_low_without_high_nibble =
        (folded15.lo >> 4) +
        k_f41a2270[folded15.lo & 0x0fu].lo;
    const uint32_t mixed = pre.lo * 0x76d34b3fu + folded7.lo * 0xd0000000u + 0x9c317154u;
    const uint32_t result =
        mixed * k_f41911b0[idx & 7u] + k_f41911d0[idx & 7u];
    if (next_pre) {
        const uint32_t next_mul_lo =
            u32_mullo(folded7.lo, 0xd7f1315du);
        next_pre->lo = next_mul_lo + 0x0ea93149u;
        next_pre->hi =
            folded7.hi * 0xd7f1315du +
            folded7.lo * 0x421ea893u +
            u32_mulhi(folded7.lo, 0xd7f1315du) +
            folded16_low_without_high_nibble * 0x80ecea30u +
            0xbd2d8e78u +
            u32_add_carry(next_mul_lo, 0x0ea93149u);
    }
    return result;
}

int l3_authorization_secondary_local3c4_local460_convolution26_materialize4c8(
    const l3_authorization_secondary_pair32 raw2170_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix2170_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 raw21f0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix21f0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    uint32_t out_local4c8_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_local3c4_local460_convolution26_trace *out) {
    if (!raw2170_13 || !prefix2170_13 || !raw21f0_13 || !prefix21f0_13 ||
        !out_local4c8_words26) return -1;

    l3_authorization_secondary_local3c4_local460_convolution26_trace tmp;
    l3_authorization_secondary_local3c4_local460_convolution26_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    uint32_t u = 0x02d47058u;
    uint32_t v = 0x623c32d1u;
    for (unsigned idx = 0; idx < L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26; ++idx) {
        unsigned first = idx - 12u;
        unsigned last;
        if (idx < 12u || first == 0u) {
            first = 0u;
            last = idx;
        } else {
            last = 12u;
        }

        l3_authorization_secondary_pair32 conv = {0u, 0u};
        l3_authorization_secondary_pair32 sum2170 = {0u, 0u};
        l3_authorization_secondary_pair32 sum21f0 = {0u, 0u};

        dst->first_last_count26[idx][0] = first;
        dst->first_last_count26[idx][1] = last;
        dst->first_last_count26[idx][2] = (first <= last) ? ((last - first) + 1u) : 0u;

        if (first <= last) {
            const uint32_t base_u = u;
            const uint32_t base_v = v;
            for (unsigned k = first; k <= last; ++k) {
                const l3_authorization_secondary_pair32 *a2170 = &raw2170_13[k];
                const l3_authorization_secondary_pair32 *b21f0 = &raw21f0_13[idx - k];
                const uint64_t prod = (uint64_t)a2170->lo * (uint64_t)b21f0->lo;
                const uint32_t old = conv.lo;
                conv.lo += (uint32_t)prod;
                conv.hi += b21f0->hi * a2170->lo + b21f0->lo * a2170->hi +
                           (uint32_t)(prod >> 32) + (uint32_t)(conv.lo < old);
            }

            secondary_pair_range_sum(prefix2170_13, first, last, &sum2170);
            secondary_pair_range_sum(prefix21f0_13, idx - last, idx - first, &sum21f0);

            const uint32_t count = (last - first) + 1u;
            const uint64_t count_prod = (uint64_t)count * 0x780071e2u;
            const uint32_t count_lo = (uint32_t)count_prod;
            const uint32_t count_hi = (uint32_t)(count_prod >> 32);
            const uint64_t conv_scaled = (uint64_t)conv.lo * 0x21941d0fu;
            const uint32_t conv_scaled_lo = (uint32_t)conv_scaled;
            const uint64_t sum2170_scaled = (uint64_t)sum2170.lo * 0xbcc235fbu;
            const uint32_t sum2170_scaled_lo = (uint32_t)sum2170_scaled;
            const uint64_t sum21f0_scaled = (uint64_t)sum21f0.lo * 0x4275245au;
            const uint32_t sum21f0_scaled_lo = (uint32_t)sum21f0_scaled;

            const uint32_t t0 = count_lo + base_u;
            const uint32_t t1 = t0 + conv_scaled_lo;
            const uint32_t t2 = t1 + sum2170_scaled_lo;
            u = t2 + sum21f0_scaled_lo;

            v = count * 0xfe384ba2u + count_hi + base_v +
                u32_add_carry(count_lo, base_u) +
                conv.hi * 0x21941d0fu + conv.lo * 0xdcfb09c4u +
                (uint32_t)(conv_scaled >> 32) + u32_add_carry(t0, conv_scaled_lo) +
                sum2170.hi * 0xbcc235fbu + sum2170.lo * 0x9dc8379du +
                (uint32_t)(sum2170_scaled >> 32) + u32_add_carry(t1, sum2170_scaled_lo) +
                sum21f0.hi * 0x4275245au + sum21f0.lo * 0x4b094bb8u +
                (uint32_t)(sum21f0_scaled >> 32) + u32_add_carry(t2, sum21f0_scaled_lo);
        }

        dst->convolution26[idx] = conv;
        dst->range2170_26[idx] = sum2170;
        dst->range21f0_26[idx] = sum21f0;
        dst->pre_reduce26[idx].lo = u;
        dst->pre_reduce26[idx].hi = v;
        l3_authorization_secondary_pair32 next_pre;
        out_local4c8_words26[idx] =
            materialize_local4c8_word(
                idx, dst->pre_reduce26[idx], &next_pre);
        dst->local4c8_words26[idx] = out_local4c8_words26[idx];
        u = next_pre.lo;
        v = next_pre.hi;
    }
    return 0;
}

int l3_authorization_secondary_local3c4_local460_accum_convolution26_materialize4c8(
    const uint32_t local3c4_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local460_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_local4c8_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_local3c4_local460_convolution26_trace *out) {
    if (!local3c4_words13 || !local460_words13 || !out_local4c8_words26) return -1;

    l3_authorization_secondary_pair32 raw2170[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 prefix2170[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 raw21f0[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 prefix21f0[L3_AUTH_ROUND_SECONDARY_PAIRS13];

    int rc = l3_authorization_secondary_local3c4_local460_accum2170_21f0_pairs13(
        local3c4_words13, local460_words13, raw2170, prefix2170, raw21f0, prefix21f0, NULL);
    if (rc != 0) return rc;
    return l3_authorization_secondary_local3c4_local460_convolution26_materialize4c8(
        raw2170, prefix2170, raw21f0, prefix21f0, out_local4c8_words26, out);
}

/* ---- v290: FUN_f3fcc2d0 param_2/local_42c generated-pair accumulators ---- */

int l3_authorization_secondary_param2_accum22f0_pairs13(
    const int32_t param2_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_param2_accum22f0_trace *out) {
    if (!param2_words13 || !out_pairs13 || !out_cumulative13) return -1;

    l3_authorization_secondary_param2_accum22f0_trace tmp;
    l3_authorization_secondary_param2_accum22f0_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    for (unsigned i = 0; i < L3_AUTH_ROUND_SECONDARY_WORDS13; ++i) {
        const unsigned j = i & 7u;
        const uint32_t pre = (uint32_t)param2_words13[i] * k_f41911f0[j] + k_f4191210[j];
        const uint32_t x = pre * 0x0b73db43u + 0x00f6503bu;
        dst->transformed_words13[i] = x;
        out_pairs13[i] = make_22f0_pair(x, &dst->final_fold_lows13[i]);
        if (i == 0u) out_cumulative13[i] = out_pairs13[i];
        else {
            out_cumulative13[i].lo = out_cumulative13[i - 1u].lo + out_pairs13[i].lo;
            out_cumulative13[i].hi = out_cumulative13[i - 1u].hi + out_pairs13[i].hi +
                                     u32_add_carry(out_cumulative13[i - 1u].lo, out_pairs13[i].lo);
        }
        dst->generated_pairs13[i] = out_pairs13[i];
        dst->cumulative_pairs13[i] = out_cumulative13[i];
    }
    return 0;
}

int l3_authorization_secondary_local42c_accum2370_pairs13(
    const uint32_t local42c_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_local42c_accum2370_trace *out) {
    if (!local42c_words13 || !out_pairs13 || !out_cumulative13) return -1;

    l3_authorization_secondary_local42c_accum2370_trace tmp;
    l3_authorization_secondary_local42c_accum2370_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    for (unsigned i = 0; i < L3_AUTH_ROUND_SECONDARY_WORDS13; ++i) {
        const unsigned j = i & 7u;
        const uint32_t pre = local42c_words13[i] * k_f4191230[j] + k_f4191250[j];
        const uint32_t x = pre * 0x1449c9c7u + 0x492e8f8au;
        dst->transformed_words13[i] = x;
        out_pairs13[i] = make_2370_pair(x, &dst->final_fold_lows13[i]);
        if (i == 0u) out_cumulative13[i] = out_pairs13[i];
        else {
            out_cumulative13[i].lo = out_cumulative13[i - 1u].lo + out_pairs13[i].lo;
            out_cumulative13[i].hi = out_cumulative13[i - 1u].hi + out_pairs13[i].hi +
                                     u32_add_carry(out_cumulative13[i - 1u].lo, out_pairs13[i].lo);
        }
        dst->generated_pairs13[i] = out_pairs13[i];
        dst->cumulative_pairs13[i] = out_cumulative13[i];
    }
    return 0;
}

int l3_authorization_secondary_param2_local42c_accum22f0_2370_pairs13(
    const int32_t param2_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local42c_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 raw22f0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix22f0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 raw2370_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix2370_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_param2_local42c_accum22f0_2370_trace *out) {
    if (!param2_words13 || !local42c_words13 || !raw22f0_13 || !prefix22f0_13 ||
        !raw2370_13 || !prefix2370_13) return -1;

    l3_authorization_secondary_param2_local42c_accum22f0_2370_trace tmp;
    l3_authorization_secondary_param2_local42c_accum22f0_2370_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    int rc = l3_authorization_secondary_param2_accum22f0_pairs13(
        param2_words13, raw22f0_13, prefix22f0_13, &dst->accum22f0);
    if (rc != 0) return rc;
    return l3_authorization_secondary_local42c_accum2370_pairs13(
        local42c_words13, raw2370_13, prefix2370_13, &dst->accum2370);
}



/* ---- v291: FUN_f3fcc2d0 param_2/local_42c convolution plus local_530 materialization ---- */

static uint32_t materialize_local530_word(
    unsigned idx,
    l3_authorization_secondary_pair32 pre,
    l3_authorization_secondary_pair32 *next_pre) {
    const uint32_t lo_mul0 = u32_mullo(pre.lo, 0x3c95eb47u);
    l3_authorization_secondary_pair32 seed = {
        lo_mul0 + 0x929f3fbau,
        (uint32_t)(pre.hi * 0x3c95eb47u + pre.lo * 0xd1a92c65u +
                   u32_mulhi(pre.lo, 0x3c95eb47u) + 0x5a5d9218u +
                   u32_add_carry(lo_mul0, 0x929f3fbau))
    };
    const l3_authorization_secondary_pair32 folded7 = fold_rounds(seed, k_f41a23f0, 7u);
    const l3_authorization_secondary_pair32 folded15 =
        fold_rounds(folded7, k_f41a23f0, 8u);
    const uint32_t folded16_low_without_high_nibble =
        (folded15.lo >> 4) +
        k_f41a23f0[folded15.lo & 0x0fu].lo;
    const uint32_t mixed = pre.lo * 0xbc2bc1f9u + folded7.lo * 0x10000000u + 0x6fad1d06u;
    const uint32_t result =
        mixed * k_f4191270[idx & 7u] + k_f4191290[idx & 7u];
    if (next_pre) {
        const uint32_t next_mul_lo =
            u32_mullo(folded7.lo, 0xa2604e77u);
        next_pre->lo = next_mul_lo + 0xb8ceb48au;
        next_pre->hi =
            folded7.hi * 0xa2604e77u +
            folded7.lo * 0xe58297a0u +
            u32_mulhi(folded7.lo, 0xa2604e77u) +
            folded16_low_without_high_nibble * 0xd9fb1890u +
            0xfef16abbu +
            u32_add_carry(next_mul_lo, 0xb8ceb48au);
    }
    return result;
}

int l3_authorization_secondary_param2_local42c_convolution26_materialize530(
    const l3_authorization_secondary_pair32 raw22f0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix22f0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 raw2370_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix2370_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    uint32_t out_local530_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_param2_local42c_convolution26_trace *out) {
    if (!raw22f0_13 || !prefix22f0_13 || !raw2370_13 || !prefix2370_13 ||
        !out_local530_words26) return -1;

    l3_authorization_secondary_param2_local42c_convolution26_trace tmp;
    l3_authorization_secondary_param2_local42c_convolution26_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    uint32_t u = 0x7b67165cu;
    uint32_t v = 0xdb7c4d23u;
    for (unsigned idx = 0; idx < L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26; ++idx) {
        unsigned first = idx - 12u;
        unsigned last;
        if (idx < 12u || first == 0u) {
            first = 0u;
            last = idx;
        } else {
            last = 12u;
        }

        l3_authorization_secondary_pair32 conv = {0u, 0u};
        l3_authorization_secondary_pair32 sum22f0 = {0u, 0u};
        l3_authorization_secondary_pair32 sum2370 = {0u, 0u};

        dst->first_last_count26[idx][0] = first;
        dst->first_last_count26[idx][1] = last;
        dst->first_last_count26[idx][2] = (first <= last) ? ((last - first) + 1u) : 0u;

        if (first <= last) {
            const uint32_t base_u = u;
            const uint32_t base_v = v;
            for (unsigned k = first; k <= last; ++k) {
                const l3_authorization_secondary_pair32 *a22f0 = &raw22f0_13[k];
                const l3_authorization_secondary_pair32 *b2370 = &raw2370_13[idx - k];
                const uint64_t prod = (uint64_t)a22f0->lo * (uint64_t)b2370->lo;
                const uint32_t old = conv.lo;
                conv.lo += (uint32_t)prod;
                conv.hi += b2370->hi * a22f0->lo + b2370->lo * a22f0->hi +
                           (uint32_t)(prod >> 32) + (uint32_t)(conv.lo < old);
            }

            secondary_pair_range_sum(prefix22f0_13, first, last, &sum22f0);
            secondary_pair_range_sum(prefix2370_13, idx - last, idx - first, &sum2370);

            const uint32_t count = (last - first) + 1u;
            const uint64_t count_prod = (uint64_t)count * 0x73c73c20u;
            const uint32_t count_lo = (uint32_t)count_prod;
            const uint32_t count_hi = (uint32_t)(count_prod >> 32);
            const uint64_t conv_scaled = (uint64_t)conv.lo * 0xbed1366bu;
            const uint32_t conv_scaled_lo = (uint32_t)conv_scaled;
            const uint64_t sum22f0_scaled = (uint64_t)sum22f0.lo * 0xc9c3c308u;
            const uint32_t sum22f0_scaled_lo = (uint32_t)sum22f0_scaled;
            const uint64_t sum2370_scaled = (uint64_t)sum2370.lo * 0x5baf9bacu;
            const uint32_t sum2370_scaled_lo = (uint32_t)sum2370_scaled;

            const uint32_t t0 = count_lo + base_u;
            const uint32_t t1 = t0 + conv_scaled_lo;
            const uint32_t t2 = t1 + sum22f0_scaled_lo;
            u = t2 + sum2370_scaled_lo;

            v = count * 0x92e565adu + count_hi + base_v +
                u32_add_carry(count_lo, base_u) +
                conv.hi * 0xbed1366bu + conv.lo * 0xe20e3e45u +
                (uint32_t)(conv_scaled >> 32) + u32_add_carry(t0, conv_scaled_lo) +
                sum22f0.hi * 0xc9c3c308u + sum22f0.lo * 0xdc0cb70fu +
                (uint32_t)(sum22f0_scaled >> 32) + u32_add_carry(t1, sum22f0_scaled_lo) +
                sum2370.hi * 0x5baf9bacu + sum2370.lo * 0x8fcf80ddu +
                (uint32_t)(sum2370_scaled >> 32) + u32_add_carry(t2, sum2370_scaled_lo);
        }

        dst->convolution26[idx] = conv;
        dst->range22f0_26[idx] = sum22f0;
        dst->range2370_26[idx] = sum2370;
        dst->pre_reduce26[idx].lo = u;
        dst->pre_reduce26[idx].hi = v;
        l3_authorization_secondary_pair32 next_pre;
        out_local530_words26[idx] =
            materialize_local530_word(
                idx, dst->pre_reduce26[idx], &next_pre);
        dst->local530_words26[idx] = out_local530_words26[idx];
        u = next_pre.lo;
        v = next_pre.hi;
    }
    return 0;
}

int l3_authorization_secondary_param2_local42c_accum_convolution26_materialize530(
    const int32_t param2_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local42c_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_local530_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_param2_local42c_convolution26_trace *out) {
    if (!param2_words13 || !local42c_words13 || !out_local530_words26) return -1;

    l3_authorization_secondary_pair32 raw22f0[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 prefix22f0[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 raw2370[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 prefix2370[L3_AUTH_ROUND_SECONDARY_PAIRS13];

    int rc = l3_authorization_secondary_param2_local42c_accum22f0_2370_pairs13(
        param2_words13, local42c_words13, raw22f0, prefix22f0, raw2370, prefix2370, NULL);
    if (rc != 0) return rc;
    return l3_authorization_secondary_param2_local42c_convolution26_materialize530(
        raw22f0, prefix22f0, raw2370, prefix2370, out_local530_words26, out);
}


/* ---- v292: FUN_f3fcc2d0 local_4c8/param4+0x194/local_530 mixed fold to local_598 ---- */

static uint32_t fold2470_step(uint32_t x) {
    return k_f41a2470[x & 0x0fu] + (x >> 4);
}

static void seed_native_words(
    const uint32_t carry_seed_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    uint32_t seed[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26]) {
    static const uint32_t k_seed8[8] = {
        0xa506620bu, 0x67973ba1u, 0x559d45b1u, 0x9985821au,
        0x340b9387u, 0xdbd4ef2eu, 0x14383e08u, 0x0d0fbc26u
    };
    (void)carry_seed_words26;
    for (unsigned i = 0; i < L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26; ++i) {
        seed[i] = k_seed8[i & 7u];
    }
}

int l3_authorization_secondary_local4c8_param4_local530_fold598_words26(
    const uint32_t local4c8_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    const int32_t param4_194_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    const uint32_t local530_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    const uint32_t carry_seed_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    uint32_t out_local598_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_local4c8_param4_local530_fold598_trace *out) {
    if (!local4c8_words26 || !param4_194_words26 || !local530_words26 ||
        !carry_seed_words26 || !out_local598_words26) return -1;

    l3_authorization_secondary_local4c8_param4_local530_fold598_trace tmp;
    l3_authorization_secondary_local4c8_param4_local530_fold598_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    seed_native_words(carry_seed_words26, dst->native_seed26);

    for (unsigned i = 0; i < L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26; ++i) {
        const unsigned j = i & 7u;
        dst->after_local4c8_26[i] = dst->native_seed26[i] + local4c8_words26[i] * k_f41912b0[j];
        dst->after_param4_26[i] = dst->after_local4c8_26[i] +
                                  (uint32_t)param4_194_words26[i] * k_f41912d0[j];
        dst->pre_fold26[i] = dst->after_param4_26[i] + local530_words26[i] * k_f41912f0[j];
    }

    uint32_t rolling = 0x7ed58406u;
    for (unsigned i = 0; i < L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26; ++i) {
        const unsigned j = i & 7u;
        const uint32_t mixed = rolling * 0xc56f1b5bu + dst->pre_fold26[i];
        uint32_t folded = mixed * 0x08fac4c7u + 0x4685c5f6u;
        for (unsigned r = 0; r < 7u; ++r) folded = fold2470_step(folded);
        dst->fold7_26[i] = folded;
        dst->fold8_26[i] = fold2470_step(folded);
        rolling = folded * 0xd6694395u + dst->fold8_26[i] * 0x996bc6b0u + 0x06416256u;
        out_local598_words26[i] =
            (mixed * 0xf72d3e81u + folded * 0x90000000u + 0x1cdee259u) * k_f4191310[j] +
            k_f4191330[j];
        dst->local598_words26[i] = out_local598_words26[i];
    }
    return 0;
}


/* ---- v293: FUN_f3fcc2d0 local_598 transform through DAT_f41a24b0 to local_328 ---- */

int l3_authorization_secondary_local598_transform24b0_pairs26(
    const uint32_t local598_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_local598_transform24b0_trace *out) {
    if (!local598_words26 || !out_pairs26) return -1;

    l3_authorization_secondary_local598_transform24b0_trace tmp;
    l3_authorization_secondary_local598_transform24b0_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    for (unsigned i = 0; i < L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26; ++i) {
        const unsigned j = i & 7u;
        const uint32_t transformed =
            (local598_words26[i] * k_f4191350[j] + k_f4191370[j]) * 0xbec7a6edu +
            0xe4aeb4c4u;
        uint32_t fold_low = 0u;
        out_pairs26[i] = make_24b0_pair(transformed, &fold_low);
        dst->transformed_words26[i] = transformed;
        dst->final_fold_lows26[i] = fold_low;
        dst->output_pairs26[i] = out_pairs26[i];
    }
    return 0;
}

/* ---- v294: bind V293 local_328 into f3fdc45c plus param_8 export ---- */

int l3_authorization_secondary_post_transform24b0_param8_words13(
    const uint32_t secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_param8_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_post_transform24b0_param8_trace *out) {
    if (!secondary_vector_reduce_words13 || !out_param8_words13) return -1;

    static const uint32_t k_group0_or_2[4] = {
        0x7855bcbfu, 0xd16d0851u, 0xfbacd307u, 0xec9b62cfu
    };
    static const uint32_t k_group1[4] = {
        0xc02cd675u, 0x8b205187u, 0xb9e6a021u, 0xef03d1f1u
    };
    static const uint32_t k_bias_group0_or_2[4] = {
        0x50d69d21u, 0x50ac976cu, 0x7eb8cf97u, 0xbb3b36b5u
    };
    static const uint32_t k_bias_group1[4] = {
        0x5fc723a5u, 0xd70be87du, 0xeb1a9e97u, 0x9ca9856du
    };

    l3_authorization_secondary_post_transform24b0_param8_trace tmp;
    l3_authorization_secondary_post_transform24b0_param8_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));
    memcpy(dst->input_words13, secondary_vector_reduce_words13, sizeof(dst->input_words13));

    for (unsigned i = 0; i < 4u; ++i) {
        out_param8_words13[i] =
            secondary_vector_reduce_words13[i] * k_group0_or_2[i] + k_bias_group0_or_2[i];
        out_param8_words13[4u + i] =
            secondary_vector_reduce_words13[4u + i] * k_group1[i] + k_bias_group1[i];
        out_param8_words13[8u + i] =
            secondary_vector_reduce_words13[8u + i] * k_group0_or_2[i] + k_bias_group0_or_2[i];
        dst->param8_group0_4[i] = out_param8_words13[i];
        dst->param8_group1_4[i] = out_param8_words13[4u + i];
        dst->param8_group2_4[i] = out_param8_words13[8u + i];
    }
    out_param8_words13[12] = secondary_vector_reduce_words13[12] * 0xc02cd675u + 0x5fc723a5u;
    dst->param8_tail_word12 = out_param8_words13[12];
    memcpy(dst->output_words13, out_param8_words13, sizeof(dst->output_words13));
    return 0;
}

int l3_authorization_secondary_local598_bound_secondary_vector_reduce_param8_words13(
    const uint32_t local598_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    const int32_t caller_seed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t caller_scalar0,
    uint32_t caller_scalar1,
    uint32_t out_param8_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_bound_secondary_vector_reduce_param8_trace *out) {
    if (!local598_words26 || !caller_seed_words13 || !out_param8_words13) return -1;

    l3_authorization_secondary_bound_secondary_vector_reduce_param8_trace tmp;
    l3_authorization_secondary_bound_secondary_vector_reduce_param8_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));
    memcpy(dst->local598_words26, local598_words26, sizeof(dst->local598_words26));
    for (unsigned i = 0; i < L3_AUTH_ROUND_SECONDARY_WORDS13; ++i) {
        dst->caller_seed_words13[i] = (uint32_t)caller_seed_words13[i];
    }
    dst->caller_scalar0 = caller_scalar0;
    dst->caller_scalar1 = caller_scalar1;

    l3_authorization_secondary_pair32 local328_pairs[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    int rc = l3_authorization_secondary_local598_transform24b0_pairs26(
        local598_words26, local328_pairs, &dst->transform24b0);
    if (rc != 0) return rc;

    uint32_t state_pairs[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    for (unsigned i = 0; i < L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26; ++i) {
        state_pairs[i][0] = local328_pairs[i].lo;
        state_pairs[i][1] = local328_pairs[i].hi;
    }
    memcpy(dst->state_before_f3fdc45c26, state_pairs, sizeof(state_pairs));

    uint32_t secondary_vector_reduce_words[L3_AUTH_ROUND_SECONDARY_WORDS13];
    rc = l3_authorization_secondary_vector_reduce_run13_update_export(
        caller_seed_words13, caller_scalar0, caller_scalar1,
        state_pairs, secondary_vector_reduce_words, &dst->f3fdc45c);
    if (rc != 0) return rc;
    memcpy(dst->state_after_f3fdc45c26, state_pairs, sizeof(state_pairs));
    memcpy(dst->secondary_vector_reduce_words13, secondary_vector_reduce_words, sizeof(secondary_vector_reduce_words));

    return l3_authorization_secondary_post_transform24b0_param8_words13(
        secondary_vector_reduce_words, out_param8_words13, &dst->param8_export);
}

int l3_authorization_secondary_build_terminal_output_a(
    const uint32_t local4c8_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    const int32_t param4_194_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    const uint32_t local530_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    const uint32_t carry_seed_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    const int32_t caller_seed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t caller_scalar0,
    uint32_t caller_scalar1,
    uint32_t out_param8_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_bound_fold598_secondary_vector_reduce_param8_trace *out) {
    if (!local4c8_words26 || !param4_194_words26 || !local530_words26 ||
        !carry_seed_words26 || !caller_seed_words13 || !out_param8_words13) return -1;

    l3_authorization_secondary_bound_fold598_secondary_vector_reduce_param8_trace tmp;
    l3_authorization_secondary_bound_fold598_secondary_vector_reduce_param8_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    uint32_t local598_words[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    int rc = l3_authorization_secondary_local4c8_param4_local530_fold598_words26(
        local4c8_words26, param4_194_words26, local530_words26, carry_seed_words26,
        local598_words, &dst->fold598);
    if (rc != 0) return rc;
    memcpy(dst->local598_words26, local598_words, sizeof(local598_words));

    return l3_authorization_secondary_local598_bound_secondary_vector_reduce_param8_words13(
        local598_words, caller_seed_words13, caller_scalar0, caller_scalar1,
        out_param8_words13, &dst->bound_param8);
}



/* ---- v295: FUN_f3fcc2d0 post-param8 param_3/local_390 accumulators ---- */

int l3_authorization_secondary_param3_accum2530_pairs13(
    const int32_t param3_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_param3_accum2530_trace *out) {
    if (!param3_words13 || !out_pairs13 || !out_cumulative13) return -1;

    l3_authorization_secondary_param3_accum2530_trace tmp;
    l3_authorization_secondary_param3_accum2530_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    for (unsigned i = 0; i < L3_AUTH_ROUND_SECONDARY_WORDS13; ++i) {
        uint32_t x;
        if (i == 0u) {
            x = (uint32_t)param3_words13[0] * 0x03004743u + 0xcb419217u;
        } else {
            const unsigned j = i & 7u;
            const uint32_t pre = (uint32_t)param3_words13[i] * k_f4191390[j] + k_f41913b0[j];
            x = pre * 0x491ad6edu + 0xa84209c3u;
        }
        dst->transformed_words13[i] = x;
        out_pairs13[i] = make_2530_pair(x, &dst->final_fold_lows13[i]);
        if (i == 0u) out_cumulative13[i] = out_pairs13[i];
        else {
            out_cumulative13[i].lo = out_cumulative13[i - 1u].lo + out_pairs13[i].lo;
            out_cumulative13[i].hi = out_cumulative13[i - 1u].hi + out_pairs13[i].hi +
                                     u32_add_carry(out_cumulative13[i - 1u].lo, out_pairs13[i].lo);
        }
        dst->generated_pairs13[i] = out_pairs13[i];
        dst->cumulative_pairs13[i] = out_cumulative13[i];
    }
    return 0;
}

int l3_authorization_secondary_local390_accum25b0_pairs13(
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_local390_accum25b0_trace *out) {
    if (!local390_words13 || !out_pairs13 || !out_cumulative13) return -1;

    l3_authorization_secondary_local390_accum25b0_trace tmp;
    l3_authorization_secondary_local390_accum25b0_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    for (unsigned i = 0; i < L3_AUTH_ROUND_SECONDARY_WORDS13; ++i) {
        uint32_t x;
        if (i == 0u) {
            x = local390_words13[0] * 0xad7a7d9bu + 0x6a02c11fu;
        } else {
            const unsigned j = i & 7u;
            const uint32_t pre = local390_words13[i] * k_f41913d0[j] + k_f41913f0[j];
            x = pre * 0xd2d97dadu + 0xb7e5690cu;
        }
        dst->transformed_words13[i] = x;
        out_pairs13[i] = make_25b0_pair(x, &dst->final_fold_lows13[i]);
        if (i == 0u) out_cumulative13[i] = out_pairs13[i];
        else {
            out_cumulative13[i].lo = out_cumulative13[i - 1u].lo + out_pairs13[i].lo;
            out_cumulative13[i].hi = out_cumulative13[i - 1u].hi + out_pairs13[i].hi +
                                     u32_add_carry(out_cumulative13[i - 1u].lo, out_pairs13[i].lo);
        }
        dst->generated_pairs13[i] = out_pairs13[i];
        dst->cumulative_pairs13[i] = out_cumulative13[i];
    }
    return 0;
}

int l3_authorization_secondary_param3_local390_accum2530_25b0_pairs13(
    const int32_t param3_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 raw2530_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix2530_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 raw25b0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix25b0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_param3_local390_accum2530_25b0_trace *out) {
    if (!param3_words13 || !local390_words13 || !raw2530_13 || !prefix2530_13 ||
        !raw25b0_13 || !prefix25b0_13) return -1;

    l3_authorization_secondary_param3_local390_accum2530_25b0_trace tmp;
    l3_authorization_secondary_param3_local390_accum2530_25b0_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    int rc = l3_authorization_secondary_param3_accum2530_pairs13(
        param3_words13, raw2530_13, prefix2530_13, &dst->accum2530);
    if (rc != 0) return rc;
    return l3_authorization_secondary_local390_accum25b0_pairs13(
        local390_words13, raw25b0_13, prefix25b0_13, &dst->accum25b0);
}

/* ---- v296: FUN_f3fcc2d0 post-param8 param_3/local_390 convolution/reduction ---- */

int l3_authorization_secondary_param3_local390_convolution26(
    const l3_authorization_secondary_pair32 raw2530_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix2530_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 raw25b0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix25b0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_param3_local390_convolution26_trace *out) {
    if (!raw2530_13 || !prefix2530_13 || !raw25b0_13 || !prefix25b0_13 || !out_pairs26) return -1;

    l3_authorization_secondary_param3_local390_convolution26_trace tmp;
    l3_authorization_secondary_param3_local390_convolution26_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    for (unsigned idx = 0; idx < L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26; ++idx) {
        unsigned first = idx - 12u;
        unsigned last;
        if (idx < 12u || first == 0u) {
            first = 0u;
            last = idx;
        } else {
            last = 12u;
        }

        uint32_t u = 0x2a742871u;
        uint32_t v = 0x4d2cd6f2u;
        l3_authorization_secondary_pair32 conv = {0u, 0u};
        l3_authorization_secondary_pair32 sum2530 = {0u, 0u};
        l3_authorization_secondary_pair32 sum25b0 = {0u, 0u};

        dst->first_last_count26[idx][0] = first;
        dst->first_last_count26[idx][1] = last;
        dst->first_last_count26[idx][2] = (first <= last) ? ((last - first) + 1u) : 0u;

        if (first <= last) {
            for (unsigned k = first; k <= last; ++k) {
                const l3_authorization_secondary_pair32 *a2530 = &raw2530_13[k];
                const l3_authorization_secondary_pair32 *b25b0 = &raw25b0_13[idx - k];
                const uint64_t prod = (uint64_t)a2530->lo * (uint64_t)b25b0->lo;
                const uint32_t old = conv.lo;
                conv.lo += (uint32_t)prod;
                conv.hi += b25b0->hi * a2530->lo + b25b0->lo * a2530->hi +
                           (uint32_t)(prod >> 32) + (uint32_t)(conv.lo < old);
            }

            secondary_pair_range_sum(prefix2530_13, first, last, &sum2530);
            secondary_pair_range_sum(prefix25b0_13, idx - last, idx - first, &sum25b0);

            const uint32_t count = (last - first) + 1u;
            const uint64_t count_prod = (uint64_t)count * 0x725af1b1u;
            const uint32_t count_lo = (uint32_t)count_prod;
            const uint32_t count_hi = (uint32_t)(count_prod >> 32);
            const uint64_t conv_scaled = (uint64_t)conv.lo * 0x6801fe6fu;
            const uint32_t conv_scaled_lo = (uint32_t)conv_scaled;
            const uint64_t sum2530_scaled = (uint64_t)sum2530.lo * 0x443fe827u;
            const uint32_t sum2530_scaled_lo = (uint32_t)sum2530_scaled;
            const uint64_t sum25b0_scaled = (uint64_t)sum25b0.lo * 0x17aa98a9u;
            const uint32_t sum25b0_scaled_lo = (uint32_t)sum25b0_scaled;

            const uint32_t t0 = count_lo + 0x2a742871u;
            const uint32_t t1 = t0 + conv_scaled_lo;
            const uint32_t t2 = t1 + sum2530_scaled_lo;
            u = t2 + sum25b0_scaled_lo;

            v = count * 0xb3865321u + count_hi + 0x4d2cd6f2u +
                u32_add_carry(count_lo, 0x2a742871u) +
                conv.hi * 0x6801fe6fu + conv.lo * 0xd602e0d7u +
                (uint32_t)(conv_scaled >> 32) + u32_add_carry(t0, conv_scaled_lo) +
                sum2530.hi * 0x443fe827u + sum2530.lo * 0xfdca5bd0u +
                (uint32_t)(sum2530_scaled >> 32) + u32_add_carry(t1, sum2530_scaled_lo) +
                sum25b0.hi * 0x17aa98a9u + sum25b0.lo * 0xa9094996u +
                (uint32_t)(sum25b0_scaled >> 32) + u32_add_carry(t2, sum25b0_scaled_lo);
        }

        const uint32_t out_lo_mul = u * 0x2afd0a19u;
        out_pairs26[idx].lo = out_lo_mul + 0x4638e32bu;
        out_pairs26[idx].hi = v * 0x2afd0a19u + u * 0xda36d7b1u +
                              u32_mulhi(u, 0x2afd0a19u) + 0x867bac58u +
                              u32_add_carry(out_lo_mul, 0x4638e32bu);
        dst->convolution26[idx] = conv;
        dst->range2530_26[idx] = sum2530;
        dst->range25b0_26[idx] = sum25b0;
        dst->pre_reduce26[idx].lo = u;
        dst->pre_reduce26[idx].hi = v;
        dst->output26[idx] = out_pairs26[idx];
    }
    return 0;
}

int l3_authorization_secondary_param3_local390_accum_convolution26(
    const int32_t param3_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_param3_local390_convolution26_trace *out) {
    if (!param3_words13 || !local390_words13 || !out_pairs26) return -1;

    l3_authorization_secondary_pair32 raw2530[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 prefix2530[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 raw25b0[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 prefix25b0[L3_AUTH_ROUND_SECONDARY_PAIRS13];

    int rc = l3_authorization_secondary_param3_local390_accum2530_25b0_pairs13(
        param3_words13, local390_words13, raw2530, prefix2530, raw25b0, prefix25b0, NULL);
    if (rc != 0) return rc;
    return l3_authorization_secondary_param3_local390_convolution26(
        raw2530, prefix2530, raw25b0, prefix25b0, out_pairs26, out);
}


/* ---- v297: bind V296 local_328 into f3fdc45c plus param_9 export ---- */

int l3_authorization_secondary_post_param3_local390_param9_words13(
    const uint32_t secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_param9_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_post_param3_local390_param9_trace *out) {
    if (!secondary_vector_reduce_words13 || !out_param9_words13) return -1;

    static const uint32_t k_group0_or_2[4] = {
        0x6372562bu, 0xa7a9f09fu, 0x8431ff4bu, 0x8bba90afu
    };
    static const uint32_t k_group1[4] = {
        0x0d1925bfu, 0x4ce707b7u, 0x41763e49u, 0x11b3445fu
    };
    static const uint32_t k_bias_group0_or_2[4] = {
        0xe2273afdu, 0xa57d0cb0u, 0xb709cdc8u, 0x96a8b7eau
    };
    static const uint32_t k_bias_group1[4] = {
        0xb9daff66u, 0x1a3fe823u, 0x18f34030u, 0xc549f7fcu
    };

    l3_authorization_secondary_post_param3_local390_param9_trace tmp;
    l3_authorization_secondary_post_param3_local390_param9_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));
    memcpy(dst->input_words13, secondary_vector_reduce_words13, sizeof(dst->input_words13));

    for (unsigned i = 0; i < 4u; ++i) {
        out_param9_words13[i] =
            secondary_vector_reduce_words13[i] * k_group0_or_2[i] + k_bias_group0_or_2[i];
        out_param9_words13[4u + i] =
            secondary_vector_reduce_words13[4u + i] * k_group1[i] + k_bias_group1[i];
        out_param9_words13[8u + i] =
            secondary_vector_reduce_words13[8u + i] * k_group0_or_2[i] + k_bias_group0_or_2[i];
        dst->param9_group0_4[i] = out_param9_words13[i];
        dst->param9_group1_4[i] = out_param9_words13[4u + i];
        dst->param9_group2_4[i] = out_param9_words13[8u + i];
    }
    out_param9_words13[12] = secondary_vector_reduce_words13[12] * 0x0d1925bfu + 0xb9daff66u;
    dst->param9_tail_word12 = out_param9_words13[12];
    memcpy(dst->output_words13, out_param9_words13, sizeof(dst->output_words13));
    return 0;
}

int l3_authorization_secondary_build_terminal_output_b(
    const int32_t param3_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t caller_seed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t caller_scalar0,
    uint32_t caller_scalar1,
    uint32_t out_param9_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_bound_secondary_vector_reduce_param9_trace *out) {
    if (!param3_words13 || !local390_words13 || !caller_seed_words13 || !out_param9_words13) return -1;

    l3_authorization_secondary_bound_secondary_vector_reduce_param9_trace tmp;
    l3_authorization_secondary_bound_secondary_vector_reduce_param9_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));
    for (unsigned i = 0; i < L3_AUTH_ROUND_SECONDARY_WORDS13; ++i) {
        dst->param3_words13[i] = (uint32_t)param3_words13[i];
        dst->local390_words13[i] = local390_words13[i];
        dst->caller_seed_words13[i] = (uint32_t)caller_seed_words13[i];
    }
    dst->caller_scalar0 = caller_scalar0;
    dst->caller_scalar1 = caller_scalar1;

    l3_authorization_secondary_pair32 conv_pairs[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    int rc = l3_authorization_secondary_param3_local390_accum_convolution26(
        param3_words13, local390_words13, conv_pairs, &dst->convolution);
    if (rc != 0) return rc;

    uint32_t state_pairs[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    for (unsigned i = 0; i < L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26; ++i) {
        state_pairs[i][0] = conv_pairs[i].lo;
        state_pairs[i][1] = conv_pairs[i].hi;
    }
    memcpy(dst->state_before_f3fdc45c26, state_pairs, sizeof(state_pairs));

    uint32_t secondary_vector_reduce_words[L3_AUTH_ROUND_SECONDARY_WORDS13];
    rc = l3_authorization_secondary_vector_reduce_run13_update_export(
        caller_seed_words13, caller_scalar0, caller_scalar1,
        state_pairs, secondary_vector_reduce_words, &dst->f3fdc45c);
    if (rc != 0) return rc;

    memcpy(dst->state_after_f3fdc45c26, state_pairs, sizeof(state_pairs));
    memcpy(dst->secondary_vector_reduce_words13, secondary_vector_reduce_words, sizeof(secondary_vector_reduce_words));

    return l3_authorization_secondary_post_param3_local390_param9_words13(
        secondary_vector_reduce_words, out_param9_words13, &dst->param9_export);
}


/* ---- v298: adapter-facing terminal param_8/param_9 composition ---- */

int l3_authorization_secondary_build_terminal_outputs(
    const uint32_t local4c8_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    const int32_t param4_194_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    const uint32_t local530_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    const uint32_t carry_seed_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    const int32_t param3_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t caller_seed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t caller_scalar0,
    uint32_t caller_scalar1,
    uint32_t out_param8_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_param9_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_terminal_param8_param9_trace *out) {
    if (!local4c8_words26 || !param4_194_words26 || !local530_words26 ||
        !carry_seed_words26 || !param3_words13 || !local390_words13 ||
        !caller_seed_words13 || !out_param8_words13 || !out_param9_words13) {
        return -1;
    }

    l3_authorization_secondary_terminal_param8_param9_trace tmp;
    l3_authorization_secondary_terminal_param8_param9_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    int rc = l3_authorization_secondary_build_terminal_output_a(
        local4c8_words26, param4_194_words26, local530_words26, carry_seed_words26,
        caller_seed_words13, caller_scalar0, caller_scalar1,
        out_param8_words13, &dst->param8_tail);
    if (rc != 0) return rc;

    rc = l3_authorization_secondary_build_terminal_output_b(
        param3_words13, local390_words13, caller_seed_words13,
        caller_scalar0, caller_scalar1, out_param9_words13, &dst->param9_tail);
    if (rc != 0) return rc;

    memcpy(dst->param8_words13, out_param8_words13, sizeof(dst->param8_words13));
    memcpy(dst->param9_words13, out_param9_words13, sizeof(dst->param9_words13));
    return 0;
}

/* ---- v328: local_3f8-derived local_42c/sixth export into V325 preimage ---- */

int l3_authorization_secondary_local3f8_bound_sixth_local42c_preimage_param7_local460_words13(
    const uint32_t local3f8_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t *param4_ctx_words,
    const int32_t param1_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t caller_seed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t caller_scalar0,
    uint32_t caller_scalar1,
    const uint32_t local3c4_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_local42c_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_sixth_secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_local5b0_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_local460_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_local3f8_bound_sixth_local42c_preimage_trace *out) {
    if (!local3f8_words13 || !local390_words13 || !param4_ctx_words ||
        !param1_words13 || !caller_seed_words13 || !local3c4_words13 || !out_local42c_words13 ||
        !out_sixth_secondary_vector_reduce_words13 || !out_local5b0_words13 ||
        !out_local460_words13) {
        return -1;
    }

    l3_authorization_secondary_local3f8_bound_sixth_local42c_preimage_trace tmp;
    l3_authorization_secondary_local3f8_bound_sixth_local42c_preimage_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    memcpy(dst->local3f8_words13, local3f8_words13, sizeof(dst->local3f8_words13));
    memcpy(dst->local390_words13, local390_words13, sizeof(dst->local390_words13));
    for (unsigned i = 0; i < L3_AUTH_ROUND_SECONDARY_WORDS13; ++i) {
        dst->param1_words13[i] = (uint32_t)param1_words13[i];
    }
    memcpy(dst->local3c4_words13, local3c4_words13, sizeof(dst->local3c4_words13));

    int rc = l3_authorization_secondary_derive_state_stage4(
        local3f8_words13,
        local390_words13,
        local390_words13[0],
        caller_seed_words13,
        caller_scalar0,
        caller_scalar1,
        out_local42c_words13,
        &dst->local42c_trace);
    if (rc != 0) return rc;
    memcpy(dst->local42c_words13, out_local42c_words13, sizeof(dst->local42c_words13));

    rc = l3_authorization_secondary_derive_feedback_state(
        param1_words13,
        local3f8_words13,
        caller_seed_words13,
        caller_scalar0,
        caller_scalar1,
        out_sixth_secondary_vector_reduce_words13,
        &dst->sixth_trace);
    if (rc != 0) return rc;
    memcpy(dst->sixth_secondary_vector_reduce_words13, out_sixth_secondary_vector_reduce_words13,
           sizeof(dst->sixth_secondary_vector_reduce_words13));

    rc = l3_authorization_secondary_vector_preimage_param7_local460_words13(
        out_sixth_secondary_vector_reduce_words13,
        local3c4_words13,
        out_local42c_words13,
        param4_ctx_words,
        caller_seed_words13,
        caller_scalar0,
        caller_scalar1,
        out_local5b0_words13,
        out_local460_words13,
        &dst->preimage_trace);
    if (rc != 0) return rc;

    memcpy(dst->local5b0_words13, out_local5b0_words13, sizeof(dst->local5b0_words13));
    memcpy(dst->local460_words13, out_local460_words13, sizeof(dst->local460_words13));
    return 0;
}

int l3_authorization_secondary_run(
    const int32_t param1_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t param2_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t param3_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const void *parent_context,
    const int32_t caller_param5_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t caller_param6_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_param7_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_param8_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_param9_words13[L3_AUTH_ROUND_SECONDARY_WORDS13]) {
    if (!param1_words13 || !param2_words13 || !param3_words13 ||
        !parent_context || !caller_param5_words13 || !caller_param6_words13 ||
        !out_param7_words13 || !out_param8_words13 || !out_param9_words13) {
        return -1;
    }

    const uint8_t *context = (const uint8_t *)parent_context;
    const int32_t *caller_seed_words13 =
        (const int32_t *)(const void *)(context + 0x090u);
    uint32_t scalar0;
    uint32_t scalar1;
    memcpy(&scalar0, context + 0x268u, sizeof(scalar0));
    memcpy(&scalar1, context + 0x26cu, sizeof(scalar1));

    uint32_t affine_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t local3c4_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t local3f8_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t local42c_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t sixth_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t local5b0_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t local460_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];

    int rc = l3_authorization_secondary_param3_bound_secondary_vector_reduce_affine_words13(
        param3_words13, caller_seed_words13, scalar0, scalar1,
        affine_words13, NULL);
    if (rc != 0) return rc;
    rc = l3_authorization_secondary_derive_state_stage1(
        caller_param5_words13, affine_words13, caller_seed_words13,
        scalar0, scalar1, param1_words13,
        (const int32_t *)(const void *)(context + 0x12cu),
        local390_words13, NULL);
    if (rc != 0) return rc;

    l3_authorization_secondary_pair32 third_call_pairs26[
        L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    rc = l3_authorization_secondary_param3_affine_accum_convolution26(
        param3_words13, affine_words13, third_call_pairs26, NULL);
    if (rc != 0) return rc;
    uint32_t third_call_state26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    for (unsigned i = 0; i < L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26; ++i) {
        third_call_state26[i][0] = third_call_pairs26[i].lo;
        third_call_state26[i][1] = third_call_pairs26[i].hi;
    }
    uint32_t third_call_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    rc = l3_authorization_secondary_vector_reduce_run13_update_export(
        caller_seed_words13, scalar0, scalar1, third_call_state26,
        third_call_words13, NULL);
    if (rc != 0) return rc;

    l3_authorization_secondary_pair32 fourth_call_pairs26[
        L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    rc = l3_authorization_secondary_param6_third_export_convolution26(
        caller_param6_words13, third_call_words13, fourth_call_pairs26);
    if (rc != 0) return rc;
    uint32_t fourth_call_state26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    for (unsigned i = 0; i < L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26; ++i) {
        fourth_call_state26[i][0] = fourth_call_pairs26[i].lo;
        fourth_call_state26[i][1] = fourth_call_pairs26[i].hi;
    }
    uint32_t fourth_call_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    rc = l3_authorization_secondary_vector_reduce_run13_update_export(
        caller_seed_words13, scalar0, scalar1, fourth_call_state26,
        fourth_call_words13, NULL);
    if (rc != 0) return rc;
    rc = l3_authorization_secondary_post_third_secondary_vector_reduce_mix3c4_words13(
        fourth_call_words13, local390_words13, param2_words13,
        (const int32_t *)(const void *)(context + 0x0c4u),
        local3c4_words13, NULL);
    if (rc != 0) return rc;

    rc = l3_authorization_secondary_derive_state_stage3(
        local390_words13, caller_seed_words13, scalar0, scalar1,
        local3f8_words13, NULL);
    if (rc != 0) return rc;
    rc = l3_authorization_secondary_local3f8_bound_sixth_local42c_preimage_param7_local460_words13(
        local3f8_words13, local390_words13,
        (const int32_t *)parent_context, param1_words13,
        caller_seed_words13, scalar0, scalar1, local3c4_words13,
        local42c_words13, sixth_words13, local5b0_words13,
        local460_words13, NULL);
    if (rc != 0) return rc;

    l3_authorization_secondary_pair32 raw22f0[L3_AUTH_ROUND_SECONDARY_WORDS13];
    l3_authorization_secondary_pair32 prefix22f0[L3_AUTH_ROUND_SECONDARY_WORDS13];
    l3_authorization_secondary_pair32 raw2370[L3_AUTH_ROUND_SECONDARY_WORDS13];
    l3_authorization_secondary_pair32 prefix2370[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t carry_seed_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26] = {0};
    rc = l3_authorization_secondary_param2_local42c_accum22f0_2370_pairs13(
        param2_words13, local42c_words13,
        raw22f0, prefix22f0, raw2370, prefix2370, NULL);
    if (rc != 0) return rc;
    memcpy(carry_seed_words26 + 2u, prefix22f0, 24u * sizeof(uint32_t));

    uint32_t local4c8_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    uint32_t local530_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    rc = l3_authorization_secondary_local3c4_local460_accum_convolution26_materialize4c8(
        local3c4_words13, local460_words13, local4c8_words26, NULL);
    if (rc != 0) return rc;
    rc = l3_authorization_secondary_param2_local42c_accum_convolution26_materialize530(
        param2_words13, local42c_words13,
        local530_words26, NULL);
    if (rc != 0) return rc;

    uint32_t result8[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t result9[L3_AUTH_ROUND_SECONDARY_WORDS13];
    rc = l3_authorization_secondary_build_terminal_outputs(
        local4c8_words26,
        (const int32_t *)(const void *)(context + 0x194u),
        local530_words26, carry_seed_words26, param3_words13,
        local390_words13, caller_seed_words13, scalar0, scalar1,
        result8, result9, NULL);
    if (rc != 0) return rc;

    memcpy(out_param7_words13, local5b0_words13, sizeof(local5b0_words13));
    memcpy(out_param8_words13, result8, sizeof(result8));
    memcpy(out_param9_words13, result9, sizeof(result9));
    return 0;
}
