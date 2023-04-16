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
/*      Tue Apr 11 15:42:30 CEST 2023                                                 */
#include "maxsendtohost.h"

extern pid_t getTid();

#pragma once
struct mirrorstatus_t {
	struct {
		int sendmessage=-1;
		int runs=0;
		bool recv=false;
		void running(bool on) {
			if(on) {
				++runs;
				sendmessage=-1;
				recv=false;
				}
			else
				--runs;
			
			}
		} toblue[2];
	struct {
		pid_t tid=0;
		bool activereceivethread=false;
		//bool hassocket;
//		bool ingetcom=false;
	 	bool ininterpret=false;
		bool ingetcom() {
			return tid!=0;
			}
		void running(bool on) {
			if(on) {
				tid=getTid();
				ininterpret=false;
				}
			else
				tid=0;
			};
		} receive;

	struct {
//		bool hassocket;
		bool running=false;
		bool locked=false;
		void start() {
			running=true;
			}
		void stop() {
			running=false;
			}
		} sender;
	};
extern mirrorstatus_t mirrorstatus[maxallhosts];
