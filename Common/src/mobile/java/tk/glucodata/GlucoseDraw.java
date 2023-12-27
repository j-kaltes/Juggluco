/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2 and 3 sensors.                         */
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
/*      Fri Jan 27 15:32:11 CET 2023                                                 */


package tk.glucodata;

import static tk.glucodata.Applic.mgdLmult;
import static tk.glucodata.Applic.unit;

class GlucoseDraw  {

static private final int[] mmolicon ={
R.drawable.glucose_2_2
,R.drawable.glucose_2_3
,R.drawable.glucose_2_4
,R.drawable.glucose_2_5
,R.drawable.glucose_2_6
,R.drawable.glucose_2_7
,R.drawable.glucose_2_8
,R.drawable.glucose_2_9
,R.drawable.glucose_3
,R.drawable.glucose_3_1
,R.drawable.glucose_3_2
,R.drawable.glucose_3_3
,R.drawable.glucose_3_4
,R.drawable.glucose_3_5
,R.drawable.glucose_3_6
,R.drawable.glucose_3_7
,R.drawable.glucose_3_8
,R.drawable.glucose_3_9
,R.drawable.glucose_4
,R.drawable.glucose_4_1
,R.drawable.glucose_4_2
,R.drawable.glucose_4_3
,R.drawable.glucose_4_4
,R.drawable.glucose_4_5
,R.drawable.glucose_4_6
,R.drawable.glucose_4_7
,R.drawable.glucose_4_8
,R.drawable.glucose_4_9
,R.drawable.glucose_5
,R.drawable.glucose_5_1
,R.drawable.glucose_5_2
,R.drawable.glucose_5_3
,R.drawable.glucose_5_4
,R.drawable.glucose_5_5
,R.drawable.glucose_5_6
,R.drawable.glucose_5_7
,R.drawable.glucose_5_8
,R.drawable.glucose_5_9
,R.drawable.glucose_6
,R.drawable.glucose_6_1
,R.drawable.glucose_6_2
,R.drawable.glucose_6_3
,R.drawable.glucose_6_4
,R.drawable.glucose_6_5
,R.drawable.glucose_6_6
,R.drawable.glucose_6_7
,R.drawable.glucose_6_8
,R.drawable.glucose_6_9
,R.drawable.glucose_7
,R.drawable.glucose_7_1
,R.drawable.glucose_7_2
,R.drawable.glucose_7_3
,R.drawable.glucose_7_4
,R.drawable.glucose_7_5
,R.drawable.glucose_7_6
,R.drawable.glucose_7_7
,R.drawable.glucose_7_8
,R.drawable.glucose_7_9
,R.drawable.glucose_8
,R.drawable.glucose_8_1
,R.drawable.glucose_8_2
,R.drawable.glucose_8_3
,R.drawable.glucose_8_4
,R.drawable.glucose_8_5
,R.drawable.glucose_8_6
,R.drawable.glucose_8_7
,R.drawable.glucose_8_8
,R.drawable.glucose_8_9
,R.drawable.glucose_9
,R.drawable.glucose_9_1
,R.drawable.glucose_9_2
,R.drawable.glucose_9_3
,R.drawable.glucose_9_4
,R.drawable.glucose_9_5
,R.drawable.glucose_9_6
,R.drawable.glucose_9_7
,R.drawable.glucose_9_8
,R.drawable.glucose_9_9
,R.drawable.glucose_10
,R.drawable.glucose_10_1
,R.drawable.glucose_10_2
,R.drawable.glucose_10_3
,R.drawable.glucose_10_4
,R.drawable.glucose_10_5
,R.drawable.glucose_10_6
,R.drawable.glucose_10_7
,R.drawable.glucose_10_8
,R.drawable.glucose_10_9
,R.drawable.glucose_11
,R.drawable.glucose_11_1
,R.drawable.glucose_11_2
,R.drawable.glucose_11_3
,R.drawable.glucose_11_4
,R.drawable.glucose_11_5
,R.drawable.glucose_11_6
,R.drawable.glucose_11_7
,R.drawable.glucose_11_8
,R.drawable.glucose_11_9
,R.drawable.glucose_12
,R.drawable.glucose_12_1
,R.drawable.glucose_12_2
,R.drawable.glucose_12_3
,R.drawable.glucose_12_4
,R.drawable.glucose_12_5
,R.drawable.glucose_12_6
,R.drawable.glucose_12_7
,R.drawable.glucose_12_8
,R.drawable.glucose_12_9
,R.drawable.glucose_13
,R.drawable.glucose_13_1
,R.drawable.glucose_13_2
,R.drawable.glucose_13_3
,R.drawable.glucose_13_4
,R.drawable.glucose_13_5
,R.drawable.glucose_13_6
,R.drawable.glucose_13_7
,R.drawable.glucose_13_8
,R.drawable.glucose_13_9
,R.drawable.glucose_14
,R.drawable.glucose_14_1
,R.drawable.glucose_14_2
,R.drawable.glucose_14_3
,R.drawable.glucose_14_4
,R.drawable.glucose_14_5
,R.drawable.glucose_14_6
,R.drawable.glucose_14_7
,R.drawable.glucose_14_8
,R.drawable.glucose_14_9
,R.drawable.glucose_15
,R.drawable.glucose_15_1
,R.drawable.glucose_15_2
,R.drawable.glucose_15_3
,R.drawable.glucose_15_4
,R.drawable.glucose_15_5
,R.drawable.glucose_15_6
,R.drawable.glucose_15_7
,R.drawable.glucose_15_8
,R.drawable.glucose_15_9
,R.drawable.glucose_16
,R.drawable.glucose_16_1
,R.drawable.glucose_16_2
,R.drawable.glucose_16_3
,R.drawable.glucose_16_4
,R.drawable.glucose_16_5
,R.drawable.glucose_16_6
,R.drawable.glucose_16_7
,R.drawable.glucose_16_8
,R.drawable.glucose_16_9
,R.drawable.glucose_17
,R.drawable.glucose_17_1
,R.drawable.glucose_17_2
,R.drawable.glucose_17_3
,R.drawable.glucose_17_4
,R.drawable.glucose_17_5
,R.drawable.glucose_17_6
,R.drawable.glucose_17_7
,R.drawable.glucose_17_8
,R.drawable.glucose_17_9
,R.drawable.glucose_18
,R.drawable.glucose_18_1
,R.drawable.glucose_18_2
,R.drawable.glucose_18_3
,R.drawable.glucose_18_4
,R.drawable.glucose_18_5
,R.drawable.glucose_18_6
,R.drawable.glucose_18_7
,R.drawable.glucose_18_8
,R.drawable.glucose_18_9
,R.drawable.glucose_19
,R.drawable.glucose_19_1
,R.drawable.glucose_19_2
,R.drawable.glucose_19_3
,R.drawable.glucose_19_4
,R.drawable.glucose_19_5
,R.drawable.glucose_19_6
,R.drawable.glucose_19_7
,R.drawable.glucose_19_8
,R.drawable.glucose_19_9
,R.drawable.glucose_20
,R.drawable.glucose_20_1
,R.drawable.glucose_20_2
,R.drawable.glucose_20_3
,R.drawable.glucose_20_4
,R.drawable.glucose_20_5
,R.drawable.glucose_20_6
,R.drawable.glucose_20_7
,R.drawable.glucose_20_8
,R.drawable.glucose_20_9
,R.drawable.glucose_21
,R.drawable.glucose_21_1
,R.drawable.glucose_21_2
,R.drawable.glucose_21_3
,R.drawable.glucose_21_4
,R.drawable.glucose_21_5
,R.drawable.glucose_21_6
,R.drawable.glucose_21_7
,R.drawable.glucose_21_8
,R.drawable.glucose_21_9
,R.drawable.glucose_22
,R.drawable.glucose_22_1
,R.drawable.glucose_22_2
,R.drawable.glucose_22_3
,R.drawable.glucose_22_4
,R.drawable.glucose_22_5
,R.drawable.glucose_22_6
,R.drawable.glucose_22_7
,R.drawable.glucose_22_8
,R.drawable.glucose_22_9
,R.drawable.glucose_23
,R.drawable.glucose_23_1
,R.drawable.glucose_23_2
,R.drawable.glucose_23_3
,R.drawable.glucose_23_4
,R.drawable.glucose_23_5
,R.drawable.glucose_23_6
,R.drawable.glucose_23_7
,R.drawable.glucose_23_8
,R.drawable.glucose_23_9
,R.drawable.glucose_24
,R.drawable.glucose_24_1
,R.drawable.glucose_24_2
,R.drawable.glucose_24_3
,R.drawable.glucose_24_4
,R.drawable.glucose_24_5
,R.drawable.glucose_24_6
,R.drawable.glucose_24_7
,R.drawable.glucose_24_8
,R.drawable.glucose_24_9
,R.drawable.glucose_25
,R.drawable.glucose_25_1
,R.drawable.glucose_25_2
,R.drawable.glucose_25_3
,R.drawable.glucose_25_4
,R.drawable.glucose_25_5
,R.drawable.glucose_25_6
,R.drawable.glucose_25_7
,R.drawable.glucose_25_8
,R.drawable.glucose_25_9
,R.drawable.glucose_26
,R.drawable.glucose_26_1
,R.drawable.glucose_26_2
,R.drawable.glucose_26_3
,R.drawable.glucose_26_4
,R.drawable.glucose_26_5
,R.drawable.glucose_26_6
,R.drawable.glucose_26_7
,R.drawable.glucose_26_8
,R.drawable.glucose_26_9
,R.drawable.glucose_27
,R.drawable.glucose_27_1
,R.drawable.glucose_27_2
,R.drawable.glucose_27_3
,R.drawable.glucose_27_4
,R.drawable.glucose_27_5
,R.drawable.glucose_27_6
,R.drawable.glucose_27_7
,R.drawable.glucose_27_8
};

static private final int[] mgicon ={
R.drawable.glucose_40
,R.drawable.glucose_41
,R.drawable.glucose_42
,R.drawable.glucose_43
,R.drawable.glucose_44
,R.drawable.glucose_45
,R.drawable.glucose_46
,R.drawable.glucose_47
,R.drawable.glucose_48
,R.drawable.glucose_49
,R.drawable.glucose_50
,R.drawable.glucose_51
,R.drawable.glucose_52
,R.drawable.glucose_53
,R.drawable.glucose_54
,R.drawable.glucose_55
,R.drawable.glucose_56
,R.drawable.glucose_57
,R.drawable.glucose_58
,R.drawable.glucose_59
,R.drawable.glucose_60
,R.drawable.glucose_61
,R.drawable.glucose_62
,R.drawable.glucose_63
,R.drawable.glucose_64
,R.drawable.glucose_65
,R.drawable.glucose_66
,R.drawable.glucose_67
,R.drawable.glucose_68
,R.drawable.glucose_69
,R.drawable.glucose_70
,R.drawable.glucose_71
,R.drawable.glucose_72
,R.drawable.glucose_73
,R.drawable.glucose_74
,R.drawable.glucose_75
,R.drawable.glucose_76
,R.drawable.glucose_77
,R.drawable.glucose_78
,R.drawable.glucose_79
,R.drawable.glucose_80
,R.drawable.glucose_81
,R.drawable.glucose_82
,R.drawable.glucose_83
,R.drawable.glucose_84
,R.drawable.glucose_85
,R.drawable.glucose_86
,R.drawable.glucose_87
,R.drawable.glucose_88
,R.drawable.glucose_89
,R.drawable.glucose_90
,R.drawable.glucose_91
,R.drawable.glucose_92
,R.drawable.glucose_93
,R.drawable.glucose_94
,R.drawable.glucose_95
,R.drawable.glucose_96
,R.drawable.glucose_97
,R.drawable.glucose_98
,R.drawable.glucose_99
,R.drawable.glucose_100
,R.drawable.glucose_101
,R.drawable.glucose_102
,R.drawable.glucose_103
,R.drawable.glucose_104
,R.drawable.glucose_105
,R.drawable.glucose_106
,R.drawable.glucose_107
,R.drawable.glucose_108
,R.drawable.glucose_109
,R.drawable.glucose_110
,R.drawable.glucose_111
,R.drawable.glucose_112
,R.drawable.glucose_113
,R.drawable.glucose_114
,R.drawable.glucose_115
,R.drawable.glucose_116
,R.drawable.glucose_117
,R.drawable.glucose_118
,R.drawable.glucose_119
,R.drawable.glucose_120
,R.drawable.glucose_121
,R.drawable.glucose_122
,R.drawable.glucose_123
,R.drawable.glucose_124
,R.drawable.glucose_125
,R.drawable.glucose_126
,R.drawable.glucose_127
,R.drawable.glucose_128
,R.drawable.glucose_129
,R.drawable.glucose_130
,R.drawable.glucose_131
,R.drawable.glucose_132
,R.drawable.glucose_133
,R.drawable.glucose_134
,R.drawable.glucose_135
,R.drawable.glucose_136
,R.drawable.glucose_137
,R.drawable.glucose_138
,R.drawable.glucose_139
,R.drawable.glucose_140
,R.drawable.glucose_141
,R.drawable.glucose_142
,R.drawable.glucose_143
,R.drawable.glucose_144
,R.drawable.glucose_145
,R.drawable.glucose_146
,R.drawable.glucose_147
,R.drawable.glucose_148
,R.drawable.glucose_149
,R.drawable.glucose_150
,R.drawable.glucose_151
,R.drawable.glucose_152
,R.drawable.glucose_153
,R.drawable.glucose_154
,R.drawable.glucose_155
,R.drawable.glucose_156
,R.drawable.glucose_157
,R.drawable.glucose_158
,R.drawable.glucose_159
,R.drawable.glucose_160
,R.drawable.glucose_161
,R.drawable.glucose_162
,R.drawable.glucose_163
,R.drawable.glucose_164
,R.drawable.glucose_165
,R.drawable.glucose_166
,R.drawable.glucose_167
,R.drawable.glucose_168
,R.drawable.glucose_169
,R.drawable.glucose_170
,R.drawable.glucose_171
,R.drawable.glucose_172
,R.drawable.glucose_173
,R.drawable.glucose_174
,R.drawable.glucose_175
,R.drawable.glucose_176
,R.drawable.glucose_177
,R.drawable.glucose_178
,R.drawable.glucose_179
,R.drawable.glucose_180
,R.drawable.glucose_181
,R.drawable.glucose_182
,R.drawable.glucose_183
,R.drawable.glucose_184
,R.drawable.glucose_185
,R.drawable.glucose_186
,R.drawable.glucose_187
,R.drawable.glucose_188
,R.drawable.glucose_189
,R.drawable.glucose_190
,R.drawable.glucose_191
,R.drawable.glucose_192
,R.drawable.glucose_193
,R.drawable.glucose_194
,R.drawable.glucose_195
,R.drawable.glucose_196
,R.drawable.glucose_197
,R.drawable.glucose_198
,R.drawable.glucose_199
,R.drawable.glucose_200
,R.drawable.glucose_201
,R.drawable.glucose_202
,R.drawable.glucose_203
,R.drawable.glucose_204
,R.drawable.glucose_205
,R.drawable.glucose_206
,R.drawable.glucose_207
,R.drawable.glucose_208
,R.drawable.glucose_209
,R.drawable.glucose_210
,R.drawable.glucose_211
,R.drawable.glucose_212
,R.drawable.glucose_213
,R.drawable.glucose_214
,R.drawable.glucose_215
,R.drawable.glucose_216
,R.drawable.glucose_217
,R.drawable.glucose_218
,R.drawable.glucose_219
,R.drawable.glucose_220
,R.drawable.glucose_221
,R.drawable.glucose_222
,R.drawable.glucose_223
,R.drawable.glucose_224
,R.drawable.glucose_225
,R.drawable.glucose_226
,R.drawable.glucose_227
,R.drawable.glucose_228
,R.drawable.glucose_229
,R.drawable.glucose_230
,R.drawable.glucose_231
,R.drawable.glucose_232
,R.drawable.glucose_233
,R.drawable.glucose_234
,R.drawable.glucose_235
,R.drawable.glucose_236
,R.drawable.glucose_237
,R.drawable.glucose_238
,R.drawable.glucose_239
,R.drawable.glucose_240
,R.drawable.glucose_241
,R.drawable.glucose_242
,R.drawable.glucose_243
,R.drawable.glucose_244
,R.drawable.glucose_245
,R.drawable.glucose_246
,R.drawable.glucose_247
,R.drawable.glucose_248
,R.drawable.glucose_249
,R.drawable.glucose_250
,R.drawable.glucose_251
,R.drawable.glucose_252
,R.drawable.glucose_253
,R.drawable.glucose_254
,R.drawable.glucose_255
,R.drawable.glucose_256
,R.drawable.glucose_257
,R.drawable.glucose_258
,R.drawable.glucose_259
,R.drawable.glucose_260
,R.drawable.glucose_261
,R.drawable.glucose_262
,R.drawable.glucose_263
,R.drawable.glucose_264
,R.drawable.glucose_265
,R.drawable.glucose_266
,R.drawable.glucose_267
,R.drawable.glucose_268
,R.drawable.glucose_269
,R.drawable.glucose_270
,R.drawable.glucose_271
,R.drawable.glucose_272
,R.drawable.glucose_273
,R.drawable.glucose_274
,R.drawable.glucose_275
,R.drawable.glucose_276
,R.drawable.glucose_277
,R.drawable.glucose_278
,R.drawable.glucose_279
,R.drawable.glucose_280
,R.drawable.glucose_281
,R.drawable.glucose_282
,R.drawable.glucose_283
,R.drawable.glucose_284
,R.drawable.glucose_285
,R.drawable.glucose_286
,R.drawable.glucose_287
,R.drawable.glucose_288
,R.drawable.glucose_289
,R.drawable.glucose_290
,R.drawable.glucose_291
,R.drawable.glucose_292
,R.drawable.glucose_293
,R.drawable.glucose_294
,R.drawable.glucose_295
,R.drawable.glucose_296
,R.drawable.glucose_297
,R.drawable.glucose_298
,R.drawable.glucose_299
,R.drawable.glucose_300
,R.drawable.glucose_301
,R.drawable.glucose_302
,R.drawable.glucose_303
,R.drawable.glucose_304
,R.drawable.glucose_305
,R.drawable.glucose_306
,R.drawable.glucose_307
,R.drawable.glucose_308
,R.drawable.glucose_309
,R.drawable.glucose_310
,R.drawable.glucose_311
,R.drawable.glucose_312
,R.drawable.glucose_313
,R.drawable.glucose_314
,R.drawable.glucose_315
,R.drawable.glucose_316
,R.drawable.glucose_317
,R.drawable.glucose_318
,R.drawable.glucose_319
,R.drawable.glucose_320
,R.drawable.glucose_321
,R.drawable.glucose_322
,R.drawable.glucose_323
,R.drawable.glucose_324
,R.drawable.glucose_325
,R.drawable.glucose_326
,R.drawable.glucose_327
,R.drawable.glucose_328
,R.drawable.glucose_329
,R.drawable.glucose_330
,R.drawable.glucose_331
,R.drawable.glucose_332
,R.drawable.glucose_333
,R.drawable.glucose_334
,R.drawable.glucose_335
,R.drawable.glucose_336
,R.drawable.glucose_337
,R.drawable.glucose_338
,R.drawable.glucose_339
,R.drawable.glucose_340
,R.drawable.glucose_341
,R.drawable.glucose_342
,R.drawable.glucose_343
,R.drawable.glucose_344
,R.drawable.glucose_345
,R.drawable.glucose_346
,R.drawable.glucose_347
,R.drawable.glucose_348
,R.drawable.glucose_349
,R.drawable.glucose_350
,R.drawable.glucose_351
,R.drawable.glucose_352
,R.drawable.glucose_353
,R.drawable.glucose_354
,R.drawable.glucose_355
,R.drawable.glucose_356
,R.drawable.glucose_357
,R.drawable.glucose_358
,R.drawable.glucose_359
,R.drawable.glucose_360
,R.drawable.glucose_361
,R.drawable.glucose_362
,R.drawable.glucose_363
,R.drawable.glucose_364
,R.drawable.glucose_365
,R.drawable.glucose_366
,R.drawable.glucose_367
,R.drawable.glucose_368
,R.drawable.glucose_369
,R.drawable.glucose_370
,R.drawable.glucose_371
,R.drawable.glucose_372
,R.drawable.glucose_373
,R.drawable.glucose_374
,R.drawable.glucose_375
,R.drawable.glucose_376
,R.drawable.glucose_377
,R.drawable.glucose_378
,R.drawable.glucose_379
,R.drawable.glucose_380
,R.drawable.glucose_381
,R.drawable.glucose_382
,R.drawable.glucose_383
,R.drawable.glucose_384
,R.drawable.glucose_385
,R.drawable.glucose_386
,R.drawable.glucose_387
,R.drawable.glucose_388
,R.drawable.glucose_389
,R.drawable.glucose_390
,R.drawable.glucose_391
,R.drawable.glucose_392
,R.drawable.glucose_393
,R.drawable.glucose_394
,R.drawable.glucose_395
,R.drawable.glucose_396
,R.drawable.glucose_397
,R.drawable.glucose_398
,R.drawable.glucose_399
,R.drawable.glucose_400
,R.drawable.glucose_401
,R.drawable.glucose_402
,R.drawable.glucose_403
,R.drawable.glucose_404
,R.drawable.glucose_405
,R.drawable.glucose_406
,R.drawable.glucose_407
,R.drawable.glucose_408
,R.drawable.glucose_409
,R.drawable.glucose_410
,R.drawable.glucose_411
,R.drawable.glucose_412
,R.drawable.glucose_413
,R.drawable.glucose_414
,R.drawable.glucose_415
,R.drawable.glucose_416
,R.drawable.glucose_417
,R.drawable.glucose_418
,R.drawable.glucose_419
,R.drawable.glucose_420
,R.drawable.glucose_421
,R.drawable.glucose_422
,R.drawable.glucose_423
,R.drawable.glucose_424
,R.drawable.glucose_425
,R.drawable.glucose_426
,R.drawable.glucose_427
,R.drawable.glucose_428
,R.drawable.glucose_429
,R.drawable.glucose_430
,R.drawable.glucose_431
,R.drawable.glucose_432
,R.drawable.glucose_433
,R.drawable.glucose_434
,R.drawable.glucose_435
,R.drawable.glucose_436
,R.drawable.glucose_437
,R.drawable.glucose_438
,R.drawable.glucose_439
,R.drawable.glucose_440
,R.drawable.glucose_441
,R.drawable.glucose_442
,R.drawable.glucose_443
,R.drawable.glucose_444
,R.drawable.glucose_445
,R.drawable.glucose_446
,R.drawable.glucose_447
,R.drawable.glucose_448
,R.drawable.glucose_449
,R.drawable.glucose_450
,R.drawable.glucose_451
,R.drawable.glucose_452
,R.drawable.glucose_453
,R.drawable.glucose_454
,R.drawable.glucose_455
,R.drawable.glucose_456
,R.drawable.glucose_457
,R.drawable.glucose_458
,R.drawable.glucose_459
,R.drawable.glucose_460
,R.drawable.glucose_461
,R.drawable.glucose_462
,R.drawable.glucose_463
,R.drawable.glucose_464
,R.drawable.glucose_465
,R.drawable.glucose_466
,R.drawable.glucose_467
,R.drawable.glucose_468
,R.drawable.glucose_469
,R.drawable.glucose_470
,R.drawable.glucose_471
,R.drawable.glucose_472
,R.drawable.glucose_473
,R.drawable.glucose_474
,R.drawable.glucose_475
,R.drawable.glucose_476
,R.drawable.glucose_477
,R.drawable.glucose_478
,R.drawable.glucose_479
,R.drawable.glucose_480
,R.drawable.glucose_481
,R.drawable.glucose_482
,R.drawable.glucose_483
,R.drawable.glucose_484
,R.drawable.glucose_485
,R.drawable.glucose_486
,R.drawable.glucose_487
,R.drawable.glucose_488
,R.drawable.glucose_489
,R.drawable.glucose_490
,R.drawable.glucose_491
,R.drawable.glucose_492
,R.drawable.glucose_493
,R.drawable.glucose_494
,R.drawable.glucose_495
,R.drawable.glucose_496
,R.drawable.glucose_497
,R.drawable.glucose_498
,R.drawable.glucose_499
,R.drawable.glucose_500
,R.drawable.glucose_501
};

static private final int lowvalue=R.drawable.logmmol;
/*
private Bitmap createBitmapFromString(String inputNumber) {

        Paint paint = new Paint();
        paint.setAntiAlias(true);
        paint.setTextSize(100);
        paint.setTextAlign(Paint.Align.CENTER);

        Rect textBounds = new Rect();
        paint.getTextBounds(inputNumber, 0, inputNumber.length(), textBounds);

        Bitmap bitmap = Bitmap.createBitmap(textBounds.width() + 10, 90,
                Bitmap.Config.ARGB_8888);

        Canvas canvas = new Canvas(bitmap);
        canvas.drawText(inputNumber, textBounds.width() / 2 + 5, 70, paint);
        return bitmap;
    }
        Bitmap bitmap = createBitmapFromString(text.trim());

        //setting bitmap to staus bar icon.
        builder.setSmallIcon(Icon.createWithBitmap(bitmap));
	Icon only from Android 23
*/
/*
int statusBarHeight = 0;
int resourceId = getResources().getIdentifier("status_bar_height", "dimen", "android");
if (resourceId > 0) statusBarHeight = getResources().getDimensionPixelSize(resourceId);

final WindowManager.LayoutParams parameters = new WindowManager.LayoutParams(
        WindowManager.LayoutParams.WRAP_CONTENT,
        statusBarHeight,
        WindowManager.LayoutParams.TYPE_SYSTEM_ERROR,   // Allows the view to be on top of the StatusBar
        WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN,    // Keeps the button presses from going to the background window and Draws over status bar
        PixelFormat.TRANSLUCENT);
parameters.gravity = Gravity.TOP | Gravity.CENTER;

LinearLayout ll = new LinearLayout(this);
ll.setBackgroundColor(Color.TRANSPARENT);
LinearLayout.LayoutParams layoutParameteres = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.MATCH_PARENT);
ll.setLayoutParams(layoutParameteres);

TextView tv = new TextView(this);
ViewGroup.LayoutParams tvParameters = new ViewGroup.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.MATCH_PARENT);
tv.setLayoutParams(tvParameters);
tv.setTextColor(Color.WHITE);
tv.setGravity(Gravity.CENTER);
tv.setText("123");
ll.addView(tv);

WindowManager windowManager = (WindowManager) getSystemService(WINDOW_SERVICE);
windowManager.addView(ll, parameters);
*/

static private final int lowmgvalue=R.drawable.alarm39;

static private final int highmmolvalue=R.drawable.highmmol;

static int getgludraw(float val) {
	if(unit==1) {
		if(val<2.2f)
			return lowvalue;
		if(val>(500.0/mgdLmult))
			return highmmolvalue;
	        int ind=Math.round(val*10)-22;
		return mmolicon[ind];
		}
	else {
		int intval=(int)val;
		if(intval<40)
			return lowmgvalue;
		if(intval>500)
			return R.drawable.alarm501;
		int ind=intval-40;
		return mgicon[ind];
		}
	}




	};
