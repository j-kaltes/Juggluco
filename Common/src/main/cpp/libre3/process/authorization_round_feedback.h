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
/*      Sun Aug 30 10:21:11 CEST 2026                                                */

#ifndef L3_AUTHORIZATION_ROUND_FEEDBACK_H
#define L3_AUTHORIZATION_ROUND_FEEDBACK_H

#include <stdint.h>

int l3_authorization_materialize_round_output_a(
    const uint32_t vector_feedback_words13[13],
    uint32_t out_param5_words13[13]);

int l3_authorization_materialize_round_output_b(
    const uint32_t vector_feedback_words13[13],
    uint32_t out_param6_words13[13]);

int l3_authorization_materialize_round_output_c(
    const uint32_t vector_feedback_words13[13],
    uint32_t out_param7_words13[13]);

#endif /* L3_AUTHORIZATION_ROUND_FEEDBACK_H */
